/* dsistub.c — provedor DSI de mentira para o sandbox (estudo, sem carro). Marco 3, 11/10/2026.
 * Sucede o dmstub.c de 10/10. Liga-se ao kipc_hub e faz duas coisas que a HMI precisa para
 * sair da tela apagada e compor a tela sem esperas:
 *
 * 1. Serviço de display (domínio PID_DISPLAY_MGR = 0x4000). Anuncia o serviço (interface 9000)
 *    e RESPONDE aos pedidos da HMI (interface 100), cada um com a resposta que ela espera
 *    (interface 101):
 *      getDisplaySize 1043   -> getDisplaySizeResult 2029   {displayID, largura, altura, 0}
 *      deferUpdates 1040     -> deferUpdatesResult 2031     {displayID, displayable, 0}
 *      showDisplayables 1039 -> showDisplayablesResult 2027 {displayID, os mesmos displayables, 0, sessão}
 *      getExtents 1005       -> getExtentsResult 2000       {cid, largura, altura}
 *      getDisplayPower 1010  -> getDisplayPowerResult 2004  {displayID, 1}
 *      setDisplayBrightness 1011 {displayID, porcentagem}   guarda; não tem resposta própria
 *      getDisplayBrightness 1022 -> getDisplayBrightnessResult 2012 {displayID, porcentagem}
 *    O dmstub mandava o tamanho sem ninguém pedir, e a HMI descartava ("unexpected").
 *    O brilho (12/10/2026): ao escolher o nível de luminosidade na tela, e também na partida, a
 *    HMI grava a porcentagem (nível 0 a 4 = 100, 75, 50, 25 e 0 %) e pede a leitura logo atrás
 *    (DisplayBrightness.setDsiDisplayManagement). É da resposta da leitura que ela tira o nível
 *    que mostra e que guarda na memória (ASLDisplayManagementTarget.
 *    processGetDisplayBrightnessResponse). Sem resposta a escolha não pegava, a tela mostrava
 *    sempre "Muito alta" e o registro ganhava um tempo esgotado de 10 s por partida. O stub
 *    devolve o que a HMI gravou, e só isso: leitura sem gravação anterior fica sem resposta,
 *    para a HMI não guardar um brilho que ninguém escolheu. Nada é feito com o valor: a imagem
 *    não escurece.
 *
 * 2. Estado de energia (domínio PID_MCU_INTERPRETER = 0x400000, interface 1101). No carro quem
 *    informa é o mcu-interpreter, a partir do microcontrolador; aqui não há microcontrolador e a
 *    HMI fica em powerstate -1, tela apagada. O stub publica:
 *      updatePowerManagementState (api 1) {estado, evento 0, válido 1}   1 = POWERSTATE_ON
 *      updateClampSignal (api 4)          {{S, 15, X, 50}, válido 1}     ignição ligada
 *    de segundo em segundo e, desde o Marco 8, também NA HORA em que a HMI assina o atributo
 *    (api 9999 na interface 1100, atributos em zigue-zague: 1 = estado, 4 = ignição), como faz
 *    um fornecedor DSI de verdade. Sem isso a HMI ficava até 1 s sem saber que a central estava
 *    ligada, e o aplicativo da tela Welcome, quando subia antes, lia o estado inicial
 *    (WelcomeScreenLogicController.handlePowerState( 5 ), POWER_STATE_NOT_INIT). Isso não é o
 *    que fazia a Welcome sumir: ela era pedida do mesmo jeito; quem a segurava era a câmera de
 *    ré, abaixo. Na assinatura vai também, uma vez só:
 *      updateRVCActive (api 7)            {ativa 0, válido 1}            câmera de ré não ativa
 *    A HMI segura todas as telas da partida até receber isso, ou até passarem 6 s
 *    (ContentProcessorManagerImpl, fase "early RVC"; o prazo é a propriedade
 *    de.vw.mib.contentmanager.earlyrvc.timeout). Sem a resposta, a primeira tela só saía 6 s
 *    depois, e a tela Welcome, que dura 5 s, já tinha acabado quando o carro de mentira
 *    (veiculostub) encurtava o resto da partida.
 *
 * 3. Entrada (Marco 5): teclas e toque, pelo serviço DSIKeyPanel (domínio 0x80000000000,
 *    interface 301). No carro quem publica é o touch-app; aqui os eventos vêm de um arquivo de
 *    comandos, uma linha por evento, que qualquer um pode ir acrescentando:
 *      tecla <teclado> <tecla> <estado>           -> updateKey2 (api 25)
 *          teclado 1 = KBD_FCC; tecla 116 = HOME, 78 = MENU, 13 = BACK; estado 1 aperta, 0 solta
 *      gesto <gesto> <x> <y> [p1 [p2 [dedos]]]    -> updateGesture2 (api 20), teclado 13 (tela)
 *          gesto 4 = encosta, 5 = arrasta, 3 = solta, 1 = toque (p1 = 1, a contagem de cliques)
 *      encoder <teclado> <tecla> <incremento>     -> updateEncoder2 (api 23): botão giratório
 *          tecla 16 = seleção (o da direita), 17 = volume; incremento negativo gira para trás
 *    O que já estava no arquivo quando o stub sobe não é reenviado. Linha que não segue o
 *    formato é ignorada. Os inteiros do DSIKeyPanel são int32 comuns, como os do display.
 *
 * 4. Ajustes genéricos do painel de teclas (Marco 8). A HMI grava ajustes no controlador de toque
 *    e os lê de volta, pela interface 300 do mesmo serviço:
 *      setGenericSetting 1005 {teclado, chave, valor}  -> genericSettingResponse 2000 {teclado, chave, valor}
 *      requestGenericSetting 1009 {teclado, chave}     -> genericSettingResponse 2000, com o valor guardado
 *    No carro quem responde é o touch-app, que aqui não tem controlador com que falar e fica
 *    calado: a HMI registrava um erro de tempo esgotado a cada 10 s. O stub guarda o valor e o
 *    devolve; chave nunca gravada vale 0. Nada é feito com o valor.
 *
 * Fontes: classes decompiladas do MIBHMI-DSI.jar (DSIInterfaceId, ListenerWorkerThread,
 * DSIDisplayManagementImpl, DSIDisplayManagementListenerService, DSIPowerManagementImplListener,
 * DSIKeyPanelImpl, DSIKeyPanelListenerService e os protobufs) e do jar da HMI
 * (AslTargetSystemPowerManagementHandling, AslTargetSystemKeyPanelHandling, TouchInputDeviceImpl,
 * TouchInputDeviceHandler, org.dsi.ifc.keypanel.Constants). Detalhe em ../modelagem-soc.md.
 *
 * Barramento (igual kipc_bus/kipc_hub): datagrama = [canal:8][interface:4][instância:4][api:4][protobuf].
 * Estático (-static): roda dentro do chroot sob qualquer glibc.
 *
 * uso: dsistub [socket] [largura] [altura] [estado de energia; 0 = não publicar] [arquivo de comandos]
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define PID_DISPLAY_MGR     0x4000ULL
#define PID_MCU_INTERPRETER 0x400000ULL
#define PID_KEYPANEL        0x80000000000ULL
#define IF_WELCOME 9000
#define IF_DISPLAY 100
#define IF_DISPLAY_LISTENER 101
#define IF_POWER 1100
#define IF_POWER_LISTENER 1101
#define IF_KEYPANEL 300
#define IF_KEYPANEL_LISTENER 301
#define API_ASSINA 9999
#define KBD_TOUCHSCREEN_FRONT 13

static int W = -1, R = -1;

static int connect_hub(const char *path, char role)
{
    struct sockaddr_un a;
    char hs[5];
    int s = socket(AF_UNIX, SOCK_SEQPACKET, 0), pid = getpid();
    if (s < 0) return -1;
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
    hs[0] = role; hs[1] = (char)pid; hs[2] = (char)(pid >> 8); hs[3] = (char)(pid >> 16); hs[4] = (char)(pid >> 24);
    send(s, hs, 5, 0);
    return s;
}

static void send_msg(uint64_t chan, uint32_t ifid, uint32_t inst, uint32_t api, const unsigned char *pb, int n)
{
    unsigned char b[8 + 12 + 512];
    memcpy(b, &chan, 8); memcpy(b + 8, &ifid, 4); memcpy(b + 12, &inst, 4); memcpy(b + 16, &api, 4);
    if (n > 0) memcpy(b + 20, pb, (size_t)n);
    send(W, b, (size_t)(20 + (n > 0 ? n : 0)), MSG_NOSIGNAL);
}

/* ---- protobuf mínimo: só varints e blocos ---- */
static int pb_varint(unsigned char *b, uint64_t v)
{
    int o = 0;
    do { unsigned char x = v & 0x7f; v >>= 7; if (v) x |= 0x80; b[o++] = x; } while (v);
    return o;
}
static int pb_int(unsigned char *b, int field, uint64_t v)
{
    int o = pb_varint(b, (uint64_t)(field << 3));
    return o + pb_varint(b + o, v);
}
/* o serviço de energia declara os inteiros como sint32 (zigue-zague: 1 vai como 2); o de
 * display usa int32 comum. Visto no tráfego real: a HMI pede os atributos 3,4,7 como 06 08 0e. */
