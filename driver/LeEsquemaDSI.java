/* LeEsquemaDSI.java — tira das classes do próprio firmware o esquema dos serviços DSI de um
 * domínio: interfaces, números de api, atributos e os campos de cada mensagem protobuf.
 * Marco 8, 11/10/2026. Estudo num computador; só lê o firmware, nada vai para o carro.
 *
 * Roda na JVM do firmware (driver/m8_esquema.sh), sem decompilador:
 *  - o bytecode é lido com o javap da própria JVM (/lge/hmi/jvm/lib/tools.jar):
 *      ListenerWorkerThread.run        que método trata cada domínio
 *      …process…ListenerService        que classe trata cada interface de eventos, e que
 *                                      serviços o anúncio (interface 9000) liga
 *      <Serviço>ListenerService        api -> mensagem, atributo e método update… do ouvinte
 *    O código da LG tem duas formas. Nos serviços de veículo o process… tem um caso por interface
 *    de eventos e cada ouvinte tem um executeListenerService(Message, int) com um switch de api.
 *    No sintonizador (Marco 11) o process… só tem o caso do anúncio; o resto cai no default, que
 *    entrega a mensagem a uma classe intermediária (DSITunerListenerService). É ela que separa por
 *    interface, e cada ouvinte tem um método próprio (UnifiedTunerMsgParse…), com um switch para as
 *    apis abaixo de 2000 e outro para as demais. O programa segue as duas.
 *    Nos domínios do telefone (12/10/2026: PID_BT, 256, e PID_DCMANAGER, 4096) aparecem mais três
 *    coisas. O despacho do 256 entrega também as interfaces de eventos de serviços de outros donos
 *    (mídia, UPnP e busca): só fica no esquema o serviço que o anúncio do domínio liga (linhas W),
 *    e os outros vão no relatório. Na agenda e nas mensagens o ouvinte procura quem assinou com
 *    getUpdateListenerList(<o …Impl da instância>, atributo), e não com (instância, atributo). E em
 *    DSIMobileEquipmentImpl os pedidos passam por um método auxiliar, messageToBytes(api, mensagem),
 *    que é quem chama postDSIProviderMsg, com um domínio que depende da instância: o programa lê a
 *    api em cada chamada ao auxiliar e deixa no relatório para que domínio vai cada instância.
 *      <Serviço>Impl                   pedidos: api, interface e domínio de cada método
 *  - que pedidos têm resposta, qual é ela e em quanto tempo a HMI desiste (Marco 9) vem da tabela
 *    do proxy DSI da própria HMI (de.vw.mib.asl.framework.internal.framework.dsi.util.Cfg, as
 *    mesmas chamadas que o DSIProxyTarget faz para registrar "DSI timeout occurred"), lida por
 *    reflexão e casada pelo nome do método com os pedidos e eventos da camada da LG;
 *  - o que cada pedido set<X> grava (linhas G) e o que cada resposta devolve do pedido (linhas X)
 *    o firmware não diz: são DEDUÇÃO deste programa, pela forma das mensagens, e vão marcadas;
 *  - as mensagens protobuf são GeneratedMessageLite (sem descritor). Os campos vêm das
 *    constantes …_FIELD_NUMBER e dos campos da classe; a codificação de cada um é PROVADA:
 *    o programa monta a mensagem com o Builder do firmware, serializa e olha os bytes
 *    (-1 num sint32 sai "01"; num int32, dez bytes);
 *  - os nomes dos atributos vêm das constantes ATTR_ das interfaces org.dsi.ifc.
 *
 * Saídas (pasta do 2º argumento):
 *   esquema.txt            o esquema, uma coisa por linha (formato no cabeçalho do arquivo)
 *   vetores-perfil.txt     um perfil de teste que dá valor a TODA folha de todo evento
 *   vetores-esperado.txt   os bytes que as classes do firmware produzem para esse perfil
 *   constantes.txt         os nomes dos valores (unidades, estados, tipos…) das interfaces org.dsi.ifc
 *   relatorio.txt          contagens e avisos
 * Os dois arquivos de vetores são a prova do veiculostub: com o mesmo perfil ele tem de
 * produzir os mesmos bytes (driver/tests/t_veiculo.sh).
 *
 * uso: LeEsquemaDSI <MIBHMI-DSI.jar> <pasta de saída> [domínios, separados por vírgula; padrão 16]
 *                    [como chamar os serviços no cabeçalho dos arquivos; padrão "de veículo"]
 */
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class LeEsquemaDSI {
    static final String PK = "com.lge.mib3oi.dsi.";
    static String jar;
    static Method javapRun;
    static final List<String> avisos = new ArrayList<String>();

    static void aviso(String s) { avisos.add(s); System.err.println("AVISO: " + s); }

    // ------------------------------------------------------------------ javap
    static final class Ins {
        int off; String op, rest;
        TreeMap<Integer, Integer> sw;      // só nos switch: valor do caso -> destino
        int swDefault = -1;
    }

    static final Pattern P_INS = Pattern.compile("^\\s+(\\d+): ([a-z_][a-z_0-9]*)\\s*(.*)$");
    static final Pattern P_CASO = Pattern.compile("^\\s+(-?\\d+|default): (\\d+)$");

    /** cabeçalho de cada método -> instruções, na ordem do arquivo */
    static LinkedHashMap<String, List<Ins>> javap(String cls) throws Exception {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        Object rc = javapRun.invoke(null, new Object[] { new String[] { "-c", "-p", "-classpath", jar, cls }, pw });
        pw.flush();
        if (((Integer) rc).intValue() != 0) throw new IllegalStateException("javap falhou em " + cls + ": " + sw);
        LinkedHashMap<String, List<Ins>> ms = new LinkedHashMap<String, List<Ins>>();
        List<Ins> cur = null;
        Ins swi = null;
        for (String l : sw.toString().split("\n")) {
            if (swi != null) {
                Matcher m = P_CASO.matcher(l);
                if (m.matches()) {
                    if (m.group(1).equals("default")) swi.swDefault = Integer.parseInt(m.group(2));
                    else swi.sw.put(Integer.valueOf(m.group(1)), Integer.valueOf(m.group(2)));
                } else if (l.trim().equals("}")) swi = null;
                continue;
            }
            if (l.startsWith("  ") && !l.startsWith("   ") && l.indexOf('(') >= 0 && l.trim().endsWith(";")) {
                cur = new ArrayList<Ins>();
                String k = l.trim();
                while (ms.containsKey(k)) k += " ";
                ms.put(k, cur);
                continue;
            }
            Matcher m = P_INS.matcher(l);
            if (cur == null || !m.matches()) continue;
            Ins i = new Ins();
            i.off = Integer.parseInt(m.group(1)); i.op = m.group(2); i.rest = m.group(3);
            cur.add(i);
            if (i.op.equals("lookupswitch") || i.op.equals("tableswitch")) { i.sw = new TreeMap<Integer, Integer>(); swi = i; }
        }
        return ms;
    }

    static List<Ins> metodo(Map<String, List<Ins>> ms, String trecho) {
        for (Map.Entry<String, List<Ins>> e : ms.entrySet()) if (e.getKey().indexOf(trecho) >= 0) return e.getValue();
        return null;
    }

    static final Pattern P_INT = Pattern.compile("//\\s+int\\s+(-?\\d+)");
    static final Pattern P_LONG = Pattern.compile("//\\s+long\\s+(-?\\d+)l");

    static Integer intConst(Ins i) {
        if (i.op.equals("iconst_m1")) return Integer.valueOf(-1);
        if (i.op.startsWith("iconst_")) return Integer.valueOf(i.op.substring(7));
        if (i.op.equals("bipush") || i.op.equals("sipush")) return Integer.valueOf(i.rest.trim());
        if (i.op.equals("ldc") || i.op.equals("ldc_w")) { Matcher m = P_INT.matcher(i.rest); if (m.find()) return Integer.valueOf(m.group(1)); }
        return null;
    }

    static Long longConst(Ins i) {
        if (i.op.equals("lconst_0")) return Long.valueOf(0);
        if (i.op.equals("lconst_1")) return Long.valueOf(1);
        if (i.op.equals("ldc2_w")) { Matcher m = P_LONG.matcher(i.rest); if (m.find()) return Long.valueOf(m.group(1)); }
        return null;
    }

    static Ins primeiroSwitch(List<Ins> c) {
        for (Ins i : c) if (i.sw != null) return i;
        return null;
    }

    /** onde acaba o trecho de um caso: no próximo destino do mesmo switch */
    static int fimDoCaso(Ins s, int alvo) {
        TreeSet<Integer> alvos = new TreeSet<Integer>(s.sw.values());
        if (s.swDefault >= 0) alvos.add(Integer.valueOf(s.swDefault));
        Integer prox = alvos.higher(Integer.valueOf(alvo));
        return prox == null ? Integer.MAX_VALUE : prox.intValue();
    }

    // ------------------------------------------------------------------ modelo
    /** attr: atributo que o evento atualiza (o número da api, quando existe a constante ATTR_ e o
     *  caso entrega a quem assinou). entrega: o número que a HMI usa para achar os ouvintes; quase
     *  sempre é o mesmo, e quando não é a atualização só chega a quem assinou esse outro atributo. */
    static final class Evento { int api; Integer attr, entrega; String metodo, sig, ifc, msg, rotulo; }
    static final class Pedido { int api; String metodo, msg; }
    /** o que a HMI espera de um pedido: tipo, prazo em ms e, para cada resposta possível, o método do
     *  ouvinte e o evento da camada da LG que o chama (null: nenhuma api chama esse método) */
    static final class Resposta {
        Pedido p; String tipo; int prazo;
        List<String> nomes = new ArrayList<String>();
        List<Evento> evs = new ArrayList<Evento>();
    }
    static final class Servico {
        String nome, pacote, ouvinte, ifc;         // DSICarKombi, carkombi, classe do ListenerService, interface org.dsi.ifc
        String parse;                              // método do ouvinte que separa as apis, quando não é o executeListenerService
        long dominio; int ifPedidos = -1, ifEventos;
        List<Evento> eventos = new ArrayList<Evento>();
        List<Pedido> pedidos = new ArrayList<Pedido>();
        TreeMap<Integer, String> atributos = new TreeMap<Integer, String>();
        List<Resposta> respostas = new ArrayList<Resposta>();
        List<String> linhasG = new ArrayList<String>(), linhasX = new ArrayList<String>();   // já no formato do esquema
        List<String> setSemRegra = new ArrayList<String>(), relato = new ArrayList<String>();
        int nSet, gPosicao, gNomes;
    }
    static final class Campo { int num; String nome, tipo, rotulo; Class<?> sub; }

    static final LinkedHashMap<Class<?>, List<Campo>> mensagens = new LinkedHashMap<Class<?>, List<Campo>>();

    static String nomeMsg(String cls) {            // com.lge.mib3oi.dsi.x.proto.AMsg$b -> AMsg.b
        int p = cls.lastIndexOf('.');
        return cls.substring(p + 1).replace('$', '.');
    }

    // ------------------------------------------------------------------ ListenerWorkerThread
    static final Pattern P_PROC = Pattern.compile("Method (process\\w+):\\(Lcom/lge/mib3oi/dsi/Message;\\)V");
    static final Pattern P_NEW = Pattern.compile("// class (com/lge/mib3oi/dsi/[\\w/$]+)");
    static final Pattern P_CLS = Pattern.compile("// class (org/dsi/ifc/[\\w/$]+)");

    /** linhas "W": que serviços o anúncio de cada domínio liga (classe org.dsi.ifc, instância, atraso) */
    static final List<String> ligados = new ArrayList<String>();

    static List<Servico> leDespacho(Set<Long> quais) throws Exception {
        LinkedHashMap<String, List<Ins>> ms = javap(PK + "ListenerWorkerThread");
        Map<String, List<Long>> dominios = new HashMap<String, List<Long>>();
        Long ult = null;
        for (Ins i : metodo(ms, " run()")) {
            Long l = longConst(i);
            if (l != null) ult = l;
            Matcher m = P_PROC.matcher(i.rest);
            if (i.op.equals("invokespecial") && m.find() && ult != null) {
                List<Long> ds = dominios.get(m.group(1));
                if (ds == null) dominios.put(m.group(1), ds = new ArrayList<Long>());
                ds.add(ult);
            }
        }
        List<Servico> out = new ArrayList<Servico>();
        for (Map.Entry<String, List<Ins>> e : ms.entrySet()) {
            String nomeMet = null;
            for (String k : dominios.keySet()) if (e.getKey().indexOf(" " + k + "(") >= 0) nomeMet = k;
            if (nomeMet == null) continue;
            List<Ins> c = e.getValue();
            Ins s = primeiroSwitch(c);
            if (s == null) continue;
            for (Long dom : dominios.get(nomeMet)) {
                if (!quais.contains(dom)) continue;
                // dois casos que caem no mesmo trecho (no CarPlay, "case 606: case 607:") são UM serviço
                // que a camada da LG aceita por duas interfaces de eventos: entra uma vez, com a primeira
                Map<Integer, Servico> porDestino = new HashMap<Integer, Servico>();
                for (Map.Entry<Integer, Integer> caso : s.sw.entrySet()) {
                    int ini = caso.getValue().intValue(), fim = fimDoCaso(s, ini);
                    if (ini == s.swDefault) continue;
                    for (int k = 0; k < c.size(); k++) {
                        Ins i = c.get(k);
                        if (i.off < ini || i.off >= fim) continue;
                        Matcher m;
                        if (caso.getKey().intValue() == 9000) {
                            if (i.op.startsWith("ldc") && (m = P_CLS.matcher(i.rest)).find() && k + 2 < c.size() && intConst(c.get(k + 2)) != null)
                                ligados.add("W " + dom + " " + m.group(1).replace('/', '.') + " " + intConst(c.get(k + 2)) + " 0");
                            if (i.op.equals("new") && i.rest.indexOf("BAPTaskScheduler") >= 0) {
                                Integer atraso = null;
                                for (int j = k + 1; j < c.size() && j < k + 4; j++) if (intConst(c.get(j)) != null) atraso = intConst(c.get(j));
                                List<Ins> r = metodo(javap(PK + "BAPTaskScheduler"), " run()");
                                for (int j = 0; j + 2 < r.size(); j++)
                                    if (r.get(j).op.startsWith("ldc") && (m = P_CLS.matcher(r.get(j).rest)).find() && intConst(r.get(j + 2)) != null)
                                        ligados.add("W " + dom + " " + m.group(1).replace('/', '.') + " " + intConst(r.get(j + 2)) + " " + atraso);
                            }
                        } else if (i.op.equals("new") && (m = P_NEW.matcher(i.rest)).find()) {
                            Servico ja = porDestino.get(Integer.valueOf(ini));
                            if (ja != null) {
                                ja.relato.add("o despacho entrega os eventos deste serviço também pela interface " + caso.getKey() + " (o mesmo trecho da " + ja.ifEventos + "); o esquema usa a " + ja.ifEventos);
                                break;
                            }
                            Servico sv = new Servico();
                            sv.ouvinte = m.group(1).replace('/', '.');
                            sv.dominio = dom.longValue();
                            sv.ifEventos = caso.getKey().intValue();
                            out.add(sv);
                            porDestino.put(Integer.valueOf(ini), sv);
                            break;
                        }
                    }
                }
                // o default: tudo o que não é anúncio vai para uma classe intermediária, que separa por interface
                if (s.swDefault >= 0) {
                    int fim = fimDoCaso(s, s.swDefault);
                    for (Ins i : c) {
                        Matcher m;
                        if (i.off < s.swDefault || i.off >= fim || !i.op.equals("new") || !(m = P_NEW.matcher(i.rest)).find()) continue;
                        leIntermediaria(m.group(1).replace('/', '.'), dom.longValue(), out);
                        break;
                    }
                }
            }
        }
        return out;
    }

    static final Pattern P_PARSEMET = Pattern.compile("Method (com/lge/mib3oi/dsi/[\\w/$]+)\\.(\\w+):\\(Lcom/lge/mib3oi/dsi/Message;\\)V");

    /** A classe intermediária do sintonizador (DSITunerListenerService): um executeListenerService(int)
     *  com um caso por interface de eventos; em cada um, o ouvinte do serviço e o método dele que
     *  recebe a mensagem. Classe de outra forma fica de fora, com aviso. */
    static void leIntermediaria(String cls, long dom, List<Servico> out) throws Exception {
        List<Ins> c = metodo(javap(cls), " executeListenerService(int)");
        Ins s = c == null ? null : primeiroSwitch(c);
        if (s == null) { aviso(cls + ": o default do domínio " + dom + " entrega a esta classe, e ela não tem um executeListenerService(int) com switch; serviços dela ficam de fora"); return; }
        for (Map.Entry<Integer, Integer> caso : s.sw.entrySet()) {
            int ini = caso.getValue().intValue(), fim = fimDoCaso(s, ini);
            if (ini == s.swDefault) continue;
            Servico sv = new Servico();
            sv.dominio = dom; sv.ifEventos = caso.getKey().intValue();
            for (Ins i : c) {
                Matcher m;
                if (i.off < ini || i.off >= fim) continue;
                if (sv.ouvinte == null && i.op.equals("new") && (m = P_NEW.matcher(i.rest)).find()) sv.ouvinte = m.group(1).replace('/', '.');
                if (sv.parse == null && i.op.equals("invokevirtual") && (m = P_PARSEMET.matcher(i.rest)).find()) {
                    if (sv.ouvinte == null) sv.ouvinte = m.group(1).replace('/', '.');
                    sv.parse = m.group(2);
                }
            }
            if (sv.ouvinte == null || sv.parse == null) { aviso(cls + ": interface " + sv.ifEventos + " sem ouvinte ou sem o método que recebe a mensagem"); continue; }
            out.add(sv);
        }
    }

    // ------------------------------------------------------------------ <Serviço>ListenerService
    static final Pattern P_PARSE = Pattern.compile("Method ([\\w/$]+)\\.parseFrom:\\(\\[B\\)");
    static final Pattern P_IFC = Pattern.compile("InterfaceMethod (org/dsi/ifc/[\\w/$]+)\\.(\\w+):(\\(.*\\)V)");
    /** Como o ouvinte acha quem assinou um atributo. Nos serviços de veículo e no Bluetooth é
     *  getUpdateListenerList(instância, atributo); na agenda e nas mensagens (telefone, 12/10/2026)
     *  o primeiro argumento é o próprio …Impl da instância. Nos dois o atributo é o último. */
    static final Pattern P_ENTREGA = Pattern.compile("getUpdateListenerList:\\((I|L" + PK.replace('.', '/') + "[\\w/$]+;)I\\)");

    /** os switches que separam as apis num método de ouvinte: o primeiro, no executeListenerService;
     *  no método próprio do sintonizador, todos os que vêm logo depois de Message.getApiId() */
    static List<Ins> switchesDeApi(List<Ins> c, boolean proprio) {
        List<Ins> out = new ArrayList<Ins>();
        if (!proprio) { Ins s = primeiroSwitch(c); if (s != null) out.add(s); return out; }
        for (int k = 1; k < c.size(); k++)
            if (c.get(k).sw != null && c.get(k - 1).op.equals("invokevirtual") && c.get(k - 1).rest.indexOf("Message.getApiId:()I") >= 0) out.add(c.get(k));
        return out;
    }

    static void leOuvinte(Servico sv) throws Exception {
        String met = sv.parse == null ? "executeListenerService" : sv.parse;
        List<Ins> c = metodo(javap(sv.ouvinte), " " + met + "(");
        if (c == null) { aviso(sv.ouvinte + ": sem " + met); return; }
        List<Ins> sws = switchesDeApi(c, sv.parse != null);
        if (sws.isEmpty()) { aviso(sv.ouvinte + ": " + met + " sem switch"); return; }
        Map<String, Integer> vezes = new HashMap<String, Integer>();
        for (Ins s : sws) for (Map.Entry<Integer, Integer> caso : s.sw.entrySet()) {
            int ini = caso.getValue().intValue(), fim = fimDoCaso(s, ini);
            if (ini == s.swDefault) continue;        // buraco de um tableswitch: cai no default
            Evento ev = new Evento();
            ev.api = caso.getKey().intValue();
            for (int k = 0; k < c.size(); k++) {
                Ins i = c.get(k);
                if (i.off < ini || i.off >= fim) continue;
                Matcher m;
                if (ev.msg == null && i.op.equals("invokestatic") && (m = P_PARSE.matcher(i.rest)).find()) ev.msg = m.group(1).replace('/', '.');
                if (ev.entrega == null && i.op.equals("invokespecial") && P_ENTREGA.matcher(i.rest).find() && k > 0) ev.entrega = intConst(c.get(k - 1));
                if (ev.metodo == null && i.op.equals("invokeinterface") && (m = P_IFC.matcher(i.rest)).find()) {
                    ev.ifc = m.group(1).replace('/', '.'); ev.metodo = m.group(2); ev.sig = m.group(3);
                }
            }
            if (ev.metodo == null) { aviso(sv.ouvinte + ": api " + ev.api + " não chama método de ouvinte"); ev.metodo = "api" + ev.api; }
            // O nome do evento é o da MENSAGEM, que acompanha o atributo. O método do ouvinte que o
            // firmware chama nem sempre é o de mesmo nome (há trocas no código da LG; vão no relatório).
            ev.rotulo = ev.msg == null ? ev.metodo : ev.msg.substring(ev.msg.lastIndexOf('$') + 1);
            Integer n = vezes.get(ev.rotulo);
            vezes.put(ev.rotulo, Integer.valueOf(n == null ? 1 : n.intValue() + 1));
            sv.eventos.add(ev);
        }
        // a interface org.dsi.ifc do serviço: a dona da maioria dos métodos chamados, sem o "Listener"
        Map<String, Integer> donos = new HashMap<String, Integer>();
        for (Evento ev : sv.eventos) if (ev.ifc != null) { Integer n = donos.get(ev.ifc); donos.put(ev.ifc, Integer.valueOf(n == null ? 1 : n.intValue() + 1)); }
        String dono = null;
        for (Map.Entry<String, Integer> d : donos.entrySet()) if (dono == null || d.getValue().intValue() > donos.get(dono).intValue()) dono = d.getKey();
        if (dono == null) { aviso(sv.ouvinte + ": não achei a interface do ouvinte"); dono = sv.ouvinte; }
        sv.ifc = dono.endsWith("Listener") ? dono.substring(0, dono.length() - 8) : dono;
        sv.nome = sv.ifc.substring(sv.ifc.lastIndexOf('.') + 1);
        String resto = sv.ouvinte.substring(PK.length());
        sv.pacote = resto.substring(0, resto.indexOf('.'));
        // mesma mensagem em mais de uma api: o nome ganha ":<api>" ('#' é comentário no perfil)
        for (Evento ev : sv.eventos) if (vezes.get(ev.rotulo).intValue() > 1) ev.rotulo = ev.rotulo + ":" + ev.api;
        try {
            Class<?> ifc = Class.forName(sv.ifc, false, LeEsquemaDSI.class.getClassLoader());
            for (Field f : ifc.getFields())
                if (Modifier.isStatic(f.getModifiers()) && f.getType() == int.class && f.getName().startsWith("ATTR_"))
                    sv.atributos.put(Integer.valueOf(f.getInt(null)), f.getName());
        } catch (Throwable t) { aviso(sv.nome + ": não li as constantes ATTR_ de " + sv.ifc + ": " + t); }
        for (Evento ev : sv.eventos) if (ev.entrega != null && sv.atributos.containsKey(Integer.valueOf(ev.api))) ev.attr = Integer.valueOf(ev.api);
    }

    // ------------------------------------------------------------------ <Serviço>Impl
    static final Pattern P_MSGCLS = Pattern.compile("(" + PK.replace('.', '/') + "[\\w/]+/proto/[\\w$]+)");

    static final Pattern P_AUX = Pattern.compile("Method (?:[\\w/$]+\\.)?(\\w+):\\(I\\[B\\)V");

    static boolean eInstancia(Ins i) { return i.op.equals("getfield") && i.rest.indexOf("instanceId:I") >= 0; }

    /** Um desvio que compara o campo instanceId com uma constante: TRUE se, para a instância dada,
     *  o desvio é tomado; FALSE se não é; null se o desvio compara outra coisa. */
    static Boolean desvioPelaInstancia(List<Ins> c, int k, int inst) {
        String op = c.get(k).op;
        if ((op.equals("if_icmpeq") || op.equals("if_icmpne")) && k >= 3) {
            Integer cte = null;
            if (eInstancia(c.get(k - 1)) && intConst(c.get(k - 3)) != null) cte = intConst(c.get(k - 3));         // cte, this.instanceId
            else if (intConst(c.get(k - 1)) != null && eInstancia(c.get(k - 2))) cte = intConst(c.get(k - 1));     // this.instanceId, cte
            if (cte == null) return null;
            boolean igual = cte.intValue() == inst;
            return Boolean.valueOf(op.equals("if_icmpeq") ? igual : !igual);
        }
        if ((op.equals("ifeq") || op.equals("ifne")) && k >= 1 && eInstancia(c.get(k - 1))) return Boolean.valueOf(op.equals("ifeq") ? inst == 0 : inst != 0);
        return null;
    }

    /** Para que domínios um método auxiliar manda quando o serviço é a instância `inst`: anda pelo
     *  bytecode dele e segue os desvios que comparam a instância com uma constante. Desvio de outra
     *  coisa (a api, por exemplo) é seguido pelos dois lados, e o que vem depois dele sai em `talvez`. */
    static void dominiosDaInstancia(List<Ins> c, int inst, Set<Long> certos, Set<Long> talvez) {
        Map<Integer, Integer> pos = new HashMap<Integer, Integer>();
        for (int k = 0; k < c.size(); k++) pos.put(Integer.valueOf(c.get(k).off), Integer.valueOf(k));
        List<int[]> fila = new ArrayList<int[]>();           // {índice, 1 se já passou por desvio não decidido}
        Set<Long> visto = new HashSet<Long>();
        fila.add(new int[] { 0, 0 });
        while (!fila.isEmpty()) {
            int[] f = fila.remove(fila.size() - 1);
            int k = f[0], duvida = f[1];
            while (k >= 0 && k < c.size() && visto.add(Long.valueOf(k * 2L + duvida))) {
                Ins i = c.get(k);
                if (i.op.equals("invokevirtual") && i.rest.indexOf("DSIProvider.postDSIProviderMsg:(") >= 0) {
                    int d = k - 1;
                    while (d >= 0 && longConst(c.get(d)) == null) d--;
                    if (d >= 0) (duvida == 1 ? talvez : certos).add(longConst(c.get(d)));
                }
                boolean desvio = i.op.startsWith("if"), salto = i.op.equals("goto") || i.op.equals("goto_w");
                if (desvio || salto) {
                    Integer alvo = null;
                    try { alvo = pos.get(Integer.valueOf(i.rest.trim())); } catch (NumberFormatException x) { alvo = null; }
                    if (alvo == null) break;
                    if (salto) { k = alvo.intValue(); continue; }
                    Boolean pula = desvioPelaInstancia(c, k, inst);
                    if (pula == null) { fila.add(new int[] { alvo.intValue(), 1 }); duvida = 1; k++; continue; }
                    k = pula.booleanValue() ? alvo.intValue() : k + 1;
                    continue;
                }
                if (i.op.endsWith("return") || i.op.equals("athrow")) break;
                k++;
            }
        }
        talvez.removeAll(certos);
    }

    /** Métodos auxiliares de pedido (telefone, 12/10/2026). Em DSIMobileEquipmentImpl (messageToBytes)
     *  e em DSISearchImpl (postMessage) os pedidos não chamam postDSIProviderMsg direto: chamam um
     *  método da própria classe, (int api, byte[] mensagem), e é ele que manda, com a api do
     *  parâmetro e um domínio que depende da instância do serviço. Devolve nome -> instruções de
     *  cada auxiliar; acerta a interface de pedidos do serviço e deixa no relato para que domínio
     *  vai cada instância que o anúncio liga. Classe sem auxiliar devolve vazio, e nada muda. */
    static Map<String, List<Ins>> leAuxiliares(Servico sv, String impl, Map<String, List<Ins>> metodos) {
        Map<String, List<Ins>> out = new HashMap<String, List<Ins>>();
        for (Map.Entry<String, List<Ins>> e : metodos.entrySet()) {
            String cab = e.getKey().trim();
            if (cab.indexOf("(int, byte[])") < 0) continue;
            int par = cab.indexOf('(');
            String nome = cab.substring(cab.lastIndexOf(' ', par) + 1, par);
            List<Ins> c = e.getValue();
            TreeSet<Long> doms = new TreeSet<Long>();
            Integer ifid = null;
            boolean ok = true;
            int n = 0;
            for (int k = 0; k < c.size() && ok; k++) {
                Ins i = c.get(k);
                if (!i.op.equals("invokevirtual") || i.rest.indexOf("DSIProvider.postDSIProviderMsg:(I") < 0) continue;
                int d = k - 1;
                while (d >= 0 && longConst(c.get(d)) == null) d--;
                int g = d - 1;
                while (g >= 0 && !(c.get(g).op.equals("getfield") && c.get(g).rest.indexOf("Lcom/lge/mib3oi/dsi/DSIProvider;") >= 0)) g--;
                if (d < 0 || g < 0 || d + 1 >= k || !c.get(g + 1).op.equals("iload_1") || intConst(c.get(d + 1)) == null) { ok = false; break; }
                if (ifid == null) ifid = intConst(c.get(d + 1));
                else if (!ifid.equals(intConst(c.get(d + 1)))) { ok = false; break; }
                doms.add(longConst(c.get(d)));
                n++;
            }
            if (!ok || n == 0) continue;
            out.put(nome, c);
            if (sv.ifPedidos < 0) sv.ifPedidos = ifid.intValue();
            else if (sv.ifPedidos != ifid.intValue()) aviso(impl + "." + nome + ": interface " + ifid + ", diferente de " + sv.ifPedidos);
            if (!doms.contains(Long.valueOf(sv.dominio))) aviso(impl + "." + nome + ": o auxiliar manda para " + doms + ", e não para o domínio " + sv.dominio);
            sv.relato.add("os pedidos saem pelo método auxiliar " + nome + "(api, mensagem), na interface " + ifid + ", para os domínios " + doms + "; pela instância (as que o anúncio liga):");
            TreeSet<Integer> insts = new TreeSet<Integer>();
            for (String l : ligados) { String[] t = l.split(" "); if (t.length >= 4 && t[2].equals(sv.ifc)) insts.add(Integer.valueOf(t[3])); }
            for (Integer inst : insts) {
                TreeSet<Long> certos = new TreeSet<Long>(), talvez = new TreeSet<Long>();
                dominiosDaInstancia(c, inst.intValue(), certos, talvez);
                sv.relato.add("  instância " + inst + ": domínio " + certos + (talvez.isEmpty() ? "" : "; em parte dos pedidos também " + talvez + " (desvio que não é pela instância)")
                        + (certos.contains(Long.valueOf(sv.dominio)) ? "" : " — NÃO é deste domínio: no carro quem responde por ela é outro processo"));
            }
        }
        return out;
    }

    static void lePedidos(Servico sv, Set<String> classes) throws Exception {
        String impl = PK + sv.pacote + "." + sv.nome + "Impl";
        if (!classes.contains(impl)) { aviso(sv.nome + ": não achei " + impl + "; pedidos ficam de fora"); return; }
        Map<Integer, String> vistos = new HashMap<Integer, String>();      // api -> o método que a usa primeiro
        LinkedHashMap<String, List<Ins>> metodos = javap(impl);
        Map<String, List<Ins>> auxiliares = leAuxiliares(sv, impl, metodos);
        for (Map.Entry<String, List<Ins>> e : metodos.entrySet()) {
            String cab = e.getKey().trim();
            int par = cab.indexOf('(');
            String nome = cab.substring(cab.lastIndexOf(' ', par) + 1, par);
            List<Ins> c = e.getValue();
            if (auxiliares.containsKey(nome) && cab.indexOf("(int, byte[])") >= 0) continue;     // já lido em leAuxiliares
            for (int k = 0; k < c.size(); k++) {
                Ins i = c.get(k);
                // pedido que passa por um método auxiliar: a api é a constante logo antes da mensagem
                Matcher ma = P_AUX.matcher(i.rest);
                if ((i.op.equals("invokevirtual") || i.op.equals("invokespecial")) && ma.find() && auxiliares.containsKey(ma.group(1))) {
                    int g = k - 1;           // "this.auxiliar(API, mensagem)": a constante que vem logo depois do aload_0
                    while (g >= 1 && !(intConst(c.get(g)) != null && c.get(g - 1).op.equals("aload_0"))) g--;
                    if (g < 1) { aviso(impl + "." + nome + ": chama " + ma.group(1) + " sem api constante"); continue; }
                    Integer api = intConst(c.get(g));
                    String msg = null;
                    for (int j = k - 1; j >= 0 && msg == null; j--) {      // a mensagem mais próxima, para trás
                        Matcher m = P_MSGCLS.matcher(c.get(j).rest);
                        if ((c.get(j).op.equals("checkcast") || c.get(j).rest.indexOf(".toByteArray:") >= 0) && m.find()) msg = m.group(1).replace('/', '.');
                        if (j < g && P_AUX.matcher(c.get(j).rest).find()) break;        // não passa de outra chamada ao auxiliar
                    }
                    String antes = vistos.get(api);
                    if (antes != null) {
                        if (!antes.equals(nome)) sv.relato.add("os pedidos " + antes + " e " + nome + " saem com a mesma api " + api + " na camada da LG; o esquema fica com o primeiro");
                        continue;
                    }
                    vistos.put(api, nome);
                    Pedido p = new Pedido();
                    p.api = api.intValue(); p.metodo = nome; p.msg = msg;
                    sv.pedidos.add(p);
                    continue;
                }
                if (!i.op.equals("invokevirtual") || i.rest.indexOf("DSIProvider.postDSIProviderMsg:(") < 0) continue;
                boolean comApi = i.rest.indexOf("postDSIProviderMsg:(I") >= 0;
                int d = k - 1;
                while (d >= 0 && longConst(c.get(d)) == null) d--;
                if (d < 0 || d + 1 >= k) { aviso(impl + "." + nome + ": não achei o domínio do pedido"); continue; }
                long dom = longConst(c.get(d)).longValue();
                Integer ifid = intConst(c.get(d + 1));
                int g = d - 1;
                while (g >= 0 && !(c.get(g).op.equals("getfield") && c.get(g).rest.indexOf("Lcom/lge/mib3oi/dsi/DSIProvider;") >= 0)) g--;
                Integer api = !comApi ? Integer.valueOf(-1) : g >= 0 ? intConst(c.get(g + 1)) : null;   // sem api no código: vai -1
                if (ifid == null || api == null) { aviso(impl + "." + nome + ": pedido sem api ou interface constantes"); continue; }
                if (dom != sv.dominio) aviso(impl + "." + nome + ": pedido vai para o domínio " + dom + ", não " + sv.dominio);
                if (sv.ifPedidos < 0) sv.ifPedidos = ifid.intValue();
                else if (sv.ifPedidos != ifid.intValue()) aviso(impl + "." + nome + ": interface " + ifid + ", diferente de " + sv.ifPedidos);
                String msg = null;
                for (int j = g + 1; j < d && msg == null; j++) {
                    Matcher m = P_MSGCLS.matcher(c.get(j).rest);
                    if ((c.get(j).op.equals("checkcast") || c.get(j).rest.indexOf(".toByteArray:") >= 0) && m.find()) msg = m.group(1).replace('/', '.');
                }
                String antes = vistos.get(api);
                if (antes != null) {                 // as três formas de setNotification usam a mesma api, e está certo
                    if (!antes.equals(nome)) sv.relato.add("os pedidos " + antes + " e " + nome + " saem com a mesma api " + api + " na camada da LG; o esquema fica com o primeiro");
                    continue;
                }
                vistos.put(api, nome);
                Pedido p = new Pedido();
                p.api = api.intValue(); p.metodo = nome; p.msg = msg;
                sv.pedidos.add(p);
            }
        }
    }

    // ------------------------------------------------------------------ respostas (tabela do proxy DSI da HMI)
    static final String CFG = "de.vw.mib.asl.framework.internal.framework.dsi.util.";

    /** O evento que chega à HMI como o método `nome` do ouvinte. Havendo mais de um (o firmware tem
     *  eventos entregues pelo método de outro), vale o que leva a mensagem de mesmo nome. */
    static Evento eventoDoMetodo(Servico sv, String nome) {
        Evento achado = null;
        for (Evento ev : sv.eventos) {
            if (!nome.equals(ev.metodo)) continue;
            if (ev.msg != null && ev.msg.endsWith("$" + nome)) return ev;
            if (achado == null) achado = ev;
        }
        return achado;
    }

    static void leRespostas(Servico sv) {
        try {
            ClassLoader cl = LeEsquemaDSI.class.getClassLoader();
            String[] tipos = (String[]) Class.forName(CFG + "RuntimeGeneratedConstants", true, cl).getField("SERVICE_TS_NS").get(null);
            int t = -1;
            for (int i = 0; i < tipos.length; i++) if (tipos[i].equals(sv.ifc)) t = i;
            if (t < 0) { aviso(sv.nome + ": a tabela do proxy DSI da HMI não tem " + sv.ifc + "; respostas ficam de fora"); return; }
            Class<?> pt = Class.forName(CFG + "Cfg$PT", true, cl), u = Class.forName(CFG + "Cfg$U", true, cl);
            Integer T = Integer.valueOf(t);
            int[] reqs = (int[]) pt.getMethod("getSyncRequests", int.class).invoke(null, T);
            int[] tempos = (int[]) pt.getMethod("getSyncRequestTimeouts", int.class).invoke(null, T);
            Method respostas = pt.getMethod("getSyncResponses", int.class, int.class), tipo = pt.getMethod("getRequestType", int.class, int.class);
            Method nomeP = u.getMethod("getServiceSyncMethodName", int.class, int.class), nomeR = u.getMethod("getListenerSyncMethodName", int.class, int.class, int.class);
            for (int i = 0; i < reqs.length; i++) {
                Integer I = Integer.valueOf(i);
                String metodo = (String) nomeP.invoke(null, T, I);
                int k = ((Number) tipo.invoke(null, T, Integer.valueOf(reqs[i]))).intValue();
                String tp = k == 2 ? "uma" : k == 3 ? "sequencia" : k == 4 ? "escolha" : k == 5 ? "complexa" : null;
                if (tp == null) { aviso(sv.nome + "." + metodo + ": tipo de pedido " + k + " na tabela da HMI, que não conheço"); continue; }
                int nres = ((int[]) respostas.invoke(null, T, I)).length;
                boolean achou = false;
                for (Pedido p : sv.pedidos) {
                    if (!p.metodo.equals(metodo)) continue;
                    achou = true;
                    Resposta r = new Resposta();
                    r.p = p; r.tipo = tp; r.prazo = tempos != null && i < tempos.length ? tempos[i] : -1;
                    for (int j = 0; j < nres; j++) {
                        String nome = (String) nomeR.invoke(null, T, I, Integer.valueOf(j));
                        Evento ev = eventoDoMetodo(sv, nome);
                        r.nomes.add(nome); r.evs.add(ev);
                        if (ev == null) sv.relato.add("pedido " + metodo + " (api " + p.api + "): a HMI espera " + nome + ", e nenhuma api da camada da LG chama esse método do ouvinte");
                    }
                    sv.respostas.add(r);
                }
                if (!achou) sv.relato.add("a tabela da HMI tem o pedido " + metodo + " com resposta, mas a camada da LG não o manda por api nenhuma");
            }
        } catch (Throwable e) {
            aviso(sv.nome + ": não li a tabela de respostas do proxy DSI da HMI: " + (e.getCause() != null ? e.getCause() : e));
        }
    }

    // ------------------------------------------------------------------ o que cada pedido grava ou devolve (dedução)
    /** as folhas de uma mensagem, por caminhos de campos simples: caminho -> tipo. false se há campo
     *  repetido, bytes ou tipo desconhecido no meio: aí não dá para copiar campo a campo */
    static boolean folhas(Class<?> m, String pre, LinkedHashMap<String, String> out, List<Class<?>> pilha) throws Exception {
        boolean inteira = true;
        pilha.add(m);
        for (Campo c : descreve(m)) {
            if (!c.rotulo.equals("um") || c.tipo.equals("bytes") || c.tipo.equals("desconhecido")) inteira = false;
            else if (!c.tipo.equals("msg")) out.put(pre + c.nome, c.tipo);
            else if (pilha.contains(c.sub)) inteira = false;
            else inteira &= folhas(c.sub, pre + c.nome + ".", out, pilha);
        }
        pilha.remove(pilha.size() - 1);
        return inteira;
    }

    /** Regra "posicao": os campos de a (pedido) são os de b (evento), número a número e tipo a tipo;
     *  no nível de cima o evento tem a mais só o validFlag. Enche pares {caminho em a, caminho em b}. */
    static boolean casaPorPosicao(Class<?> a, Class<?> b, boolean topo, String pa, String pb, List<String[]> pares, List<Class<?>> pilha) throws Exception {
        List<Campo> fa = descreve(a), fb = new ArrayList<Campo>();
        for (Campo c : descreve(b)) if (!(topo && c.nome.equalsIgnoreCase("validFlag"))) fb.add(c);
        if (fa.size() != fb.size()) return false;
        for (int i = 0; i < fa.size(); i++) {
            Campo x = fa.get(i), y = fb.get(i);
            if (x.num != y.num || !x.tipo.equals(y.tipo) || !x.rotulo.equals("um") || !y.rotulo.equals("um")) return false;
            if (x.tipo.equals("bytes") || x.tipo.equals("desconhecido")) return false;
            if (!x.tipo.equals("msg")) { pares.add(new String[] { pa + x.nome, pb + y.nome }); continue; }
            if (pilha.contains(x.sub)) return false;
            pilha.add(x.sub);
            boolean ok = casaPorPosicao(x.sub, y.sub, false, pa + x.nome + ".", pb + y.nome + ".", pares, pilha);
            pilha.remove(pilha.size() - 1);
            if (!ok) return false;
        }
        return true;
    }

    static String fim(String caminho) { return caminho.substring(caminho.lastIndexOf('.') + 1); }

    /** Linhas G: o pedido set<X> grava o atributo de nome <X>. O firmware não traz essa ligação (quem
     *  a faz no carro é o carifservice, com o painel); é o costume dos serviços DSI, e a própria HMI
     *  trata a atualização do atributo como a resposta do pedido (por exemplo em
     *  AslTargetLscTNGBordComputerLAC: "got Response for setMenuLanguage"). */
    static void deduzGravacoes(Servico sv, ClassLoader cl) throws Exception {
        Map<String, Integer> porNome = new HashMap<String, Integer>();
        for (Map.Entry<Integer, String> at : sv.atributos.entrySet()) porNome.put(at.getValue(), at.getKey());
        for (Pedido p : sv.pedidos) {
            if (!p.metodo.startsWith("set") || p.metodo.length() < 4 || p.api >= 9000) continue;
            sv.nSet++;
            Integer attr = porNome.get("ATTR_" + p.metodo.substring(3).toUpperCase());
            Evento ev = null;
            if (attr != null) for (Evento e : sv.eventos) if (attr.equals(e.attr)) ev = e;
            String motivo = null;
            List<String[]> pares = new ArrayList<String[]>();
            String regra = "posicao";
            if (attr == null) motivo = "não há atributo ATTR_" + p.metodo.substring(3).toUpperCase();
            else if (ev == null || ev.msg == null) motivo = "o atributo " + attr + " não tem evento com mensagem";
            else if (p.msg == null) motivo = "o pedido não leva dados";
            else {
                Class<?> mp = Class.forName(p.msg, true, cl), me = Class.forName(ev.msg, true, cl);
                if (!casaPorPosicao(mp, me, true, "", "", pares, new ArrayList<Class<?>>())) {
                    pares.clear(); regra = "nomes";
                    LinkedHashMap<String, String> fp = new LinkedHashMap<String, String>(), fe = new LinkedHashMap<String, String>();
                    boolean inteiro = folhas(mp, "", fp, new ArrayList<Class<?>>());
                    folhas(me, "", fe, new ArrayList<Class<?>>());
                    for (String k : new ArrayList<String>(fe.keySet())) if (k.equalsIgnoreCase("validFlag")) fe.remove(k);
                    Set<String> usados = new HashSet<String>();
                    if (!inteiro) motivo = "o pedido leva lista ou campo que não sei copiar";
                    for (Map.Entry<String, String> a : fp.entrySet()) {
                        String unico = null; int n = 0;
                        for (Map.Entry<String, String> b : fe.entrySet())
                            if (fim(b.getKey()).equals(fim(a.getKey())) && b.getValue().equals(a.getValue())) { unico = b.getKey(); n++; }
                        if (motivo == null && (n != 1 || !usados.add(unico))) motivo = "o campo " + a.getKey() + " do pedido tem " + n + " campos de mesmo nome e tipo no evento " + ev.rotulo;
                        pares.add(new String[] { a.getKey(), unico });
                    }
                }
                if (motivo == null && pares.isEmpty()) motivo = "o pedido não tem campo nenhum";
            }
            if (motivo != null) { sv.setSemRegra.add(p.metodo + " (api " + p.api + "): " + motivo); continue; }
            if (regra.equals("posicao")) sv.gPosicao++; else sv.gNomes++;
            for (String[] par : pares) sv.linhasG.add("G " + sv.nome + " " + p.api + " " + ev.api + " " + regra + " " + par[0] + " " + par[1]);
        }
    }

    /** Linhas X: o que a resposta devolve do pedido. Campos de mesmo caminho e tipo nas duas mensagens
     *  (o ListUpdateInfo de um pedido de lista, com o número da transação que a HMI confere). */
    static void deduzDevolucoes(Servico sv, ClassLoader cl) throws Exception {
        for (Resposta r : sv.respostas) for (Evento ev : r.evs) {
            if (ev == null || ev.msg == null || r.p.msg == null) continue;
            LinkedHashMap<String, String> fp = new LinkedHashMap<String, String>(), fe = new LinkedHashMap<String, String>();
            folhas(Class.forName(r.p.msg, true, cl), "", fp, new ArrayList<Class<?>>());
            folhas(Class.forName(ev.msg, true, cl), "", fe, new ArrayList<Class<?>>());
            for (Map.Entry<String, String> a : fp.entrySet())
                if (a.getValue().equals(fe.get(a.getKey()))) sv.linhasX.add("X " + sv.nome + " " + r.p.api + " " + ev.api + " " + a.getKey() + " " + a.getKey());
        }
    }

    // ------------------------------------------------------------------ mensagens protobuf
    static String chave(String s) { return s.replace("_", "").toLowerCase(); }
    static String cap(String s) { return Character.toUpperCase(s.charAt(0)) + s.substring(1); }

    static Method acha(Class<?> c, String nome, Class<?>... args) {
        try { return c.getMethod(nome, args); } catch (NoSuchMethodException e) { /* tenta sem olhar maiúsculas */ }
        for (Method m : c.getMethods()) {
            if (!m.getName().equalsIgnoreCase(nome) || m.getParameterTypes().length != args.length) continue;
            boolean ok = true;
            for (int k = 0; k < args.length; k++) if (m.getParameterTypes()[k] != args[k]) ok = false;
            if (ok) return m;
        }
        return null;
    }

    static Object novoBuilder(Class<?> m) throws Exception { return m.getMethod("newBuilder").invoke(null); }

    static byte[] bytesDe(Object builder) throws Exception {
        Object msg = builder.getClass().getMethod("build").invoke(builder);
        return (byte[]) msg.getClass().getMethod("toByteArray").invoke(msg);
    }

    /** lê a etiqueta do começo de w; devolve {campo, tipo de fio, onde a etiqueta acaba} */
    static int[] etiqueta(byte[] w) {
        long v = 0; int s = 0, o = 0;
        while (o < w.length) { int b = w[o++] & 0xff; v |= (long) (b & 0x7f) << s; if ((b & 0x80) == 0) break; s += 7; }
        return new int[] { (int) (v >>> 3), (int) (v & 7), o };
    }

    static String tipoInteiro(Class<?> dono, String campo, int num, byte[] w, boolean de64, int quantos) {
        int[] t = etiqueta(w);
        if (t[0] != num) aviso(dono.getName() + "." + campo + ": a constante diz campo " + num + ", o fio diz " + t[0]);
        int resto = w.length - t[2];
        if (t[1] == 2) {                             // empacotado: etiqueta, tamanho, valores
            resto -= 1;
            if (resto == quantos) return de64 ? "sint64" : "sint32";
            if (resto == 10 * quantos) return de64 ? "int64" : "int32";
            if (resto == 5 * quantos && !de64) return "uint32";
            if (resto == 4 * quantos && !de64) return "fixed32";
            if (resto == 8 * quantos && de64) return "fixed64";
        } else {
            resto = w.length / quantos - t[2];
            if (t[1] == 0 && resto == 1) return de64 ? "sint64" : "sint32";
            if (t[1] == 0 && resto == 10) return de64 ? "int64" : "int32";
            if (t[1] == 0 && resto == 5 && !de64) return "uint32";
            if (t[1] == 5 && !de64) return "fixed32";
            if (t[1] == 1 && de64) return "fixed64";
        }
        aviso(dono.getName() + "." + campo + ": não reconheci a codificação do inteiro (fio " + t[1] + ", " + w.length + " bytes)");
        return "desconhecido";
    }

    static List<Campo> descreve(Class<?> m) throws Exception {
        List<Campo> fs = mensagens.get(m);
        if (fs != null) return fs;
        fs = new ArrayList<Campo>();
        mensagens.put(m, fs);                        // antes da recursão: tipo que contém a si mesmo não entra em laço
        Class<?> lite = Class.forName("com.google.protobuf.GeneratedMessageLite");
        Map<String, Integer> nums = new HashMap<String, Integer>();
        for (Field f : m.getDeclaredFields())
            if (Modifier.isStatic(f.getModifiers()) && f.getName().endsWith("_FIELD_NUMBER")) {
                f.setAccessible(true);
                nums.put(chave(f.getName().substring(0, f.getName().length() - 13)), Integer.valueOf(f.getInt(null)));
            }
        for (Field f : m.getDeclaredFields()) {
            String n = f.getName();
            if (Modifier.isStatic(f.getModifiers()) || !n.endsWith("_")) continue;
            String base = n.substring(0, n.length() - 1);
            Integer num = nums.remove(chave(base));
            if (num == null) continue;               // bitField0_ e outros campos de serviço
            Campo c = new Campo();
            c.num = num.intValue(); c.nome = base; c.rotulo = "um";
            Class<?> t = f.getType();
            String C = cap(base), tn = t.getName();
            Object b = novoBuilder(m);
            Class<?> bc = b.getClass();
            if (t == int.class) {
                Method sv = acha(bc, "set" + C + "Value", int.class), s = sv != null ? sv : acha(bc, "set" + C, int.class);
                s.invoke(b, Integer.valueOf(-1));
                c.tipo = tipoInteiro(m, base, c.num, bytesDe(b), false, 1);
                if (sv != null) { if (!c.tipo.equals("int32")) aviso(m.getName() + "." + base + ": enum com codificação " + c.tipo); c.tipo = "enum"; }
            } else if (t == long.class) {
                acha(bc, "set" + C, long.class).invoke(b, Long.valueOf(-1));
                c.tipo = tipoInteiro(m, base, c.num, bytesDe(b), true, 1);
            } else if (t == boolean.class) {
                acha(bc, "set" + C, boolean.class).invoke(b, Boolean.TRUE);
                c.tipo = etiqueta(bytesDe(b))[1] == 0 ? "bool" : "desconhecido";
            } else if (t == float.class) {
                acha(bc, "set" + C, float.class).invoke(b, Float.valueOf(1.5f));
                c.tipo = etiqueta(bytesDe(b))[1] == 5 ? "float" : "desconhecido";
            } else if (t == double.class) {
                acha(bc, "set" + C, double.class).invoke(b, Double.valueOf(1.5));
                c.tipo = etiqueta(bytesDe(b))[1] == 1 ? "double" : "desconhecido";
            } else if (t == String.class) {
                acha(bc, "set" + C, String.class).invoke(b, "x");
                c.tipo = etiqueta(bytesDe(b))[1] == 2 ? "string" : "desconhecido";
            } else if (tn.equals("com.google.protobuf.ByteString")) {
                c.tipo = "bytes";
            } else if (lite.isAssignableFrom(t)) {
                c.tipo = "msg"; c.sub = t;
                acha(bc, "set" + C, t).invoke(b, t.getMethod("getDefaultInstance").invoke(null));
                int[] e = etiqueta(bytesDe(b));
                if (e[0] != c.num || e[1] != 2) aviso(m.getName() + "." + base + ": submensagem fora do esperado no fio");
                descreve(t);
            } else if (tn.equals("com.google.protobuf.Internal$IntList") || tn.equals("com.google.protobuf.Internal$LongList")) {
                boolean de64 = tn.endsWith("LongList");
                Class<?> pt = de64 ? long.class : int.class;
                Object um = de64 ? (Object) Long.valueOf(-1) : (Object) Integer.valueOf(-1);
                Method av = acha(bc, "add" + C + "Value", pt), a = av != null ? av : acha(bc, "add" + C, pt);
                a.invoke(b, um); a.invoke(b, um);
                byte[] w = bytesDe(b);
                c.rotulo = etiqueta(w)[1] == 2 ? "pac" : "rep";
                c.tipo = tipoInteiro(m, base, c.num, w, de64, 2);
                if (av != null) c.tipo = "enum";
            } else if (tn.equals("com.google.protobuf.Internal$BooleanList") || tn.equals("com.google.protobuf.Internal$FloatList")
                    || tn.equals("com.google.protobuf.Internal$DoubleList")) {
                boolean eb = tn.endsWith("BooleanList"), ef = tn.endsWith("FloatList");
                Class<?> pt = eb ? boolean.class : ef ? float.class : double.class;
                Object um = eb ? (Object) Boolean.TRUE : ef ? (Object) Float.valueOf(1.5f) : (Object) Double.valueOf(1.5);
                Method a = acha(bc, "add" + C, pt);
                a.invoke(b, um); a.invoke(b, um);
                c.rotulo = etiqueta(bytesDe(b))[1] == 2 ? "pac" : "rep";
                c.tipo = eb ? "bool" : ef ? "float" : "double";
            } else if (tn.equals("com.google.protobuf.Internal$ProtobufList")) {
                Method g = acha(m, "get" + C, int.class);
                Class<?> et = g == null ? null : g.getReturnType();
                c.rotulo = "rep";
                if (et == String.class) c.tipo = "string";
                else if (et != null && et.getName().equals("com.google.protobuf.ByteString")) c.tipo = "bytes";
                else if (et != null && lite.isAssignableFrom(et)) { c.tipo = "msg"; c.sub = et; descreve(et); }
                else { c.tipo = "desconhecido"; aviso(m.getName() + "." + base + ": lista de tipo que não conheço: " + et); }
            } else {
                c.tipo = "desconhecido";
                aviso(m.getName() + "." + base + ": tipo Java que não conheço: " + tn);
            }
            if (c.tipo.equals("desconhecido")) aviso(m.getName() + "." + base + ": ficou como desconhecido");
            fs.add(c);
        }
        if (!nums.isEmpty()) aviso(m.getName() + ": constantes de campo sem campo correspondente (oneof?): " + nums.keySet());
        java.util.Collections.sort(fs, new java.util.Comparator<Campo>() {
            public int compare(Campo a, Campo c2) { return a.num - c2.num; }
        });
        return fs;
    }

    // ------------------------------------------------------------------ vetores de prova
    static int conta;                                // contador de folhas: dá valores diferentes a cada uma

    static String aspas(String s) {
        return "\"" + s.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
    }

    /** dá valor a um campo escalar pelo Builder e devolve o texto do valor, como vai no perfil */
    static String poeValor(Object b, Campo c, boolean lista) throws Exception {
        int k = ++conta;
        String C = cap(c.nome), pre = lista ? "add" : "set";
        Class<?> bc = b.getClass();
        if (c.tipo.equals("sint32") || c.tipo.equals("int32")) {
            int v = (k % 2 == 1) ? -(k * 37 % 100000 + 1) : (k * 91 % 2000000 + 1);
            acha(bc, pre + C, int.class).invoke(b, Integer.valueOf(v));
            return Integer.toString(v);
        }
        if (c.tipo.equals("enum")) {
            int v = k % 5 + 1;
            acha(bc, pre + C + "Value", int.class).invoke(b, Integer.valueOf(v));
            return Integer.toString(v);
        }
        if (c.tipo.equals("uint32") || c.tipo.equals("fixed32")) {
            int v = k * 91 % 2000000 + 1;
            acha(bc, pre + C, int.class).invoke(b, Integer.valueOf(v));
            return Integer.toString(v);
        }
        if (c.tipo.equals("sint64") || c.tipo.equals("int64") || c.tipo.equals("fixed64")) {
            long v = (k % 2 == 1 && !c.tipo.equals("fixed64")) ? -(k * 100003L + 1) : (k * 30000000019L + 1);
            acha(bc, pre + C, long.class).invoke(b, Long.valueOf(v));
            return Long.toString(v);
        }
        if (c.tipo.equals("bool")) { acha(bc, pre + C, boolean.class).invoke(b, Boolean.TRUE); return "1"; }
        if (c.tipo.equals("float")) {
            float v = (k % 2 == 1 ? -1 : 1) * ((k % 200) + 0.5f);
            acha(bc, pre + C, float.class).invoke(b, Float.valueOf(v));
            return Float.toString(v);
        }
        if (c.tipo.equals("double")) {
            double v = (k % 2 == 1 ? -1 : 1) * (k + 0.25);
            acha(bc, pre + C, double.class).invoke(b, Double.valueOf(v));
            return Double.toString(v);
        }
        if (c.tipo.equals("string")) {
            String v = (k % 7 == 0) ? "aspas \" barra \\ é " + k : "t" + k;
            acha(bc, pre + C, String.class).invoke(b, v);
            return aspas(v);
        }
        return null;                                 // bytes e desconhecido: sem vetor
    }

    static Object preenche(Class<?> m, String caminho, List<String> linhas, List<Class<?>> pilha) throws Exception {
        Object b = novoBuilder(m);
        pilha.add(m);
        for (Campo c : descreve(m)) {
            int vezes = c.rotulo.equals("um") ? 1 : 2;
            for (int v = 0; v < vezes; v++) {
                String onde = caminho + c.nome + (vezes > 1 ? "[" + v + "]" : "");
                if (c.tipo.equals("msg")) {
                    if (pilha.contains(c.sub)) continue;
                    Object sub = preenche(c.sub, onde + ".", linhas, pilha);
                    acha(b.getClass(), (vezes > 1 ? "add" : "set") + cap(c.nome), c.sub).invoke(b, sub);
                } else {
                    String txt = poeValor(b, c, vezes > 1);
                    if (txt != null) linhas.add(onde + " = " + txt);
                }
            }
        }
        pilha.remove(pilha.size() - 1);
        return b.getClass().getMethod("build").invoke(b);
    }

    static String hexa(byte[] w) {
        StringBuilder sb = new StringBuilder();
        for (int k = 0; k < w.length; k++) { if (k > 0) sb.append(' '); sb.append(String.format("%02x", Integer.valueOf(w[k] & 0xff))); }
        return sb.toString();
    }

    // ------------------------------------------------------------------ principal
    static PrintWriter abre(File f) throws Exception {
        return new PrintWriter(new OutputStreamWriter(new FileOutputStream(f), StandardCharsets.UTF_8));
    }

    public static void main(String[] a) throws Exception {
        if (a.length < 2) { System.err.println("uso: LeEsquemaDSI <MIBHMI-DSI.jar> <pasta de saída> [domínios] [rótulo]"); System.exit(2); }
        jar = a[0];
        File out = new File(a[1]);
        out.mkdirs();
        Set<Long> quais = new TreeSet<Long>();
        for (String d : (a.length > 2 ? a[2] : "16").split(",")) quais.add(Long.decode(d.trim()));
        String rotulo = a.length > 3 && a[3].trim().length() > 0 ? a[3].trim() : "de veículo";      // como os arquivos chamam os serviços
        javapRun = Class.forName("com.sun.tools.javap.Main").getMethod("run", String[].class, PrintWriter.class);

        Set<String> classes = new HashSet<String>();
        JarFile jf = new JarFile(jar);
        for (Enumeration<JarEntry> en = jf.entries(); en.hasMoreElements();) {
            String n = en.nextElement().getName();
            if (n.endsWith(".class")) classes.add(n.substring(0, n.length() - 6).replace('/', '.'));
        }
        jf.close();

        List<Servico> svs = leDespacho(quais);
        ClassLoader cl = LeEsquemaDSI.class.getClassLoader();
        // Só fica no esquema o serviço que o anúncio do domínio liga (linhas W). O despacho do domínio
        // 256 (telefone, 12/10/2026) entrega também interfaces de serviços de outros donos (mídia, UPnP
        // e busca, cujos pedidos vão para outros domínios): esses ficam de fora, e vão no relatório.
        // Nos domínios de veículo e do sintonizador todo serviço do despacho é ligado pelo anúncio.
        Set<String> anunciados = new HashSet<String>(), dominiosComAnuncio = new HashSet<String>();
        for (String l : ligados) { String[] t = l.split(" "); if (t.length >= 4) { anunciados.add(t[1] + " " + t[2]); dominiosComAnuncio.add(t[1]); } }
        List<Servico> fora = new ArrayList<Servico>();
        for (Servico sv : svs) {
            leOuvinte(sv);
            if (dominiosComAnuncio.contains(Long.toString(sv.dominio)) && !anunciados.contains(sv.dominio + " " + sv.ifc)) { fora.add(sv); continue; }
            lePedidos(sv, classes);
            for (Evento ev : sv.eventos) if (ev.msg != null) descreve(Class.forName(ev.msg, true, cl));
            for (Pedido p : sv.pedidos) if (p.msg != null) descreve(Class.forName(p.msg, true, cl));
            leRespostas(sv);
            deduzGravacoes(sv, cl);
            deduzDevolucoes(sv, cl);
        }

        svs.removeAll(fora);

        // ---- esquema.txt
        PrintWriter w = abre(new File(out, "esquema.txt"));
        w.println("# Esquema dos serviços DSI " + rotulo + " da HMI do MOI3 (firmware MU0883, HMI H40.272.304).");
        w.println("# GERADO por driver/m8_esquema.sh (driver/LeEsquemaDSI.java) a partir das classes do próprio");
        w.println("# firmware, na JVM dele. Não editar à mão. Estudo num computador; nada daqui vai para o carro.");
        w.println("#");
        w.println("# Uma coisa por linha, campos separados por espaço:");
        w.println("#   S <serviço> <domínio> <interface de pedidos> <interface de eventos> <pacote da LG>");
        w.println("#   W <domínio> <interface org.dsi.ifc> <instância> <atraso em ms>   serviço que a HMI liga ao receber o anúncio (interface 9000)");
        w.println("#   A <serviço> <número> <constante ATTR_>                          atributo que a HMI pode assinar");
        w.println("#   E <serviço> <api> <atributo ou -> <entrega ou -> <evento> <mensagem ou ->");
        w.println("#        mensagem que o serviço manda. Com atributo, é a atualização dele (a api tem o mesmo número).");
        w.println("#        entrega: o número com que a HMI procura os ouvintes; quando difere do atributo, a");
        w.println("#        atualização só chega a quem também assinou esse outro (é assim no firmware).");
        w.println("#   P <serviço> <api> <método> <mensagem ou ->                      pedido que a HMI manda (9999 assina atributos, 9998 desassina)");
        w.println("#   R <serviço> <api do pedido> <tipo> <prazo em ms> <api da resposta ou -> <método do ouvinte>");
        w.println("#        pedido de que a HMI espera resposta, pela tabela do proxy DSI dela (Cfg.PT e Cfg.U). Uma linha");
        w.println("#        por resposta possível. tipo: uma = uma resposta só; escolha = uma dentre as listadas;");
        w.println("#        sequencia = todas; complexa = quem decide é um analisador da HMI. prazo: quanto ela espera antes");
        w.println("#        de registrar \"DSI timeout occurred\" (-1 = sem prazo). \"-\" = nenhuma api da camada da LG chama o método.");
        w.println("#   G <serviço> <api do pedido> <api do evento> <regra> <campo do pedido> <campo do evento>");
        w.println("#        o que o pedido set<X> grava no atributo de nome <X>, campo a campo. DEDUÇÃO do gerador: o firmware");
        w.println("#        não traz essa ligação. regra posicao = os campos do pedido são os do evento, número a número e");
        w.println("#        tipo a tipo, fora o validFlag; regra nomes = cada campo do pedido tem um só campo de mesmo nome e");
        w.println("#        tipo no evento (é a menos segura: conferir antes de ligar o serviço em \"grava\").");
        w.println("#   X <serviço> <api do pedido> <api da resposta> <campo do pedido> <campo da resposta>");
        w.println("#        o que a resposta devolve do pedido: campos de mesmo caminho e tipo nas duas mensagens. DEDUÇÃO.");
        w.println("#   M <mensagem>                                                    mensagem protobuf; seguem os campos dela");
        w.println("#   C <número> <nome> <tipo> <um|rep|pac> [<mensagem do campo>]      campo da mensagem da linha M de cima");
        w.println("#        tipo: sint32 e sint64 vão em zigue-zague; int32, int64, uint32 e enum, em varint comum;");
        w.println("#        fixed32, fixed64, bool, float, double, string, bytes; msg é outra mensagem.");
        w.println("#        um = campo simples; rep = repetido, um por vez; pac = repetido empacotado.");
        w.println("# No barramento: [canal:8][interface:4][instância:4][api:4][protobuf]. O canal é o remetente:");
        w.println("# os eventos saem com o domínio do serviço; os pedidos da HMI, com 0x8000.");
        int nE = 0, nA = 0, nP = 0, nR = 0, nG = 0, nX = 0;
        for (Servico sv : svs) {
            w.println("S " + sv.nome + " " + sv.dominio + " " + sv.ifPedidos + " " + sv.ifEventos + " " + sv.pacote);
            for (Map.Entry<Integer, String> at : sv.atributos.entrySet()) { w.println("A " + sv.nome + " " + at.getKey() + " " + at.getValue()); nA++; }
            for (Evento ev : sv.eventos) {
                w.println("E " + sv.nome + " " + ev.api + " " + (ev.attr == null ? "-" : ev.attr.toString()) + " " + (ev.entrega == null ? "-" : ev.entrega.toString())
                        + " " + ev.rotulo + " " + (ev.msg == null ? "-" : nomeMsg(ev.msg)));
                nE++;
            }
            for (Pedido p : sv.pedidos) { w.println("P " + sv.nome + " " + p.api + " " + p.metodo + " " + (p.msg == null ? "-" : nomeMsg(p.msg))); nP++; }
            for (Resposta r : sv.respostas) {
                for (int k = 0; k < r.nomes.size(); k++)
                    w.println("R " + sv.nome + " " + r.p.api + " " + r.tipo + " " + r.prazo + " " + (r.evs.get(k) == null ? "-" : Integer.toString(r.evs.get(k).api)) + " " + r.nomes.get(k));
                nR++;
            }
            for (String l : sv.linhasG) w.println(l);
            for (String l : sv.linhasX) w.println(l);
            nG += sv.gPosicao + sv.gNomes; nX += sv.linhasX.size();
        }
        for (String l : ligados) w.println(l);
        int nC = 0;
        for (Map.Entry<Class<?>, List<Campo>> me : mensagens.entrySet()) {
            String nm = nomeMsg(me.getKey().getName());
            w.println("M " + nm);
            for (Campo c : me.getValue()) {
                w.println("C " + c.num + " " + c.nome + " " + c.tipo + " " + c.rotulo + (c.sub == null ? "" : " " + nomeMsg(c.sub.getName())));
                nC++;
            }
        }
        w.close();

        // ---- vetores: toda folha de todo evento com valor, serializado pelas classes do firmware
        PrintWriter vp = abre(new File(out, "vetores-perfil.txt")), ve = abre(new File(out, "vetores-esperado.txt"));
        vp.println("# Perfil de PROVA do veiculostub: dá valor a toda folha de todo evento. GERADO por");
        vp.println("# driver/m8_esquema.sh; os valores não querem dizer nada. Não é perfil de carro.");
        ve.println("# O que as classes do firmware serializam para o vetores-perfil.txt, no formato do dsi_escuta.");
        int nV = 0, folhas = 0;
        for (Servico sv : svs) for (Evento ev : sv.eventos) {
            if (ev.msg == null) continue;
            List<String> linhas = new ArrayList<String>();
            Object msg = preenche(Class.forName(ev.msg, true, cl), "", linhas, new ArrayList<Class<?>>());
            byte[] by = (byte[]) msg.getClass().getMethod("toByteArray").invoke(msg);
            vp.println("[" + sv.nome + "." + ev.rotulo + "]");
            for (String l : linhas) vp.println(l);
            ve.println("canal=0x" + Long.toHexString(sv.dominio) + " if=" + sv.ifEventos + " inst=0 api=" + ev.api + " pb=" + hexa(by));
            nV++; folhas += linhas.size();
        }
        vp.close(); ve.close();

        // ---- constantes.txt: o que cada número quer dizer, para quem escreve um perfil
        PrintWriter kc = abre(new File(out, "constantes.txt"));
        kc.println("# Constantes das interfaces DSI dos serviços " + rotulo + " (firmware MU0883, HMI H40.272.304).");
        kc.println("# GERADO por driver/m8_esquema.sh a partir das classes do próprio firmware. Não editar à mão.");
        kc.println("# K <interface> <nome> <valor>. Os atributos (ATTR_) estão no esquema.txt e não se repetem aqui.");
        int nK = 0;
        List<String> ifcs = new ArrayList<String>();
        ifcs.add("org.dsi.ifc.base.DSIListener");
        ifcs.add("org.dsi.ifc.global.CarConstants");
        for (Servico sv : svs) ifcs.add(sv.ifc);
        for (String nome : ifcs) {
            try {
                Class<?> ifc = Class.forName(nome, false, cl);
                for (Field f : ifc.getDeclaredFields()) {
                    if (!Modifier.isStatic(f.getModifiers()) || f.getType() != int.class || f.getName().startsWith("ATTR_")) continue;
                    f.setAccessible(true);
                    kc.println("K " + nome.substring(nome.lastIndexOf('.') + 1) + " " + f.getName() + " " + f.getInt(null));
                    nK++;
                }
            } catch (Throwable t) { aviso("constantes de " + nome + ": " + t); }
        }
        kc.close();

        PrintWriter r = abre(new File(out, "relatorio.txt"));
        r.println("serviços: " + svs.size() + "; atributos: " + nA + "; eventos: " + nE + "; pedidos: " + nP
                + "; mensagens: " + mensagens.size() + "; campos: " + nC + "; vetores: " + nV + " (" + folhas + " folhas); constantes: " + nK);
        r.println("pedidos com resposta na tabela da HMI: " + nR + "; pedidos set… com regra de gravação (dedução): " + nG + "; campos devolvidos pelas respostas (dedução): " + nX);
        for (Servico sv : svs) {
            int comAttr = 0;
            for (Evento ev : sv.eventos) if (ev.attr != null) comAttr++;
            r.println(sv.nome + ": domínio " + sv.dominio + ", pedidos " + sv.ifPedidos + ", eventos " + sv.ifEventos + "; "
                    + sv.atributos.size() + " atributos, " + sv.eventos.size() + " eventos (" + comAttr + " de atributo), " + sv.pedidos.size() + " pedidos");
            for (Evento ev : sv.eventos) {
                if (ev.entrega != null && ev.attr == null) r.println("  evento " + ev.rotulo + " (api " + ev.api + "): entrega por atributo " + ev.entrega + ", mas não há constante ATTR_ com o número da api");
                if (ev.attr != null && ev.entrega.intValue() != ev.api) r.println("  evento " + ev.rotulo + " (api e atributo " + ev.api + "): a HMI entrega a quem assinou o atributo " + ev.entrega);
                String semApi = ev.rotulo.indexOf(':') < 0 ? ev.rotulo : ev.rotulo.substring(0, ev.rotulo.indexOf(':'));
                if (ev.attr != null && !("update" + sv.atributos.get(ev.attr).substring(5)).equalsIgnoreCase(semApi))
                    r.println("  evento " + ev.rotulo + " (api " + ev.api + "): o nome não casa com " + sv.atributos.get(ev.attr));
                if (!ev.metodo.equals(semApi))
                    r.println("  evento " + ev.rotulo + " (api " + ev.api + "): a HMI o entrega pelo método " + ev.metodo + " do ouvinte");
            }
            Set<Integer> comEvento = new HashSet<Integer>();
            for (Evento ev : sv.eventos) if (ev.attr != null) comEvento.add(ev.attr);
            for (Map.Entry<Integer, String> at : sv.atributos.entrySet()) if (!comEvento.contains(at.getKey())) r.println("  atributo " + at.getKey() + " " + at.getValue() + ": sem evento que o atualize");
            int semApi = 0;
            for (Resposta rp : sv.respostas) if (rp.evs.contains(null)) semApi++;
            r.println("  pedidos com resposta: " + sv.respostas.size() + (semApi > 0 ? " (" + semApi + " com resposta que a camada da LG não entrega)" : "")
                    + "; pedidos set…: " + sv.nSet + ", dos quais gravam atributo " + (sv.gPosicao + sv.gNomes) + " (" + sv.gPosicao + " pela regra posicao, " + sv.gNomes + " pela regra nomes)");
            for (String l : sv.relato) r.println("  " + l);
            for (String l : sv.setSemRegra) r.println("  set sem regra de gravação: " + l);
        }
        if (!fora.isEmpty()) {
            r.println("fora do esquema: " + fora.size() + " serviço(s) de que o despacho do domínio entrega eventos e que o anúncio dele não liga (são de outro dono)");
            for (Servico sv : fora) r.println("  " + sv.nome + ": eventos na interface " + sv.ifEventos + ", pelo domínio " + sv.dominio);
        }
        r.println("avisos: " + avisos.size());
        for (String s : avisos) r.println("  " + s);
        r.close();
        System.out.println("serviços " + svs.size() + ", atributos " + nA + ", eventos " + nE + ", pedidos " + nP + " (" + nR + " com resposta, " + nG + " que gravam), mensagens "
                + mensagens.size() + ", campos " + nC + ", vetores " + nV + ", avisos " + avisos.size());
    }
}
