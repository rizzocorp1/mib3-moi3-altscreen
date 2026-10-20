/* midiastub.c — a "mídia de mentira" do sandbox: faz o papel do tocador de mídia diante da HMI.
 * Marco 15, 12/10/2026. Estudo num computador, sem carro, sem aparelho e sem som; nada daqui vai
 * para o carro.
 *
 * No carro quem fornece os serviços DSIMediaBase, DSIMediaPlayer e DSIMediaBrowser é o programa de
 * mídia do firmware (domínio PID_MEDIA, canal 0x800000), que lê pendrive, Bluetooth e o resto. Na
 * bancada não há nada disso, então este programa ocupa o lugar dele.
 *
 * É o veiculostub com outro esquema, mais um modelo. O motor é o mesmo arquivo, incluído aqui: lê
 * o esquema (stubs/midia/gerado/esquema.txt, tirado das classes do firmware) e os perfis
 * (stubs/midia/), anuncia o domínio uma vez, responde a cada assinatura com o valor do perfil ou
 * com o padrão marcado como válido, e atende os pedidos que o perfil liga. O formato do perfil, o
 * barramento e o modo de uso (-n confere, -m mostra) estão no cabeçalho do veiculostub.c.
 *
 * O perfil diz o que há para tocar, usando os nomes do esquema:
 *   [DSIMediaBase.MBLupdateDeviceList] e [DSIMediaBase.MBLupdateMediaList]   a fonte e a mídia dela
 *   [DSIMediaPlayer.MPLupdateCapabilities], […PlaybackModeList] e […PlaybackMode]   o que o tocador sabe
 *   [DSIMediaBrowser.MBRLupdateBrowseFolder]   a pasta (há uma só)
 *   [DSIMediaBrowser.MBRLresponseList]         as faixas dessa pasta: é a discoteca do modelo
 *
 * O que o motor não sabe fazer, e o modelo faz (lido em de.vw.mib.asl.internal.media e medido nas
 * rodadas m1 a m5 e p1 a p4 de 12/10/2026):
 *
 *   DSIMediaPlayer
 *   setActiveMedia(aparelho, mídia, grupo)     a HMI ativa a fonte (CmdActivatePlayer)
 *     Ela só dá a ativação por feita quando, DEPOIS da mídia ativa, chegam as capacidades, os
 *     modos e um estado de reprodução "ativado" (MediaPlayer.detectActivationCompleted). Por
 *     isso saem, nesta ordem: a mídia ativa, as capacidades, a lista de modos, o modo, a posição
 *     (a faixa de agora, no tempo zero) e o estado "em pausa". Sem campos (0, 0) é a HMI
 *     saindo da mídia: sai só a mídia ativa vazia. Mídia que não está no perfil não é ativada.
 *   setPlaybackMode(modo)                      modo da lista do perfil: fica valendo. Negativo
 *     ("assumir", "alternar") ou fora da lista: nada muda. Nos dois casos sai o modo de agora.
 *   play, resume, pause, stop                  mudam o estado (3, 3, 5 e 4). A faixa não anda
 *     sozinha: não há som nem relógio de faixa, e o tempo fica onde está.
 *   skip(modo, quantas)                        0 = próxima, 1 = anterior, 2 = começo da faixa;
 *     do fim volta ao começo. Saem a posição e o estado.
 *   setEntry(entrada, tempo)                   vai para a faixa, no tempo pedido.
 *   setPlaySelection(navegador, entrada, …)    o toque numa faixa da lista: sai a resposta
 *     (resultado 0) e depois a posição e o estado.
 *   requestDetailInfo(entrada)                 os dados da faixa: título, artista, álbum, número.
 *   requestPlayView(entrada, índice, quantas, cliente)   um trecho da lista de reprodução.
 *
 *   DSIMediaBrowser (cada instância recebe só o que pediu; a 0 é a da tela, a 3 a do painel)
 *   setContentFilter(máscara)                  a HMI só segue quando, depois do filtro, chegam a
 *     pasta atual e um tamanho de lista maior que zero (AbstractMediaBrowser.
 *     detectContentFilterSet). Saem os três, nessa ordem.
 *   changeFolder(caminho)                      só há uma pasta: saem ela e o tamanho da lista.
 *   requestBrowsePath(…)                       o caminho da pasta do perfil.
 *   requestList(entrada, tipo, índice, quantas)   um trecho da lista. Com entrada diferente de
 *     zero é a HMI procurando a entrada em foco: volta essa entrada, com o índice dela.
 *   resetSelection, undoLastSelection, addSelection   a "seleção" que a HMI monta antes de tocar
 *     uma faixa da lista: resultado 0; ao selecionar, o estado "tudo" e a conta das faixas.
 *
 * Fica de fora o DSISearch (a busca). O esquema do domínio o traz, mas cada instância dele pertence
 * a um domínio diferente (a camada da LG manda as instâncias 2, 6 e 7 para cá e as outras para o
 * Bluetooth e a navegação), e o hub da bancada entrega tudo a todos: respondendo, este programa
 * falaria pelas instâncias dos outros. Aqui ele não recebe pedido nenhum.
 *
 * O modelo acha serviço, eventos, pedidos e campos pelo NOME, no esquema; não escreve byte de
 * mensagem à mão. Se o esquema não for o de mídia, ou se o perfil não tiver fonte ou faixa, recusa
 * tudo e diz o que faltou. Os números de estado, de modo de salto e de seleção são constantes de
 * org.dsi.ifc.media (PLAYBACKSTATE_, SKIP_ e SELECTIONSTATE_); estão em
 * stubs/midia/gerado/constantes.txt.
 *
 * uso: midiastub <socket> <esquema> [<perfil>...]     fica no barramento
 *      midiastub -n <esquema> [<perfil>...]           só confere os arquivos (0 = tudo certo)
 *      midiastub -m <esquema> [<perfil>...]           mostra o que seria mandado
 */