static int pb_sint(unsigned char *b, int field, int32_t v)
{
    return pb_int(b, field, (uint32_t)((v << 1) ^ (v >> 31)));
}
static int pb_bytes(unsigned char *b, int field, const unsigned char *d, int n)
{
    int o = pb_varint(b, (uint64_t)((field << 3) | 2));
    o += pb_varint(b + o, (uint64_t)n);
    memcpy(b + o, d, (size_t)n);
    return o + n;
}
static int rd_varint(const unsigned char *p, int n, int *o, uint64_t *v)
{
    int s = 0;
    *v = 0;
    while (*o < n) {
        unsigned char c = p[(*o)++];
        *v |= (uint64_t)(c & 0x7f) << s;
        if (!(c & 0x80)) return 1;
        s += 7;
        if (s > 63) return 0;
    }
    return 0;
}
/* lê até max valores inteiros do campo `field` (repetido simples ou empacotado) */
static int pb_get(const unsigned char *p, int n, int field, uint64_t *out, int max, int *packed)
{
    int o = 0, k = 0;
    uint64_t tag, v;
    while (o < n && rd_varint(p, n, &o, &tag)) {
        int f = (int)(tag >> 3), wt = (int)(tag & 7);
        if (wt == 0) {
            if (!rd_varint(p, n, &o, &v)) break;
            if (f == field && k < max) out[k++] = v;
        } else if (wt == 2) {
            int end;
            if (!rd_varint(p, n, &o, &v) || v > (uint64_t)(n - o)) break;
            end = o + (int)v;
            if (f == field) {
                if (packed) *packed = 1;
                while (o < end && k < max && rd_varint(p, end, &o, &v)) out[k++] = v;
            }
            o = end;
        } else if (wt == 5) o += 4;
        else if (wt == 1) o += 8;
        else break;
    }
    return k;
}

