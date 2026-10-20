/* somstub.c — o "som de mentira" da bancada (estudo num computador, sem carro). 12/10/2026.
 *
 * O som desta HMI não passa pelo barramento DSI. A tela de som, a barra de volume e o mudo falam
 * com o serviço RSI "audiomanagement" (HTTP e WebSocket locais, achado pelo registro de serviços).
 * No carro quem serve esse serviço é o gerente de áudio do firmware (/lge/app_ro/audiomgnt/audiomgnt).
 * Na bancada ele sobe, mas sem o resto do carro entra no mudo de proteção de componentes, e isso
 * não é contornado. Quem serve o audiomanagement aqui é o mock RSI que o próprio firmware traz
 * (/lge/app_ro/bin/rsi-mockup), com os dados de stubs/som/gerado/ (o que o gerente de áudio serve
 * em repouso; ver driver/som_tabela.sh). O mock guarda o que lhe gravam e avisa quem assina, e a
 * tela de som (grave, médio, agudo, balanço, fader) já funciona só com ele. O que o mock não tem
 * é regra. Este programa é um CLIENTE do mock e põe por cima três coisas:
 *
 * 1. O perfil (stubs/som/som-de-mentira.txt): o que no carro viria do amplificador e da última
 *    sessão. Sem a saída de som "entertainment" ligada e sem uma conexão de entretenimento com o
 *    foco de ajuste a HMI mostra "A configuração do som estará disponível novamente…"
 *    (HsmSoundDPHandling.updateSettingfocusConnection, propriedade 4138 SOUND_SETUP_ACCESSIBLE).
 *    O perfil NÃO é gravado pelo serviço: o mock segue a especificação e recusa gravação na saída
 *    de som (403). Ele é aplicado aos arquivos de dados ANTES de o mock subir (opção -w), junto
 *    com o estado de partida: a fonte de entretenimento iniciada e o objeto global apontando
 *    para ela.
 *
 *    O mock obedece à especificação do serviço que o firmware traz
 *    (/lge/app_ro/sai/spec/viwi.service.audiomanagement/…json), e ela é mais estreita que o
 *    serviço de verdade em dois pontos que fazem falta. A opção -s escreve uma cópia dela com os
 *    dois afrouxados, para o lançador montar por cima da original (só o mock a lê):
 *    - soundsettings passa a avisar quem assina o RECURSO (systemTriggeredEvents ganha "resource").
 *      A HMI acha o conjunto de som "entertainment" por uma assinatura de recurso
 *      (AudioManagementRsi.subscribeToSoundsettingsEntertainment), e é só depois dela que assina
 *      grave, médio, agudo, balanço e fader. O gerente de áudio de verdade atende essa assinatura
 *      (medido: rodada r1, "found soundsetting entertainment"); o mock, com a especificação
 *      original, não responde, e a tela aceita o toque mas nunca recebe o valor de volta.
 *    - controls passa a aceitar DELETE de elemento, para o controle atendido sair da lista. Sem
 *      isso o mock devolve 403 e a lista cresce a cada passo de volume, e a HMI recebe a lista
 *      inteira a cada controle novo. Que o gerente de verdade consuma os controles é dedução.
 *
 * 2. As regras mínimas do gerente de áudio. SIMULAÇÃO: é o bastante para a tela, não a tabela de
 *    áudio do firmware (prioridades e ações cruzadas de 124 conexões ficam de fora).
 *    - Controle de volume (elemento criado em /audiomanagement/controls, tipo volumeIncrease ou
 *      volumeDecrease, com o número de passos): soma ou tira do volume da conexão que tem o foco
 *      de ajuste, dentro da faixa do próprio volume. Chegar a zero liga o mudo; aumentar com o
 *      mudo ligado desliga o mudo. (Na tabela: eventos onMuteByVolumeDecrease e onVolumeIncrease.)
 *    - Controle de mudo (tipo mute): liga e desliga a conexão FCT_AMP_USER_MUTE, que leva o foco
 *      de ajuste enquanto está iniciada (na tabela ela tem <settingFocus>true</settingFocus>).
 *    - Conexão que um cliente inicia ou para (a HMI grava "status" na conexão): se é de
 *      entretenimento (ENT_AMP_…), passa a ser a do objeto global e a anterior para; se é o mudo
 *      do usuário, vale como o controle de mudo; se é outra que tem volume próprio (as de menu,
 *      como FCT_AMP_ONVOLUME_MENU), fica com o foco de ajuste enquanto estiver iniciada.
 *    - O objeto global (/audiomanagement/globals) é regravado com entertainmentConnection,
 *      foregroundConnection e settingFocus, cada um com a conexão por inteiro (a HMI pede com
 *      $expand, e o mock não expande: devolve o que estiver guardado).
 *
 * 3. O papel do touch-app. No carro, quando o botão de volume gira, é o touch-app que cria o
 *    controle (no binário dele: "/audiomanagement/controls", "volumeUp_gui", "volumeIncrease",
 *    "hmi", "cluster"); a HMI só abre a barra. Aqui o botão vem do dsistub, pelo barramento
 *    (DSIKeyPanel, interface 301: api 23 = updateEncoder2, api 25 = updateKey2), e este programa
 *    cria o controle: botão 17 (volume) e rolete 44 pelo número de passos; teclas 42 e 43 do
 *    volante (teclado 4), um passo ao apertar; e o controle de mudo ao apertar a tecla de mudo
 *    do volante (4/57) ou da moldura (13/88). O mudo também é papel de quem está abaixo da HMI:
 *    ao receber a tecla ela só registra "ASL_SOUND_PIN_MUTE. Not handled by HMI anymore, supplier
 *    resposibility now" (visto na rodada r4). Que seja o touch-app quem o faz no carro é dedução
 *    (o binário dele tem o texto "volumemuted"); o nome do controle de mudo é o que a HMI usa.
 *
 * Perfil, um ou mais arquivos lidos em ordem (o de depois vale sobre o de antes):
 *     # comentário
 *     entretenimento = ENT_AMP_TUNER            fonte que toca ao ligar (vazio = nenhuma)
 *     [sinks.entertainment]                     seção: <recurso>.<nome do elemento>; * = todos
 *     amplifierId = "VWBAS000"                  <campo> = <valor em JSON>
 * O campo tem de existir no elemento e o valor tem de ser do mesmo tipo do que está lá.
 *
 * 4. A memória (-m <arquivo>, 12/10/2026). O que a tela mudou volta na partida seguinte: o valor
 *    dos volumes e dos parâmetros de som (grave, médio, agudo, balanço, fader…) e o mudo do
 *    usuário. No ar, o arquivo é regravado a cada mudança (ao lado, e trocado de nome); com -w, o
 *    que ele traz entra por cima do perfil, antes do estado de partida. O formato é o do perfil,
 *    só com "mudo = ligado|desligado" antes das seções e "value" nas de volumesettings e de
 *    soundparameters. Ao contrário do perfil, é lido com tolerância: linha que não serve (elemento
 *    que não existe, valor fora da faixa, outro recurso) é ignorada com aviso, porque memória
 *    estragada não pode impedir a central de subir. SIMULAÇÃO: no carro quem guarda esses valores
 *    é o gerente de áudio, e se o mudo volta depois de desligar e ligar eu não sei. A fonte que
 *    toca não entra aqui: quem lembra dela é a HMI.
 *
 * 5. O volante abre a barra de volume (volante_abre_a_barra = sim, no perfil). SIMULAÇÃO, pedida
 *    pelo Lipe em 12/10/2026: no carro dele (firmware MU0803), mudar o volume pelo volante faz a
 *    barra aparecer na tela da central; foi dito por ele, de memória, sem foto. A HMI deste
 *    firmware (MU0883) não faz isso sozinha: a barra é aberta pelo evento de giro do botão da
 *    central, e a chave de recurso que a faria abrir pelo volante (isMFLVolumePopup, 271) é
 *    constante e vem desligada no configurationmanager.res; ali quem mostra o volume do volante
 *    é o painel de instrumentos (função CurrentVolume do BAP, evento 232). Com a opção ligada,
 *    este programa, ao ver Vol +, Vol − ou o rolete do volante no barramento, NÃO cria o controle
 *    direto: escreve "encoder 13 17 ±n" no arquivo de comandos do dsistub (-c, default
 *    /tmp/dsi-entrada), ou seja, gira também o botão da central. A HMI abre a barra como abre
 *    para o botão, e o controle de volume sai pelo caminho do botão, uma vez só. Sem ter onde
 *    escrever, volta ao que era (o volume muda, a barra não abre), com um aviso. A opção sai do
 *    perfil para <destino>/opcoes.txt (-w), que é de onde o programa a lê no ar.
 *
 * uso: somstub -n <pasta> <perfil>...              só confere o perfil contra os dados (0 = certo)
 *      somstub -w <destino> [-m <memória>] <pasta> <perfil>...
 *                                                  confere e escreve em <destino> os dados com o
 *                                                  perfil, a memória e o estado de partida; é o
 *                                                  que o mock lê
 *      somstub -s <especificação> <saída>          escreve a especificação afrouxada (ver o item 1)
 *      somstub [-p porta] [-e segundos] [-b socket do hub | -] [-m <memória>] [-c <comandos do dsistub>] <destino>
 *                                                  fica no ar, por cima do mock que lê <destino>
 *   <pasta>  a que tem audiomanagement/<recurso>.json (stubs/som/gerado)
 *   -p       porta do mock RSI (padrão 7770)
 *   -e       quanto esperar o serviço aparecer (padrão 30 s)
 *   -b       socket do kipc_hub, para ouvir o botão de volume (padrão /run/kipc_hub.sock; - = não ouvir)
 * Sem perfil nenhum, o -w escreve os mesmos bytes que leu (é a prova do leitor e do escritor de
 * JSON; está no teste). Cada controle é atendido uma vez só e apagado em seguida; se o mock não
 * deixar apagar (403, sem a especificação afrouxada), a lista dele cresce até a central desligar.
 * Estático (-static): roda dentro do chroot sob qualquer glibc. Registro na saída padrão.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int PORTA = 7770;
static void diz(const char *f, ...) { va_list a; va_start(a, f); printf("[somstub] "); vprintf(f, a); printf("\n"); va_end(a); }

/* ------------------------------------------------------------------ texto que cresce */
typedef struct { char *p; size_t n, cap; } sb;
static void sb_poe(sb *b, const char *s, size_t n) {
    if (b->n + n + 1 > b->cap) { b->cap = (b->n + n + 1) * 2; b->p = realloc(b->p, b->cap); if (!b->p) exit(9); }
    memcpy(b->p + b->n, s, n); b->n += n; b->p[b->n] = 0;
}
static void sb_str(sb *b, const char *s) { sb_poe(b, s, strlen(s)); }
static void sb_zera(sb *b) { b->n = 0; if (b->p) b->p[0] = 0; }

