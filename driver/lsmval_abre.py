#!/usr/bin/env python3
# lsmval_abre.py — lê a memória de estado da central (o arquivo lsmval) e diz o que há em cada
# linha (Marco 6, 11/10/2026). Só lê; estudo num computador, nada vai para o carro.
#
#   python3 -I driver/lsmval_abre.py <lsmval>              uma entrada por valor guardado
#   python3 -I driver/lsmval_abre.py <antes> <depois>      o que entrou, o que saiu e o que mudou
#
# Rodar sempre com -I. Com dois arquivos sai com 0 se nada mudou e com 1 se mudou; arquivo
# ausente ou linha fora do formato: 2, com o arquivo e a linha.
#
# O que o programa desfaz, lido no jar da HMI (decompilado):
#   * a chave (PersistenceKeyUtil.calcKey): (número do "persistable" << 32) | (perfil << 24).
#     Perfil 0xFF é a parte comum a todos os usuários; 1 é o usuário 1. A chave 0 é a declaração
#     do módulo. Os namespaces de 5050 a 5095 são os módulos do /vw/hmi/res/persistenceIds.res
#     (lista em traces/m6_lsm_persistenceIds.txt); os mais antigos (1101, 5000, 5008, 6000) usam
#     chaves pequenas, sem persistable nem perfil.
#   * o embrulho do valor (Compression e RecordByteArray.writeToDsi): com 50 bytes ou mais, se
#     comprimir, "11 22 33 44" + gzip(tamanho em 4 bytes + dados); senão, com mais de 4 bytes,
#     "55 66 77 88" + 8 bytes com o Adler-32 dos dados + dados; com até 4 bytes vai cru.
#   * os campos (o serializador de cada módulo: SystemCommonSerializer, SoundSettingsRootSerializer…).
#     Só estão aqui os valores cujo formato foi lido no código; a tabela do que cada um guarda e
#     de como foi conferido está em stubs/persistencia/README.md. Valor sem tabela sai em hexa.
import sys, os, gzip, zlib, struct

MODULOS = {5050: 'SystemEarly', 5051: 'SystemCommon', 5052: 'AmFm', 5055: 'Navi', 5056: 'Sound',
           5057: 'Media', 5058: 'Speech', 5059: 'InstrumentCluster', 5060: 'GlobalSetupWizard',
           5061: 'EntertainmentManager', 5062: 'ExboxKorea', 5063: 'PictureStore', 5064: 'VicsEtc',
           5065: 'Homescreen', 5066: 'GridMenu', 5067: 'NavInfoTile', 5068: 'Car', 5069: 'CarEarly',
           5070: 'WebRadio', 5071: 'FavoritesConfiguration', 5072: 'OnlineUota', 5077: 'SimpleGridMenu',
           5078: 'Iaa', 5079: 'WebAppManagement', 5080: 'ClockTileM', 5081: 'MediaControl',
           5082: 'Homescreen2', 5083: 'DigitalKey', 5084: 'PendingPsoIdExport', 5085: 'HomescreenSeat',
           5086: 'CarFas', 5087: 'Mirrorlink', 5088: 'Climate2', 5092: 'OffClock',
           5094: 'Smartphoneintegration', 5095: 'Online'}
ANTIGOS = {(1101, 10): 'idioma (LSC)', (5000, 200): 'dados "early" (PersistableEarlyData)',
           (5000, 300): 'tipo de motor (PersistableEngineType)', (5008, 100): 'telefone (PhonePersistence)',
           (6000, 100): 'último modo, para o lado da LG (writeLastModeToSouthSide)'}

