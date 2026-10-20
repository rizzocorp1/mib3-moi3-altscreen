/* telefonestub.c — o "telefone de mentira" do sandbox, com modelo (12/10/2026).
 *
 * ESTUDO NUM COMPUTADOR. Nada daqui vai para o carro. E é SIMULAÇÃO do começo ao fim: não há
 * Bluetooth, aparelho, pareamento nem chamada de verdade, e o aparelho que aparece é FICTÍCIO (o
 * nome, o endereço e a operadora vêm do perfil, inventados e marcados; ver stubs/telefone/).
 *
 * É o veiculostub com o esquema do telefone, mais um modelo. O motor é o mesmo arquivo, incluído
 * aqui: lê esquema e perfis, anuncia os domínios, responde às assinaturas e aos pedidos que o
 * perfil manda responder. O formato do perfil, o barramento e o modo de uso (-n confere, -m
 * mostra) estão no cabeçalho do veiculostub.c. Sem aparelho no perfil o modelo só liga e desliga
 * o Bluetooth, como o perfil fazia.
 *
 * O que o modelo acrescenta: CONECTAR e DESCONECTAR o aparelho fictício quando a tela pede.
 *
 *   O aparelho é o primeiro da lista de aparelhos conhecidos do perfil
 *   ([DSIPhoneManager.updateKnownDevices]). Os valores de "conectado" são as seções da instância 0
 *   do aparelho ([DSIMobileEquipment[0].…]); os de "desconectado" são as seções sem instância, que
 *   as outras instâncias já recebem. O modelo troca um conjunto pelo outro e avisa quem assinou.
 *   Na lista, o mãos-livres vai a 10 (SERVICESTATE_SUPPORTED | SERVICESTATE_CONNECTED) conectado
 *   e a 2 (só SERVICESTATE_SUPPORTED: conhecido, não conectado) desconectado. Começa conectado se
 *   o perfil traz o bit de conectado (8) no mãos-livres do aparelho; senão começa desconectado.
 *
 *   DSIPhoneManager.requestConnectService (api 1000: identificador, tipo de serviço): é o que a
 *     HMI manda quando se toca no aparelho em "Celulares conhecidos" (rodada e2c), com o aviso
 *     "Estabelecendo conexão…" na tela por até 180 s. Conecta e responde responseConnectService
 *     com resultado 0 (RESULT_OK). Identificador que não é o da lista: 1
 *     (RESULT_ERROR_INVALID_DEVICE_ID). Serviço que não é o mãos-livres (tipo 1,
 *     SERVICETYPE_TELEPHONY_HFP; aceita também o 7, não especificado): 8
 *     (RESULT_ERROR_SERVICE_NOT_SUPPORTED). Bluetooth desligado: 4 (RESULT_ERROR_GENERAL).
 *   DSIPhoneManager.requestDisconnectService (api 1001): desconecta e responde
 *     responseDisconnectService. A tela desta variante não o manda ao tocar no aparelho conectado
 *     (a linha dele é só informação); a HMI o usa em outros caminhos.
 *   DSIBluetooth.requestSwitchBTState (api 1012: o estado pedido): grava o estado no atributo, avisa
 *     e responde, como a regra do perfil fazia; e, se o pedido é desligar, DESCONECTA o aparelho
 *     antes, porque sem Bluetooth não há aparelho conectado. Ligar de novo não reconecta: quem
 *     conecta é a tela.
 *
 * A CHAMADA SIMULADA. A tela disca (teclado "Discar nº" e tecla verde) e encerra:
 *   DSIMobileEquipment.dialNumber (api 1005: o número): só na instância 0 e com o aparelho
 *     conectado (senão resultado 7, RESULT_ERROR_NOT_ALLOWED; com uma chamada em curso, 6,
 *     RESULT_ERROR_SERV_BUSY). Responde responseDialNumber com resultado 0; manda o tipo do número
 *     (updateServiceCodeType: 0, número comum), que é o que a HMI espera para deixar o teclado; põe
 *     o aparelho "em chamada" (estado de ativação 2, ACTIVATIONSTATE_TEL_ACTIVE_CALL); e põe na
 *     lista de chamadas da instância 0 uma chamada, a 1, com o número que a tela mandou, em
 *     "discando" (CALLSTATE_DIALING, 1). Dois segundos depois ela passa a "chamando"
 *     (CALLSTATE_ALERTING, 2) e, aos cinco, a "em chamada" (CALLSTATE_ACTIVE, 4). Ninguém atende
 *     de verdade: é o relógio. Não há áudio, e o número é o que foi digitado; nada é discado.
 *   DSIMobileEquipment.hangupCall (api 1001): tira a chamada da lista, devolve o aparelho a
 *     "ligado" (5) e responde responseHangupCall. Desconectar o aparelho ou desligar o Bluetooth
 *     no meio de uma chamada derruba a chamada antes.
 *   A CHAMADA ACABA SOZINHA aos 20 s do pedido de discar, como se o outro lado desligasse,
 *     ENQUANTO A HMI NÃO OUVE O CARPLAY. É preciso: numa central sem CarPlay na adaptação (as
 *     de teste sem codificação, ou o modo ao vivo com CARPLAY=0) o botão vermelho da tela não
 *     manda o hangupCall. Ao tratá-lo a HMI consulta os estados de chamada do CarPlay e, como a
 *     parte de CarPlay dela não subiu, estoura (NullPointerException em
 *     ASLCarplayServicesImpl.getCarPlayCallStates; rodadas h1, h3 e r1). Sem este fim a chamada
 *     ficaria na tela até a central desligar.
 *     A parte de CarPlay da HMI só sobe com o CarPlay marcado na adaptação da central (campo
 *     ID_APPLE_DIO, DID 0555) e com o serviço DSICarplay2 no barramento (rodada r2); quando sobe,
 *     ela assina o estado de chamadas do CarPlay. O modelo usa essa assinatura como sinal: DEPOIS
 *     DELA O BOTÃO VERMELHO FUNCIONA, e a chamada simulada deixa de acabar por tempo: fica até o
 *     botão, a desconexão do aparelho ou o Bluetooth desligado. É o que vale no modo ao vivo
 *     desde a decisão do Lipe de 12/10/2026 (o CarPlay no padrão, como dedução marcada). O
 *     serviço só existe no esquema stubs/telefone/gerado-carplay/, que o modo ao vivo usa; com
 *     o stubs/telefone/gerado/ (o das centrais de teste) nada disto muda.
 *   A CHAMADA RECEBIDA (12/10/2026). Não há aparelho de verdade para ligar; o gatilho é um número
 *     COMBINADO e fictício, o 5550199 (da faixa 555-01xx, que não é de ninguém): discá-lo sai
 *     como qualquer chamada e, 5 s depois de ela acabar (pelo botão ou por tempo), o aparelho
 *     fictício "liga de volta" do mesmo número. A chamada entra na lista tocando
 *     (CALLSTATE_RINGING, 3) e o aparelho fica "em chamada" (2).
 *   DSIMobileEquipment.acceptCall (api 1000): atende a chamada que toca: ela passa a "em chamada"
 *     (4) e daí em diante é uma chamada como a discada. Sem chamada tocando, resultado 7.
 *     Recusar é o hangupCall com a chamada tocando. Se ninguém atende nem recusa, ela para de
 *     tocar aos 20 s. Desconectar o aparelho ou desligar o Bluetooth cancela a volta marcada.
 *   O PAREAMENTO (12/10/2026). Esta variante não busca aparelhos pela tela; o caminho que ela tem
 *     é o inverso: o aparelho pede e a central confirma um código. Se o perfil traz a seção
 *     [DSIBluetooth.updatePasskeyState] (stubs/telefone/segundo-aparelho.txt: um SEGUNDO aparelho
 *     fictício, com nome, endereço e código inventados), o modelo a guarda e NÃO a manda na
 *     partida. O gatilho é a tela "Celulares conhecidos": ao abri-la a HMI manda
 *     DSIPhoneManager.requestReconnectSuspend com suspend verdadeiro, e ao fechá-la, com falso.
 *     4 s depois de aberta, uma vez por abertura, o modelo publica o pedido guardado (o estado do
 *     código vai a PS_SSP_SHOW_AND_CONFIRM, 7) e a HMI mostra a confirmação.
 *   DSIBluetooth.requestPasskeyResponse (api 1009): "Sim" (1) põe o segundo aparelho na lista de
 *     aparelhos conhecidos, conhecido e não conectado, e ele não pede mais; "Cancelar" (0) não
 *     põe, e ele pede de novo na próxima abertura. Nos dois o estado do código volta a zero, que
 *     é o que a HMI lê (ela não olha a resposta). Fechar a tela com o pedido no ar o retira.
 *     O segundo aparelho só existe para o pareamento: não conecta.
 * Para a HMI mandar discar são precisas duas coisas do perfil, que o modelo só deixa valer com o
 * aparelho conectado (desconectado, saem os valores padrão, como sempre saíram): a agenda "pronta"
 * ([DSIAdb.updateAdbState], 2) e um perfil de agenda ativo ([DSIAdb.updateProfileInfo] e
 * [DSIAdbUserProfile.updateActiveProfile]). O motivo de cada uma está no aparelho-conectado.txt.
 *
 * A LISTA É SEGURADA NA PARTIDA. A HMI guarda o endereço do aparelho principal e o do associado
 * quando processa os dados do aparelho (updatePhoneInformation) das instâncias 0 e 1, e ao receber
 * a lista de aparelhos conhecidos compara cada aparelho com esses endereços sem ver se já os tem.
 * Lista não vazia que chega antes disso dá NullPointerException em
 * TargetASLPhoneConnectionManager.validateCanBePrimaryOrAssociated, e a entrega no barramento na
 * ordem certa não basta: a corrida é dentro da HMI (4 de 7 partidas nas séries "n" e "g"). Por
 * isso, no barramento, o modelo responde à assinatura da lista com a lista VAZIA e só manda a de
 * verdade um a dois segundos depois de as instâncias 0 e 1 terem assinado os dados do aparelho;
 * se isso não acontecer, 6 s depois de a lista ser assinada. Com -n e -m a lista sai como é.
 *
 * Tudo o mais é instantâneo: não há os estados "conectando" e "desconectando" de um rádio de verdade.
 * O modelo acha serviços, eventos, pedidos e campos pelo NOME, no esquema; não escreve byte de
 * mensagem à mão. Se o esquema não for o do telefone, recusa tudo e diz o que faltou. Os números
 * de estado e de resultado são constantes de org.dsi.ifc.phonemanager.DSIPhoneManager e de
 * org.dsi.ifc.bluetooth.DSIBluetooth; estão em stubs/telefone/gerado/constantes.txt.
 *
 * uso: telefonestub <socket> <esquema> [<perfil>...]     fica no barramento
 *      telefonestub -n <esquema> [<perfil>...]           só confere os arquivos (0 = tudo certo)
 *      telefonestub -m <esquema> [<perfil>...]           mostra o que seria mandado
 */