/* ------------------------------------------------------------------ JSON */
typedef struct jv { char t; char *s; int n; struct jv **v; char **k; } jv;   /* t: o a s n b z */
static jv *jnovo(char t) { jv *j = calloc(1, sizeof *j); if (!j) exit(9); j->t = t; return j; }
static void jfree(jv *j) {
    int i; if (!j) return;
    for (i = 0; i < j->n; i++) { jfree(j->v[i]); if (j->k) free(j->k[i]); }
    free(j->v); free(j->k); free(j->s); free(j);
}
static void jpoe(jv *pai, const char *chave, jv *f) {
    pai->v = realloc(pai->v, (pai->n + 1) * sizeof *pai->v);
    if (pai->t == 'o') { pai->k = realloc(pai->k, (pai->n + 1) * sizeof *pai->k); pai->k[pai->n] = strdup(chave); }
    pai->v[pai->n++] = f;
}
static jv *jget(const jv *o, const char *chave) {
    int i; if (!o || o->t != 'o') return NULL;
    for (i = 0; i < o->n; i++) if (!strcmp(o->k[i], chave)) return o->v[i];
    return NULL;
}
static const char *jstr(const jv *o, const char *chave) { jv *f = jget(o, chave); return f && f->t == 's' ? f->s : ""; }
static long jnum(const jv *o, const char *chave, long padrao) { jv *f = jget(o, chave); return f && f->t == 'n' ? atol(f->s) : padrao; }
static void jset(jv *o, const char *chave, jv *f) {    /* troca ou acrescenta; fica dono de f */
    int i;
    for (i = 0; i < o->n; i++) if (!strcmp(o->k[i], chave)) { jfree(o->v[i]); o->v[i] = f; return; }
    jpoe(o, chave, f);
}
static jv *jtexto(const char *s) { jv *j = jnovo('s'); j->s = strdup(s); return j; }
static jv *jnumero(long v) { jv *j = jnovo('n'); char b[32]; snprintf(b, sizeof b, "%ld", v); j->s = strdup(b); return j; }
static jv *jclona(const jv *j) {
    jv *c; int i; if (!j) return NULL;
    c = jnovo(j->t); if (j->s) c->s = strdup(j->s);
    for (i = 0; i < j->n; i++) jpoe(c, j->k ? j->k[i] : NULL, jclona(j->v[i]));
    return c;
}
static void jbranco(const char **p) { while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++; }
static jv *jle(const char **p);
static char *jle_str(const char **p) {      /* *p na aspa de abertura; devolve o texto já sem as barras */
    sb b = {0}; const char *s = *p + 1;
    sb_poe(&b, "", 0);
    while (*s && *s != '"') {
        if (*s == '\\' && s[1]) {
            s++;
            if (*s == 'n') sb_poe(&b, "\n", 1); else if (*s == 't') sb_poe(&b, "\t", 1);
            else if (*s == 'r') sb_poe(&b, "\r", 1); else if (*s == 'b') sb_poe(&b, "\b", 1);
            else if (*s == 'f') sb_poe(&b, "\f", 1);
            else if (*s == 'u' && strlen(s) >= 5) {
                unsigned c = 0; char h[5]; memcpy(h, s + 1, 4); h[4] = 0; c = (unsigned)strtoul(h, NULL, 16); s += 4;
                if (c < 0x80) { char x = (char)c; sb_poe(&b, &x, 1); }
                else if (c < 0x800) { char x[2] = { (char)(0xc0 | (c >> 6)), (char)(0x80 | (c & 0x3f)) }; sb_poe(&b, x, 2); }
                else { char x[3] = { (char)(0xe0 | (c >> 12)), (char)(0x80 | ((c >> 6) & 0x3f)), (char)(0x80 | (c & 0x3f)) }; sb_poe(&b, x, 3); }
            } else sb_poe(&b, s, 1);
            s++;
        } else sb_poe(&b, s++, 1);
    }
    if (*s != '"') { free(b.p); return NULL; }
    *p = s + 1; return b.p;
}
static jv *jle(const char **p) {
    jv *j; jbranco(p);
    if (**p == '{' || **p == '[') {
        char fecha = **p == '{' ? '}' : ']';
        j = jnovo(**p == '{' ? 'o' : 'a'); (*p)++; jbranco(p);
        if (**p == fecha) { (*p)++; return j; }
        for (;;) {
            char *chave = NULL; jv *f;
            jbranco(p);
            if (j->t == 'o') {
                if (**p != '"' || !(chave = jle_str(p))) { jfree(j); return NULL; }
                jbranco(p); if (**p != ':') { free(chave); jfree(j); return NULL; } (*p)++;
            }
            f = jle(p);
            if (!f) { free(chave); jfree(j); return NULL; }
            jpoe(j, chave, f); free(chave); jbranco(p);
            if (**p == ',') { (*p)++; continue; }
            if (**p == fecha) { (*p)++; return j; }
            jfree(j); return NULL;
        }
    }
    if (**p == '"') { char *s = jle_str(p); if (!s) return NULL; j = jnovo('s'); j->s = s; return j; }
    if (!strncmp(*p, "true", 4)) { *p += 4; j = jnovo('b'); j->n = 0; j->s = strdup("true"); return j; }
    if (!strncmp(*p, "false", 5)) { *p += 5; j = jnovo('b'); j->s = strdup("false"); return j; }
    if (!strncmp(*p, "null", 4)) { *p += 4; return jnovo('z'); }
    if (**p == '-' || (**p >= '0' && **p <= '9')) {
        const char *i = *p; if (**p == '-') (*p)++;
        if (**p < '0' || **p > '9') return NULL;
        while ((**p >= '0' && **p <= '9') || **p == '.' || **p == 'e' || **p == 'E' || **p == '+' || **p == '-') (*p)++;
        j = jnovo('n'); j->s = strndup(i, (size_t)(*p - i)); return j;
    }
    return NULL;
}
static jv *jparse(const char *texto) {      /* o texto inteiro tem de ser um valor só */
    const char *p = texto; jv *j = jle(&p);
    if (j) { jbranco(&p); if (*p) { jfree(j); return NULL; } }
    return j;
}
static void jescreve(sb *b, const jv *j) {
    int i; const char *s;
    switch (j->t) {
    case 'o': case 'a':
        sb_str(b, j->t == 'o' ? "{" : "[");
        for (i = 0; i < j->n; i++) {
            if (i) sb_str(b, ",");
            if (j->t == 'o') { jv k = { 's', j->k[i], 0, NULL, NULL }; jescreve(b, &k); sb_str(b, ":"); }
            jescreve(b, j->v[i]);
        }
        sb_str(b, j->t == 'o' ? "}" : "]"); break;
    case 's':
        sb_str(b, "\"");
        for (s = j->s; *s; s++) {
            if (*s == '"' || *s == '\\' || *s == '/') { sb_poe(b, "\\", 1); sb_poe(b, s, 1); }
            else if ((unsigned char)*s < 0x20) { char x[8]; snprintf(x, sizeof x, "\\u%04x", (unsigned char)*s); sb_str(b, x); }
            else sb_poe(b, s, 1);
        }
        sb_str(b, "\""); break;
    case 'z': sb_str(b, "null"); break;
    default: sb_str(b, j->s);
    }
}

