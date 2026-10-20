/* veiculostub.c — o "carro de mentira" do sandbox: responde aos serviços DSI de veículo que a HMI
 * assina (relógio, computador de bordo, estados do carro, clima…). Marco 8, 11/10/2026.
 * Estudo num computador, sem carro; nada daqui vai para o carro.
 *
 * No carro quem fornece esses serviços é o carifservice do firmware (domínio
 * PID_BAP_CAR_FUNCTIONS_ADAPTER = 16), que traduz o que chega da rede do carro. Na bancada não há
 * rede do carro, então este programa ocupa o lugar dele no barramento:
 *  - anuncia o domínio (interface 9000), que é o que faz a HMI ligar os serviços. UMA vez, ao
 *    subir: o kipc_hub guarda as primeiras mensagens e as reentrega a quem se liga depois, e é
 *    assim que o anúncio chega à HMI. Repetir de segundo em segundo, como os outros stubs, aqui
 *    faz mal: a cada anúncio a HMI agenda, numa thread nova, a criação de dez serviços para 5 s
 *    depois (BAPTaskScheduler), e a criação não tem trava (ServiceAdminImpl.startService). Com
 *    a rajada de anúncios guardados as threads acordavam juntas e um serviço podia nascer duas
 *    vezes; o painel ficava mudo em parte das partidas. Só se a HMI aparecer no barramento e
 *    passar 2 s sem pedir nada o anúncio é repetido, de 2 em 2 s, até o primeiro pedido;
 *  - quando a HMI assina atributos (api 9999 na interface de pedidos do serviço), manda a
 *    atualização de cada um na interface de eventos, com a api de mesmo número do atributo;
 *  - o conteúdo vem do PERFIL (arquivos de valores, abaixo). Atributo sem valor no perfil recebe
 *    a mensagem padrão: tudo zero, marcada como válida (validFlag = 1, ATTRVALIDFLAG_VALID);
 *  - valores que mudam com o tempo (o relógio) saem de novo quando mudam;
 *  - api 9998 desassina;
 *  - os outros pedidos da HMI (Marco 9) são de três tipos, e o esquema diz qual:
 *      os que GRAVAM um atributo (set<X>; linhas G): o valor do pedido fica guardado no lugar do
 *        valor do perfil e a atualização do atributo sai para quem o assina, em toda instância.
 *        É a resposta que a HMI espera desses pedidos, e sai mesmo se o valor for o mesmo. Só
 *        vale nos serviços que o perfil lista em "grava". Se o campo de destino tem um marcador
 *        (@hora…), ele não vira um número fixo: o pedido acerta o relógio, que segue andando;
 *      os que têm RESPOSTA própria e prazo (linhas R, da tabela do proxy DSI da HMI): a resposta
 *        sai, na instância do pedido, se o perfil tiver a seção dela. Leva os valores do perfil
 *        e, onde ele não diz nada, o que as linhas X mandam devolver do pedido (o número da
 *        transação, por exemplo). Só as de uma resposta única; as outras ficam registradas;
 *      os outros, para os quais a HMI não marca prazo de resposta (avisos como setHMIIsReady, e
 *        também pedidos de lista que ela não cobra): ficam registrados.
 *    No log, cada pedido sai como "pedido atendido" (com o que foi gravado ou respondido) ou como
 *    "pedido sem resposta" (com o motivo).
 *    Atender pode pôr a HMI em laço: na rodada n4 de 11/10/2026 ela pediu o idioma 1 para o
 *    painel, recebeu a confirmação, pediu o 26, recebeu, pediu o 1… 222 mil vezes em 75 s (duas
 *    regras de AslTargetLscTNGBordComputerLAC que se desmentem quando a memória da central não
 *    traz o idioma do painel). Há duas defesas: o perfil tira pedidos do atendimento com o ajuste
 *    "ignora", e o pedido atendido mais de LACO_MAX vezes em LACO_S segundos deixa de ser atendido
 *    até o programa reiniciar, com aviso no log.
 *  - o que os pedidos gravam sobrevive à partida seguinte se o programa receber um arquivo de
 *    ESTADO (opção -e; 12/10/2026). No carro quem guarda a hora acertada e as unidades é o painel.
 *    A cada pedido que muda alguma coisa o arquivo é escrito de novo, ao lado e com troca de nome
 *    (quem lê nunca o acha pela metade, nem se o programa morrer no meio), e a partida seguinte o
 *    aplica por cima dos perfis. Tem a forma de um perfil, só com seções de evento e valores, e
 *    duas linhas só dele:
 *      acerto = <segundos>   quanto a HMI adiantou o relógio em relação à hora do computador no
 *                            fuso do perfil. Fica a diferença, e não a hora: o relógio do painel
 *                            segue andando com a central desligada, e este volta adiantado do
 *                            tempo que passou. O custo: ele anda com o relógio do computador (se
 *                            esse for corrigido, o acerto vai junto) e com o "fuso" do perfil;
 *      fim = <n>             a última linha, com a conta dos valores: é como se reconhece um
 *                            arquivo cortado.
 *    Arquivo com erro não impede a partida nem vale pela metade: é conferido inteiro antes de
 *    qualquer valor ser aplicado; com erro, vale só o perfil, o motivo sai no log e o arquivo
 *    fica de lado, em <estado>.recusado. Valor guardado para um campo que o perfil de agora
 *    enche com marcador é descartado, com aviso: ali quem manda é o relógio. Sem -e nada é
 *    guardado, e toda partida nasce igual, dos perfis.
 *
 * O programa não conhece nenhum serviço: tudo vem do ESQUEMA, que o driver/m8_esquema.sh tira das
 * classes do próprio firmware (stubs/veiculo/gerado/esquema.txt; o formato está no cabeçalho dele).
 *
 * Perfil: um ou mais arquivos, lidos em ordem; o de depois vale sobre o de antes, campo a campo.
 *   # comentário
 *   fuso = -3                       ajuste, antes da primeira seção: horas a somar ao UTC
 *   grava = Serviço1 Serviço2       ajuste, antes da primeira seção: serviços em que o que a HMI
 *                                   grava é aceito (o de depois troca a lista; vazio = nenhum)
 *   ignora = Serviço.pedido …       ajuste, antes da primeira seção: pedidos que ficam sem resposta
 *                                   de qualquer jeito (o de depois troca a lista; vazio = nenhum)
 *   [DSICarTimeUnitsLanguage.updateClockTime]      seção: <serviço>.<evento>, como no esquema
 *   time.hours = @hora              <caminho> = <valor>; o caminho desce pelas mensagens com "."
 *   lista[0].campo = 2              campo repetido pede índice
 *   [Serviço[1].evento]             seção POR INSTÂNCIA (12/10/2026): vale só para quem assina pela
 *                                   instância 1 do serviço, e vale inteira, no lugar da seção sem
 *                                   instância (as duas não se misturam). As outras instâncias seguem
 *                                   com a seção sem instância ou, sem ela, com o valor padrão. Só em
 *                                   atualização de atributo. Serve ao DSIMobileEquipment, que tem um
 *                                   aparelho por instância. O que um pedido grava (linhas G) e o que
 *                                   o arquivo de estado guarda continuam valendo para a seção sem
 *                                   instância
 *   [Serviço.pedido -> evento]      seção de regra: o que o pedido grava além do que o esquema diz
 *   time.timeZone = offset          <caminho no evento> = <campo do pedido>, do mesmo tipo
 * A seção de um evento de resposta, mesmo vazia, é o que faz o pedido dele ser respondido.
 * Valores: inteiro (decimal ou 0x), 0/1/true/false para bool, decimal para float e double, texto
 * entre aspas (\" e \\ dentro dele). Marcadores, só em campos inteiros: @hora @minuto @segundo
 * @dia @mes @ano @ano2000 (o ano menos 2000, que é como a HMI recebe a data).
 * Marcadores que ACOMPANHAM outro campo (12/10/2026), também só em campos inteiros: o valor sai, a
 * cada envio, do que outro evento do carro de mentira guarda naquela hora (a seção sem instância
 * dele; o campo lido tem de ser inteiro, e um campo que acompanha outro, lido assim, vale zero):
 *   campo = @de Serviço.evento.campo                o valor de agora desse campo
 *   campo = @se Serviço.evento.campo V:X … senao Y  X do primeiro par cujo V é o valor de agora;
 *                                                   Y nos outros casos (V, X e Y inteiros)
 * É o que o painel do carro faz com as unidades: a tela grava a unidade nova (pedido set… de um
 * serviço em "grava") e, no segundo seguinte, quem a acompanha sai de novo, uma vez, com o número
 * e a unidade novos. O que acompanha não vai para o arquivo de estado: lá fica só o que a tela
 * gravou, e a partida seguinte refaz a conta.
 * Como no protobuf 3, valor zero não vai no fio: é o mesmo que não dizer nada.
 * Linha errada em qualquer arquivo recusa tudo: nada é anunciado e sai o arquivo e a linha.
 *
 * Barramento (igual kipc_bus/kipc_hub): datagrama = [canal:8][interface:4][instância:4][api:4][protobuf].
 * O canal é o remetente: os eventos saem com o domínio do serviço; a HMI pede com 0x8000, e só o
 * que vem desse canal é tratado como pedido (a mesma interface pode ser a de eventos de outro domínio).
 * Estático (-static): roda dentro do chroot sob qualquer glibc.
 *
 * uso: veiculostub [-e <estado>] <socket> <esquema> [<perfil>...]     fica no barramento
 *      veiculostub [-e <estado>] -n <esquema> [<perfil>...]           só confere os arquivos (0 = tudo certo)
 *      veiculostub [-e <estado>] -m <esquema> [<perfil>...]           mostra o que seria mandado, no formato do dsi_escuta
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#define IF_WELCOME 9000
#define PID_HMI 0x8000ULL    /* PID_DSI_SERVICE: o canal com que a HMI manda os pedidos dela */
#define API_ASSINA 9999
#define API_DESASSINA 9998
#define MAXPB 25000          /* a HMI corta a mensagem em 25601 bytes (com.lge.mib3oi.dsi.Message) */
#define LACO_MAX 200         /* pedido atendido mais vezes que isto em LACO_S segundos é laço com a HMI: deixa de */
#define LACO_S 10            /* ser atendido. Um dedo na tela não chega perto; o laço da rodada n4 dava 3000 por segundo */

enum { T_SINT32, T_INT32, T_UINT32, T_ENUM, T_FIXED32, T_SINT64, T_INT64, T_FIXED64, T_BOOL, T_FLOAT, T_DOUBLE, T_STRING, T_BYTES, T_MSG, T_N };
static const char *TNOME[T_N] = { "sint32", "int32", "uint32", "enum", "fixed32", "sint64", "int64", "fixed64", "bool", "float", "double", "string", "bytes", "msg" };
enum { L_UM, L_REP, L_PAC };

enum { K_UMA, K_ESCOLHA, K_SEQUENCIA, K_COMPLEXA, K_N };       /* o que a HMI espera de um pedido com resposta */
static const char *KNOME[K_N] = { "uma", "escolha", "sequencia", "complexa" };