#define VEICULOSTUB_MODELO
#include <stdarg.h>
#include "veiculostub.c"

enum { EM_REPRODUCAO = 3, PARADO = 4, EM_PAUSA = 5 };          /* PLAYBACKSTATE_PLAYING, _STOPPED e _PAUSED */

static struct evt *eAparelhos, *eMidias;
static struct evt *eAtiva, *eCapac, *eModos, *eModo, *ePosicao, *eEstado, *eDetalhe, *eSelToc, *eVista;
static struct evt *eFiltro, *ePasta, *eTamanho, *eLista, *eSelNav, *eCaminho;
static struct req *rAtiva, *rModo, *rToca, *rRetoma, *rPausa, *rPara, *rPula, *rEntrada, *rDetalhe, *rSelToc, *rVista;
static struct req *rFiltro, *rMudaPasta, *rLista, *rZera, *rDesfaz, *rSeleciona, *rCaminho;
static struct field *fAparelhos, *fMidias, *fModosLista, *fFaixas, *fPasta, *fCaminho, *fVista;
static int atual;                 /* a faixa de agora: índice na lista do perfil */
static int estado_toc;            /* o estado de reprodução de agora; 0 enquanto a HMI não ativa a fonte */
static long long tempo_ms;        /* o tempo dentro da faixa; só muda a pedido */

static const char *texto(struct val *campos, const char *nome)
{
    for (; campos; campos = campos->next) if (!strcmp(campos->f->name, nome)) return campos->txt ? campos->txt : "";
    return "";
}
static struct val *filhos(struct val *campos, const char *nome)
{
    for (; campos; campos = campos->next) if (!strcmp(campos->f->name, nome)) return campos->kids;
    return NULL;
}
static long long numero(struct val *campos, const char *nome, long long padrao)
{
    long long v;
    const char *t = texto(campos, nome);
    return *t && inteiro(t, &v) ? v : padrao;
}
/* quantos itens tem uma lista do perfil, e os campos do item i */
static int quantos(struct evt *e, struct field *f)
{
    struct val *v;
    int max = -1;
    for (v = e->vals; v; v = v->next) if (v->f == f && v->idx > max) max = v->idx;
    return max + 1;
}
static struct val *item(struct evt *e, struct field *f, int i) { struct val *v = acha_no(e->vals, f, i); return v ? v->kids : NULL; }
static int faixas(void) { return quantos(eLista, fFaixas); }
static struct val *faixa(int i) { return item(eLista, fFaixas, i); }
static int indice_da_faixa(long long id)
{
    int i;
    for (i = 0; i < faixas(); i++) if (numero(faixa(i), "entryID", -1) == id) return i;
    return -1;
}
static int midia_do_perfil(long long aparelho, long long midia)
{
    int i;
    for (i = 0; i < quantos(eMidias, fMidias); i++)
        if (numero(item(eMidias, fMidias, i), "deviceID", 0) == aparelho && numero(item(eMidias, fMidias, i), "mediaID", 0) == midia) return 1;
    return 0;
}
static int modo_da_lista(long long modo)
{
    int i;
    for (i = 0; i < quantos(eModos, fModosLista); i++) if (numero(item(eModos, fModosLista, i), "modeID", 0) == modo) return 1;
    return 0;
}