/* ------------------------------------------------------------------ HTTP (um pedido por ligação) */
static int liga_tcp(void) {
    struct sockaddr_in a; struct timeval tv = { 5, 0 };
    int s = socket(AF_INET, SOCK_STREAM, 0); if (s < 0) return -1;
    memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_port = htons((uint16_t)PORTA); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv); setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
    return s;
}
static int manda_tudo(int s, const char *p, size_t n) {
    while (n) { ssize_t k = send(s, p, n, MSG_NOSIGNAL); if (k <= 0) return -1; p += k; n -= (size_t)k; }
    return 0;
}
/* devolve o código da resposta (ou -1) e põe o corpo em *corpo */
static int http(const char *metodo, const char *caminho, const char *dados, sb *corpo) {
    sb q = {0}, r = {0}; char linha[160], buf[8192], *fim, *cl; int s = liga_tcp(), cod = -1; ssize_t k;
    sb_zera(corpo); sb_poe(corpo, "", 0);
    if (s < 0) return -1;
    sb_str(&q, metodo); sb_str(&q, " "); sb_str(&q, caminho);
    snprintf(linha, sizeof linha, " HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nUser-Agent: somstub\r\nConnection: close\r\n", PORTA); sb_str(&q, linha);
    if (dados) { snprintf(linha, sizeof linha, "Content-Type: application/json\r\nContent-Length: %zu\r\n", strlen(dados)); sb_str(&q, linha); }
    sb_str(&q, "\r\n"); if (dados) sb_str(&q, dados);
    if (manda_tudo(s, q.p, q.n) == 0) {
        while ((k = recv(s, buf, sizeof buf, 0)) > 0) {
            sb_poe(&r, buf, (size_t)k);
            /* com Content-Length não é preciso esperar o servidor fechar */
            if ((fim = strstr(r.p, "\r\n\r\n")) && (cl = strcasestr(r.p, "\r\nContent-Length:")) && cl < fim
                && r.n - (size_t)(fim + 4 - r.p) >= (size_t)atol(cl + 17)) break;
            if (fim && r.n >= 5 && !strcmp(r.p + r.n - 5, "0\r\n\r\n") && strcasestr(r.p, "\r\nTransfer-Encoding: chunked")) break;
        }
        if (r.p && !strncmp(r.p, "HTTP/1.", 7) && (fim = strstr(r.p, "\r\n\r\n"))) {
            cod = atoi(r.p + 9); *fim = 0;
            if (strcasestr(r.p, "\r\nTransfer-Encoding: chunked")) {
                char *p = fim + 4;
                for (;;) { long n = strtol(p, &p, 16); char *e = strstr(p, "\r\n"); if (n <= 0 || !e || (size_t)(e + 2 + n - r.p) > r.n) break; sb_poe(corpo, e + 2, (size_t)n); p = e + 2 + n + 2; }
            } else sb_poe(corpo, fim + 4, r.n - (size_t)(fim + 4 - r.p));
        }
    }
    close(s); free(q.p); free(r.p);
    return cod;
}

/* ------------------------------------------------------------------ o modelo: o que o serviço tem */
#define NREC 9
static const char *REC[NREC] = { "globals", "connections", "volumesettings", "soundsettings", "soundparameters", "soundpresets", "controls", "sinks", "sources" };
static jv *M[NREC];        /* cada um: a lista de elementos do recurso */
static int rec_de(const char *nome) { int i; for (i = 0; i < NREC; i++) if (!strcmp(REC[i], nome)) return i; return -1; }
static jv *acha(int r, const char *nome) { int i; for (i = 0; r >= 0 && i < M[r]->n; i++) if (!strcmp(jstr(M[r]->v[i], "name"), nome)) return M[r]->v[i]; return NULL; }
static jv *acha_id(int r, const char *id) { int i; for (i = 0; r >= 0 && i < M[r]->n; i++) if (!strcmp(jstr(M[r]->v[i], "id"), id)) return M[r]->v[i]; return NULL; }
/* o elemento do modelo a que uma referência {id, name, uri} aponta; uri = /audiomanagement/<recurso>/<id> */
static jv *alvo(const jv *ref) {
    const char *u = jstr(ref, "uri"), *b; char rec[40]; size_t n;
    if (strncmp(u, "/audiomanagement/", 17)) return NULL;
    u += 17; b = strchr(u, '/'); if (!b) return NULL;
    n = (size_t)(b - u); if (n >= sizeof rec) return NULL;
    memcpy(rec, u, n); rec[n] = 0;
    return acha_id(rec_de(rec), b + 1);
}
/* cópia do elemento com as referências trocadas pelos elementos de agora, até a profundidade dada */
static jv *expande(const jv *el, int prof) {
    jv *c = jclona(el); int i;
    for (i = 0; prof > 0 && i < c->n; i++)
        if (c->v[i]->t == 'o') { jv *a = alvo(c->v[i]); if (a) { jfree(c->v[i]); c->v[i] = expande(a, prof - 1); } }
    return c;
}
static int le_arquivo(const char *caminho, sb *b) {
    FILE *f = fopen(caminho, "r"); char buf[8192]; size_t k;
    sb_zera(b); sb_poe(b, "", 0);
    if (!f) return -1;
    while ((k = fread(buf, 1, sizeof buf, f)) > 0) sb_poe(b, buf, k);
    fclose(f); return 0;
}
static int carrega_da_pasta(const char *pasta, jv **m) {
    int i, rc = 0; sb b = {0}; char caminho[600];
    for (i = 0; i < NREC; i++) {
        snprintf(caminho, sizeof caminho, "%s/audiomanagement/%s.json", pasta, REC[i]);
        if (le_arquivo(caminho, &b) < 0) { diz("ERRO: não li %s", caminho); rc = -1; m[i] = jnovo('a'); continue; }
        m[i] = jparse(b.p);
        if (!m[i] || m[i]->t != 'a') { diz("ERRO: %s não é uma lista de elementos", caminho); rc = -1; jfree(m[i]); m[i] = jnovo('a'); }
    }
    free(b.p); return rc;
}
static int carrega_do_servico(void) {
    int i, cod; sb b = {0}; char caminho[200]; jv *r, *d;
    for (i = 0; i < NREC; i++) {
        snprintf(caminho, sizeof caminho, "/audiomanagement/%s?$limit=1000", REC[i]);
        cod = http("GET", caminho, NULL, &b);
        r = cod == 200 ? jparse(b.p) : NULL; d = jget(r, "data");
        if (!d || d->t != 'a') { diz("ERRO: o serviço respondeu %d a %s", cod, caminho); jfree(r); free(b.p); return -1; }
        M[i] = jclona(d); jfree(r);
    }
    free(b.p); return 0;
}

