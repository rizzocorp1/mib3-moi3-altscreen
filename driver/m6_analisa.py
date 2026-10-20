#!/usr/bin/env python3
"""m6_analisa.py <pasta de saídas> <tag> [<tag de referência>] — resumo de uma rodada do Marco 6.

Lê o que o driver/m6_rodada.sh gravou (<tag>.out, <tag>.barramento.log e as fotos) e mostra o
que a HMI diz que leu de codificação e adaptação, as telas por que passou e o que foi pedido ao
persistence-app pelo barramento. Com uma segunda rodada, mostra só o que difere dela.
Rodar no Mac com python3 -I. Só lê arquivos; estudo num computador.
"""
import base64
import collections
import difflib
import hashlib
import os
import re
import sys


def campos_protobuf(b):
    """Campos de um protobuf simples: [(número, inteiro ou bytes)]."""
    out, i = [], 0

    def varint():
        nonlocal i
        v = s = 0
        while True:
            c = b[i]
            i += 1
            v |= (c & 0x7F) << s
            s += 7
            if not c & 0x80:
                return v

    try:
        while i < len(b):
            tag = varint()
            campo, tipo = tag >> 3, tag & 7
            if tipo == 0:
                out.append((campo, varint()))
            elif tipo == 2:
                n = varint()
                out.append((campo, b[i:i + n]))
                i += n
            else:
                break
    except IndexError:
        pass
    return out


def zigue_zague(v):
    """Os inteiros do serviço de persistência são sint: 1 vai como 2, -1 como 1."""
    return (v >> 1) ^ -(v & 1)


def fatos(pasta, tag):
    f = collections.OrderedDict()
    linhas = open(os.path.join(pasta, tag + '.out'), errors='replace').read().splitlines()

    def conta(s):
        return sum(1 for linha in linhas if s in linha)

    f['leituras sem dado'] = conta('Diagnosis data not available')
    f['"No data received"'] = conta('No data received for')
    f['aviso do skin'] = conta('CURRENT SKIN DOES NOT BELONG TO CODED BRAND')
    f['o que a HMI registrou do diagnóstico'] = sorted(
        re.sub(r'^.*?\] ', '', linha) for linha in linhas
        if 'ASL.DIAGNOSTIC]' in linha and not re.search(
            r'Diagnosis data not available|CORE\.DIAG|No data received|calling back pending|TargetDiagnosis', linha))
    f['tipo de veículo'] = sorted(set(re.findall(r'vehicle\.id:=(\d+)', '\n'.join(linhas))))
    f['aplicativos excluídos por chave de recurso'] = sorted(
        set(re.sub(r'^.*?\] ', '', linha) for linha in linhas if 'is excluded by feature flag' in linha))
    f['telas'] = [m.group(1) for linha in linhas for m in [re.search(r'enterView\(\S+ \((\w+)', linha)] if m]
    recebidos = collections.OrderedDict()
    for linha in linhas:
        m = re.search(r'\[CORE\.DIAG\.A\]([0-9a-f]+)\.([0-9a-f]+) b\[([^\]]*)\]', linha)
        if m:
            recebidos[(int(m.group(1), 16), int(m.group(2), 16))] = base64.b64decode(m.group(3)) if m.group(3) else b''
    f['conjuntos lidos'] = recebidos
    pedidos, respostas, do_mcu = collections.Counter(), collections.Counter(), 0
    barramento = os.path.join(pasta, tag + '.barramento.log')
    if os.path.exists(barramento):
        for linha in open(barramento, errors='replace'):
            if ' if=1901 ' in linha:
                do_mcu += 1
                continue
            m = re.match(r'canal=0x[0-9a-f]+ if=(\d+) inst=\d+ api=(\d+) pb=(.*)', linha)
            if not m:
                continue
            interface, api = int(m.group(1)), int(m.group(2))
            if (interface, api) not in ((1400, 1003), (1401, 2003)):
                continue
            d = dict(campos_protobuf(bytes.fromhex(m.group(3).replace(' ', ''))))
            ns = hex(zigue_zague(d.get(1, 0)))
            if interface == 1400:
                pedidos[ns] += 1
            else:
                respostas['%s com código %d' % (ns, zigue_zague(d.get(4, 0)))] += 1
    f['mensagens do microcontrolador no barramento'] = do_mcu
    f['leituras pedidas ao persistence-app, por namespace'] = dict(pedidos)
    f['respostas do persistence-app'] = dict(respostas)
    fotos = collections.OrderedDict()
    for nome in sorted(os.listdir(pasta)):
        m = re.match(r'foto_%s_(\d+)\.(png|ppm)$' % re.escape(tag), nome)
        if m:
            fotos[int(m.group(1))] = hashlib.md5(open(os.path.join(pasta, nome), 'rb').read()).hexdigest()
    f['fotos'] = collections.OrderedDict(sorted(fotos.items()))
    return f


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    pasta, tag = sys.argv[1], sys.argv[2]
    ref = sys.argv[3] if len(sys.argv) > 3 else None
    a = fatos(pasta, tag)
    b = fatos(pasta, ref) if ref else None
    for chave, valor in a.items():
        if chave == 'conjuntos lidos':
            cheios = [(k, v) for k, v in valor.items() if v]
            print('%s: %d, dos quais %d com dado' % (chave, len(valor), len(cheios)))
            for (ns, did), dado in cheios:
                if ns == 0x2000000 or b is None and len(dado) <= 4:
                    print('   %x.%04x: %s' % (ns, did, dado.hex(' ')))
            if b is not None:
                for k, v in valor.items():
                    if b[chave].get(k) != v:
                        print('   %x.%04x mudou: %s -> %s' % (k[0], k[1], (b[chave].get(k) or b'').hex(' ') or '(vazio)',
                                                               v.hex(' ') or '(vazio)'))
            continue
        if chave == 'fotos':
            if b is None:
                print('%s: aos %s s' % (chave, ', '.join(str(t) for t in valor)))
            else:
                print('%s: %s' % (chave, '; '.join(
                    '%d s %s' % (t, 'igual' if b[chave].get(t) == h else 'DIFERENTE') for t, h in valor.items())))
            continue
        if b is None:
            print('%s: %s' % (chave, valor if not isinstance(valor, list) or len(valor) < 16 else '%d itens' % len(valor)))
        elif valor == b[chave]:
            print('%s: igual' % chave)
        elif isinstance(valor, list):
            print('%s: DIFERE' % chave)
            for linha in difflib.unified_diff(b[chave], valor, ref, tag, lineterm='', n=0):
                if not linha.startswith(('---', '+++', '@@')):
                    print('   ' + linha[:200])
        else:
            print('%s: DIFERE\n   %s: %s\n   %s: %s' % (chave, ref, b[chave], tag, valor))
    return 0


if __name__ == '__main__':
    sys.exit(main())
