#!/bin/sh
# Marco 6 (11/10/2026): gera a tabela de campos de codificação e adaptação que a HMI entende e
# o conjunto padrão, com as classes do próprio firmware (driver/LeCodificacao.java).
# Roda DENTRO da imagem moi3-bancada:1, sem rede. Só lê o firmware; nada vai para o carro.
#   docker run --rm --privileged --network none -v <bancada>/sysroot/mu0883:/fw:ro \
#     -v <bancada>/driver:/driver:ro -v <saída>:/out moi3-bancada:1 sh /driver/m6_tabela.sh
# VARIANTE: pacote das classes de acesso (default oilarge, que é o que o TargetDiagnosis da HMI
# instancia; o programa confere se a oismall tem a mesma tabela).
# O javac vem da JVM do firmware (/lge/hmi/jvm/lib/tools.jar), como no Marco 5.
LOW=/mnt/low; OUT=${OUT:-/out}; VARIANTE=${VARIANTE:-oilarge}
mkdir -p $LOW "$OUT"
mount -o loop,ro,noload /fw/system.img $LOW && mount --bind /fw/vw $LOW/vw || { echo "não montou o firmware"; exit 1; }
mount -t tmpfs tmpfs $LOW/tmp && mount -t proc proc $LOW/proc || exit 1
cp /driver/LeCodificacao.java $LOW/tmp/
chroot $LOW /bin/sh -c '
  export PATH=/usr/sbin:/usr/bin:/sbin:/bin
  export LD_LIBRARY_PATH=/lge/hmi/jvm/lib/aarch64:/lge/hmi/jvm/lib/aarch64/server:/lge/hmi/libs:/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib
  CP=$(ls /vw/hmi/jar/*.jar /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar | tr "\n" ":")
  cd /tmp || exit 1
  /lge/hmi/jvm/bin/java -cp /lge/hmi/jvm/lib/tools.jar com.sun.tools.javac.Main -nowarn -encoding UTF-8 -cp "$CP" -d /tmp LeCodificacao.java || exit 1
  /lge/hmi/jvm/bin/java -Dfile.encoding=UTF-8 -Djava.library.path=/lge/hmi/libs -cp "/tmp:$CP" LeCodificacao /vw/hmi/config '"$VARIANTE"' /tmp/saida
' || { echo "falhou"; exit 1; }
cp $LOW/tmp/saida/* "$OUT"/
ls -la "$OUT"