/* ------------------------------------------------------------------ o perfil */
typedef struct { int rec; char nome[80], campo[60]; jv *valor; char onde[300]; } ajuste;
static ajuste *AJ; static int NAJ;
static char ENT_INICIAL[80]; static char ENT_ONDE[300];
static int VOLANTE_BARRA;                       /* volante_abre_a_barra = sim (SIMULAÇÃO; ver o item 5 do cabeçalho) */
static char *apara(char *s) { char *e; while (*s == ' ' || *s == '\t') s++; e = s + strlen(s); while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' || e[-1] == '\r')) *--e = 0; return s; }
static int le_perfil(const char *arq) {
    FILE *f = fopen(arq, "r"); char l[4096], *s, *ig; int n = 0, rec = -1, rc = 0; char nome[80] = "";
    const char *base = strrchr(arq, '/'); base = base ? base + 1 : arq;
    if (!f) { diz("ERRO: não li o perfil %s", arq); return -1; }
    while (fgets(l, sizeof l, f)) {
        n++; s = apara(l);
        if (!*s || *s == '#') continue;
        if (*s == '[') {
            char *fecha = strchr(s, ']'), *pt = strchr(s, '.');
            if (!fecha || !pt || pt > fecha) { diz("ERRO: %s:%d: seção tem de ser [recurso.nome]", base, n); rc = -1; rec = -1; continue; }
            *fecha = 0; *pt = 0; rec = rec_de(s + 1);
            if (rec < 0) { diz("ERRO: %s:%d: o serviço não tem o recurso \"%s\"", base, n, s + 1); rc = -1; continue; }
            snprintf(nome, sizeof nome, "%s", pt + 1);
            /* a própria seção entra na lista, sem campo: é por ela que se confere se o elemento existe */
            AJ = realloc(AJ, (NAJ + 1) * sizeof *AJ); memset(&AJ[NAJ], 0, sizeof *AJ);
            AJ[NAJ].rec = rec; snprintf(AJ[NAJ].nome, sizeof AJ[NAJ].nome, "%s", nome); snprintf(AJ[NAJ].onde, sizeof AJ[NAJ].onde, "%s:%d", base, n);
            NAJ++;
            continue;
        }
        ig = strchr(s, '=');
        if (!ig) { diz("ERRO: %s:%d: não é seção nem \"campo = valor\"", base, n); rc = -1; continue; }
        *ig = 0; s = apara(s); ig = apara(ig + 1);
        if (rec < 0 && !*nome && !strcmp(s, "entretenimento")) {
            char *c = strchr(ig, '#'); if (c) { *c = 0; ig = apara(ig); }
            snprintf(ENT_INICIAL, sizeof ENT_INICIAL, "%s", ig); snprintf(ENT_ONDE, sizeof ENT_ONDE, "%s:%d", base, n); continue;
        }
        if (rec < 0 && !*nome && !strcmp(s, "volante_abre_a_barra")) {
            char *c = strchr(ig, '#'); if (c) { *c = 0; ig = apara(ig); }
            if (!strcmp(ig, "sim")) VOLANTE_BARRA = 1; else if (!strcmp(ig, "nao")) VOLANTE_BARRA = 0;
            else { diz("ERRO: %s:%d: volante_abre_a_barra tem de ser \"sim\" ou \"nao\", e veio \"%s\"", base, n, ig); rc = -1; }
            continue;
        }
        if (rec < 0) { if (!*nome) { diz("ERRO: %s:%d: \"%s\" fora de seção (antes das seções só valem \"entretenimento\" e \"volante_abre_a_barra\")", base, n, s); rc = -1; } continue; }
        AJ = realloc(AJ, (NAJ + 1) * sizeof *AJ); memset(&AJ[NAJ], 0, sizeof *AJ);
        AJ[NAJ].rec = rec; snprintf(AJ[NAJ].nome, sizeof AJ[NAJ].nome, "%s", nome); snprintf(AJ[NAJ].campo, sizeof AJ[NAJ].campo, "%s", s);
        snprintf(AJ[NAJ].onde, sizeof AJ[NAJ].onde, "%s:%d", base, n);
        AJ[NAJ].valor = jparse(ig);
        if (!AJ[NAJ].valor) { diz("ERRO: %s:%d: o valor de \"%s\" não é JSON (texto vai entre aspas): %s", base, n, s, ig); rc = -1; continue; }
        NAJ++;
    }
    fclose(f); return rc;
}
/* confere cada ajuste contra o modelo: elemento existe, campo existe, tipo igual */
static int confere_perfil(void) {
    int i, k, rc = 0;
    for (i = 0; i < NAJ; i++) {
        ajuste *a = &AJ[i]; int todos = !strcmp(a->nome, "*"), achou = 0;
        for (k = 0; k < M[a->rec]->n; k++) {
            jv *el = M[a->rec]->v[k], *atual;
            if (!todos && strcmp(jstr(el, "name"), a->nome)) continue;
            achou = 1; if (!*a->campo) continue;
            atual = jget(el, a->campo);
            if (!atual) { diz("ERRO: %s: %s \"%s\" não tem o campo \"%s\"", a->onde, REC[a->rec], jstr(el, "name"), a->campo); rc = -1; break; }
            if (atual->t != a->valor->t) { diz("ERRO: %s: o campo \"%s\" de %s \"%s\" é de outro tipo", a->onde, a->campo, REC[a->rec], jstr(el, "name")); rc = -1; break; }
        }
        if (!achou && !*a->campo) { diz("ERRO: %s: não há %s de nome \"%s\"", a->onde, REC[a->rec], a->nome); rc = -1; }
    }
    if (*ENT_INICIAL) {
        if (!acha(1, ENT_INICIAL)) { diz("ERRO: %s: não há conexão de nome \"%s\"", ENT_ONDE, ENT_INICIAL); rc = -1; }
        else if (strncmp(ENT_INICIAL, "ENT_AMP_", 8)) { diz("ERRO: %s: \"%s\" não é conexão de entretenimento (ENT_AMP_…)", ENT_ONDE, ENT_INICIAL); rc = -1; }
    }
    return rc;
}

/* ------------------------------------------------------------------ a memória (-m) */
typedef struct { int rec; char nome[80]; long valor; } lembranca;
static lembranca *MEMT; static int NMEM;
static const char *MEM_ARQ; static int MEM_MUDO = -1, MEM_SUJA, MEM_AVISOU;     /* MEM_MUDO -1: a memória não diz */
static void mem_poe(int rec, const char *nome, long v) {
    int i; for (i = 0; i < NMEM; i++) if (MEMT[i].rec == rec && !strcmp(MEMT[i].nome, nome)) { MEMT[i].valor = v; return; }
    MEMT = realloc(MEMT, (size_t)(NMEM + 1) * sizeof *MEMT); if (!MEMT) exit(9);
    MEMT[NMEM].rec = rec; snprintf(MEMT[NMEM].nome, sizeof MEMT[NMEM].nome, "%s", nome); MEMT[NMEM].valor = v; NMEM++;
}
/* um volume (recurso 2) ou parâmetro de som (4) mudou de valor: vai para a memória */
static void mem_valor(int r, const jv *el) {
    if (!MEM_ARQ || (r != 2 && r != 4)) return;
    mem_poe(r, jstr(el, "name"), jnum(el, "value", 0)); MEM_SUJA = 1;
}
/* lê a memória para a tabela, com tolerância; devolve quantas linhas ignorou. Precisa do modelo carregado. */
static int le_memoria(const char *arq) {
    FILE *f = fopen(arq, "r"); char l[1024], *s, *ig, *fim; int n = 0, rec = -1, ign = 0; jv *el = NULL; long v;
    const char *base = strrchr(arq, '/'); base = base ? base + 1 : arq;
    if (!f) return 0;                              /* ainda não há memória: a central nasce do perfil */
    while (fgets(l, sizeof l, f)) {
        n++; s = apara(l);
        if (!*s || *s == '#') continue;
        if (*s == '[') {
            char *fecha = strchr(s, ']'), *pt = strchr(s, '.');
            el = NULL; rec = -2;                   /* -2: seção recusada; as linhas dela passam em silêncio */
            if (!fecha || !pt || pt > fecha) { diz("memória: ignorei %s:%d: seção tem de ser [recurso.nome]", base, n); ign++; continue; }
            *fecha = 0; *pt = 0; rec = rec_de(s + 1);
            if (rec != 2 && rec != 4) { diz("memória: ignorei %s:%d: a memória só guarda volumes e parâmetros de som, e não \"%s\"", base, n, s + 1); ign++; rec = -2; continue; }
            el = acha(rec, pt + 1);
            if (!el) { diz("memória: ignorei %s:%d: não há %s de nome \"%s\"", base, n, REC[rec], pt + 1); ign++; rec = -2; }
            continue;
        }
        if (rec == -2) continue;
        ig = strchr(s, '=');
        if (!ig) { diz("memória: ignorei %s:%d: não é seção nem \"campo = valor\"", base, n); ign++; continue; }
        *ig = 0; s = apara(s); ig = apara(ig + 1);
        if (rec == -1) {
            if (!strcmp(s, "mudo") && !strcmp(ig, "ligado")) MEM_MUDO = 1;
            else if (!strcmp(s, "mudo") && !strcmp(ig, "desligado")) MEM_MUDO = 0;
            else { diz("memória: ignorei %s:%d: antes das seções só vale \"mudo = ligado\" ou \"mudo = desligado\"", base, n); ign++; }
            continue;
        }
        if (strcmp(s, "value")) { diz("memória: ignorei %s:%d: de %s a memória só guarda \"value\"", base, n, jstr(el, "name")); ign++; continue; }
        v = strtol(ig, &fim, 10);
        if (fim == ig || *fim) { diz("memória: ignorei %s:%d: o valor de %s tem de ser um número inteiro", base, n, jstr(el, "name")); ign++; continue; }
        if (jget(el, "valueMin") && jget(el, "valueMax") && (v < jnum(el, "valueMin", v) || v > jnum(el, "valueMax", v))) {
            diz("memória: ignorei %s:%d: %ld está fora da faixa de %s (%ld a %ld)", base, n, v, jstr(el, "name"), jnum(el, "valueMin", 0), jnum(el, "valueMax", 0)); ign++; continue;
        }
        mem_poe(rec, jstr(el, "name"), v);
    }
    fclose(f); return ign;
}
static int mem_ordem(const void *a, const void *b) {
    const lembranca *x = a, *y = b; return x->rec != y->rec ? x->rec - y->rec : strcmp(x->nome, y->nome);
}
/* regrava o arquivo inteiro, ao lado, e troca de nome: quem lê nunca vê metade */
static void mem_grava(void) {
    char novo[700]; FILE *f; int i, ok = 0;
    MEM_SUJA = 0;
    snprintf(novo, sizeof novo, "%s.novo", MEM_ARQ);
    qsort(MEMT, (size_t)NMEM, sizeof *MEMT, mem_ordem);
    f = fopen(novo, "w");
    if (f) {
        fprintf(f, "# Memória do som de mentira (SIMULAÇÃO): o que a tela mudou e volta na partida seguinte.\n"
                   "# Quem grava é o somstub, a cada mudança. Apagar este arquivo, com a central desligada, volta ao perfil.\n"
                   "mudo = %s\n", MEM_MUDO == 1 ? "ligado" : "desligado");
        for (i = 0; i < NMEM; i++) fprintf(f, "[%s.%s]\nvalue = %ld\n", REC[MEMT[i].rec], MEMT[i].nome, MEMT[i].valor);
        ok = !ferror(f); ok = fclose(f) == 0 && ok; ok = ok && rename(novo, MEM_ARQ) == 0;
        if (!ok) unlink(novo);
    }
    if (!ok && !MEM_AVISOU++) diz("AVISO: não gravei a memória em %s; nesta sessão o que a tela mudar não fica guardado", MEM_ARQ);
}

/* ------------------------------------------------------------------ gravar no serviço */
/* grava campos num elemento (POST) e, se o serviço aceitou, no modelo. Fica dono de campos. */
static int grava(int r, jv *el, jv *campos) {
    sb q = {0}, resp = {0}; char caminho[200]; int cod, i;
    snprintf(caminho, sizeof caminho, "/audiomanagement/%s/%s", REC[r], jstr(el, "id"));
    jescreve(&q, campos);
    cod = http("POST", caminho, q.p, &resp);
    if (cod == 200) { for (i = 0; i < campos->n; i++) jset(el, campos->k[i], jclona(campos->v[i])); if (jget(campos, "value")) mem_valor(r, el); }
    else diz("AVISO: o serviço respondeu %d ao gravar %s \"%s\"", cod, REC[r], jstr(el, "name"));
    free(q.p); free(resp.p); jfree(campos);
    return cod == 200 ? 0 : -1;
}
static jv *um(const char *chave, jv *v) { jv *o = jnovo('o'); jpoe(o, chave, v); return o; }

/* ------------------------------------------------------------------ as regras */
static jv *ENT, *MENU, *MUDO_CON;      /* conexão de entretenimento, conexão de menu com o foco, a do mudo */
static char G_ENT[64], G_PRI[64], G_FOCO[64];   /* o que o objeto global aponta agora (identificadores) */
static int iniciada(const jv *c) { return c && !strcmp(jstr(c, "status"), "started"); }
static int mudo(void) { return iniciada(MUDO_CON); }
static jv *foco(void) { return MENU ? MENU : mudo() ? MUDO_CON : ENT; }
static void atualiza_global(void) {
    jv *g = M[0]->n ? M[0]->v[0] : NULL, *campos = jnovo('o'), *f = foco(), *pri = mudo() ? MUDO_CON : ENT;
    if (!g) { jfree(campos); return; }
    if (ENT && strcmp(G_ENT, jstr(ENT, "id"))) { jpoe(campos, "entertainmentConnection", expande(ENT, 2)); snprintf(G_ENT, sizeof G_ENT, "%s", jstr(ENT, "id")); }
    if (pri && strcmp(G_PRI, jstr(pri, "id"))) { jpoe(campos, "foregroundConnection", expande(pri, 2)); snprintf(G_PRI, sizeof G_PRI, "%s", jstr(pri, "id")); }
    if (f && strcmp(G_FOCO, jstr(f, "id"))) { jpoe(campos, "settingFocus", expande(f, 2)); snprintf(G_FOCO, sizeof G_FOCO, "%s", jstr(f, "id")); diz("foco de ajuste: %s", jstr(f, "name")); }
    if (campos->n) grava(0, g, campos); else jfree(campos);
}
static void estado_da_conexao(jv *c, const char *estado) { grava(1, c, um("status", jtexto(estado))); }
static void poe_mudo(int liga, const char *motivo) {
    if (!MUDO_CON || mudo() == liga) return;
    estado_da_conexao(MUDO_CON, liga ? "started" : "stopped");
    diz("mudo do usuário: %s (%s)", liga ? "ligado" : "desligado", motivo);
    atualiza_global();
}
/* o volume em que um controle mexe: o da conexão com o foco; no mudo, o da fonte de entretenimento */
static jv *volume_do_foco(void) {
    jv *f = foco(), *v = f ? jget(f, "volumesetting") : NULL;
    if (!v && ENT) v = jget(ENT, "volumesetting");
    return v ? alvo(v) : NULL;
}
static void controle(const jv *c) {
    const char *tipo = jstr(c, "type"), *dom = jstr(c, "domain"); long n = jnum(c, "value", 0);
    if (!strcmp(tipo, "mute")) { poe_mudo(!mudo(), "controle de mudo"); return; }
    if (!strcmp(tipo, "volumeIncrease") || !strcmp(tipo, "volumeDecrease")) {
        int sobe = tipo[6] == 'I'; jv *v; long antes, depois, lo, hi; int da_fonte;
        if (n <= 0) n = 1;
        if (sobe && mudo() && !MENU) poe_mudo(0, "volume aumentado");
        v = volume_do_foco();
        if (!v) { diz("controle %s de %s sem efeito: a conexão com o foco não tem volume", tipo, dom); return; }
        antes = jnum(v, "value", 0); lo = jnum(v, "valueMin", 0); hi = jnum(v, "valueMax", 0);
        depois = antes + (sobe ? n : -n); if (depois > hi) depois = hi; if (depois < lo) depois = lo;
        da_fonte = ENT && alvo(jget(ENT, "volumesetting")) == v;
        if (depois != antes) {
            grava(2, v, um("value", jnumero(depois)));
            diz("%s: %ld -> %ld (controle %s de %s, %ld passo%s)", jstr(v, "name"), antes, depois, tipo, dom, n, n == 1 ? "" : "s");
        } else diz("%s: fica em %ld, no limite da faixa %ld a %ld (controle %s de %s)", jstr(v, "name"), antes, lo, hi, tipo, dom);
        if (!sobe && depois == 0 && da_fonte && !mudo()) poe_mudo(1, "volume em zero");
        return;
    }
    diz("controle de tipo \"%s\" sem regra", tipo);
}
/* uma conexão mudou de estado sem ser por nós: foi um cliente (a HMI) */
static void conexao_mudou(jv *c, const char *estado) {
    const char *nome = jstr(c, "name"); int ini = !strcmp(estado, "started");
    if (!strncmp(nome, "ENT_AMP_", 8)) {
        if (ini && c != ENT) {
            jv *antiga = ENT; ENT = c;
            diz("entretenimento: %s (iniciada por um cliente)", nome);
            if (iniciada(antiga)) estado_da_conexao(antiga, "stopped");
            atualiza_global();
        } else if (!ini && c == ENT) diz("a fonte de entretenimento %s foi posta em \"%s\"; o objeto global fica como está", nome, estado);
        return;
    }
    if (c == MUDO_CON) { diz("mudo do usuário: %s (pedido por um cliente)", ini ? "ligado" : "desligado"); atualiza_global(); return; }
    if (jget(c, "volumesetting")) {
        if (ini) MENU = c; else if (MENU == c) MENU = NULL;
        atualiza_global();
    }
}

/* ------------------------------------------------------------------ WebSocket (cliente) */
static int WS = -1; static sb WSBUF, WSMSG; static time_t WS_PING;
/* As conexões são assinadas uma a uma, como a HMI faz, e não pela lista: cada aviso da lista é um
 * retrato dela inteira, e um retrato tirado antes de uma gravação nossa chega depois dela e
 * parece pedido novo de cliente (na troca de fonte isso fazia o programa ir e voltar entre as
 * duas). O aviso de um elemento só fala daquela conexão, na ordem em que ela mudou. */
static int VIU_CONTROLES, VIU_CONEXOES;       /* VIU_CONEXOES: de quantas conexões já chegou o estado inicial */
static unsigned char *CON_VISTA;              /* por posição em M[1] */
#define EV_CONTROLES "/audiomanagement/controls"
#define EV_CONEXAO   "/audiomanagement/connections/"
/* Com memória (-m), cada volume e cada parâmetro de som também é assinado, um a um: grave, agudo e
 * balanço a tela grava direto no serviço, sem passar por este programa. Sem -m nada disso é
 * assinado, e o programa se comporta como antes. */
#define EV_VOLUME    "/audiomanagement/volumesettings/"
#define EV_PARAM     "/audiomanagement/soundparameters/"
static int VIU_VALORES; static unsigned char *VAL_VISTA[NREC];    /* por posição em M[2] e em M[4] */
static int ws_manda(int op, const char *p, size_t n) {
    unsigned char h[14]; size_t hn = 2, i; char *m; int rc;
    static const unsigned char masc[4] = { 0x37, 0xfa, 0x21, 0x3d };
    h[0] = (unsigned char)(0x80 | op);
    if (n < 126) h[1] = (unsigned char)(0x80 | n);
    else if (n < 65536) { h[1] = 0x80 | 126; h[2] = (unsigned char)(n >> 8); h[3] = (unsigned char)n; hn = 4; }
    else { h[1] = 0x80 | 127; for (i = 0; i < 8; i++) h[2 + i] = (unsigned char)((uint64_t)n >> (56 - 8 * i)); hn = 10; }
    memcpy(h + hn, masc, 4); hn += 4;
    m = malloc(n + 1); for (i = 0; i < n; i++) m[i] = (char)(p[i] ^ masc[i & 3]);
    rc = manda_tudo(WS, (char *)h, hn) || manda_tudo(WS, m, n); free(m); return rc;
}
static int ws_assina(const char *evento) {
    sb q = {0}; int rc; jv *o = jnovo('o');
    jpoe(o, "type", jtexto("subscribe")); jpoe(o, "event", jtexto(evento)); jescreve(&q, o);
    rc = ws_manda(1, q.p, q.n); free(q.p); jfree(o); return rc;
}
static int ws_abre(void) {
    char q[300], resp[1024] = ""; ssize_t k; size_t n = 0; struct timeval tv = { 0, 0 }; int i, r;
    WS = liga_tcp(); if (WS < 0) return -1;
    snprintf(q, sizeof q, "GET / HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nUser-Agent: somstub\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n"
             "Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: c29tIGRlIG1lbnRpcmEhIQ==\r\n\r\n", PORTA);
    if (manda_tudo(WS, q, strlen(q))) { close(WS); WS = -1; return -1; }
    while (n < sizeof resp - 1 && (k = recv(WS, resp + n, 1, 0)) == 1) { n++; resp[n] = 0; if (n >= 4 && !strcmp(resp + n - 4, "\r\n\r\n")) break; }
    if (strncmp(resp, "HTTP/1.1 101", 12)) { diz("AVISO: o serviço não aceitou o WebSocket: %.40s", n ? resp : "(sem resposta)"); close(WS); WS = -1; return -1; }
    setsockopt(WS, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    sb_zera(&WSBUF); sb_zera(&WSMSG); VIU_CONTROLES = VIU_CONEXOES = 0; WS_PING = time(NULL);
    free(CON_VISTA); CON_VISTA = calloc((size_t)M[1]->n + 1, 1);
    if (ws_assina(EV_CONTROLES)) { close(WS); WS = -1; return -1; }
    for (i = 0; i < M[1]->n; i++) {
        char ev[200]; snprintf(ev, sizeof ev, EV_CONEXAO "%s?$fields=status,postIdentifier", jstr(M[1]->v[i], "id"));
        if (ws_assina(ev)) { close(WS); WS = -1; return -1; }
    }
    VIU_VALORES = 0;
    for (r = 2; MEM_ARQ && r <= 4; r += 2) {
        free(VAL_VISTA[r]); VAL_VISTA[r] = calloc((size_t)M[r]->n + 1, 1);
        for (i = 0; i < M[r]->n; i++) {
            char ev[200]; snprintf(ev, sizeof ev, "%s%s?$fields=value", r == 2 ? EV_VOLUME : EV_PARAM, jstr(M[r]->v[i], "id"));
            if (ws_assina(ev)) { close(WS); WS = -1; return -1; }
        }
    }
    return 0;
}
/* controles já atendidos (os eventos trazem a lista inteira de cada vez) */
static char (*VISTOS)[40]; static int NVISTOS, CAPVISTOS;
static int ja_visto(const char *id) {
    int i; for (i = NVISTOS - 1; i >= 0; i--) if (!strcmp(VISTOS[i], id)) return 1;
    if (NVISTOS == CAPVISTOS) { CAPVISTOS = CAPVISTOS ? CAPVISTOS * 2 : 256; VISTOS = realloc(VISTOS, (size_t)CAPVISTOS * 40); if (!VISTOS) exit(9); }
    snprintf(VISTOS[NVISTOS++], 40, "%s", id); return 0;
}
static void mensagem(const char *texto) {
    jv *m = jparse(texto), *d; const char *ev; int i;
    if (!m) return;
    ev = jstr(m, "event"); d = jget(m, "data");
    if (!strcmp(jstr(m, "type"), "error")) diz("AVISO: o serviço recusou a assinatura de %s", ev);
    if (!strcmp(jstr(m, "type"), "data") && d && d->t == 'a') {
        if (!strncmp(ev, EV_CONTROLES, strlen(EV_CONTROLES))) {
            for (i = 0; i < d->n; i++) {
                const char *id = jstr(d->v[i], "id"); char caminho[200]; sb r = {0}; static int avisou;
                if (ja_visto(id)) continue;
                if (VIU_CONTROLES) controle(d->v[i]);        /* a primeira lista é o que já estava lá: não se reaplica */
                snprintf(caminho, sizeof caminho, "/audiomanagement/controls/%s", id);
                if (http("DELETE", caminho, NULL, &r) != 200 && !avisou++)
                    diz("AVISO: o serviço não deixa apagar controle; a lista de controles vai crescer até a central desligar");
                free(r.p);
            }
            VIU_CONTROLES = 1;
        }
    }
    if (!strcmp(jstr(m, "type"), "data") && d && d->t == 'o' && !strncmp(ev, EV_CONEXAO, strlen(EV_CONEXAO))) {
        const char *id = jstr(d, "id"), *st = jstr(d, "status"); jv *c = NULL; int primeira;
        for (i = 0; i < M[1]->n; i++) if (!strcmp(jstr(M[1]->v[i], "id"), id)) { c = M[1]->v[i]; break; }
        if (c && *st) {
            primeira = !CON_VISTA[i];                    /* o primeiro aviso de cada conexão é o estado que ela já tinha */
            if (primeira) { CON_VISTA[i] = 1; VIU_CONEXOES++; }
            if (strcmp(jstr(c, "status"), st)) {
                jset(c, "status", jtexto(st)); jset(c, "postIdentifier", jtexto(jstr(d, "postIdentifier")));
                if (!primeira) conexao_mudou(c, st);
            }
        }
    }
    if (MEM_ARQ && !strcmp(jstr(m, "type"), "data") && d && d->t == 'o') {
        int r = !strncmp(ev, EV_VOLUME, strlen(EV_VOLUME)) ? 2 : !strncmp(ev, EV_PARAM, strlen(EV_PARAM)) ? 4 : -1;
        jv *val = jget(d, "value");
        if (r > 0 && val && val->t == 'n') {
            const char *pid = ev + strlen(r == 2 ? EV_VOLUME : EV_PARAM); size_t k = strcspn(pid, "?");
            for (i = 0; i < M[r]->n; i++) { const char *id = jstr(M[r]->v[i], "id"); if (strlen(id) == k && !strncmp(id, pid, k)) break; }
            if (i < M[r]->n) {
                int primeira = !VAL_VISTA[r][i];        /* o primeiro aviso de cada um é o valor que ele já tinha */
                if (primeira) { VAL_VISTA[r][i] = 1; VIU_VALORES++; }
                if (jnum(M[r]->v[i], "value", 0) != atol(val->s)) {
                    jset(M[r]->v[i], "value", jnumero(atol(val->s)));
                    if (!primeira) mem_valor(r, M[r]->v[i]);       /* foi a tela (ou outro cliente) que gravou */
                }
            }
        }
    }
    jfree(m);
}
/* lê o que chegou e trata as mensagens completas; -1 se a ligação caiu */
static int ws_le(void) {
    char buf[16384]; ssize_t k = recv(WS, buf, sizeof buf, MSG_DONTWAIT);
    if (k == 0 || (k < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) return -1;
    if (k > 0) sb_poe(&WSBUF, buf, (size_t)k);
    for (;;) {
        unsigned char *p = (unsigned char *)WSBUF.p; size_t n = WSBUF.n, hn = 2, len, i; int op, fim;
        if (n < 2) break;
        fim = p[0] & 0x80; op = p[0] & 0x0f; len = p[1] & 0x7f;
        if (len == 126) { if (n < 4) break; len = ((size_t)p[2] << 8) | p[3]; hn = 4; }
        else if (len == 127) { if (n < 10) break; len = 0; for (i = 0; i < 8; i++) len = (len << 8) | p[2 + i]; hn = 10; }
        if (p[1] & 0x80) hn += 4;                       /* servidor não mascara; se mascarar, pula a máscara */
        if (n < hn + len) break;
        if (op == 8) return -1;
        if (op == 9) ws_manda(10, (char *)p + hn, len);
        if (op == 0 || op == 1 || op == 2) {
            sb_poe(&WSMSG, (char *)p + hn, len);
            if (fim) { mensagem(WSMSG.p); sb_zera(&WSMSG); }
        }
        memmove(WSBUF.p, WSBUF.p + hn + len, n - hn - len); WSBUF.n = n - hn - len; WSBUF.p[WSBUF.n] = 0;
    }
    return 0;
}
static int ws_espera(int ms) { struct pollfd pf = { WS, POLLIN, 0 }; if (poll(&pf, 1, ms) > 0) return ws_le(); return 0; }

/* ------------------------------------------------------------------ barramento: o papel do touch-app */
#define PID_KEYPANEL 0x80000000000ULL
static int HUB = -1;
static int hub_abre(const char *caminho) {
    struct sockaddr_un a; char hs[5]; int pid = getpid();
    HUB = socket(AF_UNIX, SOCK_SEQPACKET, 0); if (HUB < 0) return -1;
    memset(&a, 0, sizeof a); a.sun_family = AF_UNIX; strncpy(a.sun_path, caminho, sizeof a.sun_path - 1);
    if (connect(HUB, (struct sockaddr *)&a, sizeof a) < 0) { close(HUB); HUB = -1; return -1; }
    hs[0] = 'R'; hs[1] = (char)pid; hs[2] = (char)(pid >> 8); hs[3] = (char)(pid >> 16); hs[4] = (char)(pid >> 24);
    send(HUB, hs, 5, 0);
    /* O hub reenvia a quem se liga as primeiras mensagens que guardou (é assim que a HMI, que sobe
     * depois, recebe os anúncios). Num barramento de pouco movimento podem estar ali teclas e giros
     * antigos: não são de agora, e vão fora. O reenvio vem todo de uma vez, logo depois do registro. */
    { struct pollfd pf = { HUB, POLLIN, 0 }; static unsigned char fora[0x6900]; int n = 0;
      while (n < 200 && poll(&pf, 1, 250) > 0 && recv(HUB, fora, sizeof fora, MSG_DONTWAIT) > 0) n++;
      if (n) diz("barramento: %d mensagem(ns) guardada(s) pelo hub chegaram ao ligar e foram descartadas (não são teclas de agora)", n); }
    return 0;
}
/* cria o controle, como o touch-app faz: nome, domínio, estado do botão, hora, tipo e passos.
 * passos > 0 aumenta, < 0 diminui, 0 é o controle de mudo */
static void cria_controle(long passos, int teclado, const char *origem) {
    sb q = {0}, r = {0}; jv *o = jnovo('o'); char hora[32]; struct timespec ts; int cod;
    const char *tipo = passos > 0 ? "volumeIncrease" : passos < 0 ? "volumeDecrease" : "mute"; long n = passos < 0 ? -passos : passos;
    /* a HMI data os controles dela pelo relógio monotônico, em ms (MIBTimerManager.getSystemTimeMillis); o mesmo aqui */
    clock_gettime(CLOCK_MONOTONIC, &ts); snprintf(hora, sizeof hora, "%lld", (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    if (n > 5) n = 5;                                   /* a especificação do serviço dá 0 a 5 */
    /* nomes: os do volume estão no touch-app; o do mudo é o que a HMI usa quando cria o controle ela mesma */
    jpoe(o, "name", jtexto(passos > 0 ? "volumeUp_gui" : passos < 0 ? "volumeDown_gui" : "muteButtonId_tbd"));
    jpoe(o, "domain", jtexto(teclado == 13 ? "hmi" : "cluster"));
    jpoe(o, "buttonstate", jtexto("pressed")); jpoe(o, "time", jtexto(hora));
    jpoe(o, "type", jtexto(tipo)); jpoe(o, "value", jnumero(n));
    jescreve(&q, o); cod = http("POST", "/audiomanagement/controls", q.p, &r);
    if (passos) diz("%s: %s%ld (teclado %d) -> controle %s%s", origem, passos > 0 ? "+" : "-", n, teclado, tipo, cod == 201 ? "" : " RECUSADO pelo serviço");
    else diz("%s (teclado %d) -> controle mute%s", origem, teclado, cod == 201 ? "" : " RECUSADO pelo serviço");
    free(q.p); free(r.p); jfree(o);
}
/* SIMULAÇÃO (item 5 do cabeçalho): a tecla de volume do volante gira também o botão da central,
 * pelo arquivo de comandos do dsistub. Devolve 0 se escreveu; -1 se não deu (aí vale o controle direto). */
static const char *CMD_DSISTUB = "/tmp/dsi-entrada"; static int CMD_AVISOU;
static int gira_botao(long passos, const char *origem) {
    FILE *f = fopen(CMD_DSISTUB, "a"); int ok = 0;
    if (f) { ok = fprintf(f, "encoder 13 17 %ld\n", passos) > 0; ok = fclose(f) == 0 && ok; }
    if (ok) diz("%s: %s%ld -> giro do botão da central, para a barra de volume abrir (SIMULAÇÃO)", origem, passos > 0 ? "+" : "-", passos < 0 ? -passos : passos);
    else if (!CMD_AVISOU++) diz("AVISO: não escrevi no arquivo de comandos do dsistub (%s); o volante muda o volume sem abrir a barra", CMD_DSISTUB);
    return ok ? 0 : -1;
}
static void hub_le(void) {
    static unsigned char in[0x6900]; ssize_t n = recv(HUB, in, sizeof in, MSG_DONTWAIT); uint64_t ch; uint32_t ifid, api; long campo[4] = { 0, 0, 0, 0 }; ssize_t i = 20;
    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) { diz("AVISO: o barramento fechou; o botão de volume deixa de ser ouvido"); close(HUB); HUB = -1; return; }
    if (n < 20) return;
    memcpy(&ch, in, 8); memcpy(&ifid, in + 8, 4); memcpy(&api, in + 16, 4);
    if (ch != PID_KEYPANEL || ifid != 301 || (api != 23 && api != 25)) return;
    while (i < n) {                                     /* protobuf: só varints, campos 1 a 3 */
        unsigned tag = in[i++], tipo = tag & 7, num = tag >> 3; uint64_t v = 0; int sh = 0;
        if (tipo != 0) return;
        while (i < n) { unsigned char b = in[i++]; if (sh < 64) v |= (uint64_t)(b & 0x7f) << sh; sh += 7; if (!(b & 0x80)) break; }
        if (num >= 1 && num <= 3) campo[num] = (long)(int32_t)(uint32_t)v;
    }
    if (api == 23 && (campo[2] == 17 || campo[2] == 44) && campo[3] != 0) {
        if (campo[2] == 44 && VOLANTE_BARRA && gira_botao(campo[3], "rolete do volante") == 0) return;
        cria_controle(campo[3], (int)campo[1], campo[2] == 17 ? "botão de volume" : "rolete do volante");
    }
    if (api == 25 && campo[1] == 4 && (campo[2] == 42 || campo[2] == 43) && campo[3] == 1) {
        if (VOLANTE_BARRA && gira_botao(campo[2] == 42 ? 1 : -1, "tecla de volume do volante") == 0) return;
        cria_controle(campo[2] == 42 ? 1 : -1, 4, "tecla de volume do volante");
    }
    /* mudo: KEY_MFW_MUTE do volante (4/57) e KEY_MUTE da moldura (13/88), ao apertar */
    if (api == 25 && campo[3] == 1 && ((campo[1] == 4 && campo[2] == 57) || (campo[1] == 13 && campo[2] == 88)))
        cria_controle(0, (int)campo[1], "tecla de mudo");
}

/* ------------------------------------------------------------------ partida */
/* aplica o perfil e o estado de partida ao modelo, em memória */
static void aplica_perfil(void) {
    int i, k, e, n = 0; jv *g = M[0]->n ? M[0]->v[0] : NULL;
    for (i = 0; i < NAJ; i++)
        for (k = AJ[i].rec, e = 0; *AJ[i].campo && e < M[k]->n; e++)
            if (!strcmp(AJ[i].nome, "*") || !strcmp(AJ[i].nome, jstr(M[k]->v[e], "name"))) { jset(M[k]->v[e], AJ[i].campo, jclona(AJ[i].valor)); n++; }
    /* a memória entra aqui, antes do estado de partida: o objeto global leva embutido o volume da fonte */
    for (i = 0; i < NMEM; i++) { jv *el = acha(MEMT[i].rec, MEMT[i].nome); if (el) jset(el, "value", jnumero(MEMT[i].valor)); }
    if (*ENT_INICIAL && g) {
        jv *c = acha(1, ENT_INICIAL);
        jset(c, "status", jtexto("started"));
        jset(g, "entertainmentConnection", expande(c, 2)); jset(g, "foregroundConnection", expande(c, 2)); jset(g, "settingFocus", expande(c, 2));
    }
    if (MEM_MUDO == 1 && g) {                      /* o mudo volta como as regras o deixam: primeiro plano e foco de ajuste */
        jv *c = acha(1, "FCT_AMP_USER_MUTE");
        if (c) { jset(c, "status", jtexto("started")); jset(g, "foregroundConnection", expande(c, 2)); jset(g, "settingFocus", expande(c, 2)); }
    }
    diz("perfil aplicado: %d campo%s gravado%s; entretenimento: %s", n, n == 1 ? "" : "s", n == 1 ? "" : "s", *ENT_INICIAL ? ENT_INICIAL : "nenhuma");
    if (MEM_ARQ) diz("memória aplicada: %d valor%s%s%s%s", NMEM, NMEM == 1 ? "" : "es", MEM_MUDO >= 0 ? " e o mudo (" : "", MEM_MUDO == 1 ? "ligado" : MEM_MUDO == 0 ? "desligado" : "", MEM_MUDO >= 0 ? ")" : "");
}
/* escreve o modelo como o gerador escreve: uma lista por recurso, um elemento por linha */
static int escreve_pasta(const char *destino) {
    int i, e; sb b = {0}; char caminho[600]; FILE *f;
    snprintf(caminho, sizeof caminho, "mkdir -p '%s/audiomanagement'", destino);
    if (system(caminho) != 0) { diz("ERRO: não criei %s/audiomanagement", destino); return -1; }
    for (i = 0; i < NREC; i++) {
        sb_zera(&b); sb_str(&b, "[");
        for (e = 0; e < M[i]->n; e++) { if (e) sb_str(&b, ",\n"); jescreve(&b, M[i]->v[e]); }
        sb_str(&b, "]\n");
        snprintf(caminho, sizeof caminho, "%s/audiomanagement/%s.json", destino, REC[i]);
        f = fopen(caminho, "w");
        if (!f || fwrite(b.p, 1, b.n, f) != b.n || fclose(f) != 0) { diz("ERRO: não escrevi %s", caminho); return -1; }
    }
    /* o que o perfil diz e não é dado do serviço: o programa lê daqui quando fica no ar */
    snprintf(caminho, sizeof caminho, "%s/opcoes.txt", destino);
    f = fopen(caminho, "w");
    if (!f || fprintf(f, "# opções do som de mentira, tiradas do perfil pelo somstub -w\nvolante_abre_a_barra = %s\n", VOLANTE_BARRA ? "sim" : "nao") < 0 || fclose(f) != 0) { diz("ERRO: não escrevi %s", caminho); return -1; }
    free(b.p); return 0;
}
/* no ar: as opções que o -w deixou em <destino>/opcoes.txt (sem o arquivo, tudo desligado) */
static void le_opcoes(const char *destino) {
    char caminho[600], l[200], *s; FILE *f;
    snprintf(caminho, sizeof caminho, "%s/opcoes.txt", destino);
    if (!(f = fopen(caminho, "r"))) return;
    while (fgets(l, sizeof l, f)) { s = apara(l); if (!strcmp(s, "volante_abre_a_barra = sim")) VOLANTE_BARRA = 1; }
    fclose(f);
}
/* o estado das regras, tirado do que o serviço tem (vale na partida e se o programa for religado) */
static void le_estado(void) {
    jv *g = M[0]->n ? M[0]->v[0] : NULL;
    MUDO_CON = acha(1, "FCT_AMP_USER_MUTE");
    snprintf(G_ENT, sizeof G_ENT, "%s", jstr(jget(g, "entertainmentConnection"), "id"));
    snprintf(G_PRI, sizeof G_PRI, "%s", jstr(jget(g, "foregroundConnection"), "id"));
    snprintf(G_FOCO, sizeof G_FOCO, "%s", jstr(jget(g, "settingFocus"), "id"));
    ENT = *G_ENT ? acha_id(1, G_ENT) : NULL;
}
/* -s: a especificação do serviço com os dois pontos afrouxados (ver o cabeçalho) */
static int afrouxa_especificacao(const char *entrada, const char *saida) {
    sb b = {0}; jv *e, *rec, *ev, *el; FILE *f; int i, tem = 0;
    if (le_arquivo(entrada, &b) < 0) { diz("ERRO: não li a especificação %s", entrada); return -1; }
    e = jparse(b.p); rec = jget(e, "resources");
    ev = jget(jget(rec, "soundsettings"), "systemTriggeredEvents"); el = jget(jget(jget(rec, "controls"), "endpoints"), "element");
    if (strcmp(jstr(e, "name"), "audiomanagement") || !ev || ev->t != 'a' || !el || el->t != 'o') {
        diz("ERRO: %s não é a especificação do audiomanagement que eu conheço (faltam soundsettings.systemTriggeredEvents ou controls.endpoints.element)", entrada); return -1;
    }
    for (i = 0; i < ev->n; i++) tem |= ev->v[i]->t == 's' && !strcmp(ev->v[i]->s, "resource");
    if (!tem) jpoe(ev, NULL, jtexto("resource"));
    diz("especificação: soundsettings %s assinatura de recurso; controls %s DELETE de elemento",
        tem ? "já avisava" : "passa a avisar", jget(el, "delete") ? "já aceitava" : "passa a aceitar");
    if (!jget(el, "delete")) jpoe(el, "delete", jnovo('o'));
    sb_zera(&b); jescreve(&b, e); sb_str(&b, "\n");
    f = fopen(saida, "w");
    if (!f || fwrite(b.p, 1, b.n, f) != b.n || fclose(f) != 0) { diz("ERRO: não escrevi %s", saida); return -1; }
    free(b.p); return 0;
}
static int uso(void) {
    fprintf(stderr, "uso: somstub -n <pasta> <perfil>...\n     somstub -w <destino> [-m <memória>] <pasta> <perfil>...\n     somstub -s <especificação> <saída>\n     somstub [-p porta] [-e segundos] [-b socket do hub | -] [-m <memória>] [-c <comandos do dsistub>] <destino>\n");
    return 2;
}
int main(int argc, char **argv) {
    int so_confere = 0, espera = 30, i, a = 1, total = 0, aj = 0, nval; const char *hub = "/run/kipc_hub.sock", *pasta, *destino = NULL; jv *arq[NREC], *v; sb b = {0}; time_t t0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (argc == 4 && !strcmp(argv[1], "-s")) return afrouxa_especificacao(argv[2], argv[3]) < 0 ? 1 : 0;
    for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
        if (!strcmp(argv[a], "-n")) so_confere = 1;
        else if (!strcmp(argv[a], "-w") && a + 1 < argc) destino = argv[++a];
        else if (!strcmp(argv[a], "-p") && a + 1 < argc) PORTA = atoi(argv[++a]);
        else if (!strcmp(argv[a], "-e") && a + 1 < argc) espera = atoi(argv[++a]);
        else if (!strcmp(argv[a], "-b") && a + 1 < argc) hub = argv[++a];
        else if (!strcmp(argv[a], "-m") && a + 1 < argc) MEM_ARQ = argv[++a];
        else if (!strcmp(argv[a], "-c") && a + 1 < argc) CMD_DSISTUB = argv[++a];
        else return uso();
    }
    if (a >= argc) return uso();
    pasta = argv[a++];
    if (!so_confere && !destino && a < argc) return uso();       /* no ar não se passa perfil: ele já está nos dados */
    if (carrega_da_pasta(pasta, arq) < 0) return 1;
    for (i = 0; i < NREC; i++) total += arq[i]->n;
    if (so_confere || destino) {
        memcpy(M, arq, sizeof M);
        for (i = a; i < argc; i++) if (le_perfil(argv[i]) < 0) return 1;
        if (confere_perfil() < 0) return 1;
        for (i = 0; i < NAJ; i++) aj += *AJ[i].campo != 0;
        diz("conferido: %d recursos, %d elementos em %s; perfil: %d ajuste%s, entretenimento %s", NREC, total, pasta, aj, aj == 1 ? "" : "s", *ENT_INICIAL ? ENT_INICIAL : "nenhuma");
        if (!destino) return 0;
        if (MEM_ARQ) le_memoria(MEM_ARQ);
        aplica_perfil();
        return escreve_pasta(destino) < 0 ? 1 : 0;
    }
    /* espera o serviço aparecer no mock */
    for (t0 = time(NULL); ; usleep(300000)) {
        if (http("GET", "/audiomanagement/globals", NULL, &b) == 200) break;
        if (time(NULL) - t0 >= espera) { diz("ERRO: não achei o serviço audiomanagement em 127.0.0.1:%d em %d s", PORTA, espera); return 1; }
    }
    if (carrega_do_servico() < 0) return 1;
    for (i = 0; i < NREC; i++)                          /* o mock tem de estar servindo os dados da pasta, e não elementos inventados */
        if (M[i]->n != arq[i]->n || (M[i]->n && !acha_id(i, jstr(arq[i]->v[0], "id")))) {
            diz("ERRO: o serviço não está servindo os dados de %s: %s tem %d elemento(s) lá e %d no arquivo", pasta, REC[i], M[i]->n, arq[i]->n); return 1;
        }
    le_estado();
    if (MEM_ARQ) {                                  /* o que já estava guardado continua guardado, mude ou não nesta sessão */
        le_memoria(MEM_ARQ);
        diz("memória: guardo em %s o que a tela mudar (%d valor%s de antes)", MEM_ARQ, NMEM, NMEM == 1 ? "" : "es");
    }
    /* assina antes de mais nada: o que os clientes fizerem daqui em diante não se perde */
    if (ws_abre() < 0) { diz("ERRO: não abri o WebSocket do serviço"); return 1; }
    nval = MEM_ARQ ? M[2]->n + M[4]->n : 0;
    for (t0 = time(NULL); !(VIU_CONTROLES && VIU_CONEXOES == M[1]->n && VIU_VALORES == nval) && time(NULL) - t0 < 10; ) if (ws_espera(200) < 0) break;
    if (!(VIU_CONTROLES && VIU_CONEXOES == M[1]->n && VIU_VALORES == nval)) { diz("ERRO: o serviço não mandou o estado inicial das assinaturas (controles: %d; conexões: %d de %d; volumes e parâmetros: %d de %d)", VIU_CONTROLES, VIU_CONEXOES, M[1]->n, VIU_VALORES, nval); return 1; }
    v = volume_do_foco();
    diz("no ar na porta %d: %d elementos em %d recursos; entretenimento %s, foco de ajuste %s, volume %s em %ld (faixa %ld a %ld), mudo %s",
        PORTA, total, NREC, ENT ? jstr(ENT, "name") : "nenhum", foco() ? jstr(foco(), "name") : "nenhum",
        v ? jstr(v, "name") : "nenhum", v ? jnum(v, "value", 0) : 0, v ? jnum(v, "valueMin", 0) : 0, v ? jnum(v, "valueMax", 0) : 0, mudo() ? "ligado" : "desligado");
    le_opcoes(pasta);
    if (VOLANTE_BARRA) diz("o volante abre a barra de volume: SIMULAÇÃO ligada pelo perfil (a tecla de volume do volante gira também o botão da central, pelo %s)", CMD_DSISTUB);
    if (strcmp(hub, "-")) { if (hub_abre(hub) < 0) diz("AVISO: sem barramento em %s; o botão de volume não será ouvido", hub); else diz("ouvindo o botão de volume no barramento (%s)", hub); }
    for (;;) {
        struct pollfd pf[2] = { { WS, POLLIN, 0 }, { HUB, POLLIN, 0 } };
        poll(pf, HUB >= 0 ? 2 : 1, 1000);
        if (HUB >= 0 && (pf[1].revents & (POLLIN | POLLHUP | POLLERR))) hub_le();
        if ((pf[0].revents & (POLLIN | POLLHUP | POLLERR)) && ws_le() < 0) {
            diz("AVISO: a ligação com o serviço caiu; tentando de novo");
            close(WS); WS = -1;
            for (i = 0; i < 20 && ws_abre() < 0; i++) sleep(1);
            if (WS < 0) { diz("ERRO: o serviço não voltou"); return 1; }
        }
        if (time(NULL) - WS_PING >= 15) { ws_manda(9, "", 0); WS_PING = time(NULL); }
        if (MEM_ARQ) {                              /* o mudo muda por vários caminhos: confere aqui, num lugar só */
            if (MEM_MUDO != mudo()) { MEM_MUDO = mudo(); MEM_SUJA = 1; }
            if (MEM_SUJA) mem_grava();
        }
    }
}
