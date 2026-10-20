#!/usr/bin/env python2
# redirect_producer.py — Redireciona o multi-screen-server para o surface
# do CarPlay AltScreen via servico RSI DisplayManagement.
#
# Compativel com Python 2.7 (runtime embarcado da unidade MOI3 LG).
#
# Uso:
#   python2 redirect_producer.py <id_surface_carplay> [prefixo]
#
# O <id_surface_carplay> e o identificador IVI do surface onde o CarPlay
# desenha a segunda tela (AltScreen tipo 111). Descobrir via log do
# altscreen_handler.so que registra o ivi_surface_id no stderr.
#
# Contratos VIWI provados na bancada (RE 16/10/2026, gdb, Ghidra 12.1.4):
#   - Associacoes DEVEM ser objeto {"id": ...}, nunca string pura.
#   - Campo `uri` obrigatorio em TODOS os recursos (sem ele o mss pula a RS).
#   - Gate de ativacao = display.renderingSurfaces (nao currentVisibleSurfaces).
#
# Seguranca: so cria recursos novos; NAO toca na navi-kombi-renderingsurface
# nativa. A RS propria fica imune ao throttle de 1fps do NavSd.

import json
import sys
import urllib2

RSI_BASE = "http://127.0.0.1:8780"
DM       = "/displaymanagement"

CLUSTER_IP   = "ff14::2:4"
CLUSTER_PORT = 42800
FRAMERATE    = 30
WIDTH        = 1280
HEIGHT       = 480
PREFIX       = "app-redirect"


def post_resource(collection, body):
    url  = "%s%s/%s" % (RSI_BASE, DM, collection)
    data = json.dumps(body)
    req  = urllib2.Request(url, data, {"Content-Type": "application/json"})
    try:
        resp  = urllib2.urlopen(req, timeout=5)
        loc   = resp.headers.get("Location", "")
        raw   = resp.read()
        rbody = json.loads(raw) if raw else {}
        return resp.getcode(), loc, rbody
    except urllib2.HTTPError as e:
        raw = e.read()
        print("[ERRO] HTTP %d em %s: %s" % (e.code, url, raw[:300]))
        return e.code, "", {}
    except urllib2.URLError as e:
        print("[ERRO] Conexao recusada em %s: %s" % (url, e.reason))
        sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print("Uso: python2 redirect_producer.py <id_surface_carplay> [prefixo]")
        sys.exit(1)

    app_surface = sys.argv[1]
    prefix      = sys.argv[2] if len(sys.argv) > 2 else PREFIX

    disp_id = "%s-display"          % prefix
    rs_id   = "%s-renderingsurface" % prefix
    sp_id   = "%s-streamparameters" % prefix
    rs_name = "surfaceid://%s"      % app_surface

    print("[1/3] Criando streamParameters -> cluster [%s]:%d" % (CLUSTER_IP, CLUSTER_PORT))
    s, loc, b = post_resource("streamParameters", {
        "id":        sp_id,
        "name":      sp_id,
        "uri":       "%s/streamParameters/%s" % (DM, sp_id),
        "ip":        CLUSTER_IP,
        "port":      CLUSTER_PORT,
        "framerate": FRAMERATE,
        "resync":    True,
    })
    print("   -> HTTP %d  Location: %s" % (s, loc))

    print("[2/3] Criando display (gate renderingSurfaces)")
    s, loc, b = post_resource("displays", {
        "id":                     disp_id,
        "name":                   disp_id,
        "uri":                    "%s/displays/%s" % (DM, disp_id),
        "width":                  WIDTH,
        "height":                 HEIGHT,
        "renderingSurfaces":      [{"id": rs_id}],
        "currentVisibleSurfaces": [{"id": rs_id}],
    })
    print("   -> HTTP %d  Location: %s" % (s, loc))

    print("[3/3] Criando renderingSurface (fonte: IVI surface %s)" % app_surface)
    s, loc, b = post_resource("renderingSurfaces", {
        "id":                 rs_id,
        "name":               rs_name,
        "uri":                "%s/renderingSurfaces/%s" % (DM, rs_id),
        "internalIdentifier": app_surface,
        "display":            {"id": disp_id},
        "topLeftX":           0,
        "topLeftY":           0,
        "width":              WIDTH,
        "height":             HEIGHT,
        "streamParameters":   {"id": sp_id},
    })
    print("   -> HTTP %d  Location: %s" % (s, loc))

    print("")
    print("=== Redirect ativado ===")
    print("  Fonte  : %s (IVI id %s)" % (rs_name, app_surface))
    print("  Destino: [%s]:%d @ %dfps  %dx%d" % (CLUSTER_IP, CLUSTER_PORT, FRAMERATE, WIDTH, HEIGHT))
    print("  Verifique o cluster AID -- o mapa do CarPlay deve aparecer.")


if __name__ == "__main__":
    main()
