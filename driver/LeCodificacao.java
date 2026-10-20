import java.io.File;
import java.io.PrintWriter;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import com.microdoc.util.ObjectSwapper;

/* Marco 6 (11/10/2026). Lê, com as classes do próprio firmware, o que a HMI entende por
 * codificação e adaptação: cada campo com o byte, o bit, o tamanho, o valor padrão e os nomes.
 * Só leitura; estudo num computador, nada vai para o carro.
 *
 * De onde sai cada coisa:
 *   - posição, tamanho, tipo, padrão e valores aceitos: da tabela interna das classes de acesso
 *     (de.vw.mib.asl.framework.internal.diagnosis.variant.<variante>.*Impl), lida do objeto já
 *     construído, do jeito que o AccessImpl da HMI a percorre;
 *   - nome da constante: das interfaces de.vw.mib.asl.framework.api.diagnosis.config.*;
 *   - nome do requisito, descrição e significado de cada valor: dos arquivos .swap de
 *     /vw/hmi/config, abertos com o ObjectSwapper do firmware (é o que a tela de diagnóstico da
 *     própria HMI mostra, ver AslDiagGenericFetcher);
 *   - a divisão em identificadores (DID) e seus tamanhos: das listas que cada classe passa ao
 *     AccessImpl (chaves e comprimentos).
 *
 * Saída: tabela-campos.tsv (uma linha por campo), padrao.txt (os conjuntos com cada campo no
 * valor padrão, no formato que o driver/diagstub.c lê) e um resumo na tela, com a conferência
 * de que o padrão codificado dá o mesmo resultado que a HMI sem dado nenhum.
 *
 * uso: LeCodificacao <pasta de config> <variante> <pasta de saída>
 */
public class LeCodificacao {
    static final String P = "de.vw.mib.asl.framework.";
    static final int T_BOOL = 10037, T_INT = 20051, T_BYTES = 30029, E_RANGE = 1091, E_ARRAY = 2029;

    /* responde "nada" a qualquer chamada: interface devolve outro igual, primitivo devolve zero */
    static Object oco(final Class<?> c) {
        return Proxy.newProxyInstance(LeCodificacao.class.getClassLoader(), new Class<?>[] { c }, new InvocationHandler() {
            public Object invoke(Object p, Method m, Object[] a) {
                Class<?> r = m.getReturnType();
                if (m.getName().equals("toString")) return "oco";
                if (m.getName().equals("hashCode")) return Integer.valueOf(1);
                if (m.getName().equals("equals")) return Boolean.valueOf(p == a[0]);
                if (r == boolean.class) return Boolean.FALSE;
                if (r == int.class) return Integer.valueOf(0);
                if (r == long.class) return Long.valueOf(0);
                if (r == void.class || r.isPrimitive()) return null;
                if (r.isInterface()) return oco(r);
                return null;
            }
        });
    }

    static Object validade(final boolean v) throws Exception {
        Class<?> c = Class.forName(P + "internal.diagnosis.DiagnosisValidityProvider");
        return Proxy.newProxyInstance(LeCodificacao.class.getClassLoader(), new Class<?>[] { c }, new InvocationHandler() {
            public Object invoke(Object p, Method m, Object[] a) {
                if (m.getReturnType() == boolean.class) return Boolean.valueOf(m.getName().equals("isDataValid") ? v : false);
                return m.getName().equals("toString") ? "validade" : null;
            }
        });
    }

