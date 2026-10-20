/* radiostub.c — o "rádio de mentira" do sandbox: faz o papel do sintonizador diante da HMI.
 * Marco 11, 12/10/2026. Estudo num computador, sem carro, sem antena e sem som; nada daqui vai
 * para o carro.
 *
 * No carro quem fornece os serviços DSIUnifiedTuner, DSITunerAnnouncement e DSIRadioData é o
 * radio-hmi do firmware (domínio PID_TUNER_HMI, canal 0x400000000000), que fala com o programa do
 * sintonizador. Na bancada não há sintonizador, então este programa ocupa o lugar dele.
 *
 * É o veiculostub com outro esquema, mais um modelo pequeno. O motor é o mesmo arquivo, incluído
 * aqui: lê o esquema (stubs/radio/gerado/esquema.txt, tirado das classes do firmware) e os perfis
 * (stubs/radio/), anuncia o domínio uma vez, responde a cada assinatura com o valor do perfil ou
 * com o padrão marcado como válido, e atende os pedidos que o perfil liga. O formato do perfil, o
 * barramento e o modo de uso (-n confere, -m mostra) estão no cabeçalho do veiculostub.c.
 *
 * O que o motor não sabe fazer, e o modelo faz, são três pedidos do DSIUnifiedTuner em que a HMI
 * espera TRÊS mensagens em ordem (lido em de.vw.mib.asl.internal.radio: StationSelectionManager,
 * StateTopMainUnifiedRadio, StateAmFmActiveSeek e InitialAutostore, e medido nas rodadas de
 * 12/10/2026):
 *
 *   selectStation(modo, frequência, PI…)       toque na lista, numa memória ou na sintonia manual
 *     1. selectStationStatus = 1 (em curso). Enquanto espera esta, a HMI descarta a atualização
 *        da emissora (UnifiedRadioStatus.waitingForStatusRunning);
 *     2. a atualização de updateSelectedStation: a emissora da lista do perfil que tem aquela
 *        frequência, inteira (nome, PI, RDS…). Se a frequência não está na lista, que é o caso
 *        da sintonia manual, vai só a faixa, a frequência e o PI do pedido;
 *     3. selectStationStatus = 2 (feito).
 *
 *   seekStation(modo, faixa, sessão)           setas, com "Teclas de setas" em busca
 *     modo 1 (acima) ou 2 (abaixo): seekStationStatus = 1 com a faixa e a sessão do pedido; a
 *        VARREDURA (Marco 16): uma atualização da emissora por canal do caminho, só com faixa,
 *        frequência e PI -1, a cada 25 ms, até a emissora seguinte da lista naquela faixa (por
 *        frequência; passando da ponta da faixa, recomeça na outra); a emissora inteira na
 *        chegada; e seekStationStatus = 2. Caminho com mais de 40 canais pula canais, para não
 *        passar de 40 mensagens. Sem emissora na faixa, dá a volta inteira e para onde estava;
 *     modo 5 ou 6 (acima ou abaixo, sem parar): o mesmo caminho, e a busca fica "em curso" até a
 *        HMI mandar parar. Simplificação: o sintonizador de verdade seguiria andando;
 *     modo 3 (interromper) ou 4 (parar): seekStationStatus = 3 ou 4, e a emissora não muda.
 *     Sem a resposta "feito" a HMI fica em busca, e o toque seguinte só serve para interrompê-la.
 *     Enquanto varre (1 s, no máximo) o programa não atende outro pedido: a interrupção que a HMI
 *     mandar no meio do caminho só é respondida depois da chegada.
 *     A grade de cada faixa está na tabela GRADES, mais abaixo: é a que a HMI usa na bancada, e
 *     ela não a pergunta ao sintonizador (tira da adaptação do carro).
 *
 *   forceUpdateStationList(modo, faixa)        "Buscar e salvar" do assistente de configuração
 *     modo 1 (começar): forceUpdateStationListStatus = 1 com a faixa do pedido, a lista de
 *        emissoras de novo para quem a assina (se a lista do perfil é dessa faixa) e
 *        forceUpdateStationListStatus = 2. A HMI guarda então as emissoras nas memórias;
 *     modo 2 (interromper): forceUpdateStationListStatus = 3.
 *     Sem o estado final a HMI fica 65 s em "Procurando emissoras…" (InitialAutostore).
 *
 * A HMI só manda sintonizar depois de o atributo de uso do rádio dizer 1; isso é uma regra do
 * perfil ([DSIUnifiedTuner.switchRadioUsage -> updateRadioUsageStatus]), atendida pelo motor.
 * A lista de emissoras é a do perfil e não muda: não há recepção. O que a HMI sintoniza vale até
 * a central desligar.
 *
 * AM (Marco 16). A HMI guarda uma lista por faixa e as recebe em mensagens separadas, cada uma com
 * o seu tipo de faixa (StateTopMainUnifiedRadio.dsiUnifiedTunerUpdateStationList). O perfil tem
 * uma lista só, com a faixa de cada emissora (waveBand): na partida o modelo tira dela as de AM,
 * que passam a ser a "lista de AM", e acerta o "max" das duas. A de FM é a que o motor manda a
 * quem assina; a de AM o modelo manda no primeiro pedido que atende, cada vez que a HMI entra em
 * AM e quando ela pede a atualização da lista de AM. Perfil sem emissora de AM: nada muda.
 *
 * Texto de rádio (Marco 16). Se o perfil tem [DSIUnifiedTuner.updateRadioText] com um texto,
 * depois de cada sintonia e de cada busca o modelo manda esse texto com o PI da emissora de
 * agora, se ela tem "radioText = 1" na lista, ou o texto vazio, se não tem. A HMI só aceita texto
 * cujo PI é o da emissora atual (UnifiedRadioRadioTextManager.isReceivedTextValid). Perfil sem
 * essa seção: o modelo não manda texto nenhum.
 *
 * Intensidade de sinal: não há o que mandar. A HMI guarda a qualidade de recepção de cada emissora
 * (receptionQuality), mas não a mostra em tela nenhuma; a única comparação por qualidade do
 * código dela está numa classe que ninguém usa (QualityPIFrequencyHDComparatorChain).
 *
 * O modelo acha serviço, eventos, pedidos e campos pelo NOME, no esquema; não escreve byte de
 * mensagem à mão. Se o esquema não for o do sintonizador, recusa tudo e diz o que faltou.
 * Os números de modo, faixa e estado são constantes de org.dsi.ifc.radio.DSIUnifiedTuner
 * (SELECTSTATIONMODE_, SEEKSTATIONMODE_, BANDTYPE_, WAVEBAND_, SELECTSTATIONSTATUS_ e
 * SEEKSTATIONSTATUS_); estão em stubs/radio/gerado/constantes.txt.
 *
 * uso: radiostub <socket> <esquema> [<perfil>...]     fica no barramento
 *      radiostub -n <esquema> [<perfil>...]           só confere os arquivos (0 = tudo certo)
 *      radiostub -m <esquema> [<perfil>...]           mostra o que seria mandado
 */
