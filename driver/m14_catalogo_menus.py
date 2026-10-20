#!/usr/bin/env python3
# m14_catalogo_menus.py — escreve o catálogo dos menus do carro (Marco 14, 12/10/2026).
# Estudo num computador; nada daqui vai para o carro.
#
#   python3 -I driver/m14_catalogo_menus.py <pasta da bancada> <pasta de saída>
#
# Escreve <saída>/veiculo.txt e <saída>/codificacao.txt. Na pasta eles se chamam
# stubs/veiculo/catalogo-menus.txt e stubs/codificacao/catalogo-menus.txt; para refazer, gerar numa
# pasta à parte e trocar cada um com mv (o driver/tests/t_catalogo.sh confere que os dois da pasta
# são o que este gerador escreve).
#
# O catálogo liga tudo o que a HMI sabe mostrar em alguns menus do carro, para se ver na tela.
# NÃO É O CARRO, e por isso nenhum número aqui é digitado: as opções saem do esquema dos serviços
# de veículo (stubs/veiculo/gerado/esquema.txt) e o byte de cada função sai da tabela de campos
# da codificação (stubs/codificacao/tabela-campos.tsv), os dois tirados do firmware. O que este
# arquivo traz de seu é a lista abaixo: que função da adaptação e que opções de que serviço fazem
# cada menu aparecer. De onde veio cada linha da lista:
#   - a função: DeviceManager.initTable (o primeiro número de cada AdaptationData é o id do campo
#     "Menu display" do aparelho) e, para os assistentes, o isDeviceCoded de cada serviço do pacote
#     de.vw.mib.asl.internal.carfas.clientapi.impl;
#   - o serviço de opções: ViewOptionServiceFactory.createViewOption (o atributo que a HMI assina
#     quando o aparelho está na adaptação);
#   - e cada linha foi vista funcionar numa central de teste (modelagem-soc.md, Marco 14).
import os
import sys

# (título do bloco, onde aparece, [(o que aparece, constante do campo "Menu display" no DID 0B1B,
#   [(serviço, evento de opções)], nota ou None)])
BLOCOS = [
    ('Configurações do veículo', 'itens da lista que abre em Veículo > Config.', [
        ('"Pneus" (indicador de controle dos pneus, com o botão SET)',
         'ID_DISPLAY_RDK', [('DSICarComfort', 'updateRDKViewOptions')], None),
        ('"Luz", pela iluminação externa (assistência de luzes, horário para ligar, luz de circulação diurna, modo de viagem…)',
         'ID_DISPLAY_EXTERIOR_ILLUMINATION', [('DSICarLight', 'updateExtLightViewOptions')], None),
        ('"Luz", pela iluminação do interior (o mesmo item; é a parte de baixo da mesma tela)',
         'ID_DISPLAY_AMBIENT_ILLUMINATION', [('DSICarLight', 'updateIntLightViewOptions')], None),
        ('"Estacionar e manobrar" (auxílio de estacionamento: ativar automaticamente, volumes…)',
         'ID_DISPLAY_PDC', [('DSICarParkingSystem', 'updateParkingSystemViewOptions')], None),
        ('"Espelho e limpador", pelos espelhos',
         'ID_DISPLAY_CAR_MIRROR', [('DSICarComfort', 'updateMirrorViewOptions')], None),
        ('"Espelho e limpador", pelo limpador (o mesmo item)',
         'ID_DISPLAY_WIPER', [('DSICarComfort', 'updateWiperViewOptions')],
         'sozinho, faz aparecer também "Abrir e fechar": a opção easyEntrySteeringColumn do limpador é mostrada lá (OpenCloseViewOptionListenerImpl.updateWiperViewOptions)'),
        ('"Abrir e fechar" (acionamento das janelas e travamento central)',
         'ID_DISPLAY_CENTRAL_LOCKING', [('DSICarComfort', 'updateDoorLockingViewOptions')],
         'sozinho, faz aparecer também o item "Luz": a opção turnIndRepeat das travas alimenta uma linha da tela de luz, a do pisca de conforto, pelo nome do método (LightDevice.addMappingComfortBlinking)'),
    ]),
    ('Assistentes', 'o app "Assistentes" da grade de menus (tela IQ.DRIVE) e a lista "Configurações da assistência ao condutor" dele', [
        ('"ACC (contr. autom. de dist.)"',
         'ID_DISPLAY_ACC', [('DSICarDriverAssistance', 'updateACCViewOptions')], None),
        ('"Front Assist"',
         'ID_DISPLAY_AWV', [('DSICarDriverAssistance', 'updateAWVViewOptions')], None),
        ('"Ass. perm. faixa (Lane Assist)" e "Emergency Assist"',
         'ID_DISPLAY_LANE_DEPARTURE_WARNING', [('DSICarDriverAssistance', 'updateLDWHCAViewOptions')],
         'é o campo que o LaneAssistService consulta (isDeviceCoded); sem as opções o app aparece e a lista fica vazia'),
        ('"Side Assist"',
         'ID_DISPLAY_LANE_ASSISTANT', [('DSICarDriverAssistance', 'updateSWAViewOptions')],
         'as opções exitAssist e rcta deste serviço fazem aparecer também "Estacionar e manobrar" na lista de configurações do veículo (ParkingDevice.updateSWAViewOptions)'),
        ('"Sist. reconh. sinaliz. trânsito" e "Reconhecimento de reboque"',
         'ID_DISPLAY_ROAD_SIGN_IDENTIFICATION', [('DSICarDriverAssistance', 'updateTSDViewOptions')], None),
        ('"Sistema reconh. de cansaço"',
         'ID_DISPLAY_WEARINESS_RECOGNITION', [('DSICarDriverAssistance', 'updateMKEViewOptions')], None),
    ]),
]

