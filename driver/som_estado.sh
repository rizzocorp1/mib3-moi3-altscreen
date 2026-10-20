#!/bin/sh
# som_estado.sh [rótulo] — uma linha com o estado do som de mentira, lido do serviço (roda dentro
# do container de uma central; usa o rsi-send do firmware). 12/10/2026.
#   docker exec <container> sh /driver/som_estado.sh
LOW=${LOW:-/mnt/low}
g() { chroot $LOW /lge/app_ro/bin/rsi-send get "/audiomanagement/$1" 2>/dev/null | sed -n 2p; }
v() { g "$1?name=$2" | grep -o "\"$3\":\(\"[^\"]*\"\|[-0-9a-z]*\)" | head -n 1 | cut -d: -f2 | tr -d '"'; }
foco=$(g globals | sed -n 's/.*"settingFocus":{//p' | grep -o '"name":"[^"]*"' | head -n 1 | cut -d'"' -f4)
ent=$(g globals | sed -n 's/.*"entertainmentConnection":{//p' | grep -o '"name":"[^"]*"' | head -n 1 | cut -d'"' -f4)
echo "${1:+$1: }volume $(v volumesettings VolumeEntertainment value) (de $(v volumesettings VolumeEntertainment valueMin) a $(v volumesettings VolumeEntertainment valueMax)); mudo $(v connections FCT_AMP_USER_MUTE status); grave $(v soundparameters EQ1 value), médio $(v soundparameters EQ3 value), agudo $(v soundparameters EQ5 value); balanço $(v soundparameters balance value), fader $(v soundparameters fader value); fonte ${ent:-nenhuma}, foco ${foco:-nenhum}; controles na lista: $(g 'controls?$limit=1000' | grep -o '"id"' | wc -l | tr -d ' ')"