    static Field campo(Class<?> c, String n) throws Exception {
        for (; c != null; c = c.getSuperclass()) {
            try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f; } catch (NoSuchFieldException e) { }
        }
        throw new NoSuchFieldException(n);
    }

    static void interfaces(Class<?> c, Set<Class<?>> out) {
        for (; c != null; c = c.getSuperclass())
            for (Class<?> i : c.getInterfaces()) if (out.add(i)) interfaces(i, out);
    }

    static String hex(byte[] b, int off, int n) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < n; i++) s.append(i > 0 ? " " : "").append(String.format("%02X", b[off + i] & 0xff));
        return s.toString();
    }

    static String limpa(String s) { return s == null ? "" : s.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ').trim(); }

    public static void main(String[] a) throws Exception {
        String cfgDir = a[0], variante = a[1], out = a[2];
        new File(out).mkdirs();

        /* o AccessImpl pede o registrador ao arcabouço da HMI, que aqui não está de pé */
        Class<?> ff = Class.forName(P + "api.framework.ASLFrameworkFactory");
        Field inst = ff.getDeclaredField("frameworkApiInstance");
        inst.setAccessible(true);
        inst.set(null, oco(Class.forName(P + "api.framework.ASLFrameworkAPI")));

        String[][] conjuntos = {
            /* nome, classe, arquivo .swap, nome do conjunto no registro da HMI */
            { "codificacao", "CodingImpl", "coding.swap" },
            { "adaptacao", "AdaptationImpl", "adaptation.swap" },
            { "funcoes-do-carro", "CarFuncAdapImpl", "carfunction.swap" },
            { "registros", "RecordsImpl", null },
            { "identificacao", "IdentificationImpl", "identification.swap" },
        };
        PrintWriter tsv = new PrintWriter(new File(out, "tabela-campos.tsv"), "UTF-8");
        tsv.println("conjunto\tnamespace\tdid_hex\tdid_dec\tbyte_no_did\tbyte_no_conjunto\tbit_inicial\tbits\ttipo\tpadrao\tvalores_aceitos\tid\tconstante\trequisito\tdescricao\tsignificado_dos_valores\tfonte");
        PrintWriter pad = new PrintWriter(new File(out, "padrao.txt"), "UTF-8");
        pad.println("# Gerado por driver/LeCodificacao.java (variante " + variante + ") com as classes do firmware. Não editar à mão:");
        pad.println("# para mudar um campo, usar um arquivo de ajustes à parte (ver README.md desta pasta).");
        pad.println("# Cada conjunto vai com TODOS os campos no valor padrão da HMI, o que ela usa quando não recebe");
        pad.println("# dado. NÃO é a codificação de carro nenhum.");
        pad.println("# Formato: <identificador (DID) em hexa> = <bytes em hexa>");
        StringBuilder extras = new StringBuilder();

        for (String[] cj : conjuntos) {
            String nome = cj[0];
            Class<?> impl = Class.forName(P + "internal.diagnosis.variant." + variante + "." + cj[1]);
            Constructor<?> ct = impl.getConstructor(byte[].class, Class.forName(P + "internal.diagnosis.DiagnosisValidityProvider"));
            Object vazio = ct.newInstance(new byte[0], validade(false));
            int[] cfg = (int[]) campo(impl, "mConfiguration").get(vazio);
            Object eng = campo(impl, "mEngine").get(vazio);
            int[] lens = (int[]) campo(impl, "allowedLength").get(vazio);
            long[] keys = (long[]) campo(impl, "keys").get(vazio);
            int ns = campo(impl, "namespace").getInt(vazio);
            Method gi0 = eng.getClass().getMethod("getInt");
            Method gi2 = eng.getClass().getMethod("getInt", int.class, int.class);
            int total = 0;
            for (int l : lens) total += l;

            /* constantes ID_* de todas as interfaces da classe */
            Set<Class<?>> ifs = new LinkedHashSet<Class<?>>();
            interfaces(impl, ifs);
            Map<Integer, List<String>> nomes = new TreeMap<Integer, List<String>>();
            Map<String, Integer> porNome = new HashMap<String, Integer>();
            for (Class<?> i : ifs)
                for (Field f : i.getDeclaredFields())
                    if (Modifier.isStatic(f.getModifiers()) && f.getType() == int.class && f.getName().startsWith("ID_")) {
                        int id = f.getInt(null);
                        if (!nomes.containsKey(id)) nomes.put(id, new ArrayList<String>());
                        if (!nomes.get(id).contains(f.getName())) nomes.get(id).add(f.getName());
                        if (!porNome.containsKey(f.getName())) porNome.put(f.getName(), id);
                    }

            /* metadados do .swap: [requisito, descrição, tipo, constante, n, (valor, texto)*] */
            Map<Integer, String[]> meta = new HashMap<Integer, String[]>();
            int linhasSwap = 0, semId = 0;
            if (cj[2] != null && new File(cfgDir, cj[2]).exists()) {
                ObjectSwapper s = new ObjectSwapper();
                s.open(new File(cfgDir, cj[2]));
                Object[] data = s.linkAll();
                s.close();
                for (Object o : data) {
                    if (!(o instanceof String[])) continue;
                    String[] l = (String[]) o;
                    linhasSwap++;
                    if (l.length < 5) continue;
                    Integer id = porNome.get(l[3]);
                    if (id == null) { semId++; continue; }
                    if (!meta.containsKey(id)) meta.put(id, l);
                }
            }

            byte[] def = new byte[total];
            int nCampos = 0, nBool = 0, nInt = 0, nBytes = 0, foraDoConjunto = 0;
            List<int[]> campos = new ArrayList<int[]>();       /* id, tipo, bits, byte, bit, padrão */
            for (Map.Entry<Integer, List<String>> e : nomes.entrySet()) {
                int id = e.getKey();
                int idx = (Integer) gi2.invoke(eng, gi0.invoke(eng), id);
                if (idx < 0) continue;
                int tipo = cfg[idx], bits = cfg[idx + 1], sb = cfg[idx + 2], bit = cfg[idx + 3], d = cfg[idx + 4];
                nCampos++;
                String tipoTxt, padTxt, aceitos = "";
                if (tipo == T_BOOL) {
                    nBool++; tipoTxt = "bool"; padTxt = d != 0 ? "1" : "0";
                    if (sb >= 0 && sb < total) { if (d != 0) def[sb] |= (byte) (1 << bit); } else foraDoConjunto++;
                } else if (tipo == T_INT) {
                    nInt++; tipoTxt = "int"; padTxt = Integer.toString(d);
                    int cp = cfg[idx + 5];
                    if (cp >= 0) {
                        StringBuilder sbd = new StringBuilder();
                        int n = cfg[cp++];
                        for (int k = 0; k < n; k++) {
                            int pos = cfg[cp++], ct2 = cfg[pos++];
                            if (sbd.length() > 0) sbd.append(", ");
                            if (ct2 == E_RANGE) { int f1 = cfg[pos++], l1 = cfg[pos++]; sbd.append(f1 == l1 ? "" + f1 : f1 + ".." + l1); }
                            else if (ct2 == E_ARRAY) { int en = cfg[pos++]; for (int q = 0; q < en; q++) sbd.append(q > 0 ? ", " : "").append(cfg[pos++]); }
                            else sbd.append("?");
                        }
                        aceitos = sbd.toString();
                    }
                    if (bits <= 8) {
                        if (sb >= 0 && sb < total) { int m = ((1 << bits) - 1) << bit; def[sb] = (byte) ((def[sb] & ~m) | ((d << bit) & m)); } else foraDoConjunto++;
                    } else if ((bits & 7) == 0 && bit == 0) {
                        int nb = bits >> 3;
                        if (sb >= 0 && sb + nb <= total) for (int k = 0; k < nb; k++) def[sb + k] = (byte) (d >>> (8 * (nb - 1 - k))); else foraDoConjunto++;
                    }
                } else if (tipo == T_BYTES) {
                    nBytes++; tipoTxt = "bytes";
                    int nb = bits >> 3, dp = cfg[idx + 4];
                    if (dp < 0) padTxt = "(vazio)";
                    else {
                        byte[] db = new byte[nb];
                        int cnt = 0, v = cfg[dp++];
                        for (int k = 0; k < nb; k++) { db[k] = (byte) (v & 0xff); if (++cnt >= 4) { cnt = 0; v = cfg[dp++]; } else v >>= 8; }
                        padTxt = hex(db, 0, nb);
                        if (sb >= 0 && sb + nb <= total) System.arraycopy(db, 0, def, sb, nb); else foraDoConjunto++;
                    }
                } else { tipoTxt = "?" + tipo; padTxt = ""; }

                /* a que identificador pertence o byte inicial */
                long did = -1; int offDid = sb, acc = 0;
                for (int k = 0; k < lens.length; k++) { if (sb < acc + lens[k]) { did = keys[k]; offDid = sb - acc; break; } acc += lens[k]; }
                String[] m = meta.get(id);
                StringBuilder sig = new StringBuilder();
                if (m != null) {
                    int n = 0;
                    try { n = Integer.decode(m[4]); } catch (Exception ex) { }
                    for (int k = 0; k < n && 6 + 2 * k < m.length; k++) sig.append(k > 0 ? "; " : "").append(limpa(m[5 + 2 * k])).append(" = ").append(limpa(m[6 + 2 * k]));
                }
                StringBuilder cs = new StringBuilder();
                for (String nm : e.getValue()) cs.append(cs.length() > 0 ? " " : "").append(nm);
                tsv.println(nome + "\t0x" + Integer.toHexString(ns) + "\t" + (did < 0 ? "" : String.format("%04X", did)) + "\t" + did + "\t" + offDid + "\t" + sb + "\t" + bit + "\t" + bits
                    + "\t" + tipoTxt + "\t" + padTxt + "\t" + aceitos + "\t" + id + "\t" + cs + "\t" + (m != null ? limpa(m[0]) : "") + "\t" + (m != null ? limpa(m[1]) : "") + "\t" + sig
                    + "\t" + "variant." + variante + "." + cj[1] + (m != null ? " + " + cj[2] : ""));
                campos.add(new int[] { id, tipo, bits, sb, bit, d });
            }

            /* conferência: padrão codificado e válido == sem dado nenhum (o que a HMI faz hoje) */
            Object comPadrao = ct.newInstance(def.clone(), validade(true));
            Object comZeros = ct.newInstance(new byte[total], validade(true));
            Method gb = impl.getMethod("getBoolean", int.class), gv = impl.getMethod("getValue", int.class), ga = impl.getMethod("getByteArray", int.class);
            int difPadrao = 0, difZeros = 0;
            StringBuilder quais = new StringBuilder(), quaisZ = new StringBuilder();
            for (int[] c : campos) {
                Method g = c[1] == T_BOOL ? gb : c[1] == T_INT ? gv : ga;
                Object r0 = g.invoke(vazio, c[0]), r1 = g.invoke(comPadrao, c[0]), r2 = g.invoke(comZeros, c[0]);
                boolean i1 = c[1] == T_BYTES ? Arrays.equals((byte[]) r0, (byte[]) r1) : r0.equals(r1);
                boolean i2 = c[1] == T_BYTES ? Arrays.equals((byte[]) r0, (byte[]) r2) : r0.equals(r2);
                String n1 = nomes.get(c[0]).get(0);
                if (!i1) { difPadrao++; if (difPadrao <= 12) quais.append("\n      ").append(n1).append(" (id ").append(c[0]).append("): sem dado=").append(txt(r0)).append(", com o padrão=").append(txt(r1)); }
                if (!i2) { difZeros++; if (difZeros <= 400) quaisZ.append("\n      ").append(n1).append(" (id ").append(c[0]).append("): padrão=").append(txt(r0)).append(", com zeros=").append(txt(r2)); }
            }

            System.out.println("== " + nome + " (namespace 0x" + Integer.toHexString(ns) + ", " + total + " bytes, " + keys.length + " identificador(es))");
            System.out.println("   classe: " + impl.getName());
            System.out.println("   campos: " + nCampos + " (bool " + nBool + ", int " + nInt + ", bytes " + nBytes + "); fora do conjunto: " + foraDoConjunto
                + "; linhas no .swap: " + linhasSwap + " (com nome achado: " + meta.size() + ", sem: " + semId + ")");
            System.out.println("   conferência do padrão codificado: " + (difPadrao == 0 ? "igual à HMI sem dado em todos os campos" : difPadrao + " campo(s) diferem:" + quais));
            System.out.println("   com tudo zero e válido, campos que mudariam em relação ao padrão: " + difZeros + quaisZ);

            pad.println();
            pad.println("# " + nome + " (namespace 0x" + Integer.toHexString(ns) + ", " + total + " bytes)");
            int acc = 0;
            String f1b1 = "";
            for (int k = 0; k < lens.length; k++) {
                String linha = String.format("%04X", keys[k]) + " = " + hex(def, acc, lens[k]);
                /* Estes a HMI nunca procura na tabela dela: pede pelo barramento ao persistence-app
                 * (DSIPersistenceImpl.readBuffer). As chaves 1 e 2 dos registros são os dois bytes
                 * do identificador F1B1 (DiagHandler::event_handler do persistence-app). */
                if (keys[k] == 29195L || keys[k] == 29196L) extras.append("# ").append(linha).append("\n");
                else if (keys[k] == 1L || keys[k] == 2L) f1b1 += (f1b1.length() > 0 ? " " : "") + hex(def, acc, lens[k]);
                else pad.println(linha);
                acc += lens[k];
            }
            if (f1b1.length() > 0) extras.append("# F1B1 = ").append(f1b1).append("\n");
            /* a outra variante tem a mesma tabela? */
            String outra = variante.equals("oilarge") ? "oismall" : "oilarge";
            try {
                Class<?> impl2 = Class.forName(P + "internal.diagnosis.variant." + outra + "." + cj[1]);
                Object v2 = impl2.getConstructor(byte[].class, Class.forName(P + "internal.diagnosis.DiagnosisValidityProvider")).newInstance(new byte[0], validade(false));
                int[] cfg2 = (int[]) campo(impl2, "mConfiguration").get(v2);
                System.out.println("   tabela da variante " + outra + ": " + (Arrays.equals(cfg, cfg2) ? "idêntica" : "DIFERENTE"));
            } catch (ClassNotFoundException ex) { System.out.println("   variante " + outra + ": não existe"); }
        }
        pad.println();
        pad.println("# Fora do conjunto padrão: registros que a HMI pede ao persistence-app pelo barramento.");
        pad.println("# O 720B faz o persistence-app regravar o idioma da HMI (/data/lge/hmi/lsmval), e com o valor");
        pad.println("# padrão abaixo ele gravaria um idioma sem sentido. Só tirar o '#' para estudar esse caminho.");
        pad.print(extras.toString());
        tsv.close();
        pad.close();
        System.out.println("gravado: " + out + "/tabela-campos.tsv e " + out + "/padrao.txt");
    }

    static String txt(Object o) { return o instanceof byte[] ? "[" + hex((byte[]) o, 0, ((byte[]) o).length) + "]" : String.valueOf(o); }
}