# opções que ficam de fora, com a razão
FORA = {
    'intLightViewOptions.intLightRGBColorList.state':
        'com ela a HMI pede a lista de cores da luz ambiente (requestIntLightRGBColorList), que o carro de mentira não sabe responder: tempos esgotados de 10 s',
}

CAB_VEICULO = '''\
# catalogo-menus.txt — CATÁLOGO, NÃO É O CARRO: os outros menus do carro, do lado do veículo.
# Marco 14, 12/10/2026. Estudo num computador; nada daqui vai para o carro.
#
# ESTE ARQUIVO NÃO ENTRA EM NENHUMA PARTIDA POR PADRÃO. Ele diz "esta opção existe" para TODAS as
# opções que o firmware conhece em cinco menus de "Configurações do veículo" e nos assistentes do
# app "Assistentes", para se ver na tela o que a HMI sabe mostrar. Nenhuma linha é palpite sobre
# o Jetta: há aqui farol a laser, cortina elétrica e ACC, e a ficha do carro não diz quais destas
# opções ele tem. O valor de cada coisa fica no padrão do carro de mentira, que é zero ("Off",
# "Cedo", caixas desmarcadas): isso também não é ajuste de carro nenhum.
#
# Serve para comparar a tela do simulador com a do carro, item por item, e para tirar daqui,
# para o jetta-gli-2021-deducoes.txt, só as linhas das opções que o carro tiver.
# Precisa do par dele na codificação (stubs/codificacao/catalogo-menus.txt): a HMI só pede estas
# opções ao carro quando a função está na adaptação, e a função sem as opções não cria o menu.
#
# Para ver, numa central de teste à parte (não toca na central ao vivo):
#   sh driver/m14_menus.sh             sobe a central "menus" com este catálogo
#   sh janela/abre.sh --container moi3-m8-menus
#   sh driver/m14_menus.sh desce
#
# GERADO por driver/m14_catalogo_menus.py do esquema (gerado/esquema.txt); não editar à mão. Os
# nomes das opções são os do firmware. 2 = VIEWOPTIONSTATE_NORMAL_OPERATION: existe e pode ser usada.
'''

CAB_CODIFICACAO = '''\
# catalogo-menus.txt — CATÁLOGO, NÃO É O CARRO: os outros menus do carro, do lado da codificação.
# Marco 14, 12/10/2026. Estudo num computador; nada daqui vai para o carro, e nada foi lido dele.
#
# ESTE ARQUIVO NÃO ENTRA EM NENHUMA PARTIDA POR PADRÃO. Ele liga, na lista de funções do carro da
# adaptação do 5F (DID 0B1B; em cada byte o bit 7 é "Menu display"), as funções de cinco menus de
# "Configurações do veículo" e dos assistentes do app "Assistentes". Que funções a adaptação do
# Jetta traz, só a leitura do carro diz (está pedida na ficha). É o par do
# stubs/veiculo/catalogo-menus.txt, e o comando para ver está no cabeçalho de lá.
#
# A lista de aparelhos (DID 0B1D) não entra: para estes menus a HMI só consulta a função (medido
# com e sem o aparelho, rodadas pn1, l2 e l3 de 12/10/2026).
#
# GERADO por driver/m14_catalogo_menus.py da tabela de campos (tabela-campos.tsv); não editar à
# mão. O byte e o bit de cada linha são os da tabela, que saiu do firmware; a numeração dos bytes
# é a do DID nesta bancada, e o nome com que cada função aparece no ODIS não foi conferido.
'''


