#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null
echo "=== JNI symbols exportados em libDebugSPINative.so ==="
chroot $LOW /bin/sh -c 'readelf -W --dyn-syms /lge/hmi/libs/libDebugSPINative.so 2>/dev/null | grep -iE "JNI_OnLoad|initSPINative|Java_com" ' \
  || /usr/bin/llvm-objdump -T $LOW/lge/hmi/libs/libDebugSPINative.so 2>/dev/null | grep -iE "JNI_OnLoad|initSPINative|Java_com"
echo "--- (todos os símbolos de texto definidos, p/ nome exato) ---"
chroot $LOW /bin/sh -c 'readelf -W --dyn-syms /lge/hmi/libs/libDebugSPINative.so 2>/dev/null' | grep -i " FUNC " | grep -iE "SPI|JNI|init" | head -30
echo
echo "=== E as outras libs SPI? (nome -> tem JNI_OnLoad?) ==="
for so in $(ls $LOW/lge/hmi/libs/*SPI*.so $LOW/lge/hmi/libs/libSPI*.so 2>/dev/null); do
  b=$(basename $so)
  has=$(chroot $LOW /bin/sh -c "readelf -W --dyn-syms /lge/hmi/libs/$b 2>/dev/null" | grep -c JNI_OnLoad)
  jn=$(chroot $LOW /bin/sh -c "readelf -W --dyn-syms /lge/hmi/libs/$b 2>/dev/null" | grep -c "Java_")
  echo "$b : JNI_OnLoad=$has  Java_*=$jn"
done 2>/dev/null | head -40