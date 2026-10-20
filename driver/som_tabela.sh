#!/bin/sh
# som_tabela.sh — gera stubs/som/gerado/: o que o serviço de áudio da central serve em repouso.
# Som de mentira (12/10/2026). É estudo num computador: nada vai para o carro.
#
# O som desta HMI não vem pelo barramento DSI: vem pelo serviço RSI "audiomanagement". No carro
# quem o serve é o gerente de áudio do próprio firmware (/lge/app_ro/audiomgnt/audiomgnt), a
# partir da tabela de áudio que o firmware traz (/lge/app_ro/audiomgnt/audiotable/). Este roteiro
# sobe esse programa SOZINHO (hub, registro de serviços e ele; sem HMI), espera o serviço aparecer,
# pede cada recurso inteiro pela ferramenta rsi-send do firmware e guarda a resposta como arquivo
# de dados do mock RSI da LG (<recurso>.json, uma lista de elementos). Ninguém escreve elemento à
# mão: nomes, identificadores, ligações, faixas e valores de fábrica são os que o firmware dá.
#
# O gerente de áudio só é usado aqui, parado, por alguns segundos. Ele NÃO serve para rodar com a
# HMI na bancada: sem o resto do carro ele entra no mudo de proteção de componentes (a conexão
# FCT_AMP_THEFTPROTECTION_MUTE da tabela) e a HMI mostra "Proteção de componentes está ativa".
# Isso não foi nem será contornado (modelagem-soc.md, marco do som). Por isso quem serve o
# audiomanagement na bancada é o mock RSI da LG, com estes dados, e o driver/somstub.c por cima.
#
# Roda na imagem moi3-bancada:1, sem rede, só lendo o firmware. Não rodar com a saída apontada
# para a pasta da bancada enquanto uma central sobe (ela copia os dados na partida): gerar numa
# pasta à parte e trazer os arquivos com mv.
#   docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro \
#     -v "$PWD/driver":/driver:ro -v <pasta de saída>:/out moi3-bancada:1 sh /driver/som_tabela.sh
# Saída em /out: audiomanagement/<recurso>.json (nove arquivos) e relatorio.txt.
LOW=/mnt/low; W=/tmp/w; OUT=${OUT:-/out}; D=${DRIVER:-/driver}
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
RECURSOS="globals connections volumesettings soundsettings soundparameters soundpresets controls sinks sources"
mkdir -p $W $LOW "$OUT/audiomanagement" || exit 2
mount -o loop,ro,noload /fw/system.img $LOW || { echo "[som_tabela] não montou o firmware"; exit 1; }
DRIVER=$D sh "$D/m1_build.sh" $W/bin > $W/build.log 2>&1 || { echo "[som_tabela] não compilou o driver"; tail -n 5 $W/build.log; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d || exit 3; done
mkdir -p $LOW/dev/shm $LOW/dev/pts; mount -t tmpfs tmpfs $LOW/dev/shm; mount -t proc proc $LOW/proc
mknod -m 666 $LOW/dev/null c 1 3; mknod -m 666 $LOW/dev/zero c 1 5
mknod -m 666 $LOW/dev/urandom c 1 9; mknod -m 666 $LOW/dev/random c 1 8
cp $W/bin/kipc_hub $W/bin/kipc_bus.so $LOW/tmp/
chroot $LOW /tmp/kipc_hub 2>$LOW/tmp/hub.log &
sleep 1
chroot $LOW /bin/sh -c "export LD_PRELOAD=/tmp/kipc_bus.so; exec /lge/app_ro/sai/MIB_ServiceRegistryApp-00" >$LOW/tmp/registry.log 2>&1 &
sleep 3
DLLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif"
chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$DLLP LD_PRELOAD=/tmp/kipc_bus.so; exec /lge/app_ro/audiomgnt/audiomgnt --log-level=0" >$LOW/tmp/audiomgnt.log 2>&1 &
pede() { chroot $LOW /lge/app_ro/bin/rsi-send get "$1" 2>/dev/null; }
i=0; while [ $i -lt 60 ]; do pede /audiomanagement/globals | grep -q '"name":"global"' && break; sleep 0.5; i=$((i+1)); done
[ $i -lt 60 ] || { echo "[som_tabela] o gerente de áudio não se registrou em 30 s"; kill $(jobs -p) 2>/dev/null; exit 1; }
rc=0; : > $W/contagem.txt
for r in $RECURSOS; do
  pede "/audiomanagement/$r?\$limit=1000" > $W/$r.bruto
  head -n 1 $W/$r.bruto | grep -q '^status:200' || { echo "[som_tabela] $r: $(head -n 1 $W/$r.bruto)"; rc=1; continue; }
  # a resposta é {"data":[…],"paging":{…},"status":"ok","timestamp":N}; fica só a lista, um elemento por linha
  sed -n '2p' $W/$r.bruto | sed 's/^{"data"://; s/,"status":"ok","timestamp":[0-9]*}$//; s/,"paging":{[^{}]*}$//' \
    | sed 's/},{"/},\n{"/g' > "$OUT/audiomanagement/$r.json"
  # conferência: a lista começa em [ e acaba em ], e tem os elementos que o serviço diz ter
  n=$(grep -c '' "$OUT/audiomanagement/$r.json"); [ "$(cat "$OUT/audiomanagement/$r.json")" = "[]" ] && n=0
  t=$(sed -n '2p' $W/$r.bruto | grep -o '"total":[0-9]*' | cut -d: -f2)
  case "$(head -c 1 "$OUT/audiomanagement/$r.json")$(tail -c 2 "$OUT/audiomanagement/$r.json" | head -c 1)" in
    '[]') ;; *) echo "[som_tabela] $r: a resposta não tem a forma esperada"; rc=1 ;;
  esac
  [ "${t:-0}" = "$n" ] || { echo "[som_tabela] $r: o serviço diz ${t:-0} elementos e o arquivo tem $n"; rc=1; }
  echo "$r ${t:-0}" >> $W/contagem.txt