# Tabelas de campos. Tipos: h = 2 bytes, i = 4, q = 8, b = 1 byte, ? = sim/não, s = texto que
# pode faltar (1 byte "falta" + tamanho em 4 bytes + letras), L<tabela> = lista (quantidade em 4
# bytes e, por item, 1 byte "falta" + os campos da tabela), Q = lista de números de 8 bytes.
COACHMARK = [('identifier', 'b'), ('value', '?')]
SPELLER = [('key', 's'), ('value', 's')]
PAGINA = [('pageId', 'q'), ('pageFinished', '?')]
ATALHO = [('state', 'i'), ('cioId', 'q')]
CAMPOS = {
    (5051, 1, 'usuário'): [('versão', 'h'), ('displayBrightness', 'b'), ('showClock', '?'), ('offclockLayout', 'b'),
                           ('bitfieldLanguagesThatWereSetBefore', 'q'), ('acousticTouchscreenFeedback', '?'),
                           ('proximityEnabled', '?'), ('acousticSensorButtonFeedback', '?'), ('gestureActive', '?'),
                           ('gestureInformationActive', '?'), ('gestureSoundeffectActive', '?'),
                           ('lastContentId', 's'), ('backgroundIndex', 'i'), ('globalCoachmarkSetup', '?'),
                           ('coachmarks', COACHMARK)],
    (5051, 1, 'comum'): [('versão', 'h'), ('spellerLayout', 'i'), ('automaticTimePopupActivated', '?'),
                         ('valetParkingActive', '?'), ('rearSeatControlLocked', '?'), ('menuMode', 'i'),
                         ('dayNightMode', 'i'), ('onlineAvailable', '?'), ('rearSeatAlertEnabled', '?'),
                         ('spellerSettings', SPELLER)],
    (5051, 4, 'comum'): [('versão', 'h'), ('distanceUnit', 'i'), ('temperatureUnit', 'i'), ('volumeUnit', 'i'),
                         ('petrolConsumptionUnit', 'i'), ('gasConsumptionUnit', 'i'),
                         ('electricConsumptionUnit', 'i'), ('pressureUnit', 'i'), ('speedUnit', 'i'),
                         ('astaWeightUnit', 'i')],
    (5056, 2, 'comum'): [('versão', 'h'), ('lsmTabbar', 'i')],
    (5056, 2, 'usuário'): [('versão', 'h'), ('eqMode', 'i'), ('positionMode', 'i'), ('sensorButtonFeedbackOn', '?'),
                           ('spaceGestureSoundEffectOn', '?'), ('welcomeSoundOn', '?'), ('leavingSoundOn', '?'),
                           ('touchscreenToneOn', '?')],
    (5069, 1, 'comum'): [('versão', 'h'), ('ambienceLightColor', 'i'), ('carMainViewId', 'i'),
                         ('vehicleSubViewId', 'i'), ('tbtSubViewId', 'i'), ('sportSubViewId', 'i'),
                         ('offroadSubViewId', 'i'), ('etrakSubViewId', 'i'), ('drivingDataSubViewId', 'i')],
    (5077, 1, 'comum'): [('versão', 'h')],
    (5077, 1, 'usuário'): [('versão', 'h'), ('mainMenuEntryCioIds', 'Q')],
    (5060, 1, 'comum'): [('versão', 'h')],
    (5060, 1, 'usuário'): [('versão', 'h'), ('listWizardPages', PAGINA), ('dontShowAgain', '?')],
    (5061, 1, 'comum'): [('versão', 'h')],
    (5061, 1, 'usuário'): [('versão', 'h'), ('context', 'i')],
    (5058, 2, 'comum'): [('versão', 'h'), ('currentVoiceType', 'i')],
    (5058, 2, 'usuário'): [('versão', 'h'), ('displayAvailableCommandsOnHeadunit', '?'),
                           ('displayAvailableCommandsOnIC', '?'), ('useSpeechSessionStartTone', '?'),
                           ('useSpeechSessionEndTone', '?'), ('recognizerReadyTone', '?'),
                           ('recognizerEndTone', '?'), ('wakeUpPhrase', '?')],
    (5078, 1, 'comum'): [('versão', 'h')],
    (5078, 1, 'usuário'): [('versão', 'h'), ('showWelcomeScreen', '?')],
    (5092, 1, 'comum'): [('versão', 'h')],
    (5092, 1, 'usuário'): [('versão', 'h'), ('offClockLayout', 'b')],
    (5065, 1, 'comum'): [('versão', 'h'), ('clockLayout', 'i')],
    (5086, 1, 'comum'): [('versão', 'h'), ('lastView', 'i')],
    (5083, 1, 'comum'): [('versão', 'h'), ('lastSmartCard', 's')],
    (5050, 1, 'comum'): [('versão', 'h'), ('lastValueFeatureFlagSdsCoding', '?'), ('lastValueFeatureFlagSdsSwap', '?'),
                         ('lastValueFeatureFlagSdsSwapMarket', '?'), ('lastValueFeatureFlagSdsBought', '?')],
    (5059, 1, 'comum'): [('versão', 'h'), ('useDABLongLabel', '?'), ('useSDARSLongLabel', '?')],
    (5059, 2, 'comum'): [('versão', 'h'), ('instrumentClusterDisplaySize', 'i')],
    (5071, 1, 'comum'): [('versão', 'h'), ('carBar', ATALHO)],
    (5071, 1, 'usuário'): [('versão', 'h'), ('controlCenter', ATALHO), ('tileM', ATALHO)],
    (5000, 200): [('versão', 'h'), ('displayBrightness', 'b'), ('showClock', '?'), ('climateSeatsAvailable', '?'),
                  ('dayNightMode', 'b'), ('lastContext', 'b'), ('lastConnRadio', '?'), ('lastActiveSkin', 'b'),
                  ('lastActiveLanguagePrefetch', 'b'), ('acousticTouchscreenFeedback', '?'), ('userMuteStarted', 'i'),
                  ('lastClamp15OffTimestamp', 'q'), ('languageWasUsedOnceBeforeBitfield', 'i'), ('spellerLayout', 'b'),
                  ('proximityAnimationsActive', '?'), ('debugProbeConfiguration', 'i'), ('autoDaylightSaving', '?'),
                  ('sdsActive', '?'), ('testmodeAlternativeViewSetup1', '?'), ('testmodeAlternativeViewSetup2', '?'),
                  ('effectiveSkinMode', 'b'), ('lastPersistedCodedSkin', 'b'), ('navOverwriteEnableOnlineFunctions', '?'),
                  ('pqLscAutomaticModeActive', '?'), ('uotaCustomerSwdlIsRunning', '?'), ('hwrEnabled', '?'),
                  ('sportHMIEnabled', '?'), ('lastActiveAudioContext', 'i')],
    (5000, 300): [('versão', 'h'), ('primaryEngineType', 'b'), ('secondaryEngineType', 'b')],
    (6000, 100): [('lastSouthSideUserContext', 'h'), ('lastSouthSideAudioContext', 'h')],
}


