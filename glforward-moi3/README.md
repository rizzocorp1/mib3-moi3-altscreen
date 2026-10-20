# glforward-moi3 — encaminhador de OpenGL do sandbox MOI3

Faz a HMI do MIB3 LG desenhar num Mac. **Funciona desde 11/10/2026:** a HMI renderiza a tela "Welcome / Loading user profile…" em 800×480, com animação, sem erro de OpenGL. Registro da sessão em [../modelagem-soc.md](../modelagem-soc.md), seção do Marco 3. É estudo num computador; nada vai para o carro.

## A ideia

A ponte JNI de OpenGL da HMI (`libjopengl_es3.so`) liga contra `libEGL_adreno.so` e `libGLESv2_adreno.so`, o driver da Qualcomm, que precisa da GPU do carro. Aqui esses dois nomes são ocupados por um **shim que não desenha**: ele empacota cada chamada e manda por socket Unix para o **glhost**, um processo fora do chroot do firmware que executa tudo no Mesa llvmpipe (OpenGL ES 3.2 por software), grava cada quadro em arquivo e o transmite a quem estiver assistindo.

```
JVM da HMI (chroot, glibc 2.23 do firmware)          container (glibc nova)
  libjopengl_es3.so
      └─ libGLESv2_adreno.so  = shim  ── socket Unix ──►  glhost ── Mesa llvmpipe
         libEGL_adreno.so     = casca                       ├─ quadros em PPM (a página do navegador)
                                                            └─ transmissão direta (a janela do Mac)
```

O shim é ligado contra a glibc 2.23 **do próprio firmware**, então carrega dentro da JVM sem misturar glibc. Foi essa mistura que derrubou as tentativas de 10/10 com o Mesa dentro do processo.

## Arquivos

| Arquivo | O que é |
|---|---|
| `proto.h` | o protocolo: pedido `[op][len][payload]`, resposta `[len][payload]`, tudo em campos de 8 bytes |
| `gen_glfwd.py` | gerador. Lê os 358 protótipos de `GLES3/gl32.h` e escreve `gen/` |
| `gen/` | código gerado (não editar): códigos de operação, lado do shim, lado do host, tabela do `eglGetProcAddress` |
| `shim.c` | lado da HMI: transporte, EGL 1.4 e as funções GL que precisam de cuidado (pixels, buffers mapeados, strings, consultas) |
| `host.c` | o glhost: EGL sem janela, execução das chamadas, gravação e transmissão dos quadros, conferência de erros |
| `ponte.c` | leva a transmissão dos quadros para fora do container e traz a entrada, pela entrada e saída padrão de um `docker exec -i` |
| `build.sh` | compila o shim, o glhost e a ponte na imagem `moi3-bancada:1` |
| `tests/` | seis programas GLES 3 que ligam pelos nomes do Adreno e o `run_tests.sh`; para a transmissão, `t_quadros.c` (produz quadros), `le_quadros.c` (espectador de teste) e `t_transmissao.sh` |
| `gl_shim_moi3.c` | o esboço de 11/10 de manhã, em OpenGL ES 2. **Substituído** pelo que está acima; fica só como histórico |

Cobertura do gerador: 166 funções automáticas (só escalares), 139 por regra de tamanho, 36 escritas à mão, 11 resolvidas no shim sem ir ao host (depuração). Ficam **sem implementação** seis: `glGetUniformfv`, `glGetUniformiv`, `glGetUniformuiv`, `glGetVertexAttribPointerv`, `glShaderBinary`, `glCreateShaderProgramv`. Elas avisam uma vez no stderr se forem chamadas.

O shim exporta o que o Adreno de verdade exporta: OpenGL ES 3.2 e EGL 1.4. As dez funções de EGL 1.5 que a ponte importa não existem no driver real e também não existem aqui.

## Como reproduzir

Tudo roda na imagem `moi3-bancada:1` (receita em `../driver/Dockerfile.bancada`). `$FW` é `../sysroot/mu0883`, `$W` é uma pasta de trabalho.