#define VEICULOSTUB_MODELO
#define VEICULOSTUB_MODELO_TEMPO
#include <stdarg.h>
#include "veiculostub.c"

#define HFP_CONHECIDO 2          /* SERVICESTATE_SUPPORTED */
#define HFP_CONECTADO 8          /* SERVICESTATE_CONNECTED */

static struct svc *sPM, *sME, *sBT;
static struct evt *eLista, *eRespConecta, *eRespDesconecta, *eBT, *eRespBT, *eDados;
static struct req *rConecta, *rDesconecta, *rBT;
static struct field *fAparelhos;                   /* updateKnownDevices.knownDevices */
static struct field *fChamadas;                    /* updateCallList.callList */
static char id_aparelho[160];                      /* o uniqueDeviceID do aparelho da lista; vazio = a lista não tem aparelho */
static int conectado;
/* as seções da instância 0 do aparelho: os valores de "conectado", guardados aqui enquanto ele está desconectado */
struct lado { struct porinst *pi; struct val *ligado; int nligado, dligado; };
static struct lado *lados; static int nlados;
/* a lista de aparelhos do perfil, enquanto está segurada (ver o cabeçalho) */
static struct val *lista_presa; static int nlista_presa, presa, tempo_iniciado, prontos;
static time_t assinada_desde;
/* o que só vale com o aparelho conectado e não é da instância 0: a agenda pronta e o perfil de agenda */
struct extra { struct evt *e; struct val *ligado; int nligado; };
static struct extra extras[4]; static int nextras;
/* a chamada simulada */
static struct evt *eChamadas0, *eCodigo0, *eAtivacao0, *eRespDisca, *eRespEncerra;
static struct req *rDisca, *rEncerra;
static int chamada;                                /* 0 = não há; senão o estado dela: 1 discando, 2 chamando, 3 tocando (recebida), 4 em chamada */
#define NUMERO_QUE_LIGA_DE_VOLTA "5550199"         /* fictício (faixa 555-01xx): o gatilho da chamada recebida */
static struct req *rAtende; static struct evt *eRespAtende;
static int liga_de_volta;                          /* a chamada discada foi para o número combinado */
static time_t volta_em;                            /* quando o aparelho fictício "liga de volta"; 0 = não vai */
/* o pareamento: um segundo aparelho fictício pede para parear com a tela "Celulares conhecidos" aberta */
#define ID_DO_SEGUNDO "aparelho-ficticio-2"        /* identificador inventado, como o do primeiro */
static struct evt *ePasskey, *eRespPasskey, *eRespSuspende;
static struct req *rPasskey, *rSuspende;
static struct val *par_vals; static int par_nvals; /* o pedido do perfil, guardado: só sai com a tela aberta */
static char par_nome[96], par_end[40];
static time_t tela_desde;                          /* desde quando a tela de aparelhos está aberta; 0 = fechada */
static int par_pedido, par_no_ar, par_feito;       /* já pediu nesta abertura; o pedido está na tela; já pareou */
static int carplay_ouvido;                         /* a HMI assinou o estado de chamadas do CarPlay: o botão vermelho funciona */
static time_t chamada_desde;
static char numero_chamado[160];
static void poe_chamada(int estado);              /* definida adiante; o gancho de tempo a usa */
static void poe_ativacao(int estado);
static int encerra_chamada(void);
static int avisa_assinantes(struct evt *e);

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

