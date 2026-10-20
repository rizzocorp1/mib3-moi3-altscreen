# Pendrive de Deploy — AID Redirect (MOI3 LG)

Estrutura do pendrive USB que aciona nosso redirect no carro.

## Como funciona

1. A central MOI3 LG detecta um USB com firmware de engenharia.
2. Aplica as atualizações internas (Postscript apaga configurações de voz).
3. **Ao final**, executa automaticamente `fix_hw_partnumber.sh` como root.
4. Nosso script copia o payload para `/tmp` (RAM) e ativa o redirect RSI.
5. O `multi-screen-server` captura o surface do CarPlay e emite H.264 para o cluster.
6. **Reboot restaura tudo.** Nada permanente é tocado.

## Estrutura do pendrive

```
(raiz do USB — FAT32)
├── fix_hw_partnumber.sh          ← executado automaticamente pelo firmware de eng.
└── mib3-aid-redirect/
    └── payload/
        ├── redirect_producer.py  ← redirect RSI (Python 2.7)
        └── [futuro: altscreen_handler.so para AltScreen tipo 111]
```

## Pré-requisitos e Container Base

### Firmware de Engenharia Base (`MOI3_EU_SE_E9400L`)
O pacote base confirmado e disponibilizado é o **`LG-wipe-FAZIT.zip`** (apenas **5,7 KB**):
- **Train:** `MOI3_EU_SE_E9400L` (pacote oficial de engenharia da LG).
- **Compatibilidade:** Manifesto `Meta/main.mnf` assinado com wildcard universal `"Variant": ["FM3-*-*-*-*-*-LG"]` (compatível com `3G5035832C` e qualquer outra MOI3 LG).
- **Segurança:** Contém apenas o script de limpeza de voz (`MIB3OI_POST_SCRIPT`). **Não contém imagens de CPU/HMI**, portanto não sobrescreve o sistema operacional (`MU0803` permanece intocado) e não altera a identidade visual VW.
- **Trigger Root:** Executa o script `fix_hw_partnumber.sh` na raiz do pendrive com privilégios de **root**, sem checagem de checksum ou assinatura.

## Estrutura Final do Pendrive

```
(raiz do USB — FAT32)
├── Data/
│   ├── Device.TNR_2019312-0313/
│   └── MIB3OI_POST_SCRIPT.POST_SCRIPT_2023313-1121/
├── Meta/
│   ├── main.mnf
│   ├── main.mnf.cks
│   ├── main.mnf_cks_S.sig
│   └── MIB3OI_POST_SCRIPT/
├── fix_hw_partnumber.sh          ← Gatilho executado como root pela central
└── mib3-aid-redirect/
    └── payload/
        └── altscreen_handler.so  ← Handler compilado em aarch64
```

## Status dos componentes

| Componente | Status |
|---|---|
| Container base de engenharia (`MOI3_EU_SE_E9400L`) | ✅ Pronto (`LG-wipe-FAZIT.zip`, 5,7 KB) |
| Script gatilho (`fix_hw_partnumber.sh`) | ✅ Pronto (executa como root na RAM) |
| Handler AltScreen tipo 111 (`altscreen_handler.so`) | ✅ Compilado e pronto (ELF 64-bit ARM aarch64) |
| Pipeline AirPlay sem recodificação (H.264 direto) | ✅ Mapeado (`AirPlayCopyServerInfo` + `AirPlayReceiverSessionSetup`) |

## Sequência de Teste e Operação no Carro (Validada em 18/10/2026)

1. Formatar o USB como FAT32 com a estrutura oficial do container de engenharia (`Data/`, `Meta/`, `fix_hw_partnumber.sh`).
2. Plugar no carro com a central ligada (ignição ligada / motor ou bateria estável).
3. Entrar no **GEM (Red Engineering Menu)** segurando o botão físico **MENU** por 3 a 5 segundos.
4. Navegar até **Update** -> selecionar **USB1** -> tocar em **Start Update** (ou **Resume**).
5. O componente `MIB3OI_POST_SCRIPT` será executado, acionando o script `fix_hw_partnumber.sh` como root.
6. Ao finalizar a tela de resumo (`SummaryScreen` exibindo `MIB3OI_POST_SCRIPT  OK`), tocar em **Resume** para a central reiniciar e consolidar o estado.
7. Se por qualquer motivo a central demorar na transição de tela:
   - **Hard Reset Físico:** Segurar o botão de Volume / Liga-Desliga por **15 a 20 segundos contínuos**. O hardware do MICOM/PMIC força uma reinicialização a frio segura.
8. Ao reiniciar, o sistema sobe limpo. No caso de re-emparelhamento, basta remover o carro em *Ajustes -> Geral -> CarPlay* no iPhone e conectar novamente.

## Descobertas Críticas de Segurança do Firmware MOI3 LG

1. **`FailureAction=reboot-immediate` no `carplay.service`:**
   O script `carplay.sh` executado como `ExecStartPre` DEVE terminar com `exit 0` e ter todos os comandos protegidos com `|| true`. Qualquer retorno diferente de 0 dispara `/etc/vw_fs_reboot.sh`, que para a HMI (`systemctl stop vw-hmi` - tela preta) e força o reboot do hardware.
2. **Rotulagem SMACK Obrigatória:**
   - Binário `carplay`: `SMACK64="smartlink::app"`, `SMACK64EXEC="carplay::app"`
   - Binário `mdnsd`: `SMACK64="smartlink::app"`, `SMACK64EXEC="mdnsd::app"`
   - Binário `iAP2d`: `SMACK64="smartlink::app"`, `SMACK64EXEC="iAP2d::app"`
   - Script `carplay.sh`: `SMACK64="common::lib"` (ou `smartlink::app`)
   - Área de certificados e emparelhamento `/data/lge/smartlink/carplay`: rótulo `carplay::data` e dono `sint_sec_carplay_display:video`.
3. **Execução de Binários na RAM:**
   O domínio `smartlink::app` não possui permissão de execução (`x`) sobre arquivos com rótulo `_` (onde `/tmp` reside nativamente). Binários executados da RAM precisam ter seus rótulos SMACK aplicados explicitamente com `chsmack` antes de serem montados via bind mount.

## Referências

- James11123 (Drive2): vetor de execução via `fix_hw_partnumber.sh`
  https://www.drive2.ru/l/745725070915144961/
- Análise de Binários Oficiais via Ghidra/REA: `../camada3-rsi-redirect.md`
- RE do `multi-screen-server` e streaming RTP: `../camada3-rsi-redirect.md`
- RE do `DisplayManagement` / pipeline de vídeo: `../camada3/README.md`