/* o brilho de cada display: a porcentagem que a HMI gravou por último */
#define MAXDISPLAYS 8
static struct { int32_t id, pct; } brilhos[MAXDISPLAYS];
static int nbrilhos;

static void answer_display(uint32_t inst, uint32_t api, const unsigned char *pb, int n, uint32_t w, uint32_t h)
{
    unsigned char r[512], tmp[256];
    uint64_t a[64], b[2], c[2];
    int o = 0, k, i, t = 0, packed = 0;
    a[0] = b[0] = c[0] = 0;
    switch (api) {
    case 1043:
        pb_get(pb, n, 1, a, 1, NULL);
        o += pb_int(r + o, 1, a[0]); o += pb_int(r + o, 2, w); o += pb_int(r + o, 3, h); o += pb_int(r + o, 4, 0);
        send_msg(PID_DISPLAY_MGR, IF_DISPLAY_LISTENER, inst, 2029, r, o);
        break;
    case 1040:
        pb_get(pb, n, 1, a, 1, NULL); pb_get(pb, n, 2, b, 1, NULL);
        o += pb_int(r + o, 1, a[0]); o += pb_int(r + o, 2, b[0]); o += pb_int(r + o, 3, 0);
        send_msg(PID_DISPLAY_MGR, IF_DISPLAY_LISTENER, inst, 2031, r, o);
        break;
    case 1039:
        pb_get(pb, n, 1, b, 1, NULL); pb_get(pb, n, 3, c, 1, NULL);
        k = pb_get(pb, n, 2, a, 64, &packed);
        o += pb_int(r + o, 1, b[0]);
        if (packed) { for (i = 0; i < k; i++) t += pb_varint(tmp + t, a[i]); o += pb_bytes(r + o, 2, tmp, t); }
        else for (i = 0; i < k; i++) o += pb_int(r + o, 2, a[i]);
        o += pb_int(r + o, 3, 0); o += pb_int(r + o, 4, c[0]);
        send_msg(PID_DISPLAY_MGR, IF_DISPLAY_LISTENER, inst, 2027, r, o);
        break;
    case 1005:
        pb_get(pb, n, 1, a, 1, NULL);
        o += pb_int(r + o, 1, a[0]); o += pb_int(r + o, 2, w); o += pb_int(r + o, 3, h);
        send_msg(PID_DISPLAY_MGR, IF_DISPLAY_LISTENER, inst, 2000, r, o);
        break;
    case 1010:
        pb_get(pb, n, 1, a, 1, NULL);
        o += pb_int(r + o, 1, a[0]); o += pb_int(r + o, 2, 1);
        send_msg(PID_DISPLAY_MGR, IF_DISPLAY_LISTENER, inst, 2004, r, o);
        break;
    case 1011:                                   /* setDisplayBrightness: guarda; a camada da LG não tem resposta para ele */
        pb_get(pb, n, 1, a, 1, NULL); pb_get(pb, n, 2, b, 1, NULL);
        for (i = 0; i < nbrilhos && brilhos[i].id != (int32_t)a[0]; i++) ;
        if (i == nbrilhos && nbrilhos < MAXDISPLAYS) brilhos[nbrilhos++].id = (int32_t)a[0];
        if (i < nbrilhos) brilhos[i].pct = (int32_t)b[0];
        fprintf(stderr, "[dsistub] brilho do display %d %s: %d %%\n", (int32_t)a[0], i < nbrilhos ? "gravado" : "não guardado (displays demais)", (int32_t)b[0]);
        return;
    case 1022:                                   /* getDisplayBrightness: o que foi gravado, e só isso */
        pb_get(pb, n, 1, a, 1, NULL);
        for (i = 0; i < nbrilhos && brilhos[i].id != (int32_t)a[0]; i++) ;
        if (i == nbrilhos) {
            fprintf(stderr, "[dsistub] brilho do display %d: leitura sem gravação anterior; fica sem resposta\n", (int32_t)a[0]);
            return;
        }
        /* como no protobuf 3 do firmware, campo em zero não vai no fio */
        if (a[0]) o += pb_int(r + o, 1, (uint64_t)(int64_t)(int32_t)a[0]);
        if (brilhos[i].pct) o += pb_int(r + o, 2, (uint64_t)(int64_t)brilhos[i].pct);
        send_msg(PID_DISPLAY_MGR, IF_DISPLAY_LISTENER, inst, 2012, r, o);
        fprintf(stderr, "[dsistub] brilho do display %d lido: %d %%\n", (int32_t)a[0], brilhos[i].pct);
        return;
    default:
        return;
    }
    fprintf(stderr, "[dsistub] respondi ao pedido de display api=%u\n", api);
}