struct msg;
struct field { int num, type, label, line; char *name, *subname; struct msg *sub; };
struct msg { char *name; struct field *f; int nf; };
struct svc { char *name; uint64_t dom; uint32_t req, evt; int grava; };
struct attr { struct svc *s; int id; char *name; };
/* valor do perfil: uma árvore com a forma da mensagem. Folha tem txt; mensagem tem kids. */
struct val { struct field *f; int idx; struct val *kids, *next; char *txt; };
struct evt { struct svc *s; uint32_t api; int attr, line, nvals, dyn, secao; char *name, *mname; struct msg *m; struct val *vals; };
struct req { struct svc *s; uint32_t api; int line, ignora, cortado, vezes; time_t desde; char *name, *mname; struct msg *m; };
struct sub { struct evt *e; uint32_t inst; unsigned char *last; int nlast; };
/* linha R: a resposta que a HMI espera de um pedido. e = NULL: a camada da LG não a entrega. */
struct resp { struct svc *s; struct req *r; struct evt *e; uint32_t rapi, eapi; int tipo, line, semapi; long prazo; char *metodo; };
/* linhas G e X e regras do perfil: o campo "de" do pedido vai para o campo "para" do evento.
 * qual = 'G' (grava o atributo) ou 'X' (a resposta devolve o campo). */
struct copia { struct svc *s; struct req *r; struct evt *e; uint32_t rapi, eapi; int line; char qual, *de, *para; };

/* Valores por instância: a seção [Serviço[N].evento] do perfil. Cada uma é uma cópia do evento, com
 * valores próprios; as assinaturas daquela instância apontam para a cópia, e o resto do programa
 * (montar, publicar, o que muda com o tempo) trata a cópia como um evento qualquer. */
struct porinst { struct evt *base; uint32_t inst; struct evt e; };

static struct msg *msgs;   static int nmsgs;
static struct svc *svcs[256]; static int nsvcs;      /* ponteiros: eventos e pedidos apontam para cá */
static struct attr *attrs; static int nattrs;
static struct evt *evts;   static int nevts;
static struct req *reqs;   static int nreqs;
static struct sub *subs;   static int nsubs;
static struct resp *resps; static int nresps;
static struct copia *copias; static int ncopias;
static long fuso_s;                            /* segundos a somar ao UTC nos marcadores */
static long acerto_s;                          /* e o que a HMI acertou no relógio, por cima do fuso */
/* o que os pedidos gravaram, e o que veio do arquivo de estado: é o que vai para o arquivo */
struct guardado { struct evt *e; char *caminho, *txt; int aspas; };
static struct guardado *guardados; static int nguardados;
static const char *estado;                     /* o arquivo de estado (opção -e); NULL = nada fica guardado */
static int W = -1, R = -1;

/* Modelo (Marco 11). Este arquivo também serve de motor a outro programa, que o inclui e acrescenta
 * o comportamento de um domínio que não cabe no esquema nem no perfil: o driver/radiostub.c, em que
 * o sintonizador responde a um pedido com três mensagens em ordem e anda pela lista de emissoras.
 * Quem inclui define VEICULOSTUB_MODELO e escreve as três funções. Os pedidos do modelo passam pelo
 * "ignora" do perfil e pela defesa contra laço, como os outros. Sem a macro nada muda aqui. */
#ifdef VEICULOSTUB_MODELO
static int modelo_inicia(void);                /* com esquema e perfis já lidos; diferente de 0 recusa tudo */
static int modelo_trata(const struct req *r);  /* 1 = é o modelo que responde a este pedido */
static void modelo_pedido(struct req *r, uint32_t inst, const unsigned char *pb, int n);
#endif
/* Gancho de tempo, opcional (12/10/2026): o modelo que tem prazos define também
 * VEICULOSTUB_MODELO_TEMPO e escreve esta função, chamada uma vez por segundo enquanto o programa
 * está no barramento (a primeira, logo ao subir, antes de qualquer pedido). É do telefone de
 * mentira, que segura a lista de aparelhos na partida. Sem a macro nada muda aqui. */
#ifdef VEICULOSTUB_MODELO_TEMPO
static void modelo_tempo(time_t t);
#endif

static void *cresce(void *p, int n, size_t tam)
{
    if (n & (n - 1) || n == 0) return n == 0 ? realloc(p, tam) : p;      /* dobra em 1, 2, 4, 8… */
    p = realloc(p, (size_t)n * 2 * tam);
    if (!p) { fprintf(stderr, "[veiculostub] sem memória\n"); exit(1); }
    return p;
}
static char *copia(const char *s) { char *c = strdup(s); if (!c) exit(1); return c; }