def le_esquema(caminho):
    msgs, evt, cur = {}, {}, None
    for l in open(caminho, encoding='utf-8'):
        l = l.split('#')[0].split()
        if not l:
            continue
        if l[0] == 'M':
            cur = l[1]
            msgs[cur] = []
        elif l[0] == 'C':
            msgs[cur].append((l[2], l[3], l[4], l[5] if len(l) > 5 else None))
        elif l[0] == 'E':
            evt[(l[1], l[5])] = l[6] if len(l) > 6 else None
    return msgs, evt


def folhas(msgs, m, pre='', prof=0):
    out = []
    for nome, tipo, rotulo, sub in msgs[m]:
        if rotulo != 'um':
            continue
        if tipo == 'msg':
            if prof < 6:
                out += folhas(msgs, sub, pre + nome + '.', prof + 1)
        else:
            out.append(pre + nome)
    return out


def le_tabela(caminho):
    campos = {}
    for n, l in enumerate(open(caminho, encoding='utf-8')):
        c = l.rstrip('\n').split('\t')
        if n == 0 or len(c) < 15:
            continue
        campos[(c[2], c[12])] = (int(c[4]), int(c[6]), int(c[7]), c[14])
    return campos


def main():
    if len(sys.argv) != 3:
        sys.exit('uso: m14_catalogo_menus.py <pasta da bancada> <pasta de saída>')
    bancada, saida = sys.argv[1], sys.argv[2]
    msgs, evt = le_esquema(os.path.join(bancada, 'stubs/veiculo/gerado/esquema.txt'))
    campos = le_tabela(os.path.join(bancada, 'stubs/codificacao/tabela-campos.tsv'))
    v, c = [CAB_VEICULO.rstrip('\n')], [CAB_CODIFICACAO.rstrip('\n')]
    nv = nc = 0
    for titulo, onde, itens in BLOCOS:
        for o in (v, c):
            o += ['', '# ' + '=' * 96, '# %s: %s' % (titulo, onde), '# ' + '=' * 96]
        for oque, const, eventos, nota in itens:
            if ('0B1B', const) not in campos:
                sys.exit('a tabela de campos não tem %s no DID 0B1B' % const)
            byte, bit, bits, desc = campos[('0B1B', const)]
            if (bit, bits) != (7, 1) or 'Menu display' not in desc:
                sys.exit('%s não é um campo "Menu display" de um bit no bit 7: %r' % (const, (bit, bits, desc)))
            c += ['', '# ---- ' + oque]
            if nota:
                c.append('# ' + nota)
            c.append('%-20s # %s: "%s"' % ('0B1B @%d.7+1 = 1' % byte, const, desc))
            nc += 1
            v += ['', '# ---- ' + oque]
            if nota:
                v.append('# ' + nota)
            for svc, ev in eventos:
                if (svc, ev) not in evt or evt[(svc, ev)] not in msgs:
                    sys.exit('o esquema não tem o evento %s.%s' % (svc, ev))
                v.append('[%s.%s]' % (svc, ev))
                for p in folhas(msgs, evt[(svc, ev)]):
                    if not p.endswith('.state'):
                        continue
                    if p in FORA:
                        v.append('# %s fica de fora: %s' % (p, FORA[p]))
                        continue
                    v.append(p + ' = 2')
                    nv += 1
    os.makedirs(saida, exist_ok=True)
    for nome, linhas in (('veiculo.txt', v), ('codificacao.txt', c)):
        with open(os.path.join(saida, nome), 'w', encoding='utf-8') as f:
            f.write('\n'.join(linhas) + '\n')
    print('%d opções em 2 e %d funções na adaptação' % (nv, nc))


if __name__ == '__main__':
    main()
