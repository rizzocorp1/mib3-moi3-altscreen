#!/usr/bin/env python3
# redirect_producer.py — o PRODUTOR do C (opção "RS própria").
#
# Cria, via REST no serviço RSI DisplayManagement, uma renderingSurface + streamParameters NOSSAS
# (sem sequestrar a `navi-kombi-renderingsurface` nativa), com a fonte apontando para o surface IVI
# de um app (CarPlay/Android Auto) e o destino no RTP do cluster. O mss, ao reagir (prepareStreamsList),
# grabeia esse surface e transmite — ver ../camada3-rsi-redirect.md.
#
# Por que RS própria e não sequestrar a nativa: a fonte da RS nativa é ligada uma vez em
# `registerDisplays`, e trocas de visibilidade só re-POSTam framerate (1/10 fps) na streamParameters
# nativa; sequestrar a nativa expõe o nosso stream a esse afunilamento e ao re-bind. Uma RS própria
# fica imune aos dois. (RE 16/10/2026.)
#
# Escopo: é caminho de EXIBIÇÃO. Rodar isto numa unidade real precisa de execução de código na ECU,
# que está fora de escopo. Aqui é bancada.
import argparse, json, sys, urllib.request

def post(base, coll, body):
    req = urllib.request.Request(
        f"{base}/displaymanagement/{coll}",
        data=json.dumps(body).encode(), method="POST",
        headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=5) as r:
        loc = r.headers.get("Location")
        return r.status, loc, json.loads(r.read() or b"{}")

def main():
    ap = argparse.ArgumentParser(description="C (camada 3) — cria RS própria + streamParameters via RSI")
    ap.add_argument("--base", default="http://127.0.0.1:8780", help="base do serviço DisplayManagement")
    ap.add_argument("--app-surface", required=True, help="id do surface IVI do app a capturar (fonte)")
    ap.add_argument("--dest", default="ff14::2:4", help="destino RTP (grupo multicast do cluster)")
    ap.add_argument("--port", type=int, default=42800, help="porta UDP do cluster")
    ap.add_argument("--fps", type=int, default=30, help="framerate do nosso stream")
    ap.add_argument("--width", type=int, default=1280)
    ap.add_argument("--height", type=int, default=480)
    ap.add_argument("--prefix", default="app-redirect", help="prefixo dos nomes dos nossos elementos")
    args = ap.parse_args()

    disp  = f"{args.prefix}-display"           # id REST do display
    rs_id = f"{args.prefix}-renderingsurface"  # id REST da RS (o que vai no display.renderingSurfaces)
    sp    = f"{args.prefix}-streamparameters"  # id REST do streamParameters
    rs_name = f"surfaceid://{args.app_surface}"  # a FONTE: vai no campo `name` (+0x28) da RS

    B = "/displaymanagement"

    # CONTRATO VIWI (RE 16/10/2026, provado no bench com gdb; ver ../camada3-rsi-redirect.md):
    # - associação entre recursos é OBJETO {"id": ...}, nunca string (o mss faz getObject->.id).
    # - o GATE de prepareStreamsList casa rs.id contra display.renderingSurfaces (NÃO
    #   currentVisibleSurfaces), então a RS própria PRECISA entrar no renderingSurfaces do display.
    # - TODO objeto PRECISA do campo `uri` (e campos não-vazios): sem `uri`, o RenderingSurface/Display/
    #   StreamParameters::fromJSON do mss para logo depois de `name` e deixa o resto vazio -> a RS é
    #   pulada no laço por-RS e nenhum stream é emitido. Com `uri`, o mss grabeia o surface (visto:
    #   ivi_share.get_ivi_share_surface for surface_id <app>).

    # 1) streamParameters NOSSOS — o destino do stream (ip/porta/fps do cluster).
    post(args.base, "streamParameters",
         {"id": sp, "name": sp, "uri": f"{B}/streamParameters/{sp}",
          "ip": args.dest, "port": args.port, "framerate": args.fps, "resync": True})
    # 2) display NOSSO — a RS tem de estar no renderingSurfaces do display (o GATE), como objeto {id}.
    #    currentVisibleSurfaces idem (array de objetos) pra bater o fromJSON do Display.
    post(args.base, "displays",
         {"id": disp, "name": disp, "uri": f"{B}/displays/{disp}",
          "width": args.width, "height": args.height,
          "renderingSurfaces": [{"id": rs_id}],
          "currentVisibleSurfaces": [{"id": rs_id}]})
    # 3) renderingSurface NOSSA — id REST próprio; a FONTE no campo `name` como surfaceid://<app>;
    #    display e streamParameters como objetos {id} (associação VIWI).
    post(args.base, "renderingSurfaces", {
        "id": rs_id,
        "name": rs_name,
        "uri": f"{B}/renderingSurfaces/{rs_id}",
        "internalIdentifier": args.app_surface,
        "display": {"id": disp},
        "topLeftX": 0, "topLeftY": 0,
        "width": args.width, "height": args.height,
        "streamParameters": {"id": sp},
    })

    print(json.dumps({
        "created": {"display": disp, "renderingSurface": rs_id, "streamParameters": sp},
        "renderingSurface_name": rs_name,
        "source_surface": args.app_surface,
        "dest": f"{args.dest}:{args.port}", "fps": args.fps,
    }, indent=2))

if __name__ == "__main__":
    main()
