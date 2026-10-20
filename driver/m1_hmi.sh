#!/bin/sh
# Sessão de modelagem do SoC (11/10/2026): lançador ÚNICO da HMI do MOI3, igual no container
# (chroot sob o kernel do OrbStack) e dentro do QEMU -M virt (chamado pelo /init do initramfs).
# Assim os dois ambientes rodam exatamente a mesma coisa e o resultado é comparável.
# POSIX sh: roda no dash do Ubuntu e no ash do busybox.
#
# Quem chama já montou:  $LOW = partição system (ro)   e   $LOW/vw = partição da HMI (ro)
# Variáveis:
#   LOW      raiz do firmware                      (default /mnt/low)
#   BIN      ferramentas do m1_build.sh            (default /work/bin)
#   OUT      pasta de saída                        (default /out)
#   TAG      prefixo dos arquivos de saída         (default m1)
#   VARIANT  manual = linha da JVM montada à mão, como em 10/10 (run_hmi11.sh)
#            real   = o /vw/hmi/runHMI.sh do próprio firmware (opções opengles.cq.* etc.)
#   SECS     quanto tempo deixar a HMI rodar       (default 70)
#   GLDIR    pasta com libEGL_adreno.so/libGLESv2_adreno.so substitutos (default $BIN/fakegl)
#   DMW/DMH  geometria anunciada pelo dmstub       (default 1920x720, a de 10/10)
#   WAIT     espera entre os daemons e a HMI       (default 4)
#   DSISTUB  se definido, usa o dsistub (responde aos pedidos de display e publica o estado de
#            energia $POWER, default 1 = ligado) no lugar do dmstub de 10/10
#   GLHOST   binário do glhost (Marco 3); GLW/GLH = tamanho da janela (default 1280x640);
#            GLFRAMES = pasta dos quadros (default $OUT/frames)
#   RSI      se definido, sobe a camada RSI com o que o próprio firmware traz: o registro de
#            serviços (/lge/app_ro/sai/MIB_ServiceRegistryApp-00, porta 443, onde a HMI o procura)
#            e o mock de serviços da LG (/lge/app_ro/bin/rsi-mockup, porta 7770)
#   RSI_SERVICES  serviços a acrescentar à lista do mock, "nome:categoria" separados por espaço
#   RSI_DATA      pasta com dados nossos para o mock (<serviço>/<recurso>.json), por cima dos da LG
#   INPUT    arquivo de comandos de entrada (teclas e toque) que o dsistub acompanha; quem
#            escreve nele pode estar fora do container. Formato no cabeçalho do dsistub.c
#   ROTEIRO  entrada com hora marcada, "espera comando" separados por ponto e vírgula; a espera
#            é em segundos desde o passo anterior (o primeiro, desde a partida da HMI). Exemplo:
#            ROTEIRO="85 tecla 1 116 1;0.2 tecla 1 116 0" aperta e solta HOME aos 85 s
#   HMILOG   liga o registro detalhado da própria HMI. Sem isso ela só imprime INFO para cima.
#            Com HMILOG=normal passam a sair também as mensagens de nível NORMAL, com o nome do
#            classificador por extenso. Além disso aceita chaves de rastreamento (TRACE), separadas
#            por espaço, como estão no /vw/hmi/config/logging.properties
#            (ex.: "APPS.LIFECYCLE ASL_SYSTEM.KEYPANEL"); no firmware vêm todas desligadas.
#            As chaves valem a sessão inteira. Até 12/10/2026 paravam uns 6 s depois da partida:
#            na carga tardia a HMI troca as chaves do logging.properties pelas do perfil de sondas
#            de depuração (/vw/hmi/config/debugprobes_*.config, seção LOGMANAGER), que no firmware
#            não liga nenhuma. Agora as chaves pedidas entram também nessa seção, em todos os
#            perfis. O arquivo de saída passa a crescer enquanto a HMI roda. No fim o resumo
#            "registro detalhado:" diz quantas linhas TRACE saíram e que chaves a HMI aplicou
#   ESPERA   quanto a HMI espera, em ms, pelos serviços de cada etapa da partida. Sem isso vale
#            o padrão dela, 15000. Na bancada os serviços ausentes nunca chegam: com 2000 a
#            partida cai de 295 s para 61 s e a lista de ausentes fica igual (medido em 11/10/2026)
#   CODING   arquivos com os conjuntos de codificação e adaptação (Marco 6), separados por
#            espaço e lidos em ordem: o de depois vale sobre o de antes (o padrão e os ajustes).
#            O diagstub faz o papel do microcontrolador de diagnóstico e os publica antes de os
#            daemons e a HMI subirem; formato no cabeçalho do diagstub.c, arquivos prontos em
#            stubs/codificacao/. Sem isso a HMI sobe sem codificação nenhuma, como até o Marco 5.
#            Arquivo com erro: nada sobe, e sai o arquivo e a linha do erro
#   DIAGCAB  os três bytes de cabeçalho que o diagstub põe em cada conjunto, em hexa colado
#            (opção -k dele). Sem isso vale o padrão do diagstub, 83 60 22, que é o que o
#            navi-al do firmware aceita. DIAGCAB=000000 é o cabeçalho de antes de 11/10/2026:
#            serve para rodada de controle. A HMI e o persistence-app não leem esses bytes
#   LSMVAL   arquivo com que a memória de estado da central começa (/data/lge/hmi/lsmval): uma
#            linha por valor, "0 <namespace> <chave> <bytes em decimal>", como o persistence-app
#            grava. Serve para a HMI já nascer com o que no carro ficaria guardado, como o idioma
#            (stubs/persistencia/). Sem isso a memória nasce vazia e a HMI sobe em inglês.
#            Quem confere o arquivo é o lsmval_prepara.sh, que tem de estar na pasta deste
#            roteiro (o m1_qemu.sh não o leva); arquivo com erro: nada sobe, e sai a linha do erro
#   VEICULO  liga o "carro de mentira" (Marco 8): o veiculostub ocupa o lugar do carifservice do
#            firmware e responde aos serviços DSI de veículo (relógio, computador de bordo,
#            estados, clima…). O valor é a lista dos arquivos de perfil, separados por espaço e
#            lidos em ordem (o de depois vale sobre o de antes); "-" liga sem perfil nenhum, só
#            com o valor padrão marcado como válido. Perfis prontos em stubs/veiculo/; formato no
#            cabeçalho do veiculostub.c. Perfil com erro: nada sobe, e sai o arquivo e a linha.
#            Desde o Marco 9 ele atende também pedidos diretos: os que gravam um atributo, nos
#            serviços que o perfil lista em "grava", e os que têm resposta com seção no perfil.
#            Com VEICULO o carifservice de verdade NÃO sobe: ele responde às mesmas assinaturas
#            com os dados de "sem carro" e as duas respostas brigariam. VEICULO_CARIF=1 sobe os
#            dois, para comparar. VEICULO_ESQUEMA troca o esquema (default o da pasta deste
#            roteiro: ../stubs/veiculo/gerado/esquema.txt)
#            VEICULO_ESTADO é o arquivo, fora do chroot, em que o carro de mentira guarda o que a
#            tela grava nele (a hora acertada, as unidades), para a partida seguinte nascer assim,
#            como no carro, em que quem guarda é o painel (opção -e do veiculostub; o formato está
#            no cabeçalho dele). A pasta do arquivo é criada, se faltar, e vai montada dentro do
#            chroot: a gravação é ao lado, com troca de nome. Sem isso, que é o padrão deste
#            roteiro, nada fica guardado e toda partida nasce dos perfis. Arquivo com erro não
#            impede a partida: vale só o perfil, e a linha "veículo" diz "estado: RECUSADO"
#   RADIO    liga o "rádio de mentira" (Marco 11): o radiostub ocupa o lugar do radio-hmi do
#            firmware, que a bancada não sobe, e responde aos serviços do sintonizador
#            (DSIUnifiedTuner, DSITunerAnnouncement e DSIRadioData, domínio PID_TUNER_HMI). Sem
#            ele ninguém anuncia esse domínio e a tela do rádio fica no aviso "função
#            indisponível". O valor é a lista dos arquivos de perfil, separados por espaço e lidos
#            em ordem; "-" liga sem perfil nenhum: a tela abre, no começo da faixa e com a lista
#            de emissoras vazia. Perfil pronto em stubs/radio/ (emissoras inventadas, sem recepção
#            nem som); o que o programa faz está no cabeçalho do radiostub.c. Perfil com erro:
#            nada sobe, e sai o arquivo e a linha. RADIO_ESQUEMA troca o esquema (default o da
#            pasta deste roteiro: ../stubs/radio/gerado/esquema.txt)
#   TELEFONE liga o "telefone de mentira" (Marco 12): o mesmo programa do carro de mentira, o
#            veiculostub, com o esquema dos serviços de telefone e conectividade, ocupa o lugar dos
#            processos de Bluetooth e de gerência de aparelhos do firmware, que a bancada não sobe
#            (domínios PID_BT, 256: DSIBluetooth, DSIMobileEquipment, agenda e mensagens; e
#            PID_DCMANAGER, 4096: DSIPhoneManager). Sem ele ninguém anuncia esses domínios, e a
#            tecla Telefone mostra o aviso "Telefone indisponível.". O valor é a lista dos arquivos
#            de perfil, separados por espaço e lidos em ordem; "-" liga sem perfil nenhum (o
#            Bluetooth sai "ligado", mas sem estado de aparelho a tela do telefone não abre).
#            Perfil pronto em stubs/telefone/: sem-aparelho.txt abre a tela "nenhum aparelho
#            conectado". Não há Bluetooth nem aparelho de verdade: são só os estados que a tela
#            mostra. Desde 12/10/2026 quem sobe é o driver/telefonestub.c, o veiculostub mais um
#            modelo: com aparelho-conectado.txt por cima há um aparelho FICTÍCIO, que a tela
#            conecta (tocando nele em "Selecionar celular") e desconecta (desligando o Bluetooth);
#            com comeca-desconectado.txt por cima dos dois ele começa conhecido e não conectado. Perfil com erro: nada sobe, e sai o arquivo e a linha. TELEFONE_ESQUEMA troca
#            o esquema (default o da pasta deste roteiro: ../stubs/telefone/gerado/esquema.txt)
#   MIDIA    liga a "mídia de mentira" (Marco 15): o midiastub ocupa o lugar do programa de mídia
#            do firmware, que a bancada não sobe, e responde aos serviços de mídia (DSIMediaBase,
#            DSIMediaPlayer, DSIMediaBrowser e DSIFileBrowser, domínio PID_MEDIA, 0x800000). Sem
#            ele ninguém anuncia esse domínio e a tecla Mídia mostra o aviso "função indisponível".
#            O valor é a lista dos arquivos de perfil, separados por espaço e lidos em ordem.
#            Perfil pronto em stubs/midia/: uma fonte, "Minhas mídias", com cinco faixas inventadas;
#            não há aparelho nem som. O que o programa faz está no cabeçalho do midiastub.c. Sem
#            fonte ou sem faixa no perfil o programa recusa. Perfil com erro: nada sobe, e sai o
#            arquivo e a linha. MIDIA_ESQUEMA troca o esquema (default o da pasta deste roteiro:
#            ../stubs/midia/gerado/esquema.txt)
#   SOM      liga o "som de mentira" (12/10/2026). O som desta HMI não vem pelo barramento DSI: a
#            tela de som, a barra de volume e o mudo falam com o serviço RSI "audiomanagement". Com
#            SOM o mock RSI da LG passa a servir esse serviço, com os dados de stubs/som/gerado
#            (o que o gerente de áudio do firmware serve em repouso; driver/som_tabela.sh), e o
#            somstub põe por cima o perfil e as regras mínimas: a tela de som sai do aviso
#            "A configuração do som estará disponível novamente…", os ajustes pegam, e o volume
#            muda com o botão, com as teclas do volante e com o mudo. Não há áudio: é o estado
#            que a tela mostra. O valor é a lista dos arquivos de perfil, separados por espaço e
#            lidos em ordem; "-" liga sem perfil nenhum (o serviço responde, mas sem saída de som
#            ligada a tela segue no aviso). Perfil pronto: stubs/som/som-de-mentira.txt. Precisa
#            de RSI. Perfil com erro: nada sobe, e sai o arquivo e a linha. Com SOM a HMI desliga
#            o substituto de áudio que tem dentro dela ("DUMMY RSI AMT DEACTIVATED") e passa a
#            pedir ao serviço a conexão de áudio de cada fonte. SOM_DADOS troca a pasta dos dados
#            (default a da pasta deste roteiro: ../stubs/som/gerado)
#            SOM_ESTADO é o arquivo, fora do chroot, em que o som de mentira guarda o que a tela
#            mudar (volume, grave, médio, agudo, balanço, fader e o mudo), para a partida seguinte
#            nascer assim (opção -m do somstub; o formato está no cabeçalho dele). A pasta do
#            arquivo é criada, se faltar, e vai montada dentro do chroot: a gravação é ao lado,
#            com troca de nome. Sem isso, que é o padrão deste roteiro, nada fica guardado e toda
#            partida nasce do perfil. Linha estragada do arquivo é ignorada com aviso; a central
#            sobe do mesmo jeito
#   JCPUS    quantos processadores a JVM da HMI enxerga (default 4, os do SoC do carro; 0 = não
#            mexe). Com os 16 do Mac o cliente WebSocket do RSI nem inicia ("Insufficient
#            configured threads ... QueuedThreadPool[RSI_WS]"); com 4 inicia (medido em 11/10/2026).
LOW=${LOW:-/mnt/low}; BIN=${BIN:-/work/bin}; OUT=${OUT:-/out}; TAG=${TAG:-m1}
VARIANT=${VARIANT:-manual}; SECS=${SECS:-70}; GLDIR=${GLDIR:-$BIN/fakegl}
DMW=${DMW:-1920}; DMH=${DMH:-720}; WAIT=${WAIT:-4}; JCPUS=${JCPUS:-4}
[ "$JCPUS" != 0 ] && EXTRA_VMOPTIONS="-XX:ActiveProcessorCount=$JCPUS $EXTRA_VMOPTIONS"
# propriedades da própria HMI (de.vw.mib.asl.framework.internal.startupv7r.workunits.ContextInformationWorkUnit)
[ -n "$ESPERA" ] && EXTRA_VMOPTIONS="-Dde.vw.mib.asl.startup.timeout.dsi=$ESPERA -Dde.vw.mib.asl.startup.timeout.rsi=$ESPERA $EXTRA_VMOPTIONS"
# PATH completo: dentro do chroot os utilitários do firmware estão em /usr/bin (o initramfs só tem /bin)
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
mkdir -p "$OUT"
echo "[m1_hmi] kernel=$(uname -r) maq=$(uname -m) variante=$VARIANT secs=$SECS tag=$TAG"
[ -x "$LOW/lge/hmi/jvm/bin/java" ] || { echo "[m1_hmi] ERRO: $LOW não é a partição system"; exit 2; }
[ -f "$LOW/vw/hmi/runHMI.sh" ]     || { echo "[m1_hmi] ERRO: $LOW/vw não é a partição da HMI"; exit 2; }