/* dá valor a um campo de um evento, pelo caminho */
static void poe_num(struct evt *e, const char *caminho, long long v)
{
    char c[96], t[32];
    snprintf(c, sizeof c, "%s", caminho); snprintf(t, sizeof t, "%lld", v);
    atribui(e, c, t, 0);
}
static void poe_txt(struct evt *e, const char *caminho, const char *v)
{
    char c[96], t[1024];
    if (!*v) return;                                 /* texto que o perfil não deu não vai na mensagem */
    snprintf(c, sizeof c, "%s", caminho); snprintf(t, sizeof t, "%s", v);
    atribui(e, c, t, 1);
}
static long long pedido_num(struct req *r, const unsigned char *pb, int n, const char *campo)
{
    char v[64];
    le_folha(r->m, pb, n, campo, v, sizeof v);
    return atoll(v);
}

/* manda a atualização de um atributo a quem o assina: em toda instância, ou só na que pediu */
static int avisa(struct evt *e, uint32_t inst, int todas)
{
    int i, k = 0;
    for (i = 0; i < nsubs; i++) if (subs[i].e == e && (todas || subs[i].inst == inst)) { publica(&subs[i], 1); k++; }
    return k;
}
/* uma resposta: começa vazia, ganha os campos e sai na instância do pedido */
static struct evt resposta(struct evt *e) { struct evt tmp = *e; tmp.vals = NULL; tmp.nvals = 0; return tmp; }
static void envia(struct evt *tmp, struct evt *e, uint32_t inst)
{
    struct buf b = { NULL, 0, 0 };
    struct tm tm;
    agora(&tm);
    poe_msg(&b, tmp->m, tmp->vals, 0, &tm);
    manda(e->s->dom, e->s->evt, inst, e->api, b.p, b.n);
    libera(tmp->vals);
    free(b.p);
}
/* copia para a resposta um trecho da lista de faixas, a partir de de, no campo de lista f */
static int poe_trecho(struct evt *tmp, struct field *f, long long de, long long quantas)
{
    int k, postas = 0;
    for (k = (int)de; de >= 0 && k < faixas() && k < de + quantas; k++) {
        struct val *novo = no(&tmp->vals, f, k - (int)de);
        novo->kids = clona(faixa(k));
        postas++;
    }
    return postas;
}

static void publica_posicao(void)
{
    struct val *f = faixa(atual);
    poe_num(ePosicao, "entryID", numero(f, "entryID", 0));
    poe_num(ePosicao, "timePosition", tempo_ms);
    poe_num(ePosicao, "totalPlayTime", numero(f, "secLength", 0) * 1000);     /* a lista dá segundos; o tocador, milissegundos */
    avisa(ePosicao, 0, 1);
}
static void publica_estado(int novo)
{
    estado_toc = novo;
    poe_num(eEstado, "playbackState", novo);
    avisa(eEstado, 0, 1);
}
static void vai_para(int i, long long tempo) { atual = i; tempo_ms = tempo; publica_posicao(); publica_estado(estado_toc); }

#define ATENDIDO(r, ...) do { fprintf(stderr, "[midiastub] pedido atendido: %s.%s (api %u): ", (r)->s->name, (r)->name, (r)->api); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)
#define SEM_RESPOSTA(r, ...) do { fprintf(stderr, "[midiastub] pedido sem resposta: %s.%s (api %u): ", (r)->s->name, (r)->name, (r)->api); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

