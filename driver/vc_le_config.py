#!/usr/bin/env python3
"""vc_le_config.py <standardConfig.cfg> — mostra, da configuração do motor de navegação do MOI3
(núcleo TSD / Preh Car Connect), o que interessa ao painel de instrumentos: as janelas de desenho
do mapa (MapRendererConfig.windows) e a lista de submercados de mapa com os países de cada um.

Só lê bytes. O arquivo é um contêiner protobuf ("ConfigData, Preh Car Connect"); os nomes e os
números dos campos de MapRendererConfig vêm do descritor embutido no executável
tsd.nav.mainapp.mib3 (lido em 11/10/2026): Window = name 1, width 2, height 3, dpiHorizontal 4,
dpiVertical 5, fov 6, maxFPS 7, id 8, preloading 9, lanesEnabled 10. Rodar com: python3 -I.
"""
import struct
import sys


def varint(b, i):
    v = s = 0
    while True:
        c = b[i]
        i += 1
        v |= (c & 0x7F) << s
        s += 7
        if c < 0x80:
            return v, i


def campos(b, i=0, fim=None):
    fim = len(b) if fim is None else fim
    while i < fim:
        k, i = varint(b, i)
        f, w = k >> 3, k & 7
        if w == 0:
            v, i = varint(b, i)
        elif w == 2:
            n, i = varint(b, i)
            v = b[i:i + n]
            i += n
        elif w == 5:
            v = struct.unpack('<f', b[i:i + 4])[0]
            i += 4
        elif w == 1:
            v = struct.unpack('<d', b[i:i + 8])[0]
            i += 8
        else:
            raise ValueError('tipo de fio %d' % w)
        yield f, v


def janela(d, nome):
    """Acha a mensagem Window pelo nome e devolve os campos escalares."""
    marca = b'\x0a' + bytes([len(nome)]) + nome
    p = d.find(marca)
    if p < 0:
        return None
    # o comprimento da mensagem está no byte anterior (mensagens curtas: 1 byte de tamanho)
    tam = d[p - 1]
    return dict((f, v) for f, v in campos(d, p, p + tam) if not isinstance(v, bytes) or f == 1)


def submercados(d):
    p = d.find(b'\x0a\x06EUROPE')
    if p < 0:
        return
    j = p - 1
    while j > p - 4:          # recua até a etiqueta 0x12 cujo tamanho termina onde o nome começa
        if d[j] == 0x12 and varint(d, j + 1)[1] == p:
            break
        j -= 1
    while d[j] == 0x12:
        n, k = varint(d, j + 1)
        try:
            cs = list(campos(d, k, k + n))
        except (ValueError, IndexError):
            break
        if not cs or cs[0][0] != 1 or any(f not in (1, 2) or not isinstance(v, bytes) for f, v in cs):
            break
        yield cs[0][1].decode('latin1'), [v.decode('latin1') for f, v in cs[1:]]
        j = k + n


def main():
    d = open(sys.argv[1], 'rb').read()
    print('arquivo: %s (%d bytes)' % (sys.argv[1], len(d)))
    p = d.find(b'_EMBEDDED_')
    if p > 0:
        a = d.rfind(b'\n', 0, p)
        print('variante: %s' % d[a + 2:d.find(b'\n', p)].decode('latin1'))
    print('janelas de desenho (MapRendererConfig.windows):')
    for nome in (b'mainunit', b'kombi'):
        w = janela(d, nome)
        if w:
            print('  %-8s %dx%d  %.0f x %.0f dpi  fov %.0f  maxFPS %d  id %d' % (
                nome.decode(), w.get(2, 0), w.get(3, 0), w.get(4, 0), w.get(5, 0), w.get(6, 0), w.get(7, 0), w.get(8, 0)))
    print('submercados de mapa (nome, códigos de país):')
    for nome, cods in submercados(d):
        print('  %-7s %s' % (nome, ' '.join(c for c in cods if len(c) == 2)))


if __name__ == '__main__':
    main()
