import java.io.File;
import com.microdoc.util.ObjectSwapper;

/* Lê /vw/hmi/res/persistenceIds.res com o leitor do próprio firmware e o interpreta como o
 * PersistenceKeyProviderImpl da HMI faz: a lista dos namespaces da memória de estado, o nome do
 * módulo dono de cada um, os "persistables" de cada módulo (os comuns e os que têm parte por
 * usuário) e a versão. Só leitura; estudo num computador.
 * Uso: driver/m6_persistence_ids.sh (compila e roda dentro do chroot do firmware). */
public class LePersistenceIds {
    public static void main(String[] a) throws Exception {
        ObjectSwapper s = new ObjectSwapper();
        s.open(new File(a[0]));
        Object[] data = s.linkAll();
        s.close();
        System.out.println("itens no arquivo: " + data.length);
        int[] ns = (int[]) data[0];
        String[] nomes = (String[]) data[1];
        int[] ids = (int[]) data[2];
        long[] ver = (long[]) data[3];
        int n = ns[0], p = 0, linhas = 0, porUsuario = 0;
        System.out.println("namespaces: " + n);
        System.out.println("namespace\tmódulo\tversão\tpersistables comuns\tpersistables com parte por usuário");
        for (int i = 0; i < n; i++) {
            StringBuilder c = new StringBuilder(), u = new StringBuilder();
            int nc = ids[p++];
            for (int j = 0; j < nc; j++) c.append(ids[p++]).append(j + 1 < nc ? "," : "");
            int nu = ids[p++];
            for (int j = 0; j < nu; j++) u.append(ids[p++]).append(j + 1 < nu ? "," : "");
            linhas += 1 + nc + nu; porUsuario += nu;
            System.out.println(ns[i + 1] + "\t" + nomes[i] + "\t" + ver[i] + "\t" + (nc == 0 ? "-" : c.toString()) + "\t" + (nu == 0 ? "-" : u.toString()));
        }
        System.out.println("inteiros de persistables lidos: " + p + " de " + ids.length);
        // cada bloco grava a parte comum; o bloco com parte por usuário grava mais uma linha por usuário
        System.out.println("linhas possíveis sem contar as de usuário (uma declaração por módulo + a parte comum de cada bloco): " + linhas);
        System.out.println("linhas a mais por usuário (uma por bloco com parte por usuário): " + porUsuario);
        System.out.println("com um usuário: " + (linhas + porUsuario));
        if (data.length > 4) {
            int[] pso = (int[]) data[4];
            StringBuilder b = new StringBuilder();
            for (int i = 0; i < pso[0]; i++) b.append(pso[i + 1]).append(' ');
            System.out.println("PSO ids (" + pso[0] + "): " + b);
        }
    }
}