#define VEICULOSTUB_MODELO
#include <stdarg.h>
#include "veiculostub.c"

static struct evt *eEmissora, *eLista, *eSintonia, *eBusca, *eAtualiza, *eTexto;
static struct req *rSintoniza, *rBusca, *rAtualiza;
static struct field *fEmissora, *fLista;          /* updateSelectedStation.selectedStation e updateStationList.stationList */
static struct field *fTexto;                      /* updateRadioText.radioText */
static struct evt eListaAM;                       /* a lista de AM: o evento da lista, com valores próprios */
static int nFM, nAM;                              /* quantas emissoras ficaram na lista do perfil e quantas na de AM */
static int am_mandada;                            /* a lista de AM já foi para quem assina a lista */
static struct val *texto_base;                    /* o texto de rádio como veio do perfil; NULL = o perfil não tem */
static const char *nota_texto = "";               /* para o registro: que texto acompanhou a última sintonia */

/* A grade de cada faixa, em kHz: limites e passo. É a que a HMI usa na bancada, e ela não a
 * pergunta ao sintonizador. FM: a faixa "Brasil" (76,1 a 107,9 MHz, de 200 em 200 kHz), que a
 * identidade provisória do Jetta liga pela adaptação 0559, por palpite marcado em
 * stubs/codificacao. AM: a da Europa (531 a 1602 kHz, de 9 em 9), que a HMI assume com a
 * adaptação de AM "sem ajuste". Se a codificação mudar, mudar aqui também. */