1. Gerar (no Mac; só é preciso de novo se mudar `gen_glfwd.py`). O cabeçalho vem da própria imagem, em `/usr/include/GLES3/gl32.h`:
   ```sh
   python3 -I gen_glfwd.py <cópia de>/GLES3/gl32.h gen
   ```
2. Copiar para `$W/fwlibs` a `libc.so.6`, a `libpthread.so.0` e o `ld-linux-aarch64.so.1` de `/lib64` da partição `system` (`cp -L`). É contra elas que o shim é ligado.
3. Compilar e testar, dentro do container:
   ```sh
   sh /glf/build.sh /work3/glfwd
   sh /glf/tests/build_tests.sh /work3/glfwd /work3/tests
   sh /glf/tests/run_tests.sh /work3/glfwd /work3/glfwd/glhost
   sh /glf/tests/t_transmissao.sh /work3/glfwd /work3/glfwd/glhost
   ```
4. Rodar a HMI com o lançador da bancada (`../driver/m1_hmi.sh`):
   ```sh
   LOW=/mnt/low BIN=/work3/bin OUT=/work3/out TAG=g VARIANT=real SECS=100 \
   GLDIR=/work3/glfwd GLHOST=/work3/glfwd/glhost GLW=800 GLH=480 \
   DMW=800 DMH=480 DSISTUB=1 POWER=1 sh /driver/m1_hmi.sh
   ```
   Os quadros saem em `$OUT/frames/` (`latest.ppm` e os primeiros 40 numerados). Para ver no Mac: `ffmpeg -i latest.ppm latest.png`.

## Ver ao vivo

```sh
sh referencia/ferramentas/mib3-sandbox/driver/ao_vivo.sh        # 15 minutos; passe os segundos para outro tempo
```

O script roda no Mac, de qualquer pasta. Ele sobe o container (compila tudo lá dentro, não precisa de pasta de trabalho), serve a pasta `../ao-vivo/` em http://127.0.0.1:8431 e abre o navegador. A primeira tela leva cerca de um minuto. `Ctrl-C` encerra; de outro terminal, `docker stop moi3-ao-vivo`.

Desde o Marco 4 o script liga a camada RSI (registro e mock de serviços do firmware). A HMI mostra a tela Welcome terminada por uns 5 segundos, depois um popup por 30 segundos, e para na área principal ("Radio: função indisponível"). Daí em diante nada muda na tela, então a página fica em "Sem quadro novo há N s". Para ver a tela animada "Loading user profile…" de antes:

```sh
RSI= sh referencia/ferramentas/mib3-sandbox/driver/ao_vivo.sh
```

Como funciona: a cada troca de buffers o glhost grava `frames/latest.ppm` e `frames/seq.txt` (número do quadro, largura, altura), os dois por troca de nome, para nunca aparecer arquivo pela metade. A página `../ao-vivo/index.html` consulta o `seq.txt` 12 vezes por segundo e só busca o quadro quando o número muda. Ela mostra se está ao vivo, há quanto tempo não chega quadro novo (a HMI só desenha quando algo muda na tela), o ritmo e tem um botão para salvar o quadro em PNG.

Desde 11/10/2026 há uma segunda forma de ver ao vivo, sem navegador: a janela do Mac, que recebe a transmissão direta (seção abaixo) e mostra todos os quadros, e não um em cada quatro. Abre com `sh referencia/ferramentas/mib3-sandbox/janela/abre.sh`; detalhes em [../janela/README.md](../janela/README.md). As duas funcionam ao mesmo tempo.

Desde o Marco 5 a página também manda entrada: clicar na tela é tocar, arrastar é rolar, e embaixo da tela ficam as teclas da central e os dois botões giratórios (rolar em cima gira, clicar aperta). A tecla Menu abre a grade de menus. No modo ao vivo a partida da HMI leva cerca de 1 minuto; se a tecla vier antes de a parte do menu subir, a tela fica em "Starting…" até lá. Isso não passa pelo OpenGL: a página manda cada evento ao `../driver/servidor_ao_vivo.py`, que o grava num arquivo de comandos, e o `../driver/dsistub.c` o publica no barramento da central. Detalhes em [../modelagem-soc.md](../modelagem-soc.md), seção do Marco 5.

## Transmissão direta dos quadros