static void tocador(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    if (r == rAtiva) {
        long long aparelho = pedido_num(r, pb, n, "deviceID"), midia = pedido_num(r, pb, n, "mediaID");
        int vazia = !aparelho && !midia, conhecida = midia_do_perfil(aparelho, midia);
        poe_num(eAtiva, "deviceID", conhecida ? aparelho : 0);
        poe_num(eAtiva, "mediaID", conhecida ? midia : 0);
        poe_num(eAtiva, "groupID", 0);
        avisa(eAtiva, inst, 1);
        if (!conhecida) {
            if (vazia) ATENDIDO(r, "a HMI saiu da mídia: mandei a mídia ativa vazia");
            else ATENDIDO(r, "a mídia %lld do aparelho %lld não está na lista de mídias do perfil: mandei a mídia ativa vazia", midia, aparelho);
            return;
        }
        avisa(eCapac, inst, 1); avisa(eModos, inst, 1); avisa(eModo, inst, 1);
        publica_posicao();
        publica_estado(EM_PAUSA);
        ATENDIDO(r, "ativei a mídia %lld do aparelho %lld: mandei capacidades, modos, a faixa %d de %d (\"%s\") e o estado em pausa", midia, aparelho, atual + 1, faixas(), texto(faixa(atual), "title"));
    } else if (r == rModo) {
        long long modo = pedido_num(r, pb, n, "modeID");
        int serve = modo >= 0 && modo_da_lista(modo);
        if (serve) poe_num(eModo, "playbackMode", modo);
        avisa(eModo, inst, 1);
        ATENDIDO(r, "modo %lld: %s", modo, serve ? "gravei" : modo < 0 ? "modo especial (assumir ou alternar), repeti o modo de agora" : "não está na lista de modos do perfil, repeti o modo de agora");
    } else if (r == rToca || r == rRetoma || r == rPausa || r == rPara) {
        publica_estado(r == rPausa ? EM_PAUSA : r == rPara ? PARADO : EM_REPRODUCAO);
        ATENDIDO(r, "estado %d, na faixa %d (\"%s\"); a faixa não anda sozinha", estado_toc, atual + 1, texto(faixa(atual), "title"));
    } else if (r == rPula) {
        long long modo = pedido_num(r, pb, n, "skipMode"), conta = pedido_num(r, pb, n, "count");
        int nt = faixas(), de = atual;
        if (modo == 0 || modo == 1) vai_para((int)(((atual + (modo ? -conta : conta)) % nt + nt) % nt), 0);
        else if (modo == 2) vai_para(atual, 0);
        else { SEM_RESPOSTA(r, "modo de salto %lld (pasta ou capítulo), que o modelo não tem", modo); return; }
        ATENDIDO(r, "%s, %lld: da faixa %d para a %d (\"%s\")", modo == 0 ? "próxima" : modo == 1 ? "anterior" : "começo da faixa", conta, de + 1, atual + 1, texto(faixa(atual), "title"));
    } else if (r == rEntrada) {
        long long id = pedido_num(r, pb, n, "entryID"), tempo = pedido_num(r, pb, n, "timePosition");
        int i = indice_da_faixa(id);
        if (i < 0) { SEM_RESPOSTA(r, "a entrada %lld não está na lista de faixas do perfil", id); return; }
        vai_para(i, tempo);
        ATENDIDO(r, "fui para a faixa %d (\"%s\"), no tempo %lld ms", atual + 1, texto(faixa(atual), "title"), tempo);
    } else if (r == rSelToc) {
        long long nav = pedido_num(r, pb, n, "browserID"), id = pedido_num(r, pb, n, "entryID");
        struct evt tmp = resposta(eSelToc);
        int i = indice_da_faixa(id);
        poe_num(&tmp, "browserID", nav);
        poe_num(&tmp, "selectionResult", 0);
        envia(&tmp, eSelToc, inst);
        vai_para(i < 0 ? atual : i, 0);
        ATENDIDO(r, "seleção do navegador %lld a partir da entrada %lld: resultado 0, faixa %d (\"%s\")", nav, id, atual + 1, texto(faixa(atual), "title"));
    } else if (r == rDetalhe) {
        long long id = pedido_num(r, pb, n, "entryID");
        struct evt tmp = resposta(eDetalhe);
        int i = indice_da_faixa(id);
        struct val *f = faixa(i);
        if (i < 0) { SEM_RESPOSTA(r, "a entrada %lld não está na lista de faixas do perfil", id); return; }
        poe_txt(&tmp, "entryinfo.album", texto(filhos(f, "extInfo"), "album"));
        poe_txt(&tmp, "entryinfo.artist", texto(filhos(f, "extInfo"), "artist"));
        poe_num(&tmp, "entryinfo.contentType", numero(f, "contentType", 0));
        poe_num(&tmp, "entryinfo.entryID", id);
        poe_txt(&tmp, "entryinfo.filename", texto(f, "filename"));
        poe_num(&tmp, "entryinfo.numTracks", faixas());
        poe_txt(&tmp, "entryinfo.title", texto(f, "title"));
        poe_num(&tmp, "entryinfo.trackNo", i + 1);
        envia(&tmp, eDetalhe, inst);
        ATENDIDO(r, "mandei os dados da faixa %lld (\"%s\")", id, texto(f, "title"));
    } else {                                         /* rVista */
        long long de = pedido_num(r, pb, n, "index"), quantas = pedido_num(r, pb, n, "count");
        struct evt tmp = resposta(eVista);
        int postas = poe_trecho(&tmp, fVista, de, quantas);
        poe_num(&tmp, "index", de);
        poe_num(&tmp, "clientID", pedido_num(r, pb, n, "clientID"));
        envia(&tmp, eVista, inst);
        ATENDIDO(r, "lista de reprodução a partir de %lld: mandei %d de %d faixa(s)", de, postas, faixas());
    }
}