static const struct grade { int familia; long long de, ate, passo; } GRADES[] = { { 3, 76100, 107900, 200 }, { 1, 531, 1602, 9 } };
#define VARRE_MAX 40                              /* no máximo tantas mensagens de caminho por busca */
#define VARRE_MS 25                               /* intervalo entre elas, em ms */

/* faixa de uma emissora (WAVEBAND_): 1 AM, 2 AM com HD, 3 FM, 4 FM com HD, 5 DAB. A "família"
 * junta a faixa com a variante HD dela, para a lista do perfil servir às duas. */
static int familia(long long faixa) { return faixa == 1 || faixa == 2 ? 1 : faixa == 3 || faixa == 4 ? 3 : (int)faixa; }
/* SELECTSTATIONMODE_: 0 AM, 1 AM HD, 2 FM, 3 FM HD, 4 DAB, 5 AM manual, 6 FM manual */
static int faixa_do_modo(long long modo)
{
    switch (modo) { case 0: case 5: return 1; case 1: return 2; case 2: case 6: return 3; case 3: return 4; case 4: return 5; }
    return 0;
}
/* BANDTYPE_ (o mesmo número de SEEKSTATIONBANDTYPE_): 0 AM, 1 AM HD, 2 FM, 3 FM HD, 4 FM e DAB */
static int faixa_do_tipo(long long tipo)
{
    switch (tipo) { case 0: return 1; case 1: return 2; case 2: case 4: return 3; case 3: return 4; }
    return 0;
}

static long long numero(struct val *campos, const char *nome, long long padrao)
{
    long long v;
    for (; campos; campos = campos->next)
        if (!strcmp(campos->f->name, nome)) return campos->txt && inteiro(campos->txt, &v) ? v : padrao;
    return padrao;
}

static const char *texto(struct val *campos, const char *nome)
{
    for (; campos; campos = campos->next) if (!strcmp(campos->f->name, nome)) return campos->txt ? campos->txt : "";
    return "";
}

/* os campos da emissora selecionada, e os de cada emissora da lista do perfil */
static struct val *selecionada(void) { struct val *v = acha_no(eEmissora->vals, fEmissora, 0); return v ? v->kids : NULL; }
static int conta_lista(const struct evt *e)
{
    struct val *v;
    int max = -1;
    for (v = e->vals; v; v = v->next) if (v->f == fLista && v->idx > max) max = v->idx;
    return max + 1;
}
/* as emissoras das duas listas, uma depois da outra: primeiro as da lista do perfil, depois as de AM */
static int tamanho_da_lista(void) { return nFM + nAM; }
static struct val *da_lista(int i)
{
    struct val *v = i < nFM ? acha_no(eLista->vals, fLista, i) : acha_no(eListaAM.vals, fLista, i - nFM);
    return v ? v->kids : NULL;
}
static const struct grade *grade_da(int faixa)
{
    unsigned i;
    for (i = 0; i < sizeof GRADES / sizeof *GRADES; i++) if (GRADES[i].familia == familia(faixa)) return &GRADES[i];
    return NULL;
}

/* Na partida: tira da lista do perfil as emissoras de AM, na ordem, e faz delas a lista de AM. As
 * que ficam são renumeradas. Havendo alguma de AM, o "max" das duas listas passa a ser o tamanho
 * de cada uma, que é o que a camada da LG exige. O tipo de faixa da lista de AM é 0, que não vai no fio. */