class Erro(Exception):
    pass


def le(caminho):
    """devolve {(namespace, chave): (tipo, bytes ou números, letras da linha)}"""
    try:
        texto = open(caminho, 'rb').read().decode('latin-1')
    except OSError as e:
        raise Erro(f"{caminho}: não consegui ler ({e.strerror})")
    d = {}
    for n, linha in enumerate(texto.split('\n'), 1):
        linha = linha.split('#', 1)[0].strip()
        if not linha:
            continue
        f = linha.split()
        try:
            tipo, ns, chave = f[0], int(f[1]), int(f[2])
            vals = [int(x) for x in f[3:]]
            if tipo not in ('0', '1') or (tipo == '0' and any(not 0 <= v <= 255 for v in vals)):
                raise ValueError
        except (ValueError, IndexError):
            raise Erro(f"{caminho}, linha {n}: fora do formato \"<tipo> <namespace> <chave> <valores>\"")
        d[(ns, chave)] = (tipo, bytes(vals) if tipo == '0' else vals, len(linha))
    return d


def desembrulha(b):
    """devolve (conteúdo, como veio embrulhado, soma certa?)"""
    if len(b) > 4 and b[:4] == bytes([11, 22, 33, 44]):
        try:
            d = gzip.decompress(b[4:])
            return d[4:], 'comprimido', struct.unpack('>i', d[:4])[0] == len(d) - 4
        except (OSError, EOFError, struct.error, zlib.error):
            return b, 'comprimido', False
    if len(b) > 4 and b[:4] == bytes([55, 66, 77, 88]) and len(b) >= 12:
        p = b[12:]
        return p, 'com soma', struct.unpack('>q', b[4:12])[0] == zlib.adler32(p)
    return b, 'cru', True


def de_quem(ns, chave):
    """devolve (texto, (namespace, persistable, parte) ou (namespace, chave) para achar a tabela)"""
    if ns in MODULOS:
        if chave == 0:
            return f"{ns} {MODULOS[ns]}, declaração do módulo", None
        pid, perfil = chave >> 32, (chave >> 24) & 0xFF
        parte = 'comum' if perfil == 0xFF else 'usuário'
        return (f"{ns} {MODULOS[ns]}, persistable {pid}, " + ('comum' if perfil == 0xFF else f"usuário {perfil}"),
                (ns, pid, parte))
    return f"{ns} ?, chave {chave}" + (f": {ANTIGOS[(ns, chave)]}" if (ns, chave) in ANTIGOS else ''), (ns, chave)


def campos(tabela, p):
    """devolve [(nome, posição, texto)] ou None se o conteúdo não couber na tabela"""
    out, i = [], 0

    def um(tipo):
        nonlocal i
        if tipo == 'h':
            v = struct.unpack('>h', p[i:i + 2])[0]; i += 2; return str(v)
        if tipo == 'i':
            v = struct.unpack('>i', p[i:i + 4])[0]; i += 4; return str(v)
        if tipo == 'q':
            v = struct.unpack('>q', p[i:i + 8])[0]; i += 8; return f"{v} (0x{v & 0xFFFFFFFFFFFFFFFF:x})"
        if tipo == 'b':
            v = p[i]; i += 1; return str(v)
        if tipo == '?':
            v = p[i]; i += 1; return {0: 'não', 1: 'sim'}.get(v, f"byte {v}")
        if tipo == 's':
            falta = p[i]; i += 1
            if falta:
                return '(falta)'
            n = struct.unpack('>i', p[i:i + 4])[0]; i += 4
            if n < 0 or i + n > len(p):
                raise IndexError
            v = p[i:i + n].decode('utf-8', 'replace'); i += n; return v
        if tipo == 'Q':
            n = struct.unpack('>i', p[i:i + 4])[0]; i += 4
            if n < 0 or i + 8 * n > len(p):
                raise IndexError
            v = [p[i + 8 * k:i + 8 * k + 8].hex() for k in range(n)]; i += 8 * n
            return f"{n} itens: " + ' '.join(v)
        n = struct.unpack('>i', p[i:i + 4])[0]; i += 4          # lista de itens com tabela própria
        if n < 0 or n > len(p):
            raise IndexError
        itens = []
        for _ in range(n):
            falta = p[i]; i += 1
            itens.append('(falta)' if falta else '{' + ', '.join(f"{nome}={um(t)}" for nome, t in tipo) + '}')
        return f"{n} itens" + (': ' + ' '.join(itens) if itens else '')

    try:
        for nome, tipo in tabela:
            pos = i
            out.append((nome, pos, um(tipo)))
    except (IndexError, struct.error):
        return None
    return out if i == len(p) else None


