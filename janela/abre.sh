#!/bin/sh
# abre.sh — abre a janela da central, compilando antes se o programa não existe ou se o
# janela.swift mudou. Roda no Mac, de qualquer pasta. O que vier depois do nome vai para o
# programa (opções no cabeçalho do janela.swift).
D=$(cd "$(dirname "$0")" && pwd); APP="$D/build/Central MOI3.app"; BIN="$APP/Contents/MacOS/central-moi3"
if [ ! -x "$BIN" ] || [ "$D/janela.swift" -nt "$BIN" ]; then sh "$D/build.sh" || exit 1; fi
open "$APP" --args "$@"
