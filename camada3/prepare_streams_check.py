#!/usr/bin/env python3
# prepare_streams_check.py — replica FIEL do MultiScreenController::prepareStreamsList do mss.
#
# Lê o estado do serviço DisplayManagement (GET nos três recursos) e aplica exatamente a lógica que
# fizemos RE do mss (`@0x130028`, REA/Ghidra 12.1.4, MU 0883) para prever a lista de streams que o
# mss montaria. É o "teste" do produtor: prova, sem o binário, que a RS própria que o C cria faz o
# mss grabear o surface certo e mandar pro destino certo.
#
# Lógica RE (ver ../camada3-rsi-redirect.md §3.3.1), confirmada por decompilação em 16/10/2026
# (Display/RenderingSurface/StreamParameters::fromJSON, Display::extractIds @0x125120,
#  prepareStreamsList @0x130028, getPreparedSurfaceId @0x12f278):
#   CONTRATO VIWI: associação entre recursos é OBJETO {"id": ...}, não string. RS.display e
#   RS.streamParameters são objetos (getObject -> .id). display.renderingSurfaces e
#   display.currentVisibleSurfaces são ARRAYS de objetos {"id": ...} (extractIds exige objeto).
#   para cada renderingSurface rs:
#     display = displays[rs.display.id]               ; se faltar/vazio -> pula
#     sp      = streamParameters[rs.streamParameters.id] ; se faltar/vazio -> pula
#     GATE    : rs.id ∈ ids(display.renderingSurfaces)  (mss: find_if em display+0x68 = a lista
#               `renderingSurfaces`, NÃO currentVisibleSurfaces) E sp.ip != ""  (sp+0x78 != 0)
#     FONTE   : n = prepared(rs.name)  (trim+lower, getPreparedSurfaceId @0x12f278)
#               se n começa com "surfaceid://" (12 chars) -> fonte = n[12:]   (substr)
#               senão                                      -> fonte = rs.internalIdentifier
#     emite StreamInfo{ id, name, streamParametersId, sourceSurfaceId=fonte,
#                       ip=sp.ip, port=sp.port, framerate=sp.framerate }
import argparse, json, sys, urllib.request

PREFIX = "surfaceid://"

def get(base, coll):
    with urllib.request.urlopen(f"{base}/displaymanagement/{coll}", timeout=5) as r:
        return json.loads(r.read()).get(coll, [])

def prepared(s):  # getPreparedSurfaceId: trim + lowercase
    return (s or "").strip().lower()

def _id(el):  # id REST do elemento (+0x8 no mss), distinto do campo `name`
    return el.get("id") or el.get("name")

def _ref(v):  # associação VIWI: objeto {"id": ...} (ou string legada) -> o id-alvo
    if isinstance(v, dict):
        return v.get("id") or v.get("name")
    return v or None

def _ids(arr):  # Display::extractIds: cada elemento é objeto {"id": ...} -> lista de ids
    out = []
    for e in (arr or []):
        rid = _ref(e)
        if rid:
            out.append(rid)
    return out

def prepare_streams_list(displays, surfaces, sparams):
    by_disp = {_id(d): d for d in displays}
    by_sp   = {_id(s): s for s in sparams}
    streams, skipped = [], []
    for rs in surfaces:
        rid = _id(rs)
        disp = by_disp.get(_ref(rs.get("display")))
        sp = by_sp.get(_ref(rs.get("streamParameters")))
        if disp is None:
            skipped.append((rid, "display não resolve (RS.display precisa ser objeto {id})")); continue
        if sp is None:
            skipped.append((rid, "streamParameters não resolve (RS.streamParameters precisa ser objeto {id})")); continue
        if rid not in _ids(disp.get("renderingSurfaces")):
            skipped.append((rid, "fora de display.renderingSurfaces (o GATE; não é currentVisibleSurfaces)")); continue
        if not (sp.get("ip") or ""):
            skipped.append((rid, "streamParameters.ip vazio (gate sp+0x78)")); continue
        n = prepared(rs.get("name"))
        source = n[len(PREFIX):] if n.startswith(PREFIX) else (rs.get("internalIdentifier") or "")
        streams.append({
            "id": rid, "name": rs.get("name"),
            "streamParametersId": _ref(rs.get("streamParameters")),
            "sourceSurfaceId": source,
            "ip": sp.get("ip"), "port": sp.get("port"), "framerate": sp.get("framerate"),
        })
    return streams, skipped

def main():
    ap = argparse.ArgumentParser(description="replica prepareStreamsList sobre o estado do DisplayManagement")
    ap.add_argument("--base", default="http://127.0.0.1:8780")
    ap.add_argument("--expect-source", help="falha se nenhum stream tiver essa fonte")
    ap.add_argument("--expect-dest", help="ip:port esperado para a fonte acima")
    args = ap.parse_args()

    displays = get(args.base, "displays")
    surfaces = get(args.base, "renderingSurfaces")
    sparams  = get(args.base, "streamParameters")
    streams, skipped = prepare_streams_list(displays, surfaces, sparams)

    print("== streams que o mss montaria (prepareStreamsList) ==")
    print(json.dumps(streams, indent=2))
    if skipped:
        print("\n== renderingSurfaces puladas ==")
        for rid, why in skipped:
            print(f"  - {rid}: {why}")

    if args.expect_source:
        hit = next((s for s in streams if s["sourceSurfaceId"] == args.expect_source), None)
        if not hit:
            print(f"\nFALHOU: nenhum stream com fonte {args.expect_source!r}", file=sys.stderr)
            sys.exit(1)
        if args.expect_dest:
            got = f"{hit['ip']}:{hit['port']}"
            if got != args.expect_dest:
                print(f"\nFALHOU: fonte {args.expect_source} vai para {got}, esperado {args.expect_dest}",
                      file=sys.stderr)
                sys.exit(1)
        print(f"\nOK: fonte {args.expect_source} -> {hit['ip']}:{hit['port']} @ {hit['framerate']}fps "
              f"(grab ivi {args.expect_source}, stream pro cluster)")
    sys.exit(0)

if __name__ == "__main__":
    main()