static void separa_am(void)
{
    int total = conta_lista(eLista), i;
    char caminho[8], valor[16];
    eListaAM = *eLista;
    eListaAM.vals = NULL; eListaAM.nvals = 0;
    nFM = nAM = 0;
    for (i = 0; i < total; i++) {
        struct val *v = acha_no(eLista->vals, fLista, i), **pp;
        if (!v) continue;
        if (familia(numero(v->kids, "waveBand", 0)) != 1) { v->idx = nFM++; continue; }
        for (pp = &eLista->vals; *pp != v; pp = &(*pp)->next) ;
        *pp = v->next;
        v->idx = nAM++; v->next = eListaAM.vals; eListaAM.vals = v;
    }
    if (!nAM) return;
    snprintf(caminho, sizeof caminho, "max"); snprintf(valor, sizeof valor, "%d", nAM); atribui(&eListaAM, caminho, valor, 0);
    snprintf(caminho, sizeof caminho, "max"); snprintf(valor, sizeof valor, "%d", nFM); atribui(eLista, caminho, valor, 0);
}

/* troca a emissora selecionada: por uma da lista (cópia inteira) ou, com origem nula, por uma só
 * com faixa, frequência e PI */
static void seleciona(struct val *origem, int faixa, long long freq, long long pi)
{
    struct val *no_sel = no(&eEmissora->vals, fEmissora, 0);
    char caminho[64], valor[32];
    libera(no_sel->kids);
    no_sel->kids = clona(origem);
    if (origem) return;
    snprintf(caminho, sizeof caminho, "%s.waveBand", fEmissora->name); snprintf(valor, sizeof valor, "%d", faixa); atribui(eEmissora, caminho, valor, 0);
    snprintf(caminho, sizeof caminho, "%s.frequency", fEmissora->name); snprintf(valor, sizeof valor, "%lld", freq); atribui(eEmissora, caminho, valor, 0);
    snprintf(caminho, sizeof caminho, "%s.piSId", fEmissora->name); snprintf(valor, sizeof valor, "%lld", pi); atribui(eEmissora, caminho, valor, 0);
}

static int avisa_assinantes(struct evt *e)
{
    int i, k = 0;
    for (i = 0; i < nsubs; i++) if (subs[i].e == e) { publica(&subs[i], 1); k++; }
    return k;
}

/* a lista de AM, para quem assina a lista de emissoras */
static int manda_lista_am(void)
{
    static struct buf b;
    int i, k = 0;
    monta(&b, &eListaAM);
    for (i = 0; i < nsubs; i++) if (subs[i].e == eLista) { manda(eLista->s->dom, eLista->s->evt, subs[i].inst, eListaAM.api, b.p, b.n); k++; }
    am_mandada = 1;
    return k;
}

/* a emissora da lista que é a selecionada de agora: mesma faixa (ou a variante HD dela) e mesma frequência */
static struct val *selecionada_na_lista(void)
{
    struct val *sel = selecionada(), *e;
    int i;
    for (i = 0; i < tamanho_da_lista(); i++)
        if ((e = da_lista(i)) && numero(e, "frequency", -1) == numero(sel, "frequency", -2) && familia(numero(e, "waveBand", 0)) == familia(numero(sel, "waveBand", 0))) return e;
    return NULL;
}

/* O texto de rádio da emissora de agora: o do perfil, com o PI dela, se ela tem a marca radioText
 * na lista; senão o texto vazio, com o PI dela. Com avisa, vai para quem assina o texto. */