static void navegador(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    if (r == rFiltro || r == rMudaPasta) {
        if (r == rFiltro) { poe_num(eFiltro, "contentFilterMask", pedido_num(r, pb, n, "contentFilterMask")); avisa(eFiltro, inst, 0); }
        poe_num(eTamanho, "listSizeCount", faixas());
        avisa(ePasta, inst, 0);
        avisa(eTamanho, inst, 0);
        ATENDIDO(r, "instância %u: mandei %sa pasta do perfil e o tamanho da lista (%d)", inst, r == rFiltro ? "o filtro, " : "", faixas());
    } else if (r == rCaminho) {
        struct evt tmp = resposta(eCaminho);
        int i, niveis = quantos(ePasta, fPasta);
        for (i = 0; i < niveis; i++) { struct val *novo = no(&tmp.vals, fCaminho, i); novo->kids = clona(item(ePasta, fPasta, i)); }
        envia(&tmp, eCaminho, inst);
        ATENDIDO(r, "instância %u: mandei o caminho da pasta do perfil (%d nível(is))", inst, niveis);
    } else if (r == rLista) {
        long long id = pedido_num(r, pb, n, "entryID"), de = pedido_num(r, pb, n, "index"), quantas = pedido_num(r, pb, n, "count");
        struct evt tmp = resposta(eLista);
        int foco = id > 0 ? indice_da_faixa(id) : -1, postas;
        if (foco >= 0) { de = foco; if (quantas < 1) quantas = 1; }       /* a entrada em foco, com o índice dela */
        postas = poe_trecho(&tmp, fFaixas, de, quantas);
        poe_num(&tmp, "index", de);
        envia(&tmp, eLista, inst);
        if (foco >= 0) ATENDIDO(r, "instância %u: a entrada %lld é a de índice %d; mandei %d faixa(s)", inst, id, foco, postas);
        else ATENDIDO(r, "instância %u: lista a partir de %lld: mandei %d de %d faixa(s)", inst, de, postas, faixas());
    } else {                                         /* rZera, rDesfaz, rSeleciona */
        struct evt tmp = resposta(eSelNav);
        int seleciona = r == rSeleciona && pedido_num(r, pb, n, "selection");
        if (seleciona) {
            char c[] = "filesExist", v[] = "1";
            poe_num(&tmp, "selectionState", 1);       /* SELECTIONSTATE_ALL */
            atribui(&tmp, c, v, 0);
            poe_num(&tmp, "entriesAvail", faixas());
            poe_num(&tmp, "entriesSelected", faixas());
        }
        envia(&tmp, eSelNav, inst);
        ATENDIDO(r, "instância %u: resultado 0%s", inst, seleciona ? ", tudo selecionado" : ", nada selecionado");
    }
}