# camada gravável por tmpfs (overlay não monta aqui; lição de 10/10)
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d || exit 3; done
mkdir -p $LOW/dev/shm $LOW/dev/pts; mount -t tmpfs tmpfs $LOW/dev/shm
mount -t proc proc $LOW/proc
mknod -m 666 $LOW/dev/null c 1 3; mknod -m 666 $LOW/dev/zero c 1 5
mknod -m 666 $LOW/dev/urandom c 1 9; mknod -m 666 $LOW/dev/random c 1 8
mkdir -p $LOW/var/lib/hmi $LOW/var/lib/nav/coredump $LOW/var/cache/hmi $LOW/media/mp000 \
         $LOW/run/platform/weston $LOW/data/lge/hmi $LOW/data/lge/persistency $LOW/tmp/fakegl
# Memória de estado (Marco 6): é em /data/lge/hmi/lsmval que o persistence-app guarda o que a
# HMI manda persistir (o idioma escolhido, por exemplo), e é de lá que a HMI lê na partida. No
# carro o arquivo sobrevive ao desligar; aqui /data é memória e nasce vazio, salvo com LSMVAL.
# O lsmval_prepara.sh confere o arquivo de partida e tira comentários e sobras de espaço: o
# firmware descarta em silêncio a linha que não entende. Arquivo com erro: nada sobe.
if [ -n "$LSMVAL" ]; then
  sh "$(dirname "$0")/lsmval_prepara.sh" "$LSMVAL" > $LOW/data/lge/hmi/lsmval \
    || { echo "[m1_hmi] ERRO: memória de estado recusada ($LSMVAL); nada sobe"; exit 4; }
  echo "[m1_hmi] memória de estado de $LSMVAL: $(wc -l < $LOW/data/lge/hmi/lsmval) linha(s)"