static void publish_power_state(uint32_t state)
{
    unsigned char r[64];
    int o = 0;
    o += pb_sint(r + o, 1, (int32_t)state); o += pb_sint(r + o, 2, 0); o += pb_sint(r + o, 3, 1);
    send_msg(PID_MCU_INTERPRETER, IF_POWER_LISTENER, 0, 1, r, o);
}
static void publish_clamp(void)
{
    unsigned char r[64], cs[16];
    int o = 0, c = 0;
    c += pb_int(cs + c, 1, 1); c += pb_int(cs + c, 2, 1); c += pb_int(cs + c, 3, 1); c += pb_int(cs + c, 4, 0);   /* booleanos */
    o += pb_bytes(r + o, 1, cs, c); o += pb_sint(r + o, 2, 1);
    send_msg(PID_MCU_INTERPRETER, IF_POWER_LISTENER, 0, 4, r, o);
}
static void publish_power(uint32_t state)
{
    publish_power_state(state);
    publish_clamp();
}
static void publish_rvc_inactive(void)
{
    unsigned char r[16];
    int o = pb_sint(r, 2, 1);                    /* active = false não vai no fio; validFlag = 1 */
    send_msg(PID_MCU_INTERPRETER, IF_POWER_LISTENER, 0, 7, r, o);
}
/* A HMI assinou atributos de energia: manda na hora os que o stub tem
 * (1 = estado, 4 = ignição, 7 = câmera de ré ativa). */
