import java.io.File;
import com.microdoc.util.ObjectSwapper;

/* Lê a tabela de teclas da HMI (item 24 do configurationmanager.res) com o leitor do próprio
 * firmware e a interpreta como o HardKeyReader da HMI faz. Só leitura; estudo num computador. */
public class LeTabelaTeclas {
    public static void main(String[] a) throws Exception {
        ObjectSwapper s = new ObjectSwapper();
        s.open(new File(a[0]));
        Object[] data = s.linkAll();
        s.close();
        System.out.println("itens no arquivo: " + data.length);
        int[] d = (int[]) data[24];
        StringBuilder raw = new StringBuilder();
        for (int v : d) raw.append(v).append(' ');
        System.out.println("bruto (" + d.length + " inteiros): " + raw);
        int p = 0, ver = d[p++];
        if (ver >= 0) { ver = 0; p = 0; }
        System.out.println("versao da tabela: " + ver);
        if (ver < -2) {
            int n = d[p++];
            System.out.println("teclas: " + n);
            for (int i = 0; i < n; i++) {
                int key = d[p++];
                int nk = d[p++];
                StringBuilder kb = new StringBuilder();
                for (int j = 0; j < nk; j++) kb.append(d[p++]).append(j + 1 < nk ? "," : "");
                int np = d[p++];
                StringBuilder pr = new StringBuilder();
                for (int j = 0; j < np; j++) { int st = d[p++], ev = d[p++]; pr.append(st).append("->").append(ev).append(' '); }
                int lp = d[p++], ulp = d[p++];
                System.out.println("tecla " + key + " teclados [" + kb + "] estado->evento: " + pr + "longo=" + lp + " ultralongo=" + ulp);
            }
        } else {
            int nt = d[p++];
            System.out.println("tempos: " + nt);
            for (int i = 0; i < nt; i++) {
                int key = d[p++];
                StringBuilder kb = new StringBuilder();
                if (ver <= -2) { int nk = d[p++]; for (int j = 0; j < nk; j++) kb.append(d[p++]).append(j + 1 < nk ? "," : ""); }
                int lp = d[p++], ulp = d[p++];
                System.out.println("tempo: tecla " + key + " teclados [" + kb + "] longo=" + lp + " ultralongo=" + ulp);
            }
            int nm = d[p++];
            System.out.println("mapeamentos: " + nm);
            for (int i = 0; i < nm; i++) {
                int key = d[p++], np = d[p++];
                StringBuilder pr = new StringBuilder();
                for (int j = 0; j < np; j++) { int st = d[p++], ev = d[p++]; pr.append(st).append("->").append(ev).append(' '); }
                System.out.println("mapa: tecla " + key + " estado->evento: " + pr);
            }
        }
        System.out.println("lidos " + p + " de " + d.length);
    }
}