static int avisa_assinantes(struct evt *e)
{
    int i, k = 0;
    for (i = 0; i < nsubs; i++) if (subs[i].e == e) { publica(&subs[i], 1); k++; }
    return k;
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

/* os campos do aparelho da lista do perfil (o primeiro); a lista pode estar segurada */
static struct val *aparelho(void) { struct val *v = acha_no(presa ? lista_presa : eLista->vals, fAparelhos, 0); return v ? v->kids : NULL; }

static void poe_hfp(int estado)
{
    char caminho[96], valor[16];
    struct val *vazia = NULL;
    if (!*id_aparelho) return;
    snprintf(caminho, sizeof caminho, "%s[0].serviceStateHFP", fAparelhos->name);
    snprintf(valor, sizeof valor, "%d", estado);
    if (presa) { vazia = eLista->vals; eLista->vals = lista_presa; }       /* a mudança vai para a lista guardada */
    atribui(eLista, caminho, valor, 0);
    if (presa) { lista_presa = eLista->vals; eLista->vals = vazia; }
}

/* segura a lista (quem assinar recebe a lista vazia) e, depois, solta e avisa quem assinou */
static void prende_lista(void)
{
    lista_presa = eLista->vals; nlista_presa = eLista->nvals;
    eLista->vals = NULL; eLista->nvals = 0; presa = 1;
}
static void solta_lista(const char *por_que)
{
    int avisados;
    libera(eLista->vals);
    eLista->vals = lista_presa; eLista->nvals = nlista_presa; lista_presa = NULL; presa = 0;
    avisados = avisa_assinantes(eLista);
    fprintf(stderr, "[telefonestub] soltei a lista de aparelhos conhecidos (%s); avisei %d assinatura(s)\n", por_que, avisados);
}

/* uma vez por segundo, no barramento: a lista segurada na partida (ver o cabeçalho) */
static void modelo_tempo(time_t t)
{
    int i, tem0 = 0, tem1 = 0, assinada = 0;
    if (!tempo_iniciado) { tempo_iniciado = 1; if (*id_aparelho) prende_lista(); return; }
    for (i = 0; i < nsubs && !carplay_ouvido; i++)         /* só há este serviço no esquema opcional (gerado-carplay) */
        if (!strcmp(subs[i].e->s->name, "DSICarplay2") && !strcmp(subs[i].e->name, "updateCallState")) {
            carplay_ouvido = 1;
            fprintf(stderr, "[telefonestub] a HMI assinou o estado de chamadas do CarPlay: o botão vermelho da tela funciona, e a chamada simulada só acaba pelo botão, ao desconectar o aparelho ou ao desligar o Bluetooth (sem fim por tempo)\n");
        }
    if (chamada == 1 && t - chamada_desde >= 2) {          /* discando -> chamando */
        poe_chamada(2);
        fprintf(stderr, "[telefonestub] a chamada simulada para \"%s\" passou a chamando; avisei %d assinatura(s)\n", numero_chamado, avisa_assinantes(eChamadas0));
    } else if (chamada == 2 && t - chamada_desde >= 5) {   /* chamando -> em chamada: ninguém atende de verdade */
        poe_chamada(4);
        fprintf(stderr, "[telefonestub] a chamada simulada para \"%s\" passou a em chamada; avisei %d assinatura(s)\n", numero_chamado, avisa_assinantes(eChamadas0));
    } else if (chamada == 4 && !carplay_ouvido && t - chamada_desde >= 20) {  /* o fim: como se o outro lado desligasse */
        fprintf(stderr, "[telefonestub] a chamada simulada para \"%s\" acabou sozinha aos 20 s; avisei %d assinatura(s)\n", numero_chamado, encerra_chamada());
        if (liga_de_volta) { liga_de_volta = 0; volta_em = t + 5; }
    } else if (chamada == 3 && t - chamada_desde >= 20) {  /* a recebida que ninguém atendeu nem recusou */
        fprintf(stderr, "[telefonestub] a chamada recebida simulada de \"%s\" parou de tocar aos 20 s: ninguém atendeu; avisei %d assinatura(s)\n", numero_chamado, encerra_chamada());
    }
    if (volta_em && t >= volta_em) {                       /* o número combinado "liga de volta" */
        volta_em = 0;
        if (conectado && !chamada) {
            snprintf(numero_chamado, sizeof numero_chamado, "%s", NUMERO_QUE_LIGA_DE_VOLTA);
            chamada_desde = t; poe_chamada(3); poe_ativacao(2);
            fprintf(stderr, "[telefonestub] chamada recebida simulada de \"%s\" (o número combinado ligou de volta), tocando; avisei %d assinatura(s)\n", numero_chamado,
                    (eAtivacao0 ? avisa_assinantes(eAtivacao0) : 0) + avisa_assinantes(eChamadas0));
        }
    }
    if (par_vals && tela_desde && !par_pedido && !par_feito && !par_no_ar && t - tela_desde >= 4) {
        par_pedido = 1; par_no_ar = 1;                     /* o segundo aparelho pede para parear */
        ePasskey->vals = clona(par_vals); ePasskey->nvals = par_nvals;
        fprintf(stderr, "[telefonestub] o segundo aparelho fictício, \"%s\", pede para parear (a tela de aparelhos está aberta há 4 s); avisei %d assinatura(s)\n", par_nome, avisa_assinantes(ePasskey));
    }
    if (!presa) return;
    for (i = 0; i < nsubs; i++) {
        struct evt *e = subs[i].e;
        int k;
        if (e == eLista) assinada = 1;
        for (k = 0; k < nporinsts && e != eDados; k++) if (&porinsts[k]->e == e) e = porinsts[k]->base;
        if (e == eDados && subs[i].inst == 0) tem0 = 1;
        if (e == eDados && subs[i].inst == 1) tem1 = 1;
    }
    if (assinada && !assinada_desde) assinada_desde = t;
    if (tem0 && tem1) prontos++;
    if (prontos >= 2) solta_lista("as instâncias 0 e 1 já receberam os dados do aparelho");
    else if (assinada_desde && t - assinada_desde >= 6) solta_lista("6 s depois de assinada, sem as duas instâncias terem pedido os dados do aparelho");
}

/* guarda os valores de "conectado" de cada seção da instância 0 e põe no lugar os que as outras instâncias recebem */
static void guarda_lados(void)
{
    int i;
    for (i = 0; i < nlados; i++) {
        struct porinst *pi = lados[i].pi;
        lados[i].ligado = pi->e.vals; lados[i].nligado = pi->e.nvals; lados[i].dligado = pi->e.dyn;
        pi->e.vals = clona(pi->base->vals); pi->e.nvals = pi->base->nvals; pi->e.dyn = pi->base->dyn;
    }
    for (i = 0; i < nextras; i++) {                /* a agenda volta ao valor padrão */
        extras[i].ligado = extras[i].e->vals; extras[i].nligado = extras[i].e->nvals;
        extras[i].e->vals = NULL; extras[i].e->nvals = 0;
    }
}
static void devolve_lados(void)
{
    int i;
    for (i = 0; i < nlados; i++) {
        struct porinst *pi = lados[i].pi;
        libera(pi->e.vals);
        pi->e.vals = lados[i].ligado; pi->e.nvals = lados[i].nligado; pi->e.dyn = lados[i].dligado;
        lados[i].ligado = NULL;
    }
    for (i = 0; i < nextras; i++) {
        libera(extras[i].e->vals);
        extras[i].e->vals = extras[i].ligado; extras[i].e->nvals = extras[i].nligado; extras[i].ligado = NULL;
    }
}
static int avisa_lados(void)
{
    int i, k = 0;
    for (i = 0; i < nlados; i++) k += avisa_assinantes(&lados[i].pi->e);
    for (i = 0; i < nextras; i++) k += avisa_assinantes(extras[i].e);
    return k;
}

/* ---- a chamada simulada ---- */
static void poe_chamada(int estado)
{
    char caminho[64], valor[176];
    libera(eChamadas0->vals); eChamadas0->vals = NULL; eChamadas0->nvals = 0;
    chamada = estado;
    if (!estado) return;
    snprintf(caminho, sizeof caminho, "callList[0].telCallID"); snprintf(valor, sizeof valor, "1"); atribui(eChamadas0, caminho, valor, 0);
    snprintf(caminho, sizeof caminho, "callList[0].telCallState"); snprintf(valor, sizeof valor, "%d", estado); atribui(eChamadas0, caminho, valor, 0);
    snprintf(caminho, sizeof caminho, "callList[0].telRemNumber"); snprintf(valor, sizeof valor, "%s", numero_chamado); atribui(eChamadas0, caminho, valor, 1);
}
static void poe_ativacao(int estado)               /* 5 = ligado, 2 = em chamada; só mexe nos valores de "conectado" */
{
    char caminho[64], valor[16];
    if (!eAtivacao0 || !conectado) return;
    snprintf(caminho, sizeof caminho, "activationState.telActivationState"); snprintf(valor, sizeof valor, "%d", estado);
    atribui(eAtivacao0, caminho, valor, 0);
}
/* acaba com a chamada, se há uma; devolve quantas assinaturas foram avisadas */
static int encerra_chamada(void)
{
    if (!chamada) return 0;
    poe_chamada(0); poe_ativacao(5);
    return avisa_assinantes(eChamadas0) + (eAtivacao0 ? avisa_assinantes(eAtivacao0) : 0);
}

/* as duas trocas de estado; devolvem quantas assinaturas foram avisadas (0 se já estava assim) */
static int desconecta(void)
{
    int k;
    if (!conectado) return 0;
    liga_de_volta = 0; volta_em = 0;               /* sem aparelho ninguém liga de volta */
    k = encerra_chamada();                         /* a chamada cai antes do aparelho */
    guarda_lados(); poe_hfp(HFP_CONHECIDO); conectado = 0;
    return k + avisa_lados() + (*id_aparelho ? avisa_assinantes(eLista) : 0);
}
static int conecta(void)
{
    if (conectado) return 0;
    devolve_lados(); poe_hfp(HFP_CONHECIDO | HFP_CONECTADO); conectado = 1;
    return avisa_lados() + (*id_aparelho ? avisa_assinantes(eLista) : 0);
}

/* confere que o esquema tem tudo o que o modelo usa, pelo nome; senão diz o que faltou e recusa */
static int modelo_inicia(void)
{
    const char *falta = NULL, *err = NULL;
    struct val *ap;
    int i;
    sPM = acha_svc("DSIPhoneManager"); sME = acha_svc("DSIMobileEquipment"); sBT = acha_svc("DSIBluetooth");
    if (!sPM || !sME || !sBT) falta = "um dos serviços DSIPhoneManager, DSIMobileEquipment e DSIBluetooth";
    else if (!(eLista = evento_de(sPM, "updateKnownDevices")) || !eLista->m) falta = "o evento updateKnownDevices";
    else if (!(fAparelhos = acha_campo(eLista->m, "knownDevices")) || fAparelhos->type != T_MSG || fAparelhos->label == L_UM) falta = "o campo knownDevices (lista de mensagens) em updateKnownDevices";
    else if (!acha_campo(fAparelhos->sub, "uniqueDeviceID") || !acha_campo(fAparelhos->sub, "serviceStateHFP")) falta = "um dos campos uniqueDeviceID e serviceStateHFP no aparelho da lista";
    else if (!(rConecta = pedido_de(sPM, "requestConnectService")) || !rConecta->m || !folha(rConecta->m, "uniqueDeviceID", &err) || !folha(rConecta->m, "serviceType", &err))
        falta = "o pedido requestConnectService, com uniqueDeviceID e serviceType";
    else if (!(rDesconecta = pedido_de(sPM, "requestDisconnectService")) || !rDesconecta->m || !folha(rDesconecta->m, "uniqueDeviceID", &err))
        falta = "o pedido requestDisconnectService, com uniqueDeviceID";
    else if (!(eRespConecta = evento_de(sPM, "responseConnectService")) || !eRespConecta->m || !folha(eRespConecta->m, "result", &err)) falta = "o evento responseConnectService, com result";
    else if (!(eRespDesconecta = evento_de(sPM, "responseDisconnectService")) || !eRespDesconecta->m || !folha(eRespDesconecta->m, "result", &err)) falta = "o evento responseDisconnectService, com result";
    else if (!(eBT = evento_de(sBT, "updateBTState")) || !eBT->m || !folha(eBT->m, "bTState", &err)) falta = "o evento updateBTState, com bTState";
    else if (!(rBT = pedido_de(sBT, "requestSwitchBTState")) || !rBT->m || !folha(rBT->m, "btStateTarget", &err)) falta = "o pedido requestSwitchBTState, com btStateTarget";
    else if (!(eRespBT = evento_de(sBT, "responseSwitchBTState")) || !eRespBT->m) falta = "o evento responseSwitchBTState";
    else if (!(eDados = evento_de(sME, "updatePhoneInformation")) || !eDados->m) falta = "o evento updatePhoneInformation";
    else if (!(rDisca = pedido_de(sME, "dialNumber")) || !rDisca->m || !folha(rDisca->m, "telNumber", &err)) falta = "o pedido dialNumber, com telNumber";
    else if (!(rEncerra = pedido_de(sME, "hangupCall")) || !rEncerra->m) falta = "o pedido hangupCall";
    else if (!(eRespDisca = evento_de(sME, "responseDialNumber")) || !eRespDisca->m || !folha(eRespDisca->m, "result", &err)) falta = "o evento responseDialNumber, com result";
    else if (!(eRespEncerra = evento_de(sME, "responseHangupCall")) || !eRespEncerra->m) falta = "o evento responseHangupCall";
    else if (!(rAtende = pedido_de(sME, "acceptCall")) || !rAtende->m) falta = "o pedido acceptCall";
    else if (!(eRespAtende = evento_de(sME, "responseAcceptCall")) || !eRespAtende->m || !folha(eRespAtende->m, "result", &err)) falta = "o evento responseAcceptCall, com result";
    else if (!evento_de(sME, "updateCallList") || !evento_de(sME, "updateCallList")->m || !(fChamadas = acha_campo(evento_de(sME, "updateCallList")->m, "callList")) || fChamadas->type != T_MSG
             || !acha_campo(fChamadas->sub, "telCallID") || !acha_campo(fChamadas->sub, "telCallState") || !acha_campo(fChamadas->sub, "telRemNumber"))
        falta = "o evento updateCallList, com callList[].telCallID, telCallState e telRemNumber";
    else if (!evento_de(sME, "updateServiceCodeType") || !evento_de(sME, "updateServiceCodeType")->m) falta = "o evento updateServiceCodeType";
    if (falta) {
        fprintf(stderr, "[telefonestub] este programa é o telefone de mentira e o esquema dado não é o do telefone: falta %s. O esquema certo é o stubs/telefone/gerado/esquema.txt; nada foi enviado\n", falta);
        return -1;
    }
    for (i = 0; i < nporinsts; i++)
        if (porinsts[i]->base->s == sME && porinsts[i]->inst == 0) {
            lados = cresce(lados, nlados, sizeof *lados);
            lados[nlados].pi = porinsts[i]; lados[nlados].ligado = NULL; lados[nlados].nligado = 0; lados[nlados].dligado = 0;
            nlados++;
        }
    /* a chamada: a lista de chamadas e o tipo do número só da instância 0; o estado de ativação dela é uma das seções do perfil */
    for (i = 0; i < nlados; i++) if (lados[i].pi->base == evento_de(sME, "updateActivationState")) eAtivacao0 = &lados[i].pi->e;
    eChamadas0 = da_instancia(evento_de(sME, "updateCallList"), 0, 1);
    eCodigo0 = da_instancia(evento_de(sME, "updateServiceCodeType"), 0, 1);
    /* o que só vale com o aparelho conectado: a agenda pronta e o perfil de agenda, se o perfil os traz */
    {
        static const char *QUAIS[][2] = { { "DSIAdb", "updateAdbState" }, { "DSIAdb", "updateProfileInfo" }, { "DSIAdbUserProfile", "updateActiveProfile" } };
        unsigned q;
        for (q = 0; q < sizeof QUAIS / sizeof QUAIS[0]; q++) {
            struct svc *sv = acha_svc(QUAIS[q][0]);
            struct evt *e = sv ? evento_de(sv, QUAIS[q][1]) : NULL;
            if (e && e->vals) { extras[nextras].e = e; extras[nextras].ligado = NULL; extras[nextras].nligado = 0; nextras++; }
        }
    }
    /* o pareamento: o pedido do segundo aparelho, se o perfil o traz, fica guardado até a tela de aparelhos abrir */
    rSuspende = pedido_de(sPM, "requestReconnectSuspend"); eRespSuspende = evento_de(sPM, "responseReconnectSuspend");
    rPasskey = pedido_de(sBT, "requestPasskeyResponse"); eRespPasskey = evento_de(sBT, "responsePasskeyResponse");
    ePasskey = evento_de(sBT, "updatePasskeyState");
    if (ePasskey && ePasskey->vals && rSuspende && rSuspende->m && eRespSuspende && eRespSuspende->m && rPasskey && rPasskey->m && eRespPasskey && eRespPasskey->m) {
        struct val *v;
        par_vals = ePasskey->vals; par_nvals = ePasskey->nvals; ePasskey->vals = NULL; ePasskey->nvals = 0;
        for (v = par_vals; v; v = v->next)
            if (!strcmp(v->f->name, "passkeyState")) {
                snprintf(par_nome, sizeof par_nome, "%s", texto(v->kids, "btDeviceName"));
                snprintf(par_end, sizeof par_end, "%s", texto(v->kids, "btDeviceAddress"));
            }
    } else { rSuspende = NULL; rPasskey = NULL; }  /* sem segundo aparelho no perfil, o motor responde como sempre */
    ap = aparelho();
    snprintf(id_aparelho, sizeof id_aparelho, "%s", texto(ap, "uniqueDeviceID"));
    if (!*id_aparelho) {
        conectado = nlados > 0;                    /* seções da instância 0 sem aparelho na lista: valem como estão */
        if (nlados) fprintf(stderr, "[telefonestub] modelo do telefone: a lista do perfil não tem aparelho, mas há %d seção(ões) da instância 0: começa conectado, e só desligar o Bluetooth desconecta\n", nlados);
        else fprintf(stderr, "[telefonestub] modelo do telefone: nenhum aparelho no perfil; o Bluetooth liga e desliga, e não há o que conectar\n");
        return 0;
    }
    conectado = (numero(ap, "serviceStateHFP", 0) & HFP_CONECTADO) != 0;
    if (!conectado) guarda_lados();
    fprintf(stderr, "[telefonestub] modelo do telefone: o aparelho \"%s\" (\"%s\"), com %d seção(ões) da instância 0 do aparelho; começa %s\n",
            id_aparelho, texto(ap, "deviceName"), nlados, conectado ? "conectado" : "desconectado");
    return 0;
}

/* a resposta ao código de pareamento: o endereço, o nome e o resultado (0 não vai no fio) */
static void responde_codigo(uint32_t inst, const char *end, const char *nome, int result)
{
    struct evt tmp = *eRespPasskey;
    struct buf b = { NULL, 0, 0 };
    struct tm tm;
    char c1[] = "btDeviceAddress", c2[] = "btDeviceName", c3[] = "result", v[176];
    tmp.vals = NULL; tmp.nvals = 0;
    snprintf(v, sizeof v, "%s", end); atribui(&tmp, c1, v, 1);
    if (*nome) { snprintf(v, sizeof v, "%s", nome); atribui(&tmp, c2, v, 1); }
    if (result) { snprintf(v, sizeof v, "%d", result); atribui(&tmp, c3, v, 0); }
    agora(&tm);
    poe_msg(&b, tmp.m, tmp.vals, 0, &tm);
    manda(eRespPasskey->s->dom, eRespPasskey->s->evt, inst, eRespPasskey->api, b.p, b.n);
    libera(tmp.vals);
    free(b.p);
}
/* tira o pedido de pareamento da tela: o estado do código volta ao padrão (zero); devolve quantos foram avisados */
static int retira_pedido(void)
{
    if (!par_no_ar) return 0;
    libera(ePasskey->vals); ePasskey->vals = NULL; ePasskey->nvals = 0; par_no_ar = 0;
    return avisa_assinantes(ePasskey);
}
/* o segundo aparelho entra na lista de aparelhos conhecidos, depois do primeiro: conhecido e não conectado */
static void poe_segundo_na_lista(void)
{
    char caminho[96], valor[176];
    struct val *vazia = NULL;
    if (presa) { vazia = eLista->vals; eLista->vals = lista_presa; }
    snprintf(caminho, sizeof caminho, "%s[1].btDeviceAddress", fAparelhos->name); snprintf(valor, sizeof valor, "%s", par_end); atribui(eLista, caminho, valor, 1);
    snprintf(caminho, sizeof caminho, "%s[1].deviceName", fAparelhos->name); snprintf(valor, sizeof valor, "%s", par_nome); atribui(eLista, caminho, valor, 1);
    snprintf(caminho, sizeof caminho, "%s[1].uniqueDeviceID", fAparelhos->name); snprintf(valor, sizeof valor, "%s", ID_DO_SEGUNDO); atribui(eLista, caminho, valor, 1);
    snprintf(caminho, sizeof caminho, "%s[1].serviceStateHFP", fAparelhos->name); snprintf(valor, sizeof valor, "%d", HFP_CONHECIDO); atribui(eLista, caminho, valor, 0);
    if (presa) { lista_presa = eLista->vals; eLista->vals = vazia; }
}

static int modelo_trata(const struct req *r)
{
    return r == rConecta || r == rDesconecta || r == rBT || r == rDisca || r == rEncerra || r == rAtende
        || (rSuspende && r == rSuspende) || (rPasskey && r == rPasskey);
}

static void modelo_pedido(struct req *r, uint32_t inst, const unsigned char *pb, int n)
{
    char v[160], tipo[32], num[16];
    int avisados = 0, res;
    if (rSuspende && r == rSuspende) {                 /* a tela "Celulares conhecidos" abriu (verdadeiro) ou fechou (falso) */
        int abre;
        le_folha(r->m, pb, n, "suspend", v, sizeof v); abre = *v && *v != '0' && *v != 'f';
        responde_com(eRespSuspende, inst, NULL);
        if (abre) { if (!tela_desde) { tela_desde = time(NULL); par_pedido = 0; } }
        else { tela_desde = 0; avisados = retira_pedido(); }
        fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): a tela de aparelhos %s; respondi %s (api %u)%s\n", r->s->name, r->name, r->api,
                abre ? "abriu" : "fechou", eRespSuspende->name, eRespSuspende->api, !abre && avisados ? " e retirei o pedido de pareamento que estava na tela" : "");
        return;
    }
    if (rPasskey && r == rPasskey) {                   /* "Sim" (1) ou "Cancelar" (0) na confirmação do código */
        char resp[16], end[64];
        le_folha(r->m, pb, n, "btAuthenticationResponse", resp, sizeof resp);
        le_folha(r->m, pb, n, "btDeviceAddress", end, sizeof end);
        if (!par_no_ar || strcmp(end, par_end)) {
            responde_codigo(inst, end, "", 4);         /* RESULT_ERROR_PAIRING_GENERAL */
            fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): não há pedido de pareamento no ar para \"%s\": resultado 4\n", r->s->name, r->name, r->api, end);
            return;
        }
        responde_codigo(inst, par_end, par_nome, 0);
        avisados = retira_pedido();
        if (atoi(resp)) {
            par_feito = 1; poe_segundo_na_lista(); avisados += avisa_assinantes(eLista);
            fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): código confirmado: pareei o segundo aparelho fictício, \"%s\", que entrou na lista de conhecidos; avisei %d assinatura(s)\n", r->s->name, r->name, r->api, par_nome, avisados);
        } else
            fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): pareamento recusado na tela: \"%s\" não entra na lista; avisei %d assinatura(s)\n", r->s->name, r->name, r->api, par_nome, avisados);
        return;
    }
    if (r == rBT) {
        char caminho[] = "bTState", valor[32];
        long long alvo;
        int caiu = 0;
        le_folha(r->m, pb, n, "btStateTarget", v, sizeof v); alvo = atoll(v);
        if (alvo != 0 && conectado) { avisados += desconecta(); caiu = 1; }      /* sem Bluetooth não há aparelho conectado */
        snprintf(valor, sizeof valor, "%lld", alvo);
        atribui(eBT, caminho, valor, 0);
        avisados += avisa_assinantes(eBT);
        responde_com(eRespBT, inst, NULL);
        fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): estado do Bluetooth = %lld%s; avisei %d assinatura(s) e respondi %s (api %u)\n", r->s->name, r->name, r->api,
                alvo, caiu ? ", e o aparelho fictício caiu junto" : "", avisados, eRespBT->name, eRespBT->api);
        return;
    }
    if (r == rDisca) {
        const char *que;
        le_folha(r->m, pb, n, "telNumber", v, sizeof v);
        if (inst != 0 || !conectado) { res = 7; que = "não há aparelho conectado nesta instância: resultado 7"; }
        else if (chamada) { res = 6; que = "já há uma chamada simulada em curso: resultado 6"; }
        else if (!*v) { res = 4; que = "número vazio: resultado 4"; }
        else { res = 0; que = "chamada simulada, discando"; }
        snprintf(num, sizeof num, "%d", res);
        responde_com(eRespDisca, inst, "result", num, NULL);
        if (!res) {
            snprintf(numero_chamado, sizeof numero_chamado, "%s", v);
            liga_de_volta = !strcmp(v, NUMERO_QUE_LIGA_DE_VOLTA); volta_em = 0;
            chamada_desde = time(NULL);
            poe_chamada(1); poe_ativacao(2);
            avisados = avisa_assinantes(eCodigo0) + (eAtivacao0 ? avisa_assinantes(eAtivacao0) : 0) + avisa_assinantes(eChamadas0);
        }
        fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): \"%s\": %s; respondi %s (api %u) e avisei %d assinatura(s)\n", r->s->name, r->name, r->api,
                v, que, eRespDisca->name, eRespDisca->api, avisados);
        return;
    }
    if (r == rEncerra) {
        int havia = chamada;
        avisados = encerra_chamada();
        if (havia && liga_de_volta) { liga_de_volta = 0; volta_em = time(NULL) + 5; }
        responde_com(eRespEncerra, inst, NULL);
        fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): %s%s; avisei %d assinatura(s) e respondi %s (api %u)\n", r->s->name, r->name, r->api,
                havia == 3 ? "recusei a chamada recebida simulada" : havia ? "encerrei a chamada simulada" : "não havia chamada",
                volta_em ? "; o número combinado liga de volta em 5 s" : "", avisados, eRespEncerra->name, eRespEncerra->api);
        return;
    }
    if (r == rAtende) {
        const char *que;
        if (inst != 0 || !conectado || chamada != 3) { res = 7; que = "não há chamada tocando nesta instância: resultado 7"; }
        else { res = 0; que = "atendi a chamada recebida simulada"; }
        snprintf(num, sizeof num, "%d", res);
        responde_com(eRespAtende, inst, "result", num, NULL);
        if (!res) { chamada_desde = time(NULL); poe_chamada(4); avisados = avisa_assinantes(eChamadas0); }
        fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): %s; respondi %s (api %u) e avisei %d assinatura(s)\n", r->s->name, r->name, r->api, que, eRespAtende->name, eRespAtende->api, avisados);
        return;
    }
    le_folha(r->m, pb, n, "uniqueDeviceID", v, sizeof v);
    le_folha(r->m, pb, n, "serviceType", tipo, sizeof tipo);
    if (r == rConecta) {
        const char *que;
        if (!*id_aparelho || strcmp(v, id_aparelho)) { res = 1; que = "não é o aparelho da lista: resultado 1"; }
        else if (atoll(tipo) != 1 && atoll(tipo) != 7) { res = 8; que = "só o mãos-livres conecta: resultado 8"; }
        else if (numero(eBT->vals, "bTState", 0) != 0) { res = 4; que = "o Bluetooth está desligado: resultado 4"; }
        else { avisados = conecta(); res = 0; que = "conectei o aparelho fictício"; }
        snprintf(num, sizeof num, "%d", res);
        responde_com(eRespConecta, inst, "result", num, NULL);
        fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): \"%s\", serviço %s: %s; avisei %d assinatura(s) e respondi %s (api %u)\n", r->s->name, r->name, r->api,
                v, tipo, que, avisados, eRespConecta->name, eRespConecta->api);
        return;
    }
    if (!*id_aparelho || strcmp(v, id_aparelho)) res = 1;
    else { avisados = desconecta(); res = 0; }
    snprintf(num, sizeof num, "%d", res);
    responde_com(eRespDesconecta, inst, "result", num, NULL);
    fprintf(stderr, "[telefonestub] pedido atendido: %s.%s (api %u): \"%s\": %s; avisei %d assinatura(s) e respondi %s (api %u)\n", r->s->name, r->name, r->api,
            v, res ? "não é o aparelho da lista: resultado 1" : "desconectei o aparelho fictício", avisados, eRespDesconecta->name, eRespDesconecta->api);
}