else
  : > $LOW/data/lge/hmi/lsmval
fi
STUB=dmstub; [ -n "$DSISTUB" ] && STUB=dsistub
cp "$BIN/kipc_hub" "$BIN/kipc_bus.so" "$BIN/$STUB" $LOW/tmp/
cp "$GLDIR"/*.so $LOW/tmp/fakegl/
[ "$VARIANT" = real ] && cp $LOW/vw/preload/* $LOW/var/lib/hmi/ 2>/dev/null

# Registro detalhado da HMI. O console dela descarta NORMAL e TRACE, salvo com a propriedade
# console.traces.enabled (lido em de.vw.mib.log4mib.writer.ConsoleLogWriter). As chaves de TRACE
# vão numa cópia do logging.properties com elas em "true", montada por cima do original (que
# fica na partição /vw, só leitura).
# O logging.properties só vale até a carga tardia, uns 6 s depois de "UI thread started". Aí a
# sonda de depuração LOGMANAGER é ativada e troca TODAS as chaves pelas da seção "BEGIN LOGMANAGER
# … END LOGMANAGER" do perfil de sondas em uso (LogManagerProbe.applySettings chama
# MainLogger.setLoggingProperties, que registra "Changed logging properties to: {…}"). O perfil é
# um dos /vw/hmi/config/debugprobes_NNN_*.config, escolhido pelo número guardado na memória de
# estado: de fábrica o 130 (-DdebugProbes.default, no runHMI.sh), que não liga chave nenhuma. Por
# isso as mesmas chaves entram também na seção LOGMANAGER de todos os perfis, em cópias montadas
# por cima (12/10/2026; modelagem-soc.md, "O rastreamento da HMI que parava na carga tardia").
# Na variante manual a HMI não recebe a pasta dos perfis, a sonda não é ativada e nada é trocado.
if [ -n "$HMILOG" ]; then
  EXTRA_VMOPTIONS="-Dde.vw.mib.log4mib.console.traces.enabled=true -Dde.vw.mib.log4mib.console.textual.classifiers.enabled=true $EXTRA_VMOPTIONS"
  LP=$LOW/vw/hmi/config/logging.properties
  cp $LP $LOW/tmp/logging.properties
  HLK=
  for k in $HMILOG; do
    [ "$k" = normal ] && continue
    if grep -q "^$k = " $LOW/tmp/logging.properties; then sed -i "s|^$k = .*|$k = true|" $LOW/tmp/logging.properties; HLK="$HLK$k = true\\n"
    else echo "[m1_hmi] HMILOG: chave desconhecida $k"; fi
  done
  mount -o bind $LOW/tmp/logging.properties $LP || exit 3
  if [ -n "$HLK" ]; then
    mkdir -p $LOW/tmp/hmilog
    for f in $LOW/vw/hmi/config/debugprobes_*.config; do
      [ -f "$f" ] || continue
      grep -q '^END LOGMANAGER' "$f" || { echo "[m1_hmi] HMILOG: $(basename "$f") não tem seção LOGMANAGER; com esse perfil as chaves param na carga tardia"; continue; }
      c=$LOW/tmp/hmilog/$(basename "$f")
      sed "s|^END LOGMANAGER|${HLK}END LOGMANAGER|" "$f" > "$c" && mount -o bind "$c" "$f" || exit 3
    done
  fi
fi

# Marco 3: se GLHOST for dado, GLDIR aponta para o encaminhador (glforward-moi3) e o glhost
# sobe aqui, FORA do chroot, escutando num socket que a HMI enxerga como /tmp/moi3-gl.sock.
if [ -n "$GLHOST" ]; then
  mkdir -p "${GLFRAMES:-$OUT/frames}"
  "$GLHOST" $LOW/tmp/moi3-gl.sock ${GLW:-1280} ${GLH:-640} "${GLFRAMES:-$OUT/frames}" > "$OUT/$TAG.glhost.log" 2>&1 &
  i=0; while [ ! -S $LOW/tmp/moi3-gl.sock ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
  export MOI3_GL_SOCKET=/tmp/moi3-gl.sock
fi

chroot $LOW /tmp/kipc_hub 2>$LOW/tmp/hub.log &
sleep 1

# Codificação e adaptação (Marco 6). No carro quem manda é o microcontrolador, logo na partida;
# a HMI só aceita esses conjuntos antes da primeira leitura de persistência. O diagstub os manda
# agora, antes de todo mundo, e o hub os guarda e entrega a cada processo que se ligar.
if [ -n "$CODING" ]; then
  cp "$BIN/diagstub" $LOW/tmp/ && mkdir -p $LOW/tmp/codificacao || exit 3
  CODARGS=; n=0
  for f in $CODING; do
    n=$((n+1)); b=$n-$(basename "$f")
    cp "$f" $LOW/tmp/codificacao/$b || { echo "[m1_hmi] ERRO: não li o arquivo de codificação $f"; exit 4; }
    CODARGS="$CODARGS /tmp/codificacao/$b"
  done
  chroot $LOW /tmp/diagstub ${DIAGCAB:+-k} ${DIAGCAB:+"$DIAGCAB"} /run/kipc_hub.sock $CODARGS >$LOW/tmp/diagstub.log 2>&1 \
    || { echo "[m1_hmi] ERRO: a codificação ($CODING) não foi aceita:"; cat $LOW/tmp/diagstub.log; exit 4; }
  echo "[m1_hmi] codificação ($CODING): $(tail -n 1 $LOW/tmp/diagstub.log)"
fi

# Dados de veículo (Marco 8). O anúncio do domínio tem de estar no ar desde a partida da HMI:
# ela assina os atributos ao subir e não volta a assinar se o serviço aparecer depois (medido em
# 11/10/2026). Por isso o veiculostub sobe aqui, antes dos daemons e da HMI. Antes de ficar no
# barramento ele confere esquema e perfis (-n); com erro, nada sobe.
if [ -n "$VEICULO" ]; then
  VESQ=${VEICULO_ESQUEMA:-$(dirname "$0")/../stubs/veiculo/gerado/esquema.txt}
  cp "$BIN/veiculostub" $LOW/tmp/ && mkdir -p $LOW/tmp/veiculo || exit 3
  cp "$VESQ" $LOW/tmp/veiculo/esquema.txt || { echo "[m1_hmi] ERRO: não li o esquema dos serviços de veículo $VESQ"; exit 4; }
  VEIARGS=; n=0
  for f in $VEICULO; do
    [ "$f" = - ] && continue
    n=$((n+1)); b=$n-$(basename "$f")
    cp "$f" $LOW/tmp/veiculo/$b || { echo "[m1_hmi] ERRO: não li o perfil de veículo $f"; exit 4; }
    VEIARGS="$VEIARGS /tmp/veiculo/$b"
  done
  # O que a tela grava no carro de mentira (ver VEICULO_ESTADO, no cabeçalho). O stub grava ao
  # lado e troca o nome: precisa da pasta, e não só do arquivo, do lado de dentro do chroot. Se a
  # pasta não montar, a central sobe do mesmo jeito, só que sem guardar.
  VEIEST=
  if [ -n "$VEICULO_ESTADO" ]; then
    if mkdir -p "$(dirname "$VEICULO_ESTADO")" $LOW/tmp/veiculo-estado && mount -o bind "$(dirname "$VEICULO_ESTADO")" $LOW/tmp/veiculo-estado; then
      VEIEST=/tmp/veiculo-estado/$(basename "$VEICULO_ESTADO")
    else
      echo "[m1_hmi] AVISO: não montei a pasta de $VEICULO_ESTADO; nesta sessão o carro de mentira não guarda o que a tela gravar"
    fi
  fi
  chroot $LOW /tmp/veiculostub ${VEIEST:+-e} ${VEIEST:+"$VEIEST"} -n /tmp/veiculo/esquema.txt $VEIARGS >$LOW/tmp/veiculostub.log 2>&1 \
    || { echo "[m1_hmi] ERRO: o perfil de veículo ($VEICULO) não foi aceito:"; cat $LOW/tmp/veiculostub.log; exit 4; }
  echo "[m1_hmi] veículo ($VEICULO): $(tail -n 1 $LOW/tmp/veiculostub.log | sed 's/^\[veiculostub\] //')"
  chroot $LOW /tmp/veiculostub ${VEIEST:+-e} ${VEIEST:+"$VEIEST"} /run/kipc_hub.sock /tmp/veiculo/esquema.txt $VEIARGS >>$LOW/tmp/veiculostub.log 2>&1 &
fi

# Rádio de mentira (Marco 11). Como no veículo, o anúncio do domínio tem de estar no ar antes de a
# HMI subir, então o radiostub sobe aqui. Antes confere esquema e perfis (-n); com erro, nada sobe.
# Se o programa não foi compilado (o m1_build.sh avisa quando isso acontece), a central sobe sem
# ele: a tela do rádio fica no aviso, e o resto não para por causa do rádio.
if [ -n "$RADIO" ] && [ ! -x "$BIN/radiostub" ]; then
  echo "[m1_hmi] AVISO: RADIO pedido, mas não há radiostub compilado em $BIN; a central sobe sem o rádio de mentira"
  RADIO=
fi
if [ -n "$RADIO" ]; then
  RESQ=${RADIO_ESQUEMA:-$(dirname "$0")/../stubs/radio/gerado/esquema.txt}
  cp "$BIN/radiostub" $LOW/tmp/ && mkdir -p $LOW/tmp/radio || exit 3
  cp "$RESQ" $LOW/tmp/radio/esquema.txt || { echo "[m1_hmi] ERRO: não li o esquema do sintonizador $RESQ"; exit 4; }
  RADARGS=; n=0
  for f in $RADIO; do
    [ "$f" = - ] && continue
    n=$((n+1)); b=$n-$(basename "$f")
    cp "$f" $LOW/tmp/radio/$b || { echo "[m1_hmi] ERRO: não li o perfil de rádio $f"; exit 4; }
    RADARGS="$RADARGS /tmp/radio/$b"
  done
  chroot $LOW /tmp/radiostub -n /tmp/radio/esquema.txt $RADARGS >$LOW/tmp/radiostub.log 2>&1 \
    || { echo "[m1_hmi] ERRO: o perfil de rádio ($RADIO) não foi aceito:"; cat $LOW/tmp/radiostub.log; exit 4; }
  echo "[m1_hmi] rádio ($RADIO): $(grep 'modelo do sintonizador' $LOW/tmp/radiostub.log | sed 's/^\[radiostub\] //')"
  chroot $LOW /tmp/radiostub /run/kipc_hub.sock /tmp/radio/esquema.txt $RADARGS >>$LOW/tmp/radiostub.log 2>&1 &
fi

# Telefone de mentira (Marco 12). Como no veículo e no rádio, o anúncio dos domínios tem de estar
# no ar antes de a HMI subir. O programa é o próprio veiculostub, com outro esquema e outros
# perfis; vai para o chroot com o nome telefonestub, para aparecer assim na lista de processos.
# Antes confere esquema e perfis (-n); com erro, nada sobe.
# Desde 12/10/2026 o programa é o telefonestub (driver/telefonestub.c): o veiculostub mais o modelo
# que conecta e desconecta o aparelho fictício. Se ele não compilou, a central sobe sem telefone.
if [ -n "$TELEFONE" ] && [ ! -x "$BIN/telefonestub" ]; then
  echo "[m1_hmi] AVISO: TELEFONE pedido, mas não há telefonestub compilado em $BIN; a central sobe sem o telefone de mentira"
  TELEFONE=
fi
if [ -n "$TELEFONE" ]; then
  TESQ=${TELEFONE_ESQUEMA:-$(dirname "$0")/../stubs/telefone/gerado/esquema.txt}
  cp "$BIN/telefonestub" $LOW/tmp/telefonestub && mkdir -p $LOW/tmp/telefone || exit 3
  cp "$TESQ" $LOW/tmp/telefone/esquema.txt || { echo "[m1_hmi] ERRO: não li o esquema do telefone $TESQ"; exit 4; }
  TELARGS=; n=0
  for f in $TELEFONE; do
    [ "$f" = - ] && continue
    n=$((n+1)); b=$n-$(basename "$f")
    cp "$f" $LOW/tmp/telefone/$b || { echo "[m1_hmi] ERRO: não li o perfil de telefone $f"; exit 4; }
    TELARGS="$TELARGS /tmp/telefone/$b"
  done
  chroot $LOW /tmp/telefonestub -n /tmp/telefone/esquema.txt $TELARGS >$LOW/tmp/telefonestub.log 2>&1 \
    || { echo "[m1_hmi] ERRO: o perfil de telefone ($TELEFONE) não foi aceito:"; cat $LOW/tmp/telefonestub.log; exit 4; }
  echo "[m1_hmi] telefone ($TELEFONE): $(grep -a 'modelo do telefone' $LOW/tmp/telefonestub.log | tail -n 1 | sed 's/^\[telefonestub\] //'); $(tail -n 1 $LOW/tmp/telefonestub.log | sed 's/^\[veiculostub\] //')"
  chroot $LOW /tmp/telefonestub /run/kipc_hub.sock /tmp/telefone/esquema.txt $TELARGS >>$LOW/tmp/telefonestub.log 2>&1 &
fi

# Mídia de mentira (Marco 15). Como no veículo, no rádio e no telefone, o anúncio do domínio tem de
# estar no ar antes de a HMI subir. Antes confere esquema e perfis (-n); com erro, nada sobe. Se o
# programa não foi compilado (o m1_build.sh avisa quando isso acontece), a central sobe sem ele: a
# tecla Mídia fica no aviso, e o resto não para por causa da mídia.
if [ -n "$MIDIA" ] && [ ! -x "$BIN/midiastub" ]; then
  echo "[m1_hmi] AVISO: MIDIA pedido, mas não há midiastub compilado em $BIN; a central sobe sem a mídia de mentira"
  MIDIA=
fi
if [ -n "$MIDIA" ]; then
  MESQ=${MIDIA_ESQUEMA:-$(dirname "$0")/../stubs/midia/gerado/esquema.txt}
  cp "$BIN/midiastub" $LOW/tmp/ && mkdir -p $LOW/tmp/midia || exit 3
  cp "$MESQ" $LOW/tmp/midia/esquema.txt || { echo "[m1_hmi] ERRO: não li o esquema de mídia $MESQ"; exit 4; }
  MIDARGS=; n=0
  for f in $MIDIA; do
    n=$((n+1)); b=$n-$(basename "$f")
    cp "$f" $LOW/tmp/midia/$b || { echo "[m1_hmi] ERRO: não li o perfil de mídia $f"; exit 4; }
    MIDARGS="$MIDARGS /tmp/midia/$b"
  done
  chroot $LOW /tmp/midiastub -n /tmp/midia/esquema.txt $MIDARGS >$LOW/tmp/midiastub.log 2>&1 \
    || { echo "[m1_hmi] ERRO: o perfil de mídia ($MIDIA) não foi aceito:"; cat $LOW/tmp/midiastub.log; exit 4; }
  echo "[m1_hmi] mídia ($MIDIA): $(grep 'modelo do tocador' $LOW/tmp/midiastub.log | sed 's/^\[midiastub\] //')"
  chroot $LOW /tmp/midiastub /run/kipc_hub.sock /tmp/midia/esquema.txt $MIDARGS >>$LOW/tmp/midiastub.log 2>&1 &
fi

# Som de mentira (12/10/2026). Quem serve o audiomanagement é o mock RSI, mais abaixo; aqui o
# somstub confere o perfil contra os dados e escreve em /tmp/som os dados com o perfil aplicado
# (-w), que é o que o mock vai ler: o mock recusa gravação na saída de som, então o perfil não
# tem como entrar depois. Isso acontece antes de qualquer coisa subir: com erro, nada sobe. Sem
# a camada RSI, ou sem o programa compilado, a central sobe sem som, com aviso.
if [ -n "$SOM" ] && [ -z "$RSI" ]; then
  echo "[m1_hmi] AVISO: SOM pede a camada RSI (RSI=1); a central sobe sem o som de mentira"; SOM=
fi
if [ -n "$SOM" ] && [ ! -x "$BIN/somstub" ]; then
  echo "[m1_hmi] AVISO: SOM pedido, mas não há somstub compilado em $BIN; a central sobe sem o som de mentira"; SOM=
fi
if [ -n "$SOM" ]; then
  SDAD=${SOM_DADOS:-$(dirname "$0")/../stubs/som/gerado}
  cp "$BIN/somstub" $LOW/tmp/ && mkdir -p $LOW/tmp/som-dados/audiomanagement || exit 3
  cp "$SDAD"/audiomanagement/*.json $LOW/tmp/som-dados/audiomanagement/ || { echo "[m1_hmi] ERRO: não li os dados do serviço de áudio em $SDAD"; exit 4; }
  SOMARGS=; n=0
  for f in $SOM; do
    [ "$f" = - ] && continue
    n=$((n+1)); b=$n-$(basename "$f")
    cp "$f" $LOW/tmp/som-dados/$b || { echo "[m1_hmi] ERRO: não li o perfil de som $f"; exit 4; }
    SOMARGS="$SOMARGS /tmp/som-dados/$b"
  done
  # O que a tela muda no som (ver SOM_ESTADO, no cabeçalho). O somstub grava ao lado e troca o
  # nome: precisa da pasta, e não só do arquivo, do lado de dentro do chroot. Se a pasta não
  # montar, a central sobe do mesmo jeito, só que sem guardar.
  SOMEST=
  if [ -n "$SOM_ESTADO" ]; then
    if mkdir -p "$(dirname "$SOM_ESTADO")" $LOW/tmp/som-estado && mount -o bind "$(dirname "$SOM_ESTADO")" $LOW/tmp/som-estado; then
      SOMEST=/tmp/som-estado/$(basename "$SOM_ESTADO")
    else
      echo "[m1_hmi] AVISO: não montei a pasta de $SOM_ESTADO; nesta sessão o som de mentira não guarda o que a tela mudar"
    fi
  fi
  chroot $LOW /tmp/somstub -w /tmp/som ${SOMEST:+-m} ${SOMEST:+"$SOMEST"} /tmp/som-dados $SOMARGS >$LOW/tmp/somstub.log 2>&1 \
    || { echo "[m1_hmi] ERRO: o perfil de som ($SOM) não foi aceito:"; cat $LOW/tmp/somstub.log; exit 4; }
  echo "[m1_hmi] som ($SOM): $(grep 'perfil aplicado' $LOW/tmp/somstub.log | sed 's/^\[somstub\] //')"
  [ -n "$SOMEST" ] && echo "[m1_hmi] som, memória ($SOM_ESTADO): $(grep -c 'memória: ignorei' $LOW/tmp/somstub.log) linha(s) ignorada(s); $(grep 'memória aplicada' $LOW/tmp/somstub.log | sed 's/^\[somstub\] //')"
  # O mock obedece à especificação do serviço que o firmware traz, e ela é mais estreita que o
  # serviço de verdade em dois pontos (assinatura de recurso dos conjuntos de som, sem a qual a
  # HMI nunca assina grave, agudo e balanço; e apagar controle atendido). O somstub escreve uma
  # cópia com os dois afrouxados (-s; o porquê está no cabeçalho dele) e ela é montada por cima
  # da original, que fica na partição system, só leitura. Quem lê esse arquivo é o mock.
  ESPEC=$(ls $LOW/lge/app_ro/sai/spec/viwi.service.audiomanagement/viwi.service.audiomanagement-*.json 2>/dev/null | head -n 1)
  [ -n "$ESPEC" ] && chroot $LOW /tmp/somstub -s "${ESPEC#$LOW}" /tmp/som/especificacao.json >>$LOW/tmp/somstub.log 2>&1 \
    && mount -o bind $LOW/tmp/som/especificacao.json "$ESPEC" \
    || { echo "[m1_hmi] ERRO: não preparei a especificação do serviço de áudio para o mock:"; tail -n 2 $LOW/tmp/somstub.log; exit 4; }
fi

# Camada RSI: registro e mock do próprio firmware. A lista de serviços do mock mora na partição
# system (só leitura), então ela é copiada para o tmpfs e montada por cima para poder mudar.
if [ -n "$RSI" ]; then
  SAIM=$LOW/lge/app_ro/sai/mockup
  if [ -n "$RSI_SERVICES$RSI_DATA$SOM" ]; then
    mkdir -p $LOW/tmp/rsi-mockup && cp -r $SAIM/. $LOW/tmp/rsi-mockup/
    [ -n "$RSI_DATA" ] && cp -r "$RSI_DATA"/. $LOW/tmp/rsi-mockup/
    # som de mentira: os dados do audiomanagement entram na pasta do mock, e o serviço na lista dele
    [ -n "$SOM" ] && cp -r $LOW/tmp/som/audiomanagement $LOW/tmp/rsi-mockup/
    # a lista da LG não termina em quebra de linha: sem o \n da frente, o primeiro serviço
    # acrescentado gruda no comentário da última linha e é ignorado
    for s in $RSI_SERVICES ${SOM:+audiomanagement:mixer}; do printf '\n%s\n' "$s" >> $LOW/tmp/rsi-mockup/services; done
    mount -o bind $LOW/tmp/rsi-mockup $SAIM || exit 3
  fi
  chroot $LOW /bin/sh -c "export LD_PRELOAD=/tmp/kipc_bus.so; exec /lge/app_ro/sai/MIB_ServiceRegistryApp-00" >$LOW/tmp/registry.log 2>&1 &
  sleep 3
  chroot $LOW /lge/app_ro/bin/rsi-mockup >$LOW/tmp/rsi-mockup.log 2>&1 &
  sleep 2
  # O somstub é cliente do mock: espera o serviço aparecer, confere que ele serve os dados de
  # /tmp/som e fica ouvindo os controles, as conexões e, pelo barramento, o botão de volume. A
  # HMI só sobe depois de ele estar no ar, para não haver controle nem conexão sem dono.
  if [ -n "$SOM" ]; then
    chroot $LOW /tmp/somstub ${SOMEST:+-m} ${SOMEST:+"$SOMEST"} /tmp/som >>$LOW/tmp/somstub.log 2>&1 &
    i=0; while ! grep -q 'no ar na porta\|ERRO' $LOW/tmp/somstub.log && [ $i -lt 150 ]; do sleep 0.1; i=$((i+1)); done
    grep -q 'no ar na porta' $LOW/tmp/somstub.log \
      || echo "[m1_hmi] AVISO: o som de mentira não ficou no ar: $(tail -n 1 $LOW/tmp/somstub.log)"
  fi
fi

DLLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif"
for d in /lge/app_ro/hmi/persistence-app /lge/app_ro/hmi/mcu-interpreter /lge/app_ro/carif/carifservice /lge/app_ro/hmi/touch-app; do
  # com o carro de mentira ligado, o carifservice de verdade fica de fora (ver VEICULO, no cabeçalho)
  [ -n "$VEICULO" ] && [ -z "$VEICULO_CARIF" ] && [ "$d" = /lge/app_ro/carif/carifservice ] && continue
  chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$DLLP LD_PRELOAD=/tmp/kipc_bus.so; exec $d" >$LOW/tmp/$(basename $d).log 2>&1 &
done
# Entrada (Marco 5): o dsistub lê comandos de /tmp/dsi-entrada, dentro do chroot. Se INPUT for
# dado, esse caminho é o próprio arquivo de fora, montado por cima; senão é um arquivo do tmpfs.
ENTRADA=
if [ -n "$DSISTUB" ] && [ -n "$INPUT$ROTEIRO" ]; then
  ENTRADA=$LOW/tmp/dsi-entrada; : > $ENTRADA
  if [ -n "$INPUT" ]; then
    [ -f "$INPUT" ] || : > "$INPUT"
    mount -o bind "$INPUT" $ENTRADA || exit 3
  fi
fi
chroot $LOW /tmp/$STUB /run/kipc_hub.sock $DMW $DMH ${POWER:-1} ${ENTRADA:+/tmp/dsi-entrada} >$LOW/tmp/dmstub.log 2>&1 &
sleep $WAIT

if [ "$VARIANT" = real ]; then
cat > $LOW/tmp/launch.sh <<'EOS'
# o que o /vw/hmiroot.sh faz, menos o vtee (que esconderia a saída)
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH=/tmp/fakegl:/lge/hmi/jvm/lib/aarch64:/lge/hmi/jvm/lib/aarch64/server:/lge/app_ro/lib64:/lge/app_rw/lib64:/lge/app_rw/smartlink/dcmanager:/usr/lib64:/usr/lib:/lib64:/lib
export LD_PRELOAD=/tmp/kipc_bus.so
export LD_PRELOAD_FORHMI=/lge/hmi/jvm/lib/aarch64/libjsig.so:/lge/hmi/libs/libImageLoader.so:/lge/hmi/libs/libDebugSPINative.so
export VMOPTIONS="-Dde.vw.mib.view.drawpointer=true -XX:ErrorFile=/tmp/hs_err.log $EXTRA_VMOPTIONS"
exec /bin/sh /vw/hmi/runHMI.sh
EOS
else
cat > $LOW/tmp/launch.sh <<'EOS'
# linha de 10/10 (driver/run_hmi11.sh), sem alteração
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH=/tmp/fakegl:/lge/hmi/libs:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64:/lge/app_rw/smartlink/dcmanager:/usr/lib:/lib:/lib64
export LD_PRELOAD="/tmp/kipc_bus.so /lge/hmi/jvm/lib/aarch64/libjsig.so"
CP=$(ls /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar /vw/hmi/jar/*.jar 2>/dev/null | tr '\n' ':')
B=/vw/hmi
exec /lge/hmi/jvm/bin/java \
  -Dde.vw.mib.installation.directory=$B -Dde.vw.mib.configuration.directory=$B/config \
  -Dde.vw.mib.resources.directory=$B/Resources -Dvw.mib.resources=$B/res \
  -Dbundles.properties=$B/config/bundles.properties -Dsystem.properties=$B/config/system.properties \
  -Dlogging.properties=$B/config/logging.properties -Dthreads.properties=$B/config/threads.properties \
  -Djava.library.path=/lge/hmi/libs -Dsun.boot.library.path=/lge/hmi/jvm/lib/aarch64:/lge/hmi/libs \
  -Dviewhandler.format=jar -Dfile.encoding=UTF-8 -Dde.vw.mib.view.gfx.init.eager \
  -XX:ErrorFile=/tmp/hs_err.log $EXTRA_VMOPTIONS \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain
EOS
fi

T0=$(date +%s)
if [ -n "$ENTRADA" ] && [ -n "$ROTEIRO" ]; then
  ( IFS=';'
    for passo in $ROTEIRO; do
      passo=${passo# }
      sleep "${passo%% *}"; echo "${passo#* }" >> $ENTRADA
    done ) &
fi
if [ -n "$STRACE" ]; then
  # ledger de plataforma: toda abertura de arquivo que FALHOU (o que a HMI procura e não acha)
  EXTRA_VMOPTIONS="$EXTRA_VMOPTIONS" timeout $SECS strace -f -qq -Z -e trace=openat,open -o "$OUT/$TAG.strace" \
    chroot $LOW /bin/sh /tmp/launch.sh > "$OUT/$TAG.out" 2>&1
else
  EXTRA_VMOPTIONS="$EXTRA_VMOPTIONS" timeout $SECS chroot $LOW /bin/sh /tmp/launch.sh > "$OUT/$TAG.out" 2>&1
fi
RC=$?
echo "[m1_hmi] rc=$RC (124 = cortado pelo timeout, ou seja, não morreu) em $(( $(date +%s) - T0 ))s; linhas=$(wc -l < "$OUT/$TAG.out")"
for f in hub.log dmstub.log diagstub.log veiculostub.log radiostub.log telefonestub.log midiastub.log persistence-app.log mcu-interpreter.log carifservice.log touch-app.log hs_err.log registry.log rsi-mockup.log somstub.log; do
  [ -f $LOW/tmp/$f ] && cp $LOW/tmp/$f "$OUT/$TAG.$f"
done
# o que o registro tinha no fim da execução (quem se registrou e em que porta)
[ -n "$RSI" ] && chroot $LOW env LD_LIBRARY_PATH=/usr/ext-library/lib64 /usr/ext-library/bin/curl -s -m 5 \
  http://127.0.0.1:443/ > "$OUT/$TAG.rsi-services.json" 2>/dev/null
ls $LOW/tmp/glcalls* >/dev/null 2>&1 && cp $LOW/tmp/glcalls* "$OUT/"

c() { grep -ac -- "$1" "$OUT/$TAG.out"; }
echo "[m1_hmi] marcos: SPI=$(c 'Starting SPI bundles') autostart=$(c 'Starting autostart bundles') managed=$(c 'Starting managed bundles') UIthread=$(c 'UI thread started') framework=$(c 'app framework initialized')"
echo "[m1_hmi] views: enterView=$(c 'enterView(') repaintViews=$(c 'repaintViews') errRepaint=$(c 'Error while repainting screen') NPE=$(c 'NullPointerException') queryFunc=$(c 'queryFunctionality')"
echo "[m1_hmi] falhas: SIGSEGV=$(c 'SIGSEGV') Unsatisfied=$(c 'UnsatisfiedLinkError') preloadIgnorado=$(c 'cannot be preloaded') hs_err=$([ -f $LOW/tmp/hs_err.log ] && echo sim || echo nao)"
echo "[m1_hmi] DSI não registrados:"; grep -a "Required DSI services not registered" -A6 "$OUT/$TAG.out" | grep -aoE "org\.dsi\.ifc\.[A-Za-z.]+\[[0-9]\]" | sort -u | tr '\n' ' '; echo
echo "[m1_hmi] RSI não registrados: $(grep -a 'Required RSI services not registered' "$OUT/$TAG.out" | sed 's/.* ms: //' | tr '\n' ';')"
echo "[m1_hmi] tela Welcome: $(grep -a 'check And Show WelcomeScreen' "$OUT/$TAG.out" | sed 's/.*WelcomeScreen: //' | uniq -c | tr -s ' \n' ' ')"
# o que a HMI diz que leu da codificação e da adaptação (linhas de nível INFO, saem sempre)
echo "[m1_hmi] diagnóstico: leituras sem dado=$(c 'Diagnosis data not available'); $(grep -a 'ASL.DIAGNOSTIC\]\|\] DIAGNOSTIC \|Clock is \|Unitmaster is ' "$OUT/$TAG.out" | grep -a 'brand is \|Navigation is \|Clock is \|Unitmaster is \|DAB is ' | sed 's/.*DIAGNOSTIC\]* *//; s/^DIAGNOSTIC //' | tr '\n' ';')"
# o que o carro de mentira atendeu, e a hora que a HMI disse ter aceitado
[ -n "$VEICULO" ] && echo "[m1_hmi] veículo: $(grep -ac ': assinou' "$OUT/$TAG.veiculostub.log") pedidos de assinatura atendidos, $(grep -ac 'pedido atendido' "$OUT/$TAG.veiculostub.log") pedidos diretos atendidos, $(grep -ac 'pedido sem resposta' "$OUT/$TAG.veiculostub.log") pedidos sem resposta; relógio:$(grep -a 'First valid ClockTime' "$OUT/$TAG.out" | head -n 1 | sed 's/.*First valid ClockTime: //; s/ triggerObserver.*//')"
# com HMILOG: quantas linhas de rastreamento saíram e que chaves a HMI disse ter aplicado na carga
# tardia (a última "Changed logging properties to:"). {} quer dizer que ela desligou todas
if [ -n "$HMILOG" ]; then
  ap=$(grep -a 'Changed logging properties to:' "$OUT/$TAG.out" | tail -n 1 | sed 's/.*properties to: //')
  echo "[m1_hmi] registro detalhado: $(c ' TRACE \[') linhas TRACE; chaves que a HMI aplicou na carga tardia: ${ap:-não houve troca (a sonda LOGMANAGER não foi ativada)}"