static char *trim(char *s)
{
    char *e;
    while (isspace((unsigned char)*s)) s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

/* ------------------------------------------------------------------ esquema */
static struct svc *acha_svc(const char *n)
{
    int i;
    for (i = 0; i < nsvcs; i++) if (!strcmp(svcs[i]->name, n)) return svcs[i];
    return NULL;
}
static struct msg *acha_msg(const char *n)
{
    int i;
    for (i = 0; i < nmsgs; i++) if (!strcmp(msgs[i].name, n)) return &msgs[i];
    return NULL;
}
static struct field *acha_campo(struct msg *m, const char *n)
{
    int i;
    for (i = 0; i < m->nf; i++) if (!strcmp(m->f[i].name, n)) return &m->f[i];
    return NULL;
}
static int por_numero(const void *a, const void *b) { return ((const struct field *)a)->num - ((const struct field *)b)->num; }

/* decimal, ou hexa com 0x; nunca octal ("08" é oito) */
static int inteiro(const char *s, long long *v)
{
    const char *d = s + (*s == '-' || *s == '+');
    char *end;
    int hexa = d[0] == '0' && (d[1] == 'x' || d[1] == 'X');
    if (!isxdigit((unsigned char)d[hexa ? 2 : 0]) || (!hexa && !isdigit((unsigned char)*d))) return 0;
    errno = 0;
    *v = strtoll(s, &end, hexa ? 16 : 10);
    return !errno && !*end;
}

/* A folha de um caminho "a.b.c" numa mensagem: desce por campos simples e acaba num campo que não
 * é mensagem. Devolve NULL e o motivo. */
static struct field *folha(struct msg *m, const char *caminho, const char **err)
{
    char tmp[256], *seg, *save = NULL;
    struct field *f = NULL;
    if (strlen(caminho) >= sizeof tmp) { *err = "caminho longo demais"; return NULL; }
    strcpy(tmp, caminho);
    for (seg = strtok_r(tmp, ".", &save); seg; seg = strtok_r(NULL, ".", &save)) {
        if (f) {
            if (f->type != T_MSG) { *err = "o caminho passa por um campo que não é mensagem"; return NULL; }
            m = f->sub;
        }
        if (!(f = acha_campo(m, seg))) { *err = "campo desconhecido"; return NULL; }
        if (f->label != L_UM) { *err = "o caminho passa por um campo repetido"; return NULL; }
    }
    if (!f) { *err = "falta o caminho do campo"; return NULL; }
    if (f->type == T_MSG) { *err = "o campo é uma mensagem: a cópia é campo a campo"; return NULL; }
    if (f->type == T_BYTES || f->type >= T_N) { *err = "campo de um tipo que este programa não sabe copiar"; return NULL; }
    return f;
}

/* confere uma cópia de campo do pedido para o evento; devolve NULL se serve */
static const char *confere_copia(const struct copia *c)
{
    static char msg[400];
    const char *err = NULL;
    struct field *a, *b;
    if (!c->r->m) return "este pedido não leva dados: não há o que copiar";
    if (!c->e->m) return "este evento não tem mensagem";
    if (!(a = folha(c->r->m, c->de, &err))) { snprintf(msg, sizeof msg, "campo %s do pedido %s: %s", c->de, c->r->name, err); return msg; }
    if (!(b = folha(c->e->m, c->para, &err))) { snprintf(msg, sizeof msg, "campo %s do evento %s: %s", c->para, c->e->name, err); return msg; }
    if (a->type != b->type) {
        snprintf(msg, sizeof msg, "o campo %s do pedido é %s e o campo %s do evento é %s", c->de, TNOME[a->type], c->para, TNOME[b->type]);
        return msg;
    }
    return NULL;
}

static struct req *acha_pedido(struct svc *s, uint32_t api)
{
    int i;
    for (i = 0; i < nreqs; i++) if (reqs[i].s == s && reqs[i].api == api) return &reqs[i];
    return NULL;
}
static struct evt *acha_evento(struct svc *s, uint32_t api)
{
    int i;
    for (i = 0; i < nevts; i++) if (evts[i].s == s && evts[i].api == api) return &evts[i];
    return NULL;
}

/* devolve 0 se o esquema carregou; senão já disse o arquivo e a linha */
static int le_esquema(const char *file)
{
    FILE *f = fopen(file, "r");
    char line[4096], *t[8];
    const char *err = NULL;
    int ln = 0, i, k, cur = -1;
    if (!f) { fprintf(stderr, "[veiculostub] não abri o esquema %s: %s\n", file, strerror(errno)); return -1; }
    while (!err && fgets(line, sizeof line, f)) {
        char *save = NULL, *p;
        long long a, b, c;
        int n = 0;
        ln++;
        if (!strchr(line, '\n') && !feof(f)) { err = "linha longa demais"; break; }
        if ((p = strchr(line, '#'))) *p = 0;
        for (p = strtok_r(line, " \t\r\n", &save); p && n < 8; p = strtok_r(NULL, " \t\r\n", &save)) t[n++] = p;
        if (n == 0 || !strcmp(t[0], "W")) continue;
        if (!strcmp(t[0], "S") && n >= 5) {
            struct svc *s;
            if (!inteiro(t[2], &a) || !inteiro(t[3], &b) || !inteiro(t[4], &c)) { err = "serviço: domínio e interfaces têm de ser números"; break; }
            if (nsvcs >= 256) { err = "serviços demais"; break; }
            if (!(s = svcs[nsvcs++] = calloc(1, sizeof *s))) exit(1);
            s->name = copia(t[1]); s->dom = (uint64_t)a; s->req = (uint32_t)b; s->evt = (uint32_t)c;
        } else if (!strcmp(t[0], "A") && n >= 4) {
            struct svc *s = acha_svc(t[1]);
            if (!s) { err = "atributo de um serviço que não foi declarado"; break; }
            if (!inteiro(t[2], &a)) { err = "atributo: o número não é número"; break; }
            attrs = cresce(attrs, nattrs, sizeof *attrs);
            attrs[nattrs].s = s; attrs[nattrs].id = (int)a; attrs[nattrs].name = copia(t[3]); nattrs++;
        } else if (!strcmp(t[0], "E") && n >= 6) {
            struct svc *s = acha_svc(t[1]);
            struct evt *e;
            if (!s) { err = "evento de um serviço que não foi declarado"; break; }
            if (!inteiro(t[2], &a) || (strcmp(t[3], "-") && !inteiro(t[3], &b))) { err = "evento: api e atributo têm de ser números"; break; }
            evts = cresce(evts, nevts, sizeof *evts);
            e = &evts[nevts++];
            memset(e, 0, sizeof *e);
            e->s = s; e->api = (uint32_t)a; e->attr = strcmp(t[3], "-") ? (int)b : -1; e->line = ln;
            e->name = copia(t[5]); e->mname = n > 6 && strcmp(t[6], "-") ? copia(t[6]) : NULL;
        } else if (!strcmp(t[0], "P") && n >= 4) {
            struct svc *s = acha_svc(t[1]);
            struct req *r;
            if (!s) { err = "pedido de um serviço que não foi declarado"; break; }
            if (!inteiro(t[2], &a)) { err = "pedido: a api não é número"; break; }
            reqs = cresce(reqs, nreqs, sizeof *reqs);
            r = &reqs[nreqs++];
            memset(r, 0, sizeof *r);
            r->s = s; r->api = (uint32_t)a; r->line = ln;
            r->name = copia(t[3]); r->mname = n > 4 && strcmp(t[4], "-") ? copia(t[4]) : NULL;
        } else if (!strcmp(t[0], "R") && n >= 7) {         /* R <serviço> <api do pedido> <tipo> <prazo> <api da resposta ou -> <método> */
            struct svc *s = acha_svc(t[1]);
            struct resp *rp;
            if (!s) { err = "resposta de um serviço que não foi declarado"; break; }
            for (k = 0; k < K_N && strcmp(KNOME[k], t[3]); k++) ;
            if (k == K_N) { err = "resposta: tipo desconhecido (há uma, escolha, sequencia e complexa)"; break; }
            if (!inteiro(t[2], &a) || !inteiro(t[4], &b) || (strcmp(t[5], "-") && !inteiro(t[5], &c))) { err = "resposta: api do pedido, prazo e api da resposta têm de ser números"; break; }
            resps = cresce(resps, nresps, sizeof *resps);
            rp = &resps[nresps++];
            memset(rp, 0, sizeof *rp);
            rp->s = s; rp->rapi = (uint32_t)a; rp->tipo = k; rp->prazo = (long)b; rp->line = ln; rp->metodo = copia(t[6]);
            if (strcmp(t[5], "-")) rp->eapi = (uint32_t)c; else rp->semapi = 1;
        } else if ((!strcmp(t[0], "G") && n >= 7) || (!strcmp(t[0], "X") && n >= 6)) {
            /* G <serviço> <api do pedido> <api do evento> <regra> <campo do pedido> <campo do evento>
             * X <serviço> <api do pedido> <api da resposta> <campo do pedido> <campo da resposta> */
            struct svc *s = acha_svc(t[1]);
            struct copia *cp;
            int g = t[0][0] == 'G';
            if (!s) { err = "regra de um serviço que não foi declarado"; break; }
            if (!inteiro(t[2], &a) || !inteiro(t[3], &b)) { err = "regra: a api do pedido e a do evento têm de ser números"; break; }
            copias = cresce(copias, ncopias, sizeof *copias);
            cp = &copias[ncopias++];
            memset(cp, 0, sizeof *cp);
            cp->s = s; cp->rapi = (uint32_t)a; cp->eapi = (uint32_t)b; cp->line = ln; cp->qual = t[0][0];
            cp->de = copia(t[g ? 5 : 4]); cp->para = copia(t[g ? 6 : 5]);
        } else if (!strcmp(t[0], "M") && n >= 2) {
            if (acha_msg(t[1])) { err = "mensagem declarada duas vezes"; break; }
            msgs = cresce(msgs, nmsgs, sizeof *msgs);
            cur = nmsgs++;
            msgs[cur].name = copia(t[1]); msgs[cur].f = NULL; msgs[cur].nf = 0;
        } else if (!strcmp(t[0], "C") && n >= 5) {
            struct field *c2;
            if (cur < 0) { err = "campo antes de qualquer mensagem"; break; }
            if (!inteiro(t[1], &a) || a < 1) { err = "campo: o número não é número"; break; }
            msgs[cur].f = cresce(msgs[cur].f, msgs[cur].nf, sizeof *msgs[cur].f);
            c2 = &msgs[cur].f[msgs[cur].nf++];
            memset(c2, 0, sizeof *c2);
            c2->num = (int)a; c2->name = copia(t[2]); c2->line = ln;
            for (k = 0; k < T_N && strcmp(TNOME[k], t[3]); k++) ;
            c2->type = k;                         /* T_N = tipo que este programa não conhece: só dá erro se o perfil usar */
            if (!strcmp(t[4], "um")) c2->label = L_UM; else if (!strcmp(t[4], "rep")) c2->label = L_REP;
            else if (!strcmp(t[4], "pac")) c2->label = L_PAC; else { err = "campo: esperava um, rep ou pac"; break; }
            if (k == T_MSG) { if (n < 6) { err = "campo msg sem o nome da mensagem"; break; } c2->subname = copia(t[5]); }
        } else { err = "linha que não entendo"; break; }
    }
    fclose(f);
    if (err) { fprintf(stderr, "[veiculostub] %s, linha %d: %s\n", file, ln, err); return -1; }
    for (i = 0; i < nmsgs; i++) {
        qsort(msgs[i].f, (size_t)msgs[i].nf, sizeof *msgs[i].f, por_numero);
        for (k = 0; k < msgs[i].nf; k++) {
            struct field *c = &msgs[i].f[k];
            if (c->type == T_MSG && !(c->sub = acha_msg(c->subname))) {
                fprintf(stderr, "[veiculostub] %s, linha %d: o campo %s usa a mensagem %s, que não existe\n", file, c->line, c->name, c->subname);
                return -1;
            }
        }
    }
    for (i = 0; i < nevts; i++)
        if (evts[i].mname && !(evts[i].m = acha_msg(evts[i].mname))) {
            fprintf(stderr, "[veiculostub] %s, linha %d: o evento %s usa a mensagem %s, que não existe\n", file, evts[i].line, evts[i].name, evts[i].mname);
            return -1;
        }
    for (i = 0; i < nreqs; i++)
        if (reqs[i].mname && !(reqs[i].m = acha_msg(reqs[i].mname))) {
            fprintf(stderr, "[veiculostub] %s, linha %d: o pedido %s usa a mensagem %s, que não existe\n", file, reqs[i].line, reqs[i].name, reqs[i].mname);
            return -1;
        }
    for (i = 0; i < nresps; i++) {                 /* as respostas e as cópias apontam para pedidos e eventos pela api */
        struct resp *rp = &resps[i];
        if (!(rp->r = acha_pedido(rp->s, rp->rapi))) {
            fprintf(stderr, "[veiculostub] %s, linha %d: a resposta é de um pedido (api %u) que não existe em %s\n", file, rp->line, rp->rapi, rp->s->name);
            return -1;
        }
        if (!rp->semapi && !(rp->e = acha_evento(rp->s, rp->eapi))) {
            fprintf(stderr, "[veiculostub] %s, linha %d: a resposta (api %u) não é evento de %s\n", file, rp->line, rp->eapi, rp->s->name);
            return -1;
        }
    }
    for (i = 0; i < ncopias; i++) {
        struct copia *cp = &copias[i];
        if (!(cp->r = acha_pedido(cp->s, cp->rapi))) err = "a regra é de um pedido que não existe neste serviço";
        else if (!(cp->e = acha_evento(cp->s, cp->eapi))) err = "a regra grava num evento que não existe neste serviço";
        else err = confere_copia(cp);
        if (err) { fprintf(stderr, "[veiculostub] %s, linha %d: %s\n", file, cp->line, err); return -1; }
    }
    return 0;
}

/* ------------------------------------------------------------------ valores */
static const char *MARCA[] = { "@hora", "@minuto", "@segundo", "@dia", "@mes", "@ano", "@ano2000", NULL };
static const char *RELNOME[] = { "hora", "minuto", "segundo", "dia", "mês", "ano", "ano" };      /* na ordem de MARCA */

static int qual_marcador(const char *t)
{
    int k;
    for (k = 0; MARCA[k] && strcmp(MARCA[k], t); k++) ;
    return MARCA[k] ? k : -1;
}

static int marcador(const char *t, const struct tm *tm, long long *v)
{
    int k;
    for (k = 0; MARCA[k] && strcmp(MARCA[k], t); k++) ;
    if (!MARCA[k]) return 0;
    if (tm) {
        const int x[] = { tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_year - 100 };
        *v = x[k];
    }
    return 1;
}

static void agora(struct tm *tm)
{
    time_t t = time(NULL) + fuso_s + acerto_s;
    gmtime_r(&t, tm);
}

/* Acerta o relógio: troca, na hora de agora, os componentes dados (na ordem de MARCA; tem[k] diz
 * quais) e guarda a diferença. Devolve -1, ou o número do componente que está fora do intervalo;
 * aí nada muda. */
static int acerta_relogio(const long long *v, const int *tem)
{
    static const int min[] = { 0, 0, 0, 1, 1, 1970, 0 }, max[] = { 23, 59, 59, 31, 12, 2099, 99 };
    struct tm tm;
    time_t antes, depois;
    int k;
    for (k = 0; k < 7; k++) if (tem[k] && (v[k] < min[k] || v[k] > max[k])) return k;
    agora(&tm);
    antes = timegm(&tm);
    if (tem[0]) tm.tm_hour = (int)v[0];
    if (tem[1]) tm.tm_min = (int)v[1];
    if (tem[2]) tm.tm_sec = (int)v[2];
    if (tem[3]) tm.tm_mday = (int)v[3];
    if (tem[4]) tm.tm_mon = (int)v[4] - 1;
    if (tem[5]) tm.tm_year = (int)v[5] - 1900;
    if (tem[6]) tm.tm_year = (int)v[6] + 100;
    depois = timegm(&tm);
    acerto_s += (long)(depois - antes);
    return -1;
}

static int eh_inteiro(int type) { return type <= T_FIXED64; }

static struct val *acha_caminho(struct evt *e, const char *caminho);

/* Marcadores que acompanham outro campo: "@de Serviço.evento.campo" e
 * "@se Serviço.evento.campo V:X … senao Y". Devolve NULL se o texto serve, ou o motivo. Com
 * avalia, põe em *v o valor de agora (tm serve ao campo lido que tiver marcador de relógio). */
static const char *acompanha(const char *t, int avalia, const struct tm *tm, long long *v)
{
    char tmp[512], *tok, *save = NULL, *nome, *ev, *caminho, *dp;
    const char *err = NULL;
    struct svc *s;
    struct evt *e = NULL;
    struct field *f;
    struct val *lugar;
    long long agora_vale = 0, de, para;
    int se = t[1] == 's', i, achou = 0, senao = 0;
    if (strlen(t) >= sizeof tmp) return "marcador longo demais";
    strcpy(tmp, t);
    strtok_r(tmp, " \t", &save);                   /* "@de" ou "@se" */
    if (!(nome = strtok_r(NULL, " \t", &save)) || !(ev = strchr(nome, '.')) || !(caminho = strchr(ev + 1, '.')) || !caminho[1])
        return "esperava Serviço.evento.campo depois de @de ou @se";
    *ev++ = 0; *caminho++ = 0;
    if (!(s = acha_svc(nome))) return "@de e @se: serviço que não está no esquema";
    for (i = 0; i < nevts && !e; i++) if (evts[i].s == s && !strcmp(evts[i].name, ev)) e = &evts[i];
    if (!e || !e->m) return "@de e @se: evento que não está no esquema deste serviço";
    if (!(f = folha(e->m, caminho, &err))) return "@de e @se: campo que não existe nesse evento";
    if (!eh_inteiro(f->type)) return "@de e @se: o campo lido tem de ser inteiro";
    if (avalia && (lugar = acha_caminho(e, caminho)) && lugar->txt) {
        if (*lugar->txt != '@') inteiro(lugar->txt, &agora_vale);
        else marcador(lugar->txt, tm, &agora_vale);            /* relógio; campo que acompanha outro vale zero */
    }
    if (!se) {
        if (strtok_r(NULL, " \t", &save)) return "@de: sobra texto depois do campo";
        if (avalia) *v = agora_vale;
        return NULL;
    }
    while ((tok = strtok_r(NULL, " \t", &save))) {
        if (senao == 1) {                          /* o valor do senao */
            if (!inteiro(tok, &para)) return "@se: depois de senao vem um número inteiro";
            if (avalia && !achou) *v = para;
            senao = 2;
        } else if (senao == 2) return "@se: sobra texto depois do valor do senao";
        else if (!strcmp(tok, "senao") || !strcmp(tok, "senão")) senao = 1;
        else {
            if (!(dp = strchr(tok, ':'))) return "@se: esperava pares V:X e, no fim, senao Y";
            *dp++ = 0;
            if (!inteiro(tok, &de) || !inteiro(dp, &para)) return "@se: cada par é V:X, com dois números inteiros";
            if (avalia && !achou && de == agora_vale) { *v = para; achou = 1; }
        }
    }
    if (senao != 2) return "@se: falta o senao Y no fim";
    return NULL;
}

/* confere o texto de um valor contra o tipo do campo; devolve NULL se serve */
static const char *confere_valor(const struct field *f, const char *t)
{
    long long v;
    char *end;
    if (f->type == T_MSG) return "é uma mensagem: dê valor a um campo dela";
    if (f->type == T_BYTES || f->type >= T_N) return "campo de um tipo que o perfil ainda não sabe escrever";
    if (f->type == T_STRING) return NULL;          /* as aspas já foram conferidas e tiradas */
    if (*t == '@') {
        if (!eh_inteiro(f->type)) return "marcador @… só vale em campo inteiro";
        if (!strncmp(t, "@de ", 4) || !strncmp(t, "@se ", 4)) return acompanha(t, 0, NULL, &v);
        return marcador(t, NULL, &v) ? NULL : "marcador desconhecido (há @hora @minuto @segundo @dia @mes @ano @ano2000)";
    }
    if (f->type == T_BOOL) return !strcmp(t, "0") || !strcmp(t, "1") || !strcmp(t, "true") || !strcmp(t, "false") ? NULL : "booleano é 0, 1, true ou false";
    if (f->type == T_FLOAT || f->type == T_DOUBLE) {
        if (!*t) return "falta o valor";
        errno = 0; strtod(t, &end);
        return !errno && !*end ? NULL : "esperava um número decimal";
    }
    if (!inteiro(t, &v)) return "esperava um número inteiro (decimal ou 0x), ou um marcador @…";
    if ((f->type == T_SINT32 || f->type == T_INT32 || f->type == T_ENUM) && (v < INT32_MIN || v > INT32_MAX)) return "valor não cabe em 32 bits";
    if ((f->type == T_UINT32 || f->type == T_FIXED32) && (v < 0 || v > 0xffffffffLL)) return "valor não cabe em 32 bits sem sinal";
    if (f->type == T_FIXED64 && v < 0) return "campo sem sinal não aceita negativo";
    return NULL;
}

static struct val *no(struct val **lista, struct field *f, int idx)
{
    struct val *v;
    for (v = *lista; v; v = v->next) if (v->f == f && v->idx == idx) return v;
    v = calloc(1, sizeof *v);
    if (!v) exit(1);
    v->f = f; v->idx = idx; v->next = *lista; *lista = v;
    return v;
}

static struct porinst **porinsts; static int nporinsts;        /* ponteiros: as assinaturas apontam para dentro */

/* o evento que vale para uma instância: a cópia da seção dela, se o perfil tem uma (ou se cria = 1), ou o próprio */
static struct evt *da_instancia(struct evt *e, uint32_t inst, int cria)
{
    struct porinst *pi;
    int i;
    for (i = 0; i < nporinsts; i++) if (porinsts[i]->base == e && porinsts[i]->inst == inst) return &porinsts[i]->e;
    if (!cria) return e;
    porinsts = cresce(porinsts, nporinsts, sizeof *porinsts);
    if (!(pi = porinsts[nporinsts++] = calloc(1, sizeof *pi))) exit(1);
    pi->base = e; pi->inst = inst;
    pi->e = *e;                                    /* mesmo serviço, api, atributo, nome e mensagem */
    pi->e.vals = NULL; pi->e.nvals = 0; pi->e.dyn = 0; pi->e.secao = 0;
    return &pi->e;
}

/* "caminho = valor" dentro da seção do evento e */
static const char *atribui(struct evt *e, char *caminho, char *valor, int aspas)
{
    struct msg *m = e->m;
    struct val **lista = &e->vals, *v = NULL;
    struct field *f = NULL;
    const char *err;
    char *seg, *save = NULL;
    if (!m) return "este evento não tem mensagem";
    if (!*caminho || caminho[strlen(caminho) - 1] == '.') return "falta o caminho do campo";
    for (seg = strtok_r(caminho, ".", &save); seg; seg = strtok_r(NULL, ".", &save)) {
        char *col = strchr(seg, '[');
        long long idx = 0;
        if (f) {                                   /* o segmento anterior tem de ser mensagem para descer */
            if (f->type != T_MSG) return "caminho passa por um campo que não é mensagem";
            m = f->sub; lista = &v->kids;
        }
        if (col) {
            char *fim = strchr(col, ']');
            if (!fim || fim[1]) return "índice mal escrito: esperava nome[0]";
            *col = 0; *fim = 0;
            if (!inteiro(trim(col + 1), &idx) || idx < 0 || idx > 9999) return "índice tem de ser um número de 0 a 9999";
        }
        seg = trim(seg);
        if (!(f = acha_campo(m, seg))) return "campo desconhecido nesta mensagem";
        if (f->label == L_UM && col) return "índice em campo que não é lista";
        if (f->label != L_UM && !col) return "campo repetido pede índice: nome[0]";
        v = no(lista, f, (int)idx);
    }
    if (aspas && f->type != T_STRING) return "texto entre aspas num campo que não é de texto";
    if (!aspas && f->type == T_STRING) return "campo de texto: o valor vai entre aspas";
    if ((err = confere_valor(f, valor))) return err;
    if (!v->txt) e->nvals++;
    free(v->txt);
    v->txt = copia(valor);
    if (*valor == '@' && !aspas) e->dyn = 1;
    return NULL;
}

static int le_perfil(const char *file)
{
    FILE *f = fopen(file, "r");
    char line[4096];
    struct evt *cur = NULL;
    struct req *rcur = NULL;                       /* dentro de uma seção de regra: o pedido dela */
    const char *err = NULL;
    int ln = 0, i;
    if (!f) { fprintf(stderr, "[veiculostub] não abri o perfil %s: %s; nada foi enviado\n", file, strerror(errno)); return -1; }
    while (!err && fgets(line, sizeof line, f)) {
        char *p, *eq, *lhs, *rhs;
        int q = 0, aspas = 0;
        ln++;
        if (!strchr(line, '\n') && !feof(f)) { err = "linha longa demais"; break; }
        for (p = line; *p; p++) {                  /* '#' fora de aspas começa comentário */
            if (*p == '\\' && q && p[1]) p++;
            else if (*p == '"') q = !q;
            else if (*p == '#' && !q) { *p = 0; break; }
        }
        p = trim(line);
        if (!*p) continue;
        if (*p == '[') {
            char *fim = strrchr(p, ']'), *pt, *seta, *col;
            struct svc *s;
            long long ninst = -1;                      /* [Serviço[N].evento]: a seção só da instância N */
            if (!fim || *trim(fim + 1)) { err = "seção mal escrita: esperava [Serviço.evento]"; break; }
            *fim = 0; p = trim(p + 1);
            if (!(pt = strchr(p, '.'))) { err = "seção mal escrita: esperava [Serviço.evento]"; break; }
            *pt++ = 0;
            if ((col = strchr(p, '['))) {
                char *fc = strchr(col, ']');
                if (!fc || fc[1] || !inteiro(trim((*fc = 0, col + 1)), &ninst) || ninst < 0 || ninst > 255) { err = "instância mal escrita: esperava [Serviço[0].evento], com um número de 0 a 255"; break; }
                *col = 0;
            }
            if (!(s = acha_svc(p))) { err = "serviço que não está no esquema"; break; }
            rcur = NULL;
            if ((seta = strstr(pt, "->"))) {           /* [Serviço.pedido -> evento]: regra de gravação */
                *seta = 0; seta = trim(seta + 2); pt = trim(pt);
                for (i = 0; i < nreqs && !rcur; i++) if (reqs[i].s == s && !strcmp(reqs[i].name, pt)) rcur = &reqs[i];
                if (!rcur) { err = "pedido que não está no esquema deste serviço"; break; }
                if (ninst >= 0) { err = "seção de regra não leva instância: o que um pedido grava vale para a seção sem instância"; break; }
                pt = seta;
            }
            for (cur = NULL, i = 0; i < nevts && !cur; i++) if (evts[i].s == s && !strcmp(evts[i].name, pt)) cur = &evts[i];
            if (!cur) { err = "evento que não está no esquema deste serviço"; break; }
            if (ninst >= 0) {
                if (cur->attr < 0) { err = "seção com instância só vale para atualização de atributo; a resposta de um pedido sai na instância do pedido"; break; }
                cur = da_instancia(cur, (uint32_t)ninst, 1);
            }
            if (!rcur) cur->secao = 1;
            continue;
        }
        if (!(eq = strchr(p, '='))) { err = "falta o '='"; break; }
        *eq = 0;
        lhs = trim(p); rhs = trim(eq + 1);
        if (*rhs == '"') {                         /* texto: tira as aspas e desfaz \" e \\ */
            char *o = rhs, *r = rhs + 1;
            aspas = 1;
            while (*r && *r != '"') { if (*r == '\\' && (r[1] == '"' || r[1] == '\\')) r++; *o++ = *r++; }
            if (*r != '"' || *trim(r + 1)) { err = "texto mal fechado: esperava \"…\""; break; }
            *o = 0;
        }
        if (!cur) {
            char *end;
            double h;
            if (!strcmp(lhs, "grava")) {               /* a lista de depois troca a de antes */
                char *nome, *sv = NULL;
                if (aspas) { err = "grava: esperava nomes de serviço, sem aspas"; break; }
                for (i = 0; i < nsvcs; i++) svcs[i]->grava = 0;
                for (nome = strtok_r(rhs, " \t", &sv); nome && !err; nome = strtok_r(NULL, " \t", &sv)) {
                    struct svc *s = acha_svc(nome);
                    if (s) s->grava = 1; else err = "grava: serviço que não está no esquema";
                }
                continue;
            }
            if (!strcmp(lhs, "ignora")) {              /* a lista de depois troca a de antes */
                char *nome, *sv = NULL;
                if (aspas) { err = "ignora: esperava Serviço.pedido, sem aspas"; break; }
                for (i = 0; i < nreqs; i++) reqs[i].ignora = 0;
                for (nome = strtok_r(rhs, " \t", &sv); nome && !err; nome = strtok_r(NULL, " \t", &sv)) {
                    char *pt = strchr(nome, '.');
                    struct svc *s;
                    int achou = 0;
                    if (!pt) { err = "ignora: esperava Serviço.pedido"; break; }
                    *pt++ = 0;
                    if (!(s = acha_svc(nome))) { err = "ignora: serviço que não está no esquema"; break; }
                    for (i = 0; i < nreqs; i++) if (reqs[i].s == s && !strcmp(reqs[i].name, pt)) { reqs[i].ignora = 1; achou = 1; }
                    if (!achou) err = "ignora: pedido que não está no esquema deste serviço";
                }
                continue;
            }
            if (strcmp(lhs, "fuso")) { err = "valor fora de seção: antes da primeira [Serviço.evento] só valem os ajustes fuso, grava e ignora"; break; }
            errno = 0; h = strtod(rhs, &end);
            if (aspas || !*rhs || errno || *end || h < -24 || h > 24) { err = "fuso: esperava as horas a somar ao UTC, de -24 a 24"; break; }
            fuso_s = (long)(h * 3600);
            continue;
        }
        if (rcur) {                                    /* regra: <campo do evento> = <campo do pedido> */
            struct copia c;
            if (aspas) { err = "numa regra o lado direito é um campo do pedido, sem aspas"; break; }
            memset(&c, 0, sizeof c);
            c.s = cur->s; c.r = rcur; c.e = cur; c.rapi = rcur->api; c.eapi = cur->api; c.line = ln; c.qual = 'G';
            c.de = rhs; c.para = lhs;
            if ((err = confere_copia(&c))) break;
            c.de = copia(rhs); c.para = copia(lhs);
            copias = cresce(copias, ncopias, sizeof *copias);
            copias[ncopias++] = c;
            continue;
        }
        err = atribui(cur, lhs, rhs, aspas);
    }
    fclose(f);
    if (err) { fprintf(stderr, "[veiculostub] %s, linha %d: %s; nada foi enviado\n", file, ln, err); return -1; }
    return 0;
}

/* ------------------------------------------------------------------ protobuf */
struct buf { unsigned char *p; int n, cap; };

static void poe(struct buf *b, const void *d, int n)
{
    if (b->n + n > b->cap) {
        b->cap = (b->n + n) * 2 + 64;
        b->p = realloc(b->p, (size_t)b->cap);
        if (!b->p) exit(1);
    }
    memcpy(b->p + b->n, d, (size_t)n);
    b->n += n;
}
static void poe_varint(struct buf *b, uint64_t v)
{
    unsigned char x[10];
    int o = 0;
    do { x[o] = v & 0x7f; v >>= 7; if (v) x[o] |= 0x80; o++; } while (v);
    poe(b, x, o);
}
static void poe_etiqueta(struct buf *b, int num, int fio) { poe_varint(b, (uint64_t)num << 3 | (unsigned)fio); }

static int fio_do_tipo(int type)
{
    switch (type) {
    case T_FIXED32: case T_FLOAT: return 5;
    case T_FIXED64: case T_DOUBLE: return 1;
    case T_STRING: case T_BYTES: case T_MSG: return 2;
    default: return 0;
    }
}

/* escreve o valor de um campo escalar, sem etiqueta; devolve 1 se o valor é o zero do tipo */
static int poe_escalar(struct buf *b, const struct field *f, const char *t, const struct tm *tm)
{
    long long v = 0;
    if (!t) t = f->type == T_STRING ? "" : "0";
    if (f->type == T_STRING) ;
    else if (*t == '@' && (!strncmp(t, "@de ", 4) || !strncmp(t, "@se ", 4))) acompanha(t, 1, tm, &v);
    else if (*t == '@') marcador(t, tm, &v);
    else if (f->type == T_BOOL) v = !strcmp(t, "1") || !strcmp(t, "true");
    else if (eh_inteiro(f->type)) inteiro(t, &v);
    switch (f->type) {
    case T_SINT32: case T_SINT64: poe_varint(b, ((uint64_t)v << 1) ^ (uint64_t)(v >> 63)); return v == 0;
    case T_INT32: case T_INT64: case T_ENUM: poe_varint(b, (uint64_t)v); return v == 0;   /* negativo: dez bytes */
    case T_UINT32: poe_varint(b, (uint32_t)v); return v == 0;
    case T_BOOL: poe_varint(b, (uint64_t)v); return v == 0;
    case T_FIXED32: { uint32_t x = (uint32_t)v; poe(b, &x, 4); return v == 0; }
    case T_FIXED64: { uint64_t x = (uint64_t)v; poe(b, &x, 8); return v == 0; }
    case T_FLOAT: { float x = strtof(t, NULL); poe(b, &x, 4); return x == 0; }
    case T_DOUBLE: { double x = strtod(t, NULL); poe(b, &x, 8); return x == 0; }
    case T_STRING: { int n = (int)strlen(t); poe_varint(b, (uint64_t)n); poe(b, t, n); return n == 0; }
    }
    return 1;
}

static struct val *acha_no(struct val *lista, const struct field *f, int idx)
{
    for (; lista; lista = lista->next) if (lista->f == f && lista->idx == idx) return lista;
    return NULL;
}

/* Serializa uma mensagem com os valores do perfil, na ordem dos números de campo, que é como as
 * classes do firmware escrevem. valido: no nível de cima de uma atualização de atributo, o campo
 * validFlag sem valor no perfil vai com 1. */
static void poe_msg(struct buf *b, const struct msg *m, struct val *vals, int valido, const struct tm *tm)
{
    int i, k, max;
    for (i = 0; i < m->nf; i++) {
        const struct field *f = &m->f[i];
        struct val *v;
        struct buf s = { NULL, 0, 0 };
        for (max = -1, v = vals; v; v = v->next) if (v->f == f && v->idx > max) max = v->idx;
        if (max < 0) {
            if (valido && f->label == L_UM && eh_inteiro(f->type) && !strcasecmp(f->name, "validFlag")) {
                poe_etiqueta(b, f->num, fio_do_tipo(f->type));
                poe_escalar(b, f, "1", tm);
            }
            continue;
        }
        if (f->type == T_MSG) {
            for (k = 0; k <= max; k++) {           /* elemento que falta no meio de uma lista vai vazio */
                v = acha_no(vals, f, k);
                s.n = 0;
                poe_msg(&s, f->sub, v ? v->kids : NULL, 0, tm);
                poe_etiqueta(b, f->num, 2); poe_varint(b, (uint64_t)s.n); poe(b, s.p, s.n);
            }
        } else if (f->label == L_PAC) {
            for (k = 0; k <= max; k++) { v = acha_no(vals, f, k); poe_escalar(&s, f, v ? v->txt : NULL, tm); }
            poe_etiqueta(b, f->num, 2); poe_varint(b, (uint64_t)s.n); poe(b, s.p, s.n);
        } else if (f->label == L_REP) {
            for (k = 0; k <= max; k++) {
                v = acha_no(vals, f, k);
                poe_etiqueta(b, f->num, fio_do_tipo(f->type));
                poe_escalar(b, f, v ? v->txt : NULL, tm);
            }
        } else {
            v = acha_no(vals, f, 0);
            if (!poe_escalar(&s, f, v->txt, tm)) { poe_etiqueta(b, f->num, fio_do_tipo(f->type)); poe(b, s.p, s.n); }
        }
        free(s.p);
    }
}

static void monta(struct buf *b, const struct evt *e)
{
    struct tm tm;
    agora(&tm);
    b->n = 0;
    if (e->m) poe_msg(b, e->m, e->vals, e->attr >= 0, &tm);
}

/* ------------------------------------------------------------------ barramento */
static int liga(const char *path, char papel)
{
    struct sockaddr_un a;
    char hs[5];
    int s = socket(AF_UNIX, SOCK_SEQPACKET, 0), pid = getpid();
    if (s < 0) return -1;
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
    hs[0] = papel; hs[1] = (char)pid; hs[2] = (char)(pid >> 8); hs[3] = (char)(pid >> 16); hs[4] = (char)(pid >> 24);
    send(s, hs, 5, 0);
    return s;
}

static void manda(uint64_t canal, uint32_t ifid, uint32_t inst, uint32_t api, const unsigned char *pb, int n)
{
    static unsigned char b[20 + MAXPB];
    memcpy(b, &canal, 8); memcpy(b + 8, &ifid, 4); memcpy(b + 12, &inst, 4); memcpy(b + 16, &api, 4);
    if (n > 0) memcpy(b + 20, pb, (size_t)n);
    send(W, b, (size_t)(20 + (n > 0 ? n : 0)), MSG_NOSIGNAL);
}

/* manda a atualização de uma assinatura, se for a primeira vez (sempre = 1) ou se mudou */
static void publica(struct sub *s, int sempre)
{
    static struct buf b;
    monta(&b, s->e);
    if (b.n > MAXPB) { fprintf(stderr, "[veiculostub] %s.%s: %d bytes, grande demais para o barramento; não mandei\n", s->e->s->name, s->e->name, b.n); return; }
    if (!sempre && b.n == s->nlast && (b.n == 0 || !memcmp(b.p, s->last, (size_t)b.n))) return;
    manda(s->e->s->dom, s->e->s->evt, s->inst, s->e->api, b.p, b.n);
    s->last = realloc(s->last, (size_t)b.n + 1);
    if (b.n) memcpy(s->last, b.p, (size_t)b.n);
    s->nlast = b.n;
}

static int le_varint(const unsigned char *p, int n, int *o, uint64_t *v)
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

/* os atributos de um pedido de assinatura: campo 1, repetido um a um ou empacotado */
static int le_atributos(const unsigned char *p, int n, int zigue, int *out, int max)
{
    int o = 0, k = 0;
    uint64_t tag, v;
    while (o < n && le_varint(p, n, &o, &tag)) {
        int fio = (int)(tag & 7), campo = (int)(tag >> 3);
        if (fio == 0) {
            if (!le_varint(p, n, &o, &v)) break;
            if (campo == 1 && k < max) out[k++] = zigue ? (int)((v >> 1) ^ (uint64_t)-(int64_t)(v & 1)) : (int)v;
        } else if (fio == 2) {
            int fim;
            if (!le_varint(p, n, &o, &v) || v > (uint64_t)(n - o)) break;
            fim = o + (int)v;
            while (campo == 1 && o < fim && k < max && le_varint(p, fim, &o, &v)) out[k++] = zigue ? (int)((v >> 1) ^ (uint64_t)-(int64_t)(v & 1)) : (int)v;
            o = fim;
        } else if (fio == 5) o += 4;
        else if (fio == 1) o += 8;
        else break;
    }
    return k;
}

/* ------------------------------------------------------------------ pedidos */
/* Procura no protobuf p[0..n) o campo num; vale a última ocorrência. Devolve o tipo de fio, ou -1
 * se o campo não veio. O valor sai em *vi (varint e fixos) ou em *v e *vn (trecho com tamanho). */
static int acha_no_fio(const unsigned char *p, int n, int num, uint64_t *vi, const unsigned char **v, int *vn)
{
    int o = 0, achou = -1;
    uint64_t tag, x;
    while (o < n && le_varint(p, n, &o, &tag)) {
        int fio = (int)(tag & 7), campo = (int)(tag >> 3);
        if (fio == 0) {
            if (!le_varint(p, n, &o, &x)) break;
        } else if (fio == 2) {
            if (!le_varint(p, n, &o, &x) || x > (uint64_t)(n - o)) break;
            if (campo == num) { *v = p + o; *vn = (int)x; }
            o += (int)x;
        } else if (fio == 5) {
            uint32_t y;
            if (n - o < 4) break;
            memcpy(&y, p + o, 4); o += 4; x = y;
        } else if (fio == 1) {
            if (n - o < 8) break;
            memcpy(&x, p + o, 8); o += 8;
        } else break;
        if (campo == num) { achou = fio; *vi = x; }
    }
    return achou;
}

/* O valor de uma folha do pedido, como texto de perfil. Campo que não veio dá o zero do tipo, como
 * no protobuf 3. O caminho já foi conferido contra o esquema (confere_copia). */
static void le_folha(struct msg *m, const unsigned char *p, int n, const char *caminho, char *out, size_t max)
{
    char tmp[256], *seg, *save = NULL;
    struct field *f = NULL;
    const unsigned char *v = NULL;
    uint64_t vi = 0;
    int vn = 0, fio = -1;
    snprintf(tmp, sizeof tmp, "%s", caminho);
    for (seg = strtok_r(tmp, ".", &save); seg; seg = strtok_r(NULL, ".", &save)) {
        if (f) {                                   /* desce: o trecho passa a ser o da mensagem de dentro */
            if (fio == 2) { p = v; n = vn; } else n = 0;
            m = f->sub;
        }
        f = acha_campo(m, seg);
        fio = acha_no_fio(p, n, f->num, &vi, &v, &vn);
    }
    switch (f->type) {
    case T_SINT32: case T_SINT64: snprintf(out, max, "%lld", fio == 0 ? (long long)((vi >> 1) ^ (uint64_t)-(int64_t)(vi & 1)) : 0LL); break;
    case T_INT32: case T_ENUM: snprintf(out, max, "%d", fio == 0 ? (int)(int32_t)vi : 0); break;
    case T_INT64: snprintf(out, max, "%lld", fio == 0 ? (long long)vi : 0LL); break;
    case T_UINT32: snprintf(out, max, "%u", fio == 0 ? (unsigned)(uint32_t)vi : 0u); break;
    case T_BOOL: snprintf(out, max, "%d", fio == 0 && vi != 0); break;
    case T_FIXED32: snprintf(out, max, "%u", fio == 5 ? (unsigned)(uint32_t)vi : 0u); break;
    case T_FIXED64: snprintf(out, max, "%llu", fio == 1 ? (unsigned long long)vi : 0ULL); break;
    case T_FLOAT: { float x = 0; uint32_t y = (uint32_t)vi; if (fio == 5) memcpy(&x, &y, 4); snprintf(out, max, "%.9g", (double)x); break; }
    case T_DOUBLE: { double x = 0; if (fio == 1) memcpy(&x, &vi, 8); snprintf(out, max, "%.17g", x); break; }
    case T_STRING: snprintf(out, max, "%.*s", fio == 2 ? vn : 0, fio == 2 ? (const char *)v : ""); break;
    default: *out = 0;
    }
}

static int eh_zero(const struct field *f, const char *v)
{
    if (f->type == T_STRING) return !*v;
    if (f->type == T_FLOAT || f->type == T_DOUBLE) return strtod(v, NULL) == 0;
    return !strcmp(v, "0");
}

/* o nó de um caminho já conferido, na árvore de valores de um evento; NULL se ninguém deu valor */
static struct val *acha_caminho(struct evt *e, const char *caminho)
{
    char tmp[256], *seg, *save = NULL;
    struct msg *m = e->m;
    struct val *lista = e->vals, *v = NULL;
    snprintf(tmp, sizeof tmp, "%s", caminho);
    for (seg = strtok_r(tmp, ".", &save); seg; seg = strtok_r(NULL, ".", &save)) {
        struct field *f;
        if (v) { m = v->f->sub; lista = v->kids; }
        if (!(f = acha_campo(m, seg)) || !(v = acha_no(lista, f, 0))) return NULL;
    }
    return v;
}

static struct val *clona(const struct val *v)
{
    struct val *c;
    if (!v) return NULL;
    if (!(c = calloc(1, sizeof *c))) exit(1);
    c->f = v->f; c->idx = v->idx;
    c->txt = v->txt ? copia(v->txt) : NULL;
    c->kids = clona(v->kids);
    c->next = clona(v->next);
    return c;
}

static void libera(struct val *v)
{
    while (v) {
        struct val *prox = v->next;
        libera(v->kids);
        free(v->txt); free(v);
        v = prox;
    }
}

/* ------------------------------------------------------------------ estado */
/* anota um valor gravado, para o arquivo de estado; devolve 1 se ele é novo ou mudou */
static int lembra(struct evt *e, const char *caminho, const char *txt, int aspas)
{
    int i;
    for (i = 0; i < nguardados; i++)
        if (guardados[i].e == e && !strcmp(guardados[i].caminho, caminho)) {
            if (!strcmp(guardados[i].txt, txt)) return 0;
            free(guardados[i].txt);
            guardados[i].txt = copia(txt);
            return 1;
        }
    guardados = cresce(guardados, nguardados, sizeof *guardados);
    guardados[nguardados].e = e; guardados[nguardados].aspas = aspas;
    guardados[nguardados].caminho = copia(caminho); guardados[nguardados].txt = copia(txt);
    nguardados++;
    return 1;
}

/* Escreve o arquivo de estado: o acerto do relógio e o que os pedidos gravaram, evento por evento.
 * Ao lado e com troca de nome, para o arquivo nunca ficar pela metade. Devolve 0, ou o errno. */
static int guarda_estado(void)
{
    char novo[4096];
    FILE *f;
    int i, k, n = 0, e;
    if (snprintf(novo, sizeof novo, "%s.novo", estado) >= (int)sizeof novo) return ENAMETOOLONG;
    if (!(f = fopen(novo, "w"))) return errno;
    fprintf(f, "# Estado do carro de mentira (veiculostub): o que a tela gravou. O programa escreve este arquivo\n"
               "# de novo a cada mudança, e a partida seguinte o aplica por cima dos perfis. \"acerto\" são os\n"
               "# segundos que o relógio foi adiantado em relação à hora do computador no fuso do perfil.\n"
               "# Para voltar ao zero: apagar o arquivo com a central desligada.\n");
    if (acerto_s) fprintf(f, "acerto = %ld\n", acerto_s);
    for (i = 0; i < nguardados; i++) {
        for (k = 0; k < i && guardados[k].e != guardados[i].e; k++) ;
        if (k < i) continue;                       /* a seção deste evento já saiu, com todos os valores dele */
        fprintf(f, "[%s.%s]\n", guardados[i].e->s->name, guardados[i].e->name);
        for (k = i; k < nguardados; k++) {
            const char *t = guardados[k].txt;
            if (guardados[k].e != guardados[i].e) continue;
            n++;
            if (!guardados[k].aspas) { fprintf(f, "%s = %s\n", guardados[k].caminho, t); continue; }
            fprintf(f, "%s = \"", guardados[k].caminho);          /* texto: numa linha só, com \" \\ e \n */
            for (; *t; t++) {
                if (*t == '\n') { fputs("\\n", f); continue; }
                if (*t == '"' || *t == '\\') fputc('\\', f);
                fputc(*t, f);
            }
            fputs("\"\n", f);
        }
    }
    fprintf(f, "fim = %d\n", n);
    e = ferror(f) ? EIO : 0;
    if (fclose(f) && !e) e = errno;
    if (!e && rename(novo, estado)) e = errno;
    if (e) unlink(novo);
    return e;
}

/* Lê o arquivo de estado e o aplica por cima dos perfis. O arquivo é conferido inteiro antes de
 * qualquer valor ser aplicado: com erro em qualquer linha, ou cortado (sem a linha "fim"), nada
 * muda. Devolve quantos valores aplicou, ou -1 com o arquivo, a linha e o erro em motivo.
 * *descartados conta os valores para campos que o perfil enche com marcador: ali manda o relógio. */
static int le_estado(const char *file, char *motivo, size_t max, int *descartados)
{
    struct { struct evt *e; char *caminho, *txt; int aspas; } *pend = NULL;
    FILE *f = fopen(file, "r");
    char line[2 * 4096 + 600];                     /* um texto de 4095 bytes, com tudo escapado, cabe */
    struct evt *cur = NULL;
    const char *err = NULL;
    long long acerto = 0, v;
    int ln = 0, n = 0, fim = 0, aplicados = 0, i;
    *descartados = 0;
    if (!f) { snprintf(motivo, max, "%s: %s", file, strerror(errno)); return -1; }
    while (!err && fgets(line, sizeof line, f)) {
        char *p, *eq, *lhs, *rhs;
        struct field *campo;
        int q = 0, aspas = 0;
        ln++;
        if (!strchr(line, '\n') && !feof(f)) { err = "linha longa demais"; break; }
        for (p = line; *p; p++) {                  /* '#' fora de aspas começa comentário */
            if (*p == '\\' && q && p[1]) p++;
            else if (*p == '"') q = !q;
            else if (*p == '#' && !q) { *p = 0; break; }
        }
        p = trim(line);
        if (!*p) continue;
        if (fim) { err = "há linha depois do fim"; break; }
        if (*p == '[') {
            char *fecha = strchr(p, ']'), *pt;
            struct svc *s;
            if (!fecha || *trim(fecha + 1)) { err = "seção mal escrita: esperava [Serviço.evento]"; break; }
            *fecha = 0; p = trim(p + 1);
            if (!(pt = strchr(p, '.'))) { err = "seção mal escrita: esperava [Serviço.evento]"; break; }
            *pt++ = 0;
            if (strstr(pt, "->")) { err = "o estado não guarda regra, só valores"; break; }
            if (!(s = acha_svc(p))) { err = "serviço que não está no esquema"; break; }
            for (cur = NULL, i = 0; i < nevts && !cur; i++) if (evts[i].s == s && !strcmp(evts[i].name, pt)) cur = &evts[i];
            if (!cur) err = "evento que não está no esquema deste serviço";
            continue;
        }
        if (!(eq = strchr(p, '='))) { err = "falta o '='"; break; }
        *eq = 0;
        lhs = trim(p); rhs = trim(eq + 1);
        if (*rhs == '"') {                         /* texto: tira as aspas e desfaz \", \\ e \n */
            char *o = rhs, *r = rhs + 1;
            aspas = 1;
            while (*r && *r != '"') {
                if (*r == '\\' && r[1] == 'n') { *o++ = '\n'; r += 2; continue; }
                if (*r == '\\' && (r[1] == '"' || r[1] == '\\')) r++;
                *o++ = *r++;
            }
            if (*r != '"' || *trim(r + 1)) { err = "texto mal fechado: esperava \"…\""; break; }
            *o = 0;
        }
        if (!aspas && !strcmp(lhs, "fim")) {
            if (!inteiro(rhs, &v) || v != n) err = "a linha \"fim\" não bate com a conta dos valores: arquivo cortado ou mexido à mão";
            fim = 1;
            continue;
        }
        if (!cur) {                                /* antes da primeira seção só o acerto do relógio */
            struct tm tm;
            time_t t;
            int fora;
            if (aspas || strcmp(lhs, "acerto")) { err = "fora de seção o estado só tem o acerto"; break; }
            if (!inteiro(rhs, &acerto)) { err = "acerto: esperava os segundos, um número inteiro"; break; }
            fora = acerto < -4200000000LL || acerto > 4200000000LL;        /* mais de 130 anos */
            t = time(NULL) + fuso_s + (fora ? 0 : (time_t)acerto);
            if (fora || !gmtime_r(&t, &tm) || tm.tm_year < 70 || tm.tm_year > 199) err = "acerto: levaria o relógio para fora de 1970 a 2099";
            continue;
        }
        if (!cur->m) { err = "este evento não tem mensagem"; break; }
        if (!(campo = folha(cur->m, lhs, &err))) break;
        if (aspas && campo->type != T_STRING) err = "texto entre aspas num campo que não é de texto";
        else if (!aspas && campo->type == T_STRING) err = "campo de texto: o valor vai entre aspas";
        else if (!aspas && *rhs == '@') err = "o estado não guarda marcador: a hora acertada vai no acerto";
        else if (n >= 20000) err = "valores demais";
        else err = confere_valor(campo, rhs);
        if (err) break;
        pend = cresce(pend, n, sizeof *pend);
        pend[n].e = cur; pend[n].aspas = aspas; pend[n].caminho = copia(lhs); pend[n].txt = copia(rhs);
        n++;
    }
    if (!err && ferror(f)) err = "erro de leitura";
    fclose(f);
    if (!err && !fim) err = "falta a linha \"fim\": o arquivo está vazio ou foi cortado";
    if (err) snprintf(motivo, max, "%s, linha %d: %s", file, ln, err);
    else acerto_s = (long)acerto;
    for (i = 0; i < n; i++) {                      /* sem erro, aplica; daqui em diante nada falha, tudo já foi conferido */
        struct val *lugar = err ? NULL : acha_caminho(pend[i].e, pend[i].caminho);
        if (!err && !pend[i].aspas && lugar && lugar->txt && qual_marcador(lugar->txt) >= 0) (*descartados)++;
        else if (!err) {
            char caminho[256];
            snprintf(caminho, sizeof caminho, "%s", pend[i].caminho);
            atribui(pend[i].e, caminho, pend[i].txt, pend[i].aspas);
            lembra(pend[i].e, pend[i].caminho, pend[i].txt, pend[i].aspas);
            aplicados++;
        }
        free(pend[i].caminho); free(pend[i].txt);
    }
    free(pend);
    return err ? -1 : aplicados;
}

/* Pedido que grava (linhas G do esquema e regras do perfil): cada campo do pedido toma o lugar do
 * valor do perfil no evento, e a atualização sai para quem assina o evento, em toda instância. É
 * a resposta que a HMI espera desses pedidos, e sai mesmo que nada tenha mudado. Campo de destino
 * com marcador (@hora…) não vira número fixo: o pedido acerta o relógio. Se há arquivo de estado,
 * o que mudou (valor ou acerto) é guardado nele logo depois de a atualização sair. */
static void grava_pedido(struct req *r, const unsigned char *pb, int n)
{
    static int sujo;                               /* há mudança que ainda não chegou ao arquivo de estado */
    struct evt *tocados[32];
    long long rel[7];
    long acerto_antes = acerto_s;
    int tem[7] = { 0 }, nt = 0, nrel = 0, avisados = 0, fora = -1, escrito = 0, falha = 0, i, k;
    char oque[700] = "", v[4096], caminho[256];
    const char *err = NULL;
    for (i = 0; i < ncopias; i++) {
        struct copia *c = &copias[i];
        struct val *lugar;
        struct field *f;
        if (c->r != r || c->qual != 'G') continue;
        le_folha(r->m, pb, n, c->de, v, sizeof v);
        f = folha(c->e->m, c->para, &err);
        lugar = acha_caminho(c->e, c->para);
        if (lugar && lugar->txt && (k = qual_marcador(lugar->txt)) >= 0) {
            rel[k] = atoll(v); tem[k] = 1; nrel++;
        } else {
            snprintf(caminho, sizeof caminho, "%s", c->para);
            if ((err = atribui(c->e, caminho, v, f->type == T_STRING)))
                fprintf(stderr, "[veiculostub] %s.%s (api %u): não gravei %s.%s = %.60s: %s\n", r->s->name, r->name, r->api, c->e->name, c->para, v, err);
            else {
                sujo |= lembra(c->e, c->para, v, f->type == T_STRING);
                if (strlen(oque) < sizeof oque - 200)
                    snprintf(oque + strlen(oque), 200, "%s%s.%s = %.60s", *oque ? ", " : "gravei ", c->e->name, c->para, v);
            }
        }
        for (k = 0; k < nt && tocados[k] != c->e; k++) ;
        if (k == nt && nt < 32) tocados[nt++] = c->e;
    }
    if (nrel) {
        size_t o = strlen(oque);
        if ((fora = acerta_relogio(rel, tem)) >= 0)
            snprintf(oque + o, sizeof oque - o, "%s%s %lld fora do intervalo: o relógio não mudou", o ? "; " : "", RELNOME[fora], rel[fora]);
        else {
            struct tm tm;
            agora(&tm);
            snprintf(oque + o, sizeof oque - o, "%sacertei o relógio para %02d:%02d:%02d de %02d/%02d/%04d", o ? "; " : "",
                     tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
        }
    }
    for (i = 0; i < nsubs; i++) {
        for (k = 0; k < nt && tocados[k] != subs[i].e; k++) ;
        if (k < nt) { publica(&subs[i], 1); avisados++; }
        else if (nrel && fora < 0 && subs[i].e->dyn) publica(&subs[i], 0);     /* o resto do relógio acompanha */
    }
    if (acerto_s != acerto_antes) sujo = 1;
    if (estado && sujo && !(falha = guarda_estado())) { sujo = 0; escrito = 1; }
    fprintf(stderr, "[veiculostub] pedido atendido: %s.%s (api %u): %s; avisei %d assinatura(s)%s\n", r->s->name, r->name, r->api, *oque ? oque : "nada a gravar", avisados,
            escrito ? "; estado guardado" : "");
    if (falha) fprintf(stderr, "[veiculostub] não consegui guardar o estado em %s: %s; o que a tela gravou vale até a central desligar\n", estado, strerror(falha));
}

/* Pedido com resposta própria (linha R do tipo "uma"): manda o evento da resposta na instância do
 * pedido. Valem os valores do perfil e, onde ele não diz nada, o que as linhas X mandam devolver
 * do pedido. Nada disso fica guardado: a resposta seguinte parte outra vez do perfil. */
static void responde(struct req *r, struct resp *rp, uint32_t inst, const unsigned char *pb, int n)
{
    struct evt tmp = *rp->e;
    struct buf b = { NULL, 0, 0 };
    struct tm tm;
    char v[4096], caminho[256];
    const char *err = NULL;
    int i;
    tmp.vals = clona(rp->e->vals);
    for (i = 0; i < ncopias; i++) {
        struct copia *c = &copias[i];
        struct val *lugar;
        struct field *f;
        if (c->r != r || c->e != rp->e || c->qual != 'X') continue;
        lugar = acha_caminho(rp->e, c->para);
        if (lugar && lugar->txt) continue;         /* o perfil disse: vale o perfil */
        le_folha(r->m, pb, n, c->de, v, sizeof v);
        f = folha(tmp.m, c->para, &err);
        if (eh_zero(f, v)) continue;               /* zero não vai no fio: não há o que devolver */
        snprintf(caminho, sizeof caminho, "%s", c->para);
        atribui(&tmp, caminho, v, f->type == T_STRING);
    }
    agora(&tm);
    if (tmp.m) poe_msg(&b, tmp.m, tmp.vals, 0, &tm);
    if (b.n > MAXPB) fprintf(stderr, "[veiculostub] %s.%s: a resposta %s tem %d bytes, grande demais para o barramento; não mandei\n", r->s->name, r->name, rp->e->name, b.n);
    else {
        manda(r->s->dom, r->s->evt, inst, rp->e->api, b.p, b.n);
        fprintf(stderr, "[veiculostub] pedido atendido: %s.%s (api %u): respondi %s (api %u, %d bytes)\n", r->s->name, r->name, r->api, rp->e->name, rp->e->api, b.n);
    }
    libera(tmp.vals);
    free(b.p);
}

static void trata_pedido(struct svc *s, uint32_t inst, uint32_t api, const unsigned char *pb, int n)
{
    static int registrados;
    struct req *r = NULL;
    int i, k;
    for (i = 0; i < nreqs && !r; i++) if (reqs[i].s == s && reqs[i].api == api) r = &reqs[i];
    if (api == API_ASSINA || api == API_DESASSINA) {
        int at[512], na, zigue = 1, mandei = 0, doperfil = 0;
        char lista[400] = "", semev[200] = "";
        if (r && r->m && r->m->nf > 0 && r->m->f[0].num == 1 && r->m->f[0].type != T_SINT32) zigue = 0;
        na = le_atributos(pb, n, zigue, at, 512);
        for (k = 0; k < na; k++) {
            struct evt *e = NULL;
            struct sub *sb = NULL;
            if (strlen(lista) < sizeof lista - 16) sprintf(lista + strlen(lista), " %d", at[k]);
            for (i = 0; i < nevts && !e; i++) if (evts[i].s == s && evts[i].attr == at[k]) e = &evts[i];
            if (!e) { if (strlen(semev) < sizeof semev - 16) sprintf(semev + strlen(semev), " %d", at[k]); continue; }
            e = da_instancia(e, inst, 0);              /* a seção [Serviço[N].evento] do perfil, se houver */
            for (i = 0; i < nsubs && !sb; i++) if (subs[i].e == e && subs[i].inst == inst) sb = &subs[i];
            if (api == API_DESASSINA) { if (sb) { free(sb->last); *sb = subs[--nsubs]; } continue; }
            if (!sb) {
                subs = cresce(subs, nsubs, sizeof *subs);
                sb = &subs[nsubs++];
                sb->e = e; sb->inst = inst; sb->last = NULL; sb->nlast = 0;
            }
            publica(sb, 1);
            mandei++; if (e->nvals) doperfil++;
        }
        if (api == API_ASSINA) fprintf(stderr, "[veiculostub] %s: assinou%s; mandei %d (%d com valores do perfil)\n", s->name, lista, mandei, doperfil);
        else fprintf(stderr, "[veiculostub] %s: desassinou%s\n", s->name, lista);
        if (*semev) fprintf(stderr, "[veiculostub] %s: atributo%s sem evento no esquema; nada a mandar\n", s->name, semev);
        return;
    }
    if (r) {
        struct resp *rp = NULL;
        struct evt *alvo = NULL;                   /* o primeiro evento que o pedido gravaria */
        char motivo[400];
        int fez = 0;
        for (i = 0; i < nresps && !rp; i++) if (resps[i].r == r) rp = &resps[i];
        for (i = 0; i < ncopias && !alvo; i++) if (copias[i].r == r && copias[i].qual == 'G') alvo = copias[i].e;
        if (!r->ignora && !r->cortado) {
            time_t now = time(NULL);
            int atende = (alvo && s->grava) || (rp && rp->tipo == K_UMA && rp->e && rp->e->secao);
#ifdef VEICULOSTUB_MODELO
            int domodelo = modelo_trata(r);
            atende = atende || domodelo;
#endif
            if (atende) {                              /* a defesa contra laço: ver o cabeçalho */
                if (now - r->desde >= LACO_S) { r->desde = now; r->vezes = 0; }
                if (++r->vezes > LACO_MAX) {
                    r->cortado = 1;
                    fprintf(stderr, "[veiculostub] %s.%s (api %u): atendido mais de %d vezes em %d s: a HMI e o carro de mentira estão se respondendo em laço; deixei de atender este pedido\n",
                            s->name, r->name, api, LACO_MAX, LACO_S);
                }
            }
            if (!r->cortado) {
#ifdef VEICULOSTUB_MODELO
                if (domodelo) { modelo_pedido(r, inst, pb, n); return; }
#endif
                if (alvo && s->grava) { grava_pedido(r, pb, n); fez = 1; }
                if (rp && rp->tipo == K_UMA && rp->e && rp->e->secao) { responde(r, rp, inst, pb, n); fez = 1; }
            }
        }
        if (fez) return;
        if (r->ignora) snprintf(motivo, sizeof motivo, "o perfil manda ignorar este pedido");
        else if (r->cortado) snprintf(motivo, sizeof motivo, "deixou de ser atendido por laço com a HMI");
        else if (rp && rp->tipo != K_UMA) snprintf(motivo, sizeof motivo, "a HMI espera uma resposta do tipo %s, que este programa não sabe montar", KNOME[rp->tipo]);
        else if (rp && !rp->e) snprintf(motivo, sizeof motivo, "a HMI espera %s, que a camada da LG não entrega por api nenhuma", rp->metodo);
        else if (rp && rp->prazo > 0) snprintf(motivo, sizeof motivo, "a HMI espera %s em até %ld ms; para responder, o perfil precisa da seção [%s.%s]", rp->metodo, rp->prazo, s->name, rp->e->name);
        else if (rp) snprintf(motivo, sizeof motivo, "a HMI espera %s, sem prazo; para responder, o perfil precisa da seção [%s.%s]", rp->metodo, s->name, rp->e->name);
        else if (alvo) snprintf(motivo, sizeof motivo, "gravaria %s, mas o serviço não está em \"grava\" no perfil", alvo->name);
        else if (!nresps) snprintf(motivo, sizeof motivo, "o esquema não diz se a HMI espera resposta");      /* esquema de antes do Marco 9 */
        else snprintf(motivo, sizeof motivo, "a HMI não marca prazo de resposta para ele");
        if (registrados++ < 400) fprintf(stderr, "[veiculostub] pedido sem resposta: %s.%s (api %u, %d bytes): %s\n", s->name, r->name, api, n, motivo);
        return;
    }
    if (registrados++ < 400) fprintf(stderr, "[veiculostub] pedido sem resposta: %s, api %u, que não está no esquema (%d bytes)\n", s->name, api, n);
}

int main(int argc, char **argv)
{
    const char *sock = NULL, *modo = "";
    char resumo_estado[900] = "";                  /* o que foi feito do arquivo de estado, para o fim do resumo */
    int arg = 1, i, k, comvalor = 0, nvals = 0, pedido_visto = 0, recusado = 0;
    time_t last = 0, t_hmi = 0, t_anuncio = 0;
    if (argc > 2 && !strcmp(argv[1], "-e")) { estado = argv[2]; arg = 3; }
    if (argc > arg && (!strcmp(argv[arg], "-n") || !strcmp(argv[arg], "-m"))) modo = argv[arg++];
    else if (argc > arg) sock = argv[arg++];
    if (argc - arg < 1) {
        fprintf(stderr, "uso: veiculostub [-e <estado>] <socket> <esquema> [<perfil>...]\n     veiculostub [-e <estado>] -n|-m <esquema> [<perfil>...]   (-n só confere; -m mostra o que seria mandado)\n");
        return 2;
    }
    if (le_esquema(argv[arg++])) return 2;
    for (; arg < argc; arg++) if (le_perfil(argv[arg])) return 2;
#ifdef VEICULOSTUB_MODELO
    if (modelo_inicia()) return 2;
#endif
    for (i = 0; i < nevts; i++) if (evts[i].nvals) { comvalor++; nvals += evts[i].nvals; }
    for (i = 0; i < nporinsts; i++) if (porinsts[i]->e.nvals) { comvalor++; nvals += porinsts[i]->e.nvals; }
    if (estado) {                                  /* por cima dos perfis, o que a tela gravou nas sessões de antes */
        char motivo[600], mais[200] = "";
        int desc = 0, n;
        if (access(estado, F_OK)) snprintf(resumo_estado, sizeof resumo_estado, "; estado: nada guardado ainda em %s", estado);
        else if ((n = le_estado(estado, motivo, sizeof motivo, &desc)) < 0) {
            recusado = 1;
            snprintf(resumo_estado, sizeof resumo_estado, "; estado: RECUSADO (%s); vale só o perfil", motivo);
        } else {
            if (acerto_s) snprintf(mais, sizeof mais, " e acerto de %+ld s no relógio", acerto_s);
            if (desc) snprintf(mais + strlen(mais), sizeof mais - strlen(mais), " (%d descartado(s): o perfil põe o relógio nesses campos)", desc);
            snprintf(resumo_estado, sizeof resumo_estado, "; estado: %d valor(es)%s, de %s", n, mais, estado);
        }
    }
    {
        char lista[600] = "", fora[600] = "";
        int comresp = 0, gravam = 0;
        for (i = 0; i < nreqs; i++) {
            for (k = 0; k < nresps && resps[k].r != &reqs[i]; k++) ;
            if (k < nresps) comresp++;
            for (k = 0; k < ncopias && !(copias[k].r == &reqs[i] && copias[k].qual == 'G'); k++) ;
            if (k < ncopias) gravam++;
            if (reqs[i].ignora && strlen(fora) + strlen(reqs[i].s->name) + strlen(reqs[i].name) + 3 < sizeof fora)
                sprintf(fora + strlen(fora), "%s%s.%s", *fora ? " " : "", reqs[i].s->name, reqs[i].name);
        }
        for (i = 0; i < nsvcs; i++)
            if (svcs[i]->grava && strlen(lista) + strlen(svcs[i]->name) + 2 < sizeof lista) { strcat(lista, *lista ? " " : ""); strcat(lista, svcs[i]->name); }
        fprintf(stderr, "[veiculostub] esquema: %d serviços, %d atributos, %d eventos, %d pedidos (%d com resposta, %d que gravam), %d mensagens; perfil: %d valores em %d eventos; fuso %+g h; grava: %s; ignora: %s%s\n",
                nsvcs, nattrs, nevts, nreqs, comresp, gravam, nmsgs, nvals, comvalor, fuso_s / 3600.0, *lista ? lista : "nenhum serviço", *fora ? fora : "nenhum pedido", resumo_estado);
    }
    if (!strcmp(modo, "-n")) return 0;
    if (!strcmp(modo, "-m")) {
        struct buf b = { NULL, 0, 0 };
        for (i = 0; i < nevts; i++) {
            if (evts[i].attr < 0 && !evts[i].nvals) continue;
            monta(&b, &evts[i]);
            printf("canal=0x%llx if=%u inst=0 api=%u pb=", (unsigned long long)evts[i].s->dom, evts[i].s->evt, evts[i].api);
            for (k = 0; k < b.n; k++) printf(k + 1 < b.n ? "%02x " : "%02x", b.p[k]);
            printf("\n");
        }
        for (i = 0; i < nporinsts; i++) {              /* as seções por instância, à parte, dizendo de qual */
            struct evt *e = &porinsts[i]->e;
            monta(&b, e);
            printf("canal=0x%llx if=%u inst=%u api=%u pb=", (unsigned long long)e->s->dom, e->s->evt, porinsts[i]->inst, e->api);
            for (k = 0; k < b.n; k++) printf(k + 1 < b.n ? "%02x " : "%02x", b.p[k]);
            printf(" (só a instância %u)\n", porinsts[i]->inst);
        }
        return 0;
    }

    if (recusado) {                                /* o arquivo com erro fica de lado, para poder ser olhado */
        char lado[4096];
        snprintf(lado, sizeof lado, "%s.recusado", estado);
        if (!rename(estado, lado)) fprintf(stderr, "[veiculostub] o estado recusado ficou em %s; o próximo pedido que gravar começa um arquivo novo\n", lado);
        else fprintf(stderr, "[veiculostub] não consegui pôr o estado recusado de lado (%s); o próximo pedido que gravar escreve por cima dele\n", strerror(errno));
    }
    W = liga(sock, 'W');
    R = liga(sock, 'R');
    if (W < 0 || R < 0) { fprintf(stderr, "[veiculostub] sem hub em %s\n", sock); return 1; }
    fprintf(stderr, "[veiculostub] ligado ao barramento\n");
    for (;;) {
        struct pollfd p = { R, POLLIN, 0 };
        time_t now = time(NULL);
        /* o anúncio: ao subir e, se a HMI apareceu e não pediu nada, de 2 em 2 s (ver o cabeçalho) */
        if (!t_anuncio || (t_hmi && !pedido_visto && now - t_hmi >= 2 && now - t_anuncio >= 2)) {
            for (i = 0; i < nsvcs; i++) {
                for (k = 0; k < i && svcs[k]->dom != svcs[i]->dom; k++) ;
                if (k == i) manda(svcs[i]->dom, IF_WELCOME, 0, 0, NULL, 0);
            }
            if (t_anuncio) fprintf(stderr, "[veiculostub] a HMI está no barramento e não pediu nada: anunciei de novo\n");
            t_anuncio = now;
        }
        if (now != last) {                         /* uma vez por segundo: o que muda com o tempo */
            last = now;
            for (i = 0; i < nsubs; i++) if (subs[i].e->dyn) publica(&subs[i], 0);
#ifdef VEICULOSTUB_MODELO_TEMPO
            modelo_tempo(now);
#endif
        }
        if (poll(&p, 1, 50) > 0 && (p.revents & (POLLIN | POLLHUP))) {
            static unsigned char in[0x6900];
            ssize_t n = recv(R, in, sizeof in, 0);
            uint64_t canal;
            uint32_t ifid, inst, api;
            if (n <= 0) break;                     /* o hub saiu */
            if (n < 20) continue;
            memcpy(&canal, in, 8); memcpy(&ifid, in + 8, 4); memcpy(&inst, in + 12, 4); memcpy(&api, in + 16, 4);
            if (canal == PID_HMI && !t_hmi) t_hmi = now;
            /* Pedido é só o que a HMI manda (12/10/2026). O número da interface não basta: a de
             * pedidos de um serviço pode ser a de EVENTOS de outro, de outro domínio. A 1101 é a de
             * pedidos do DSIMessaging (telefone) e a de eventos do DSIPowerManagement, que o dsistub
             * publica de segundo em segundo: sem este filtro cada uma delas virava um "pedido sem
             * resposta", e a primeira já contava como "a HMI pediu", desligando o anúncio de reserva. */
            if (canal != PID_HMI) continue;
            for (i = 0; i < nsvcs; i++)
                if (svcs[i]->req == ifid) {
                    if (!pedido_visto) fprintf(stderr, "[veiculostub] primeiro pedido da HMI: os serviços estão ligados\n");
                    pedido_visto = 1;
                    trata_pedido(svcs[i], inst, api, in + 20, (int)n - 20);
                    break;
                }
        }
    }
    return 0;
}