def hexa(p, limite=48):
    return p[:limite].hex(' ') + (' …' if len(p) > limite else '')


def mostra(caminho):
    d = le(caminho)
    for (ns, chave), (tipo, v, letras) in d.items():
        quem, tab = de_quem(ns, chave)
        if tipo != '0':
            print(f"{quem}: número {v}"); continue
        if (ns, chave) == (1101, 10):
            print(f"{quem}: idioma {v[:5].decode('latin-1')}, tipo de voz {v[5]}, {len(v)} bytes, {letras} letras"); continue
        p, como, certa = desembrulha(v)
        soma = {'cru': 'sem soma', 'com soma': 'soma certa' if certa else 'SOMA ERRADA',
                'comprimido': 'comprimido' if certa else 'comprimido, COM ERRO'}[como]
        print(f"{quem}: {len(v)} bytes, {letras} letras, conteúdo {len(p)} ({soma})")
        c = campos(CAMPOS[tab], p) if tab in CAMPOS and certa else None
        if c:
            for nome, pos, txt in c:
                print(f"      {pos:3d} {nome} = {txt}")
        else:
            print(f"      {hexa(p)}")


def compara(antes, depois):
    a, b = le(antes), le(depois)
    mudou = 0
    for k in sorted(set(a) | set(b)):
        quem, tab = de_quem(*k)
        if k not in a:
            mudou += 1; p = desembrulha(b[k][1])[0] if b[k][0] == '0' else b''
            print(f"entrou: {quem}: conteúdo {len(p)}: {hexa(p)}"); continue
        if k not in b:
            mudou += 1; print(f"saiu: {quem}"); continue
        if a[k][:2] == b[k][:2]:
            continue
        mudou += 1
        if a[k][0] != '0' or b[k][0] != '0':
            print(f"mudou: {quem}: {a[k][1]} -> {b[k][1]}"); continue
        pa, pb = desembrulha(a[k][1])[0], desembrulha(b[k][1])[0]
        print(f"mudou: {quem}: conteúdo de {len(pa)} para {len(pb)} bytes" if len(pa) != len(pb) else f"mudou: {quem}:")
        ca = campos(CAMPOS[tab], pa) if tab in CAMPOS else None
        cb = campos(CAMPOS[tab], pb) if tab in CAMPOS else None
        if len(pa) == len(pb):
            nomes = {}
            if cb:
                for nome, pos, _ in cb:
                    nomes[pos] = nome
            dif = [i for i in range(len(pa)) if pa[i] != pb[i]]
            if len(dif) > 8:                                   # lista reordenada, por exemplo
                print(f"      {len(dif)} bytes, da posição {dif[0]} à {dif[-1]}")
            for i in dif if len(dif) <= 8 else []:
                dono = max((pos for pos in nomes if pos <= i), default=None)
                print(f"      posição {i}: {pa[i]:02x} -> {pb[i]:02x}" + (f" ({nomes[dono]})" if dono is not None else ''))
        if ca and cb and len(ca) == len(cb):
            for (nome, _, x), (_, _, y) in zip(ca, cb):
                if x != y:
                    print(f"      {nome}: {x} -> {y}")
        elif len(pa) != len(pb):
            print(f"      antes : {hexa(pa, 64)}"); print(f"      depois: {hexa(pb, 64)}")
    if not mudou:
        print("nenhum valor mudou")
    return 1 if mudou else 0


if __name__ == '__main__':
    if len(sys.argv) not in (2, 3):
        sys.stderr.write("uso: python3 -I lsmval_abre.py <lsmval> | <antes> <depois>\n"); sys.exit(2)
    try:
        sys.exit(mostra(sys.argv[1]) or 0 if len(sys.argv) == 2 else compara(sys.argv[1], sys.argv[2]))
    except Erro as e:
        print(e); sys.exit(2)