static int texto_da_emissora(int avisa)
{
    struct val *na_lista = selecionada_na_lista(), *msg, **pp, *k;
    long long pi = numero(selecionada(), "piSId", 0);
    char caminho[64], valor[32];
    if (!texto_base) return 0;
    libera(eTexto->vals);
    eTexto->vals = clona(texto_base);
    nota_texto = "; texto de rádio: o do perfil";
    if (!(na_lista && numero(na_lista, "radioText", 0) == 1) && (msg = acha_no(eTexto->vals, fTexto, 0))) {
        nota_texto = "; texto de rádio: vazio";
        for (pp = &msg->kids; (k = *pp); pp = &k->next)
            if (!strcmp(k->f->name, "radioText")) { *pp = k->next; k->next = NULL; libera(k); break; }
    }
    if (pi) {
        snprintf(caminho, sizeof caminho, "%s.piSId", fTexto->name); snprintf(valor, sizeof valor, "%lld", pi);
        atribui(eTexto, caminho, valor, 0);
    }
    return avisa ? avisa_assinantes(eTexto) : 0;
}

/* manda um evento de resposta com os campos dados ("nome", "valor", …, NULL), na instância do pedido */
static void responde_com(struct evt *e, uint32_t inst, ...)
{
    struct evt tmp = *e;
    struct buf b = { NULL, 0, 0 };
    struct tm tm;
    va_list ap;
    const char *nome;
    tmp.vals = NULL; tmp.nvals = 0;
    va_start(ap, inst);
    while ((nome = va_arg(ap, const char *))) {
        char caminho[64], valor[64];
        snprintf(caminho, sizeof caminho, "%s", nome);
        snprintf(valor, sizeof valor, "%s", va_arg(ap, const char *));
        atribui(&tmp, caminho, valor, 0);
    }
    va_end(ap);
    agora(&tm);
    poe_msg(&b, tmp.m, tmp.vals, 0, &tm);
    manda(e->s->dom, e->s->evt, inst, e->api, b.p, b.n);
    libera(tmp.vals);
    free(b.p);
}

static void sintoniza(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    char v[64];
    long long modo, freq, pi;
    struct val *achada = NULL;
    int i, faixa, avisados;
    le_folha(r->m, pb, n, "mode", v, sizeof v); modo = atoll(v);
    le_folha(r->m, pb, n, "frequency", v, sizeof v); freq = atoll(v);
    le_folha(r->m, pb, n, "piSId", v, sizeof v); pi = atoll(v);
    faixa = faixa_do_modo(modo);
    for (i = 0; i < tamanho_da_lista() && !achada; i++) {
        struct val *e = da_lista(i);
        if (e && numero(e, "frequency", -1) == freq && familia(numero(e, "waveBand", 0)) == familia(faixa)) achada = e;
    }
    if (nAM && familia(faixa) == 1 && familia(numero(selecionada(), "waveBand", 0)) != 1) am_mandada = 0;   /* entrou em AM: a lista vai de novo */
    responde_com(eSintonia, inst, "status", "1", NULL);
    seleciona(achada, faixa, freq, pi);
    avisados = avisa_assinantes(eEmissora);
    responde_com(eSintonia, inst, "status", "2", NULL);
    texto_da_emissora(1);
    fprintf(stderr, "[radiostub] pedido atendido: %s.%s (api %u): sintonizei %lld kHz, %s%s%s; avisei %d assinatura(s)%s\n", r->s->name, r->name, r->api, freq,
            achada ? "emissora da lista" : "fora da lista: vai só a frequência", achada && *texto(achada, "longName") ? " " : "", achada ? texto(achada, "longName") : "", avisados, nota_texto);
}

/* Varre a faixa de `atual` até `alvo`, no sentido dado (1 sobe, -1 desce): manda a quem assina a
 * emissora uma atualização só com faixa e frequência para cada canal do caminho (ou a cada tantos,
 * para não passar de VARRE_MAX mensagens), com VARRE_MS entre elas. Passando da ponta da faixa,
 * recomeça na outra. Com alvo igual a atual dá a volta inteira. O canal de chegada não entra: quem
 * chama manda a emissora inteira. Devolve quantas mensagens mandou; 0 se a faixa não tem grade ou
 * se a frequência de agora não é dela. */
