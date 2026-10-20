#!/bin/sh
# Sessão de modelagem do SoC (11/10/2026): compila as ferramentas da bancada UMA vez,
# pra serem usadas tanto no container (chroot) quanto dentro do QEMU (initramfs).
# Roda na imagem moi3-bancada:1. Saída em $1 (default /work/bin).
#   kipc_hub, dmstub, dsistub, dsi_escuta, dsi_manda, diagstub, veiculostub, radiostub, midiastub, somstub
#                      estáticos (rodam sob qualquer glibc)
#   kipc_bus.so        LD_PRELOAD sem libc (syscalls inline)
#   fakegl/*.so        stub de EGL/GLES com os sonames do Adreno (o de 10/10)
set -e
B=${1:-/work/bin}; D=${DRIVER:-/driver}
mkdir -p "$B/fakegl"
gcc -static -O2 -o "$B/kipc_hub" "$D/kipc_hub.c"
gcc -static -O2 -o "$B/dmstub" "$D/dmstub.c"
gcc -static -O2 -o "$B/dsistub" "$D/dsistub.c"
gcc -static -O2 -o "$B/dsi_escuta" "$D/dsi_escuta.c"
gcc -static -O2 -o "$B/diagstub" "$D/diagstub.c"
gcc -static -O2 -o "$B/dsi_manda" "$D/dsi_manda.c"
gcc -static -O2 -Wall -o "$B/veiculostub" "$D/veiculostub.c"
# O radiostub (Marco 11) inclui o veiculostub.c. Se uma mudança lá o quebrar, as centrais que não
# pediram rádio continuam subindo: fica o aviso, e nenhum binário velho.
gcc -static -O2 -Wall -o "$B/radiostub" "$D/radiostub.c" \
  || { rm -f "$B/radiostub"; echo "[m1_build] AVISO: o radiostub não compilou; central com RADIO sobe sem o rádio de mentira"; }
# O midiastub (Marco 15) também inclui o veiculostub.c: vale o mesmo cuidado.
gcc -static -O2 -Wall -o "$B/midiastub" "$D/midiastub.c" \
  || { rm -f "$B/midiastub"; echo "[m1_build] AVISO: o midiastub não compilou; central com MIDIA sobe sem a mídia de mentira"; }
# O telefonestub (telefone de mentira com modelo, 12/10/2026) também inclui o veiculostub.c: vale o
# mesmo cuidado.
gcc -static -O2 -Wall -o "$B/telefonestub" "$D/telefonestub.c" \
  || { rm -f "$B/telefonestub"; echo "[m1_build] AVISO: o telefonestub não compilou; central com TELEFONE sobe sem o telefone de mentira"; }
# O somstub (som de mentira, 12/10/2026) é cliente do mock RSI, por HTTP e WebSocket; não inclui
# nenhum dos outros. Se não compilar, as centrais que não pediram som continuam subindo: fica o
# aviso, e nenhum binário velho.
gcc -static -O2 -Wall -o "$B/somstub" "$D/somstub.c" \
  || { rm -f "$B/somstub"; echo "[m1_build] AVISO: o somstub não compilou; central com SOM sobe sem o som de mentira"; }
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o "$B/kipc_bus.so" "$D/kipc_bus.c"
for sn in libEGL_adreno.so libGLESv2_adreno.so; do
  gcc -shared -fPIC -ffreestanding -fno-builtin -fno-stack-protector -nostdlib -w \
      -Wl,-soname,$sn -o "$B/fakegl/$sn" "${FAKEGL:-$D/fakegl.c}"
done
ls -la "$B" "$B/fakegl"
