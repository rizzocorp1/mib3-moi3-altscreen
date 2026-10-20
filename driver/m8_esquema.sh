#!/bin/sh
# Marco 8 (11/10/2026): gera o esquema dos serviços DSI de veículo (domínio 16) com as classes do
# próprio firmware (driver/LeEsquemaDSI.java), mais os vetores de prova do veiculostub.
# Roda DENTRO da imagem moi3-bancada:1, sem rede. Só lê o firmware; nada vai para o carro.
#   docker run --rm --privileged --network none -v <bancada>/sysroot/mu0883:/fw:ro \
#     -v <bancada>/driver:/driver:ro -v <saída>:/out moi3-bancada:1 sh /driver/m8_esquema.sh
# Saem em /out: esquema.txt, constantes.txt, vetores-perfil.txt, vetores-esperado.txt e relatorio.txt.
# Na pasta da bancada, /out é stubs/veiculo/gerado/. Como uma central que esteja subindo copia o
# esquema na partida, o mais seguro é gerar numa pasta à parte e trocar os arquivos com mv.
# Desde o Marco 9 o esquema traz também os pedidos com resposta (linhas R, da tabela do proxy DSI
# da HMI) e, por dedução do gerador, o que cada pedido grava (G) e o que cada resposta devolve (X).
# DOMINIOS: domínios a levantar, separados por vírgula (default 16, o dos serviços de veículo).
# ROTULO: como o cabeçalho dos arquivos chama os serviços (default "de veículo").
# Para o rádio de mentira (Marco 11), com /out em stubs/radio/gerado/:
#   -e DOMINIOS=0x400000000000 -e ROTULO="do sintonizador"
# O javac e o javap vêm da JVM do firmware (/lge/hmi/jvm/lib/tools.jar), como nos Marcos 5 e 6.
LOW=/mnt/low; OUT=${OUT:-/out}; DOMINIOS=${DOMINIOS:-16}; ROTULO=${ROTULO:-}
mkdir -p $LOW "$OUT"
mount -o loop,ro,noload /fw/system.img $LOW && mount --bind /fw/vw $LOW/vw || { echo "não montou o firmware"; exit 1; }
mount -t tmpfs tmpfs $LOW/tmp && mount -t proc proc $LOW/proc || exit 1
cp /driver/LeEsquemaDSI.java $LOW/tmp/
export DOMINIOS ROTULO
chroot $LOW /bin/sh -c '
  export PATH=/usr/sbin:/usr/bin:/sbin:/bin
  export LD_LIBRARY_PATH=/lge/hmi/jvm/lib/aarch64:/lge/hmi/jvm/lib/aarch64/server:/lge/hmi/libs:/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib
  T=/lge/hmi/jvm/lib/tools.jar
  CP=$(ls /vw/hmi/jar/*.jar /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar | tr "\n" ":")
  cd /tmp || exit 1
  /lge/hmi/jvm/bin/java -cp $T com.sun.tools.javac.Main -nowarn -encoding UTF-8 -cp "$CP" -d /tmp LeEsquemaDSI.java || exit 1
  /lge/hmi/jvm/bin/java -Dfile.encoding=UTF-8 -Djava.library.path=/lge/hmi/libs -cp "/tmp:$T:$CP" LeEsquemaDSI /lge/hmi/jar/MIBHMI-DSI.jar /tmp/saida "$DOMINIOS" "$ROTULO"
' || { echo "falhou"; exit 1; }
cp $LOW/tmp/saida/* "$OUT"/
ls -la "$OUT"