static int varre(int faixa, long long atual, long long alvo, int sentido)
{
    const struct grade *g = grade_da(faixa);
    static long long caminho[512];
    long long f, nf;
    int n = 0, i, cada, mandadas = 0;
    if (!g || atual < g->de || atual > g->ate) return 0;
    for (f = atual; n < 512; f = nf) {
        int virou = 0;
        nf = f + sentido * g->passo;
        if (nf > g->ate) { nf = g->de; virou = 1; } else if (nf < g->de) { nf = g->ate; virou = 1; }
        if (virou ? (nf == alvo || (alvo - f) * sentido > 0) : ((alvo - f) * sentido > 0 && (alvo - nf) * sentido <= 0)) break;
        caminho[n++] = nf;
    }
    cada = (n + VARRE_MAX) / VARRE_MAX;            /* n canais de caminho mais o de chegada, em no máximo VARRE_MAX passos */
    for (i = 1; i <= n; i++) {
        if (i % cada) continue;
        seleciona(NULL, faixa, caminho[i - 1], -1);
        avisa_assinantes(eEmissora);
        mandadas++;
        usleep(VARRE_MS * 1000);
    }
    return mandadas;
}

static void busca(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    char modo[64], tipo[64], sessao[64];
    long long m, atual, melhor = 0, ponta = 0;
    struct val *prox = NULL, *volta = NULL, *antes;
    int i, faixa, sentido, avisados, varridas;
    le_folha(r->m, pb, n, "seekStationMode", modo, sizeof modo); m = atoll(modo);
    le_folha(r->m, pb, n, "bandType", tipo, sizeof tipo);
    le_folha(r->m, pb, n, "sessionId", sessao, sizeof sessao);
    if (m != 1 && m != 2 && m != 5 && m != 6) {      /* interromper (3), parar (4) e o que mais vier: a busca acaba */
        const char *estado = m == 3 ? "3" : m == 4 ? "4" : "2";
        responde_com(eBusca, inst, "seekStationStatus", estado, "bandType", tipo, "sessionId", sessao, NULL);
        fprintf(stderr, "[radiostub] pedido atendido: %s.%s (api %u): modo %lld, respondi estado %s; a emissora não mudou\n", r->s->name, r->name, r->api, m, estado);
        return;
    }
    sentido = m == 1 || m == 5 ? 1 : -1;
    faixa = faixa_do_tipo(atoll(tipo));
    atual = numero(selecionada(), "frequency", 0);
    /* a seguinte naquele sentido; se não há, a da outra ponta da faixa */
    for (i = 0; i < tamanho_da_lista(); i++) {
        struct val *e = da_lista(i);
        long long f;
        if (!e || familia(numero(e, "waveBand", 0)) != familia(faixa)) continue;
        f = numero(e, "frequency", 0);
        if ((f - atual) * sentido > 0 && (!prox || (melhor - f) * sentido > 0)) { prox = e; melhor = f; }
        if (!volta || (ponta - f) * sentido > 0) { volta = e; ponta = f; }
    }
    if (!prox) prox = volta;
    antes = clona(selecionada());                  /* para a busca que não acha nada voltar ao ponto de partida */
    responde_com(eBusca, inst, "seekStationStatus", "1", "bandType", tipo, "sessionId", sessao, NULL);
    varridas = varre(faixa, atual, prox ? numero(prox, "frequency", atual) : atual, sentido);
    if (prox) seleciona(prox, 0, 0, 0);
    else if (varridas) seleciona(antes, 0, 0, 0);
    libera(antes);
    avisados = avisa_assinantes(eEmissora);
    if (m == 1 || m == 2) responde_com(eBusca, inst, "seekStationStatus", "2", "bandType", tipo, "sessionId", sessao, NULL);
    texto_da_emissora(1);
    fprintf(stderr, "[radiostub] pedido atendido: %s.%s (api %u): busca %s%s a partir de %lld kHz: varri %d frequência(s) e %s%lld kHz; avisei %d assinatura(s)%s\n", r->s->name, r->name, r->api,
            sentido > 0 ? "acima" : "abaixo", m > 2 ? " sem parar (fica em curso até a HMI parar)" : "", atual, varridas,
            prox ? "parei em " : "não há emissora dessa faixa na lista, fiquei em ", prox ? numero(prox, "frequency", 0) : atual, avisados, nota_texto);
}