Desde 11/10/2026 o glhost também manda cada quadro por um soquete Unix, no instante em que a HMI o desenha. É o que alimenta a janela do Mac ([../janela/](../janela/README.md)). A gravação em arquivo continua igual, para a página do navegador.

- **Onde.** O soquete fica em `/tmp/moi3-quadros.sock`, no sistema de arquivos do container (fora do chroot do firmware). `GLHOST_STREAM=<caminho>` muda o lugar; vazio ou `0` desliga. Vem ligado por padrão, então o `../driver/ao_vivo.sh` já sobe com ele sem mudança nenhuma.
- **Formato.** Cada mensagem tem 20 bytes de cabeçalho, em little-endian, e a carga:

  | Campo | Tamanho | O que é |
  |---|---|---|
  | `MQ01` | 4 | marca |
  | número do quadro | 4 | o mesmo do `seq.txt` |
  | largura, altura | 2 + 2 | em pixels |
  | marcas | 2 | bit 0 = quadro-chave: zerar a imagem antes de aplicar |
  | reserva | 2 | zero |
  | bytes da carga | 4 | |
  | carga | | trechos `[pular u32][copiar u32][copiar × 3 bytes RGB]`, em pixels, de cima para baixo |

  O espectador guarda a imagem e aplica os trechos: só viaja o que mudou desde o último quadro mandado **para ele**. O primeiro quadro de cada espectador é um quadro-chave, e quem liga com a tela parada recebe na hora o último quadro desenhado. Quadro igual ao anterior vai só com o cabeçalho.
- **Sem segurar a HMI.** Uma thread cuida só da transmissão. A de desenho entrega o quadro e segue. Espectador lento ou parado não atrasa a HMI nem os outros espectadores; ele só perde os quadros do meio do caminho, e o próximo que receber já traz tudo o que mudou. Cabem 4 espectadores.
- **Atravessar o container.** O container roda sem rede. A `ponte.c` é chamada por `docker exec -i <container> ponte <soquete> <arquivo de comandos>`: copia a transmissão para a saída padrão e acrescenta no arquivo de comandos cada linha válida que chega pela entrada padrão (`tecla`, `gesto`, `encoder`, a mesma regra do `../driver/servidor_ao_vivo.py`). Termina quando a entrada fecha ou quando o glhost sai. No modo ao vivo o arquivo de comandos é `/mnt/low/tmp/dsi-entrada`, o que o `../driver/dsistub.c` acompanha. **A ponte abre esse arquivo a cada linha e nunca o cria** (correção de 11/10/2026, 22h10): a janela liga antes de o lançador pôr o arquivo de verdade naquele caminho, e uma ponte que o abrisse ao ligar ficava gravando num arquivo que ninguém lia. Linha que chega sem arquivo no lugar é descartada.

Medido em 11/10/2026 numa central de teste (800×480, do container ao Mac pelo `docker exec`): o quadro-chave da tela "Radio: função indisponível" tem 21,5 kB, contra 1,15 MB do arquivo PPM; na troca para a grade de menus chegaram 40 quadros com 20 ms entre um e outro (50 por segundo), em média 37 kB cada e 433 kB o maior. O primeiro quadro chega ao Mac entre 0,08 e 0,21 s depois de chamar o `docker exec`.

## Testes

Os testes foram escritos antes do encaminhador. Contra o stub de 10/10 todos falham (limpar e ler de volta devolve lixo, a consulta de atributos do shader vem errada). Contra o encaminhador passam os oito:

| Teste | O que prova |
|---|---|
| `t_clear` | EGL sobe, a superfície tem o tamanho pedido, limpar e ler de volta devolve a cor |
| `quadro_em_arquivo` | o quadro gravado pelo glhost na troca de buffers tem o que foi desenhado |
| `contador_de_quadros` | o `seq.txt` que o visualizador ao vivo acompanha traz o número do quadro e o tamanho |
| `t_shader` | compilar, ligar, `glGetActiveAttrib`/`glGetActiveUniform` (onde a HMI parava), desenho com VAO, VBO e índices |
| `t_texture` | envio de pixels com alinhamento de linha, `glTexSubImage2D`, `glTexStorage2D` |
| `t_buffers` | `glMapBufferRange` de escrita, de leitura e parcial; `glBufferSubData` |
| `t_fbo` | renderização em textura, `glBlitFramebuffer`, leitura do FBO |
| `t_getters` | consultas que devolvem 1, 2, 4 ou N valores sem escrever além do devido |