static void answer_power_subscription(const unsigned char *pb, int n, uint32_t state)
{
    uint64_t a[32];
    int k = pb_get(pb, n, 1, a, 32, NULL), i;
    for (i = 0; i < k; i++) {
        int32_t attr = (int32_t)((a[i] >> 1) ^ (uint64_t)-(int64_t)(a[i] & 1));     /* zigue-zague */
        if (attr == 1) publish_power_state(state);
        else if (attr == 4) publish_clamp();
        else if (attr == 7) publish_rvc_inactive();
    }
}

/* ---- entrada: teclas e toque pelo DSIKeyPanel ---- */
static int32_t agora_ms(void)
{
    static struct timespec t0;
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    if (!t0.tv_sec && !t0.tv_nsec) t0 = t;
    return (int32_t)((t.tv_sec - t0.tv_sec) * 1000 + (t.tv_nsec - t0.tv_nsec) / 1000000);
}
/* int32 do protobuf: negativo vai com o sinal estendido a 64 bits */
static int pb_i32(unsigned char *b, int field, int32_t v)
{
    return pb_int(b, field, (uint64_t)(int64_t)v);
}
static void publish_key(int32_t kbd, int32_t key, int32_t state)
{
    unsigned char r[80];
    int o = 0;
    o += pb_i32(r + o, 1, kbd); o += pb_i32(r + o, 2, key); o += pb_i32(r + o, 3, state);
    o += pb_i32(r + o, 4, agora_ms()); o += pb_i32(r + o, 5, 1);
    send_msg(PID_KEYPANEL, IF_KEYPANEL_LISTENER, 0, 25, r, o);
    fprintf(stderr, "[dsistub] tecla: teclado=%d tecla=%d estado=%d\n", kbd, key, state);
}
static void publish_encoder(int32_t kbd, int32_t key, int32_t inc)
{
    unsigned char r[80];
    int o = 0;
    o += pb_i32(r + o, 1, kbd); o += pb_i32(r + o, 2, key); o += pb_i32(r + o, 3, inc);
    o += pb_i32(r + o, 4, agora_ms()); o += pb_i32(r + o, 5, 1);
    send_msg(PID_KEYPANEL, IF_KEYPANEL_LISTENER, 0, 23, r, o);
    fprintf(stderr, "[dsistub] botão giratório: teclado=%d tecla=%d incremento=%d\n", kbd, key, inc);
}
static void publish_gesture(int32_t g, int32_t x, int32_t y, int32_t p1, int32_t p2, int32_t fingers)
{
    unsigned char r[160];
    int o = 0;
    o += pb_i32(r + o, 1, KBD_TOUCHSCREEN_FRONT); o += pb_i32(r + o, 2, g); o += pb_i32(r + o, 3, fingers);
    o += pb_i32(r + o, 4, 0); o += pb_i32(r + o, 5, x); o += pb_i32(r + o, 6, y);
    o += pb_i32(r + o, 7, p1); o += pb_i32(r + o, 8, p2); o += pb_i32(r + o, 9, agora_ms()); o += pb_i32(r + o, 10, 1);
    send_msg(PID_KEYPANEL, IF_KEYPANEL_LISTENER, 0, 20, r, o);
    fprintf(stderr, "[dsistub] gesto: %d em (%d,%d) p1=%d p2=%d dedos=%d\n", g, x, y, p1, p2, fingers);
}
/* ---- ajustes genéricos do painel de teclas: guarda e devolve ---- */
#define MAXAJUSTES 256
static struct { int32_t kbd, key, val; } ajustes[MAXAJUSTES];
static int najustes;