fi
# o que o rádio de mentira atendeu: assinaturas, sintonias e buscas
[ -n "$RADIO" ] && echo "[m1_hmi] rádio: $(grep -ac ': assinou' "$OUT/$TAG.radiostub.log") pedidos de assinatura atendidos, $(grep -ac 'sintonizei' "$OUT/$TAG.radiostub.log") sintonias, $(grep -ac ': busca ' "$OUT/$TAG.radiostub.log") buscas, $(grep -ac 'pedido sem resposta' "$OUT/$TAG.radiostub.log") pedidos sem resposta"
# o que o telefone de mentira atendeu
[ -n "$TELEFONE" ] && echo "[m1_hmi] telefone: $(grep -ac ': assinou' "$OUT/$TAG.telefonestub.log") pedidos de assinatura atendidos, $(grep -ac 'pedido atendido' "$OUT/$TAG.telefonestub.log") pedidos diretos atendidos, $(grep -ac 'pedido sem resposta' "$OUT/$TAG.telefonestub.log") pedidos sem resposta"
# o que a mídia de mentira atendeu
[ -n "$MIDIA" ] && echo "[m1_hmi] mídia: $(grep -ac ': assinou' "$OUT/$TAG.midiastub.log") pedidos de assinatura atendidos, $(grep -ac 'midiastub\] pedido atendido' "$OUT/$TAG.midiastub.log") pedidos do tocador e do navegador atendidos, $(grep -ac 'pedido sem resposta' "$OUT/$TAG.midiastub.log") pedidos sem resposta"
# o que o som de mentira fez: controles criados no papel do touch-app, controles atendidos, mudo e trocas de fonte
[ -n "$SOM" ] && echo "[m1_hmi] som: $(grep -ac ' -> controle ' "$OUT/$TAG.somstub.log") controles criados pelo botão ou pelas teclas, $(grep -ac ' (controle volume' "$OUT/$TAG.somstub.log") controles de volume atendidos, mudo ligado $(grep -ac 'mudo do usuário: ligado' "$OUT/$TAG.somstub.log") vez(es), $(grep -ac '^\[somstub\] entretenimento: ' "$OUT/$TAG.somstub.log") troca(s) de fonte; $(grep -a 'no ar na porta' "$OUT/$TAG.somstub.log" | tail -n 1 | sed 's/^\[somstub\] no ar na porta [0-9]*: //')"
kill $(jobs -p) 2>/dev/null
exit 0