Além disso: o shim exporta os 391 símbolos do Adreno que a HMI usa (conferido com `nm`) e carrega sob a glibc 2.23 do firmware com `LD_BIND_NOW=1`.

Os testes da transmissão (`t_transmissao.sh`) também foram escritos antes: contra o glhost de antes, 14 das 16 conferências falhavam (as duas que passavam são as que só acusam envio que trava). Com o cenário da ponte que liga cedo, acrescentado na correção de 22h10, são 19, e passam todas:

| Cenário | O que prova |
|---|---|
| `primeiro_quadro` | quem já está ligado recebe o quadro inteiro assim que ele é desenhado, e a imagem é, byte a byte, a que o glhost gravou |
| `quem_chega_depois` | quem liga com a tela parada, e com o cliente de OpenGL já fora, recebe o último quadro sem esperar desenho novo |
| `so_o_que_mudou` | depois do primeiro quadro (2.457.608 bytes em 1280×640), um retângulo de 40×30 custa 3.840 bytes e um quadro igual ao anterior, zero; a imagem montada dos três é a que o glhost gravou |
| `espectador_parado` | um espectador que liga e não lê nada não atrasa os 120 quadros de tela cheia nem impede outro espectador de receber o último |
| `ponte` | o quadro atravessa pela saída padrão; da entrada padrão só as linhas de comando válidas chegam ao arquivo; sem soquete, sai com o código 3 |
| `ponte_liga_cedo` | a ponte que liga antes de o arquivo de comandos existir não o cria; a linha seguinte chega ao arquivo que apareceu depois; e, se outro arquivo toma o lugar no mesmo caminho, a linha vai para o novo. Contra a ponte antiga as três conferências falhavam |

Prova de que o cenário do espectador parado acusa o defeito: trocando o envio por um que espera (sem `MSG_DONTWAIT`), as duas conferências do "outro espectador" falham.

## O que a HMI usa de verdade

Medido em 60 s até a tela "Welcome", 331 quadros: **51.484 chamadas, 62 operações distintas** (55 funções GL e 7 de EGL). É praticamente OpenGL ES 2.0, mais `glGetProgramBinary` e `glProgramParameteri`. O contexto que ela pede é ES 2.0. Lista completa em `../traces/m3_g5.glhost.log`.

## Limites conhecidos

- Vetores de vértice na memória do cliente (sem VBO) não são suportados; o shim avisa se aparecerem. A HMI não usou até aqui.
- Um mapeamento de buffer é identificado pelo buffer ligado no alvo na hora do `glMapBufferRange`.
- O glhost executa o que o cliente mandar, sem validar. O cliente é o firmware: rodar só no container descartável, sem rede.
- Sem aceleração: é OpenGL por software. Para a HMI em 800×480 sobra.
- A transmissão não comprime além de pular o que não mudou: uma troca de tela inteira custa perto de 1,15 MB num quadro só. Pelo `docker exec` isso não atrasou (medido), mas não foi testado por um caminho lento.
- A ponte e a página do navegador gravam no mesmo arquivo de comandos por caminhos diferentes (a ponte de dentro do container, o servidor da página de fora). Não foi testado se linhas mandadas pelas duas no mesmo instante podem se misturar.
- Foram exercitadas a tela "Welcome" (carregando e terminada), um popup, o aviso de função indisponível e, no Marco 5, a grade de menus e as telas de veículo, clima, som, ajustes e o aviso de navegação. Em nenhuma o glhost registrou erro de OpenGL (rodadas `e8`, `k1` a `k8`, `t1`, `t2`, `n1`, `n2`). Numa sessão de uso livre do Lipe passaram ainda 16 telas diferentes (subtelas de ajustes, versões, usuários, atualização de software, assistente de configuração) e popups, também sem erro. Texturas comprimidas, mais de um contexto e as telas com mapa ainda não passaram por aqui.