done
# o objeto global em repouso não aponta conexão nenhuma: se apontar, o despejo pegou o serviço
# já fora do repouso (por exemplo no mudo de proteção) e não serve
grep -q 'Connection\|settingFocus' "$OUT/audiomanagement/globals.json" && { echo "[som_tabela] o objeto global já aponta uma conexão: o serviço não estava em repouso"; rc=1; }
T=$LOW/lge/app_ro/audiomgnt/audiotable/MIB3_Audiotable_MQB.xml
{
  echo "# relatorio.txt — gerado por driver/som_tabela.sh; não editar à mão"
  echo "# O que o gerente de áudio do firmware (audiomgnt) serve em repouso, sem HMI e sem carro."
  echo "tabela de áudio: $(basename $T), versão $(sed -n 's/.*<audiotable_version>\(.*\)<\/audiotable_version>.*/\1/p' $T), de $(sed -n 's/.*<release_date>\(.*\)<\/release_date>.*/\1/p' $T) ($(sed -n 's/.*<description>\(.*\)<\/description>.*/\1/p' $T | head -n 1))"
  echo "sha256 da tabela: $(sha256sum $T | cut -d' ' -f1)"
  echo "sha256 do audiomgnt: $(sha256sum $LOW/lge/app_ro/audiomgnt/audiomgnt | cut -d' ' -f1)"
  echo "sha256 da libAudioManagement.so: $(sha256sum $LOW/lge/app_ro/lib64/libAudioManagement.so | cut -d' ' -f1)"
  echo "especificação do serviço: $(ls $LOW/lge/app_ro/sai/spec/viwi.service.audiomanagement/)"
  echo "elementos por recurso:"
  sed 's/^/  /' $W/contagem.txt
  echo "saída de som 'entertainment' em repouso: $(grep '"name":"entertainment"' "$OUT/audiomanagement/sinks.json" | grep -o '"connectStatus":"[a-z]*"\|"amplifierId":"[^"]*"\|"audioStatus":"[a-z]*"' | tr '\n' ' ')"
  echo "volume de entretenimento em repouso: $(grep '"name":"VolumeEntertainment"' "$OUT/audiomanagement/volumesettings.json" | grep -o '"value":[0-9]*\|"valueMin":[0-9]*\|"valueMax":[0-9]*\|"locked":[a-z]*' | tr '\n' ' ')"
} > "$OUT/relatorio.txt"
cat "$OUT/relatorio.txt"
kill $(jobs -p) 2>/dev/null
exit $rc