static void atualiza_lista(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    char modo[64], tipo[64];
    int avisados = 0, mesma, am;
    le_folha(r->m, pb, n, "updateMode", modo, sizeof modo);
    le_folha(r->m, pb, n, "bandType", tipo, sizeof tipo);
    if (atoll(modo) != 1) {                          /* interromper (2) e o que mais vier */
        responde_com(eAtualiza, inst, "forceUpdateListStatus", "3", "bandType", tipo, NULL);
        fprintf(stderr, "[radiostub] pedido atendido: %s.%s (api %u): modo %s, respondi estado 3 (interrompida)\n", r->s->name, r->name, r->api, modo);
        return;
    }
    mesma = familia(faixa_do_tipo(atoll(tipo))) == familia(faixa_do_tipo(numero(eLista->vals, "bandType", 0)));
    am = nAM && familia(faixa_do_tipo(atoll(tipo))) == 1;
    responde_com(eAtualiza, inst, "forceUpdateListStatus", "1", "bandType", tipo, NULL);
    if (am) avisados = manda_lista_am();
    else if (mesma) avisados = avisa_assinantes(eLista);
    responde_com(eAtualiza, inst, "forceUpdateListStatus", "2", "bandType", tipo, NULL);
    fprintf(stderr, "[radiostub] pedido atendido: %s.%s (api %u): atualizar a lista da faixa %s: %s; avisei %d assinatura(s)\n", r->s->name, r->name, r->api, tipo,
            am ? "mandei de novo a lista de AM" : mesma ? "mandei de novo a lista do perfil" : "a lista do perfil é de outra faixa, só respondi os estados", avisados);
}

static struct evt *evento_de(struct svc *s, const char *nome)
{
    int i;
    for (i = 0; i < nevts; i++) if (evts[i].s == s && !strcmp(evts[i].name, nome)) return &evts[i];
    return NULL;
}
static struct req *pedido_de(struct svc *s, const char *nome)
{
    int i;
    for (i = 0; i < nreqs; i++) if (reqs[i].s == s && !strcmp(reqs[i].name, nome)) return &reqs[i];
    return NULL;
}