static void answer_keypanel(uint32_t inst, uint32_t api, const unsigned char *pb, int n)
{
    unsigned char r[64];
    uint64_t a[1] = { 0 }, b[1] = { 0 }, c[1] = { 0 };
    int32_t kbd, key, val = 0;
    int i, o = 0;
    if (api != 1005 && api != 1009) return;
    pb_get(pb, n, 1, a, 1, NULL); pb_get(pb, n, 2, b, 1, NULL);
    kbd = (int32_t)a[0]; key = (int32_t)b[0];
    for (i = 0; i < najustes && !(ajustes[i].kbd == kbd && ajustes[i].key == key); i++) ;
    if (api == 1005) {                           /* setGenericSetting: guarda */
        pb_get(pb, n, 3, c, 1, NULL);
        val = (int32_t)c[0];
        if (i == najustes && najustes < MAXAJUSTES) { ajustes[i].kbd = kbd; ajustes[i].key = key; najustes++; }
        if (i < najustes) ajustes[i].val = val;
    } else if (i < najustes) val = ajustes[i].val;   /* requestGenericSetting: o guardado, ou 0 */
    /* como no protobuf 3 do firmware, campo em zero não vai no fio */
    if (kbd) o += pb_i32(r + o, 1, kbd);
    if (key) o += pb_i32(r + o, 2, key);
    if (val) o += pb_i32(r + o, 3, val);
    send_msg(PID_KEYPANEL, IF_KEYPANEL_LISTENER, inst, 2000, r, o);
    fprintf(stderr, "[dsistub] ajuste do painel de teclas %s: teclado=%d chave=%d valor=%d\n", api == 1005 ? "gravado" : "lido", kbd, key, val);
}