static struct evt *evento_de(struct svc *s, const char *nome)
{
    int i;
    for (i = 0; i < nevts; i++) if (evts[i].s == s && !strcmp(evts[i].name, nome)) return evts[i].m ? &evts[i] : NULL;
    return NULL;
}
static struct req *pedido_de(struct svc *s, const char *nome)
{
    int i;
    for (i = 0; i < nreqs; i++) if (reqs[i].s == s && !strcmp(reqs[i].name, nome)) return reqs[i].m ? &reqs[i] : NULL;
    return NULL;
}

/* confere que o esquema tem tudo o que o modelo usa, pelo nome, e que o perfil tem o que tocar */
static int modelo_inicia(void)
{
    static const struct { struct evt **e; const char *svc, *nome; } EV[] = {
        { &eAparelhos, "DSIMediaBase", "MBLupdateDeviceList" }, { &eMidias, "DSIMediaBase", "MBLupdateMediaList" },
        { &eAtiva, "DSIMediaPlayer", "MPLupdateActiveMedia" }, { &eCapac, "DSIMediaPlayer", "MPLupdateCapabilities" },
        { &eModos, "DSIMediaPlayer", "MPLupdatePlaybackModeList" }, { &eModo, "DSIMediaPlayer", "MPLupdatePlaybackMode" },
        { &ePosicao, "DSIMediaPlayer", "MPLupdatePlayPosition" }, { &eEstado, "DSIMediaPlayer", "MPLupdatePlaybackState:7" },
        { &eDetalhe, "DSIMediaPlayer", "MPLresponseDetailInfo" }, { &eSelToc, "DSIMediaPlayer", "MPLresponseSetPlaySelection" },
        { &eVista, "DSIMediaPlayer", "MPLresponsePlayView" }, { &eFiltro, "DSIMediaBrowser", "MBRLupdateContentFilter" },
        { &ePasta, "DSIMediaBrowser", "MBRLupdateBrowseFolder" }, { &eTamanho, "DSIMediaBrowser", "MBRLupdateListSize" },
        { &eLista, "DSIMediaBrowser", "MBRLresponseList" }, { &eSelNav, "DSIMediaBrowser", "MBRLselectionResult" },
        { &eCaminho, "DSIMediaBrowser", "MBRLresponseBrowsePath" } };
    static const struct { struct req **r; const char *svc, *nome; } PE[] = {
        { &rAtiva, "DSIMediaPlayer", "setActiveMedia" }, { &rModo, "DSIMediaPlayer", "setPlaybackMode" }, { &rToca, "DSIMediaPlayer", "play" },
        { &rRetoma, "DSIMediaPlayer", "resume" }, { &rPausa, "DSIMediaPlayer", "pause" }, { &rPara, "DSIMediaPlayer", "stop" },
        { &rPula, "DSIMediaPlayer", "skip" }, { &rEntrada, "DSIMediaPlayer", "setEntry" }, { &rDetalhe, "DSIMediaPlayer", "requestDetailInfo" },
        { &rSelToc, "DSIMediaPlayer", "setPlaySelection" }, { &rVista, "DSIMediaPlayer", "requestPlayView" },
        { &rFiltro, "DSIMediaBrowser", "setContentFilter" }, { &rMudaPasta, "DSIMediaBrowser", "changeFolder" }, { &rLista, "DSIMediaBrowser", "requestList" },
        { &rZera, "DSIMediaBrowser", "resetSelection" }, { &rDesfaz, "DSIMediaBrowser", "undoLastSelection" },
        { &rSeleciona, "DSIMediaBrowser", "addSelection" }, { &rCaminho, "DSIMediaBrowser", "requestBrowsePath" } };
    static const struct { struct field **f; struct evt **e; const char *nome; } LI[] = {
        { &fAparelhos, &eAparelhos, "deviceList" }, { &fMidias, &eMidias, "mediaList" }, { &fModosLista, &eModos, "playbackModeList" },
        { &fFaixas, &eLista, "entrylist" }, { &fPasta, &ePasta, "activeBrowsingFolder" }, { &fCaminho, &eCaminho, "path" }, { &fVista, &eVista, "activePlayingFolder" } };
    char falta[160] = "";
    struct svc *busca;
    size_t k;
    int i, j;
    for (k = 0; !*falta && k < sizeof EV / sizeof *EV; k++)
        if (!acha_svc(EV[k].svc) || !(*EV[k].e = evento_de(acha_svc(EV[k].svc), EV[k].nome))) snprintf(falta, sizeof falta, "o evento %s do %s", EV[k].nome, EV[k].svc);
    for (k = 0; !*falta && k < sizeof PE / sizeof *PE; k++)
        if (!(*PE[k].r = pedido_de(acha_svc(PE[k].svc), PE[k].nome))) snprintf(falta, sizeof falta, "o pedido %s do %s", PE[k].nome, PE[k].svc);
    for (k = 0; !*falta && k < sizeof LI / sizeof *LI; k++)
        if (!(*LI[k].f = acha_campo((*LI[k].e)->m, LI[k].nome)) || (*LI[k].f)->type != T_MSG || (*LI[k].f)->label == L_UM) snprintf(falta, sizeof falta, "o campo de lista %s em %s", LI[k].nome, (*LI[k].e)->name);
    if (!*falta && (fVista->sub != fFaixas->sub || fCaminho->sub != fPasta->sub || fPasta->sub != fFaixas->sub)) snprintf(falta, sizeof falta, "a mesma mensagem de entrada na lista, na pasta e na lista de reprodução");
    if (*falta) {
        fprintf(stderr, "[midiastub] este programa é a mídia de mentira e o esquema dado não é o de mídia: falta %s. O esquema certo é o stubs/midia/gerado/esquema.txt; nada foi enviado\n", falta);
        return -1;
    }
    if (!quantos(eAparelhos, fAparelhos) || !quantos(eMidias, fMidias)) {
        fprintf(stderr, "[midiastub] o perfil não tem nenhuma fonte: faltam o aparelho ([DSIMediaBase.MBLupdateDeviceList]) e a mídia dele ([DSIMediaBase.MBLupdateMediaList]); sem aparelho a HMI nem liga as fontes. Nada foi enviado\n");
        return -1;
    }
    if (!faixas()) {
        fprintf(stderr, "[midiastub] o perfil não tem nenhuma faixa: a lista é a seção [DSIMediaBrowser.MBRLresponseList] (entrylist[0].entryID, .title…). Nada foi enviado\n");
        return -1;
    }
    for (i = 0; i < faixas(); i++)
        for (j = 0; j < i; j++)
            if (numero(faixa(i), "entryID", 0) == numero(faixa(j), "entryID", 0)) {
                fprintf(stderr, "[midiastub] as faixas %d e %d do perfil têm o mesmo identificador (entryID = %lld): é por ele que a HMI pede a faixa. Nada foi enviado\n", j, i, numero(faixa(i), "entryID", 0));
                return -1;
            }
    if ((busca = acha_svc("DSISearch"))) busca->req = (uint32_t)-1;        /* ver o cabeçalho: a busca é de vários domínios */
    fprintf(stderr, "[midiastub] modelo do tocador: %d fonte(s) e %d faixa(s) no perfil; começo na primeira, \"%s\". O DSISearch fica de fora: as instâncias dele são de domínios diferentes\n",
            quantos(eAparelhos, fAparelhos), faixas(), texto(faixa(0), "title"));
    return 0;
}

static int do_tocador(const struct req *r)
{
    return r == rAtiva || r == rModo || r == rToca || r == rRetoma || r == rPausa || r == rPara || r == rPula || r == rEntrada || r == rDetalhe || r == rSelToc || r == rVista;
}
static int do_navegador(const struct req *r)
{
    return r == rFiltro || r == rMudaPasta || r == rLista || r == rZera || r == rDesfaz || r == rSeleciona || r == rCaminho;
}
static int modelo_trata(const struct req *r) { return do_tocador(r) || do_navegador(r); }

static void modelo_pedido(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    if (do_tocador(r)) tocador(r, inst, pb, n);
    else navegador(r, inst, pb, n);
}