/* confere que o esquema tem tudo o que o modelo usa, pelo nome; senão diz o que faltou e recusa */
static int modelo_inicia(void)
{
    static const char *CAMPOS_SINTONIA[] = { "mode", "frequency", "piSId", NULL }, *CAMPOS_BUSCA[] = { "seekStationMode", "bandType", "sessionId", NULL };
    static const char *CAMPOS_EMISSORA[] = { "waveBand", "frequency", "piSId", NULL };
    struct svc *s = acha_svc("DSIUnifiedTuner");
    const char *falta = NULL, *err = NULL;
    int k;
    if (!s) falta = "o serviço DSIUnifiedTuner";
    else if (!(eEmissora = evento_de(s, "updateSelectedStation")) || !eEmissora->m) falta = "o evento updateSelectedStation";
    else if (!(eLista = evento_de(s, "updateStationList")) || !eLista->m) falta = "o evento updateStationList";
    else if (!(eSintonia = evento_de(s, "selectStationStatus")) || !eSintonia->m) falta = "o evento selectStationStatus";
    else if (!(eBusca = evento_de(s, "seekStationStatus")) || !eBusca->m) falta = "o evento seekStationStatus";
    else if (!(rSintoniza = pedido_de(s, "selectStation")) || !rSintoniza->m) falta = "o pedido selectStation";
    else if (!(rBusca = pedido_de(s, "seekStation")) || !rBusca->m) falta = "o pedido seekStation";
    else if (!(eAtualiza = evento_de(s, "forceUpdateStationListStatus")) || !eAtualiza->m) falta = "o evento forceUpdateStationListStatus";
    else if (!(rAtualiza = pedido_de(s, "forceUpdateStationList")) || !rAtualiza->m) falta = "o pedido forceUpdateStationList";
    else if (!folha(rAtualiza->m, "updateMode", &err) || !folha(rAtualiza->m, "bandType", &err) || !folha(eAtualiza->m, "forceUpdateListStatus", &err) || !folha(eAtualiza->m, "bandType", &err))
        falta = "um dos campos de modo, estado e faixa em forceUpdateStationList ou em forceUpdateStationListStatus";
    else if (!(fEmissora = acha_campo(eEmissora->m, "selectedStation")) || fEmissora->type != T_MSG || fEmissora->label != L_UM) falta = "o campo selectedStation (uma mensagem) em updateSelectedStation";
    else if (!(fLista = acha_campo(eLista->m, "stationList")) || fLista->type != T_MSG || fLista->label == L_UM) falta = "o campo stationList (lista de mensagens) em updateStationList";
    else if (fLista->sub != fEmissora->sub) falta = "a emissora da lista com a mesma mensagem da emissora selecionada";
    else if (!folha(eSintonia->m, "status", &err)) falta = "o campo status em selectStationStatus";
    for (k = 0; !falta && CAMPOS_SINTONIA[k]; k++) if (!folha(rSintoniza->m, CAMPOS_SINTONIA[k], &err)) falta = "um dos campos mode, frequency e piSId no pedido selectStation";
    for (k = 0; !falta && CAMPOS_BUSCA[k]; k++)
        if (!folha(rBusca->m, CAMPOS_BUSCA[k], &err) || !folha(eBusca->m, k ? CAMPOS_BUSCA[k] : "seekStationStatus", &err)) falta = "um dos campos de modo, faixa e sessão no pedido seekStation ou em seekStationStatus";
    for (k = 0; !falta && CAMPOS_EMISSORA[k]; k++) if (!acha_campo(fEmissora->sub, CAMPOS_EMISSORA[k])) falta = "um dos campos waveBand, frequency e piSId na mensagem da emissora";
    if (falta) {
        fprintf(stderr, "[radiostub] este programa é o rádio de mentira e o esquema dado não é o do sintonizador: falta %s. O esquema certo é o stubs/radio/gerado/esquema.txt; nada foi enviado\n", falta);
        return -1;
    }
    separa_am();
    /* o texto de rádio é opcional: vale se o esquema tem o evento e o perfil traz um texto */
    eTexto = evento_de(s, "updateRadioText");
    fTexto = eTexto && eTexto->m ? acha_campo(eTexto->m, "radioText") : NULL;
    if (fTexto && fTexto->type == T_MSG && fTexto->label == L_UM && acha_campo(fTexto->sub, "radioText") && acha_campo(fTexto->sub, "piSId")) {
        struct val *msg = acha_no(eTexto->vals, fTexto, 0);
        if (msg && *texto(msg->kids, "radioText")) { texto_base = clona(eTexto->vals); texto_da_emissora(0); }
    }
    fprintf(stderr, "[radiostub] modelo do sintonizador: %d emissora(s) de FM e %d de AM na lista do perfil, %s texto de rádio; começo em %lld kHz\n",
            nFM, nAM, texto_base ? "com" : "sem", numero(selecionada(), "frequency", 0));
    return 0;
}

static int modelo_trata(const struct req *r) { return r == rSintoniza || r == rBusca || r == rAtualiza; }

static void modelo_pedido(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    if (r == rSintoniza) sintoniza(r, inst, pb, n);
    else if (r == rBusca) busca(r, inst, pb, n);
    else atualiza_lista(r, inst, pb, n);
    if (nAM && !am_mandada) {                      /* no primeiro pedido atendido, e cada vez que a HMI entra em AM */
        int k = manda_lista_am();
        fprintf(stderr, "[radiostub] mandei a lista de AM (%d emissora(s)) a %d assinatura(s): primeiro pedido atendido, ou a HMI entrou em AM\n", nAM, k);
    }
}
