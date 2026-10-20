#!/bin/sh
# m14_menus.sh — sobe uma central de TESTE com o catálogo dos menus do carro (Marco 14, 12/10/2026).
# Estudo num computador; nada daqui vai para o carro.
#
#   sh driver/m14_menus.sh [segundos]   sobe a central "menus" (container moi3-m8-menus; 1800 s se
#                                       nada for dito), em /tmp/moi3-menus
#   sh driver/m14_menus.sh grava [segundos]   a mesma central, e o que se marca nos menus FICA
#   sh driver/m14_menus.sh mostra [grava]     só mostra o comando que o de cima roda
#   sh driver/m14_menus.sh desce        encerra
# Para ver e mexer:  sh janela/abre.sh --container moi3-m8-menus
#
# Com "grava" (12/10/2026) entra por último o stubs/veiculo/catalogo-menus-grava.txt, que põe os
# serviços desses menus na lista dos que aceitam o que a tela grava, e liga o arquivo de estado do
# carro de mentira (<pasta>/memoria/veiculo/estado.txt): a caixa marcada fica marcada e volta
# marcada na central seguinte. Para voltar ao zero: rm -rf /tmp/moi3-menus/memoria.
#
# O catálogo (stubs/veiculo/catalogo-menus.txt e stubs/codificacao/catalogo-menus.txt) liga tudo o
# que a HMI sabe mostrar em cinco menus de "Configurações do veículo" e no app "Assistentes". NÃO
# É O CARRO: serve para ver as telas e comparar com as do carro. Por isso ele só entra aqui, numa
# central de teste à parte; o modo ao vivo e as outras centrais de teste continuam sem ele.
# Os perfis são os padrões do modo ao vivo (tirados do m3_ao_vivo.sh, para não repetir a lista) e,
# depois deles, o catálogo. É uma central de teste como as outras: não lembra de nada e não toca
# em ao-vivo/. MENUS_PASTA troca a pasta dos quadros e do registro.
D=$(cd "$(dirname "$0")" && pwd)
padrao() { sed -n "s/^$1=\${$1-\(.*\)}; export $1\$/\1/p" "$D/m3_ao_vivo.sh"; }
COD=$(padrao CODING); VEI=$(padrao VEICULO)
if [ -z "$COD" ] || [ -z "$VEI" ]; then
  echo "m14_menus: não achei no m3_ao_vivo.sh os perfis padrão de CODING e de VEICULO; não subi nada" >&2; exit 2
fi
COD="$COD /stubs/codificacao/catalogo-menus.txt"; VEI="$VEI /stubs/veiculo/catalogo-menus.txt"
P=${MENUS_PASTA:-/tmp/moi3-menus}
MOSTRA=; EST=
[ "$1" = mostra ] && { MOSTRA=1; shift; }
[ "$1" = grava ] && { VEI="$VEI /stubs/veiculo/catalogo-menus-grava.txt"; EST=/live/memoria/veiculo/estado.txt; shift; }
case ${1:-1800} in
desce) exec sh "$D/m8_central.sh" desce menus ;;
*[!0-9]*) sed -n '2,16p' "$0"; exit 2 ;;
esac
if [ -n "$MOSTRA" ]; then
  echo "${EST:+VEICULO_ESTADO=$EST }CODING=\"$COD\" VEICULO=\"$VEI\" sh driver/m8_central.sh sobe menus $P ${1:-1800}"; exit 0
fi
[ -n "$EST" ] && { VEICULO_ESTADO=$EST; export VEICULO_ESTADO; }
CODING="$COD" VEICULO="$VEI" exec sh "$D/m8_central.sh" sobe menus "$P" "${1:-1800}"