/* uma linha de comando: palavra seguida só de inteiros. Fora do formato, ignora. */
static void run_command(char *line)
{
    char *tok[8], *save = NULL, *end, *t;
    long v[7] = { 0 };
    int n = 0, i, ok = 1;
    for (t = strtok_r(line, " \t\r", &save); t && n < 8; t = strtok_r(NULL, " \t\r", &save)) tok[n++] = t;
    if (n == 0) return;
    if (t) ok = 0;                               /* palavras demais na linha */
    for (i = 1; ok && i < n; i++) {
        errno = 0;
        v[i - 1] = strtol(tok[i], &end, 10);
        if (*end || end == tok[i] || errno || v[i - 1] < INT32_MIN || v[i - 1] > INT32_MAX) ok = 0;
    }
    if (ok && n == 4 && !strcmp(tok[0], "tecla")) publish_key((int32_t)v[0], (int32_t)v[1], (int32_t)v[2]);
    else if (ok && n == 4 && !strcmp(tok[0], "encoder")) publish_encoder((int32_t)v[0], (int32_t)v[1], (int32_t)v[2]);
    else if (ok && n >= 4 && n <= 7 && !strcmp(tok[0], "gesto"))
        publish_gesture((int32_t)v[0], (int32_t)v[1], (int32_t)v[2], n > 4 ? (int32_t)v[3] : 0,
                        n > 5 ? (int32_t)v[4] : 0, n > 6 ? (int32_t)v[5] : 1);
    else fprintf(stderr, "[dsistub] comando ignorado: %s\n", tok[0]);
}
/* lê do arquivo o que foi acrescentado desde a última vez e executa as linhas completas */
static void read_commands(const char *path)
{
    static int fd = -1;
    static off_t pos;
    static char acc[512];
    static int na;
    char buf[512];
    struct stat st;
    ssize_t n;
    if (fd < 0) {
        fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) return;
        pos = lseek(fd, 0, SEEK_END);            /* o que já estava lá não é reenviado */
        return;
    }
    if (fstat(fd, &st) == 0 && st.st_size < pos) { pos = 0; na = 0; }   /* arquivo zerado: recomeça */
    while ((n = pread(fd, buf, sizeof buf, pos)) > 0) {
        ssize_t i;
        pos += n;
        for (i = 0; i < n; i++) {
            if (buf[i] == '\n') { acc[na < 0 ? 0 : na] = 0; if (na > 0) run_command(acc); na = 0; }
            else if (na >= 0 && na < (int)sizeof acc - 1) acc[na++] = buf[i];
            else na = -1;                        /* linha longa demais: descarta até a quebra */
        }
    }
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/run/kipc_hub.sock";
    uint32_t w = argc > 2 ? (uint32_t)atoi(argv[2]) : 800, h = argc > 3 ? (uint32_t)atoi(argv[3]) : 480;
    uint32_t power = argc > 4 ? (uint32_t)atoi(argv[4]) : 1;
    const char *cmds = argc > 5 && argv[5][0] ? argv[5] : NULL;
    time_t t0 = time(NULL), last = 0;
    unsigned seen = 0;
    W = connect_hub(path, 'W');
    R = connect_hub(path, 'R');
    if (W < 0 || R < 0) { fprintf(stderr, "[dsistub] sem hub em %s\n", path); return 1; }
    fprintf(stderr, "[dsistub] ligado; display %ux%u; estado de energia publicado: %u; comandos de entrada: %s\n",
            w, h, power, cmds ? cmds : "não");
    agora_ms();
    for (;;) {
        struct pollfd p = { R, POLLIN, 0 };
        time_t now = time(NULL);
        if (now != last) {
            last = now;
            send_msg(PID_DISPLAY_MGR, IF_WELCOME, 0, 0, NULL, 0);
            if (power && now - t0 < 600) publish_power(power);
        }
        if (cmds) read_commands(cmds);
        /* com entrada ligada o laço gira a cada 20 ms, para o toque não atrasar */
        if (poll(&p, 1, cmds ? 20 : 200) > 0 && (p.revents & POLLIN)) {
            static unsigned char in[0x6900];
            ssize_t n = recv(R, in, sizeof in, 0);
            uint64_t ch;
            uint32_t ifid, inst, api;
            if (n < 20) { if (n <= 0) break; continue; }
            memcpy(&ch, in, 8); memcpy(&ifid, in + 8, 4); memcpy(&inst, in + 12, 4); memcpy(&api, in + 16, 4);
            if (seen++ < 400 && (ifid == IF_DISPLAY || ch == PID_DISPLAY_MGR))
                fprintf(stderr, "[dsistub] rx canal=0x%llx if=%u inst=%u api=%u pb=%zd bytes\n",
                        (unsigned long long)ch, ifid, inst, api, n - 20);
            if (ifid == IF_DISPLAY) answer_display(inst, api, in + 20, (int)n - 20, w, h);
            else if (ifid == IF_POWER && api == API_ASSINA && power) answer_power_subscription(in + 20, (int)n - 20, power);
            else if (ifid == IF_KEYPANEL) answer_keypanel(inst, api, in + 20, (int)n - 20);
        }
    }
    return 0;
}
