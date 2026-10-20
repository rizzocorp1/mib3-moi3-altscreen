#!/usr/bin/env python3
"""Marco 0 (sessão de modelagem do SoC, 11/10/2026): análise ESTÁTICA no host do que o
m0_inventario.sh despejou. Só lê bytes: não executa nada do firmware. Rodar com `python3 -I`.

  censo   <system_listing.tsv> <system_heads.bin>   censo de ELF (classe/máquina/tipo)
  deps    <raiz> <arquivo...>                       NEEDED/SONAME/RUNPATH/GLIBC máx de cada ELF
  syms    <arquivo> [undef|def]                     símbolos dinâmicos importados/exportados
  strs    <arquivo> <regex>                         strings imprimíveis que casam com a regex
  bootimg <boot.img> <dir-de-saida>                 cabeçalho Android, versão do kernel, DTBs
"""
import collections
import gzip
import os
import re
import struct
import sys
import zlib

EM = {3: "x86", 40: "ARM(32)", 62: "x86-64", 183: "AArch64"}
ET = {1: "REL", 2: "EXEC", 3: "DYN", 4: "CORE"}


# ---------------------------------------------------------------- censo
def censo(listing, heads):
    tam = {}
    tipos = collections.Counter()
    with open(listing, "rb") as f:
        for ln in f:
            p = ln.rstrip(b"\n").split(b"\t")
            if len(p) < 6:
                continue
            tipos[p[0].decode()] += 1
            if p[0] == b"f":
                tam[p[4]] = int(p[3])
    print("entradas por tipo:", dict(tipos), "| bytes em arquivos:", sum(tam.values()))
    data = open(heads, "rb").read()
    pos, elf, n = 0, collections.Counter(), 0
    por_dir = collections.Counter()
    elf32, exemplos = [], collections.defaultdict(list)
    pref = b"/mnt/low/"
    while pos < len(data):
        if data[pos:pos + 1] == b"\n":
            pos += 1
        if data[pos:pos + 4] != b"==> ":
            raise SystemExit(f"parser perdido em {pos}: {data[pos:pos+40]!r}")
        fim = data.index(b" <==\n", pos)
        nome = data[pos + 4:fim]
        rel = nome[len(pref):] if nome.startswith(pref) else nome
        k = min(20, tam.get(rel, 20))
        h = data[fim + 5:fim + 5 + k]
        pos = fim + 5 + k
        n += 1
        if h[:4] != b"\x7fELF" or len(h) < 20:
            continue
        cls = {1: "ELF32", 2: "ELF64"}.get(h[4], "?")
        en = "<" if h[5] == 1 else ">"
        et, em = struct.unpack(en + "HH", h[16:20])
        chave = (cls, EM.get(em, f"em{em}"), ET.get(et, f"et{et}"))
        elf[chave] += 1
        r = rel.decode("utf-8", "replace")
        por_dir["/".join(r.split("/")[:2])] += 1
        if len(exemplos[chave]) < 5:
            exemplos[chave].append(r)
        if cls == "ELF32":
            elf32.append(r)
    print(f"arquivos regulares lidos: {n} | ELF: {sum(elf.values())}")
    for k, v in elf.most_common():
        print(f"  {v:6d}  {' '.join(k)}   ex.: {', '.join(exemplos[k][:3])}")
    print("ELF por diretório (top 15):")
    for d, v in por_dir.most_common(15):
        print(f"  {v:6d}  /{d}")
    print(f"ELF de 32 bits: {len(elf32)}")
    for r in elf32[:40]:
        print("   ", r)


# ---------------------------------------------------------------- ELF
class Elf:
    def __init__(self, path):
        self.path = path
        self.d = open(path, "rb").read()
        d = self.d
        if d[:4] != b"\x7fELF":
            raise ValueError("não é ELF")
        self.c64 = d[4] == 2
        self.en = "<" if d[5] == 1 else ">"
        en = self.en
        if self.c64:
            (self.etype, self.mach, _, _, phoff, shoff, _, _, phes, phn, shes, shn, _) = \
                struct.unpack(en + "HHIQQQIHHHHHH", d[16:64])
        else:
            (self.etype, self.mach, _, _, phoff, shoff, _, _, phes, phn, shes, shn, _) = \
                struct.unpack(en + "HHIIIIIHHHHHH", d[16:52])
        self.loads, self.dyn, self.interp = [], None, None
        for i in range(phn):
            o = phoff + i * phes
            if self.c64:
                pt, _, off, va, _, fs, _, _ = struct.unpack(en + "IIQQQQQQ", d[o:o + 56])
            else:
                pt, off, va, _, fs, _, _, _ = struct.unpack(en + "IIIIIIII", d[o:o + 32])
            if pt == 1:
                self.loads.append((va, off, fs))
            elif pt == 2:
                self.dyn = (off, fs)
            elif pt == 3:
                self.interp = d[off:off + fs].rstrip(b"\0").decode()
        self.secs = []
        if shoff and shn:
            for i in range(shn):
                o = shoff + i * shes
                if self.c64:
                    nm, ty, _, _, off, sz, lk, _, _, es = struct.unpack(en + "IIQQQQIIQQ", d[o:o + 64])
                else:
                    nm, ty, _, _, off, sz, lk, _, _, es = struct.unpack(en + "IIIIIIIIII", d[o:o + 40])
                self.secs.append((ty, off, sz, lk, es))
        self._dyn()

    def off(self, va):
        for v, o, s in self.loads:
            if v <= va < v + s:
                return o + va - v
        return None

    def _dyn(self):
        self.needed, self.soname, self.runpath, self.tags = [], None, None, {}
        if not self.dyn:
            return
        o, s = self.dyn
        w, fmt = (16, "qQ") if self.c64 else (8, "iI")
        ent = []
        for i in range(0, s, w):
            t, v = struct.unpack(self.en + fmt, self.d[o + i:o + i + w])
            if t == 0:
                break
            ent.append((t, v))
            self.tags.setdefault(t, v)
        st = self.off(self.tags.get(5, 0)) if 5 in self.tags else None
        self.strtab = st

        def s_at(x):
            if st is None:
                return "?"
            e = self.d.index(b"\0", st + x)
            return self.d[st + x:e].decode("utf-8", "replace")
        self.s_at = s_at
        for t, v in ent:
            if t == 1:
                self.needed.append(s_at(v))
            elif t == 14:
                self.soname = s_at(v)
            elif t in (15, 29):
                self.runpath = s_at(v)

    def glibc_max(self):
        """Maior versão GLIBC_x.y exigida (seção verneed)."""
        vn = self.tags.get(0x6ffffffe)
        num = self.tags.get(0x6fffffff, 0)
        if not vn or self.strtab is None:
            return None
        o = self.off(vn)
        best = None
        for _ in range(num):
            _, cnt, _, aux, nxt = struct.unpack(self.en + "HHIII", self.d[o:o + 16])
            a = o + aux
            for _ in range(cnt):
                _, _, _, name, anx = struct.unpack(self.en + "IHHII", self.d[a:a + 16])
                m = re.fullmatch(r"GLIBC_(\d+)\.(\d+)(?:\.(\d+))?", self.s_at(name))
                if m:
                    v = tuple(int(x or 0) for x in m.groups())
                    best = max(best, v) if best else v
                if not anx:
                    break
                a += anx
            if not nxt:
                break
            o += nxt
        return ".".join(str(x) for x in best[:2]) if best else None

    def dynsyms(self):
        """[(nome, definido?, tipo)] da .dynsym (precisa de section headers)."""
        out = []
        for ty, off, sz, lk, es in self.secs:
            if ty != 11 or not es:
                continue
            sty, soff, ssz, _, _ = self.secs[lk]
            for i in range(1, sz // es):
                e = self.d[off + i * es:off + (i + 1) * es]
                if self.c64:
                    nm, info, _, shndx, _, _ = struct.unpack(self.en + "IBBHQQ", e)
                else:
                    nm, _, _, info, _, shndx = struct.unpack(self.en + "IIIBBH", e)
                fim = self.d.index(b"\0", soff + nm)
                out.append((self.d[soff + nm:fim].decode("utf-8", "replace"), shndx != 0, info & 15))
        return out


def deps(raiz, arquivos):
    for a in arquivos:
        p = a if os.path.isabs(a) else os.path.join(raiz, a)
        try:
            e = Elf(os.path.realpath(p) if not os.path.islink(p) else resolve(raiz, p))
        except Exception as ex:
            print(f"{a}: {ex}")
            continue
        print(f"{a}  [{'ELF64' if e.c64 else 'ELF32'} {EM.get(e.mach, e.mach)} {ET.get(e.etype, e.etype)}]"
              f" {len(e.d)} bytes")
        if e.interp:
            print("   interp :", e.interp)
        if e.soname:
            print("   soname :", e.soname)
        if e.runpath:
            print("   runpath:", e.runpath)
        print("   glibc≤ :", e.glibc_max())
        print("   needed :", ", ".join(e.needed) or "—")


def resolve(raiz, p, n=0):
    """Segue symlink absoluto/relativo SEM sair da raiz extraída."""
    while os.path.islink(p) and n < 20:
        t = os.readlink(p)
        p = os.path.join(raiz, t.lstrip("/")) if t.startswith("/") else os.path.join(os.path.dirname(p), t)
        n += 1
    return p


def syms(arq, modo="undef"):
    e = Elf(arq)
    for nome, definido, _ in sorted(set(e.dynsyms())):
        if definido == (modo == "def") and nome:
            print(nome)


def strs(arq, rx):
    r = re.compile(rx.encode())
    vistos = set()
    for m in re.finditer(rb"[\x20-\x7e]{4,}", open(arq, "rb").read()):
        s = m.group()
        if r.search(s) and s not in vistos:
            vistos.add(s)
            print(s.decode())


# ---------------------------------------------------------------- boot.img
def fdt_props(blob):
    """Devolve model/compatible/qcom,* do nó raiz de um DTB."""
    magic, total, o_struct, o_str = struct.unpack(">IIII", blob[:16])
    if magic != 0xd00dfeed:
        return {}
    p, depth, out = o_struct, 0, {}
    while p < total:
        tok = struct.unpack(">I", blob[p:p + 4])[0]
        p += 4
        if tok == 1:
            e = blob.index(b"\0", p)
            p = (e + 4) & ~3
            depth += 1
        elif tok == 2:
            depth -= 1
            if depth <= 0:
                break
        elif tok == 3:
            ln, no = struct.unpack(">II", blob[p:p + 8])
            p += 8
            val = blob[p:p + ln]
            p = (p + ln + 3) & ~3
            if depth == 1:
                e = blob.index(b"\0", o_str + no)
                nome = blob[o_str + no:e].decode()
                if nome in ("model", "compatible") or nome.startswith("qcom,"):
                    if nome in ("model", "compatible"):
                        out[nome] = [x.decode("utf-8", "replace") for x in val.split(b"\0") if x]
                    else:
                        out[nome] = [hex(x) for x in struct.unpack(">%dI" % (ln // 4), val[:ln // 4 * 4])]
        elif tok == 9:
            break
    return out


def bootimg(path, outdir):
    d = open(path, "rb").read()
    if d[:8] != b"ANDROID!":
        raise SystemExit("não é Android boot image")
    (ksz, kaddr, rsz, raddr, ssz, saddr, tags, page, dtsz_or_ver, osver) = struct.unpack("<10I", d[8:48])
    nome = d[48:64].rstrip(b"\0").decode()
    cmd = (d[64:64 + 512].rstrip(b"\0") + d[608:608 + 1024].rstrip(b"\0")).decode("utf-8", "replace")
    print(f"boot.img: {len(d)} bytes | page={page} | kernel={ksz} @0x{kaddr:x} | ramdisk={rsz} @0x{raddr:x}"
          f" | second={ssz} | tags@0x{tags:x} | campo[40]={dtsz_or_ver} | os_version=0x{osver:x} | name='{nome}'")
    print("cmdline:", cmd)
    k = d[page:page + ksz]
    os.makedirs(outdir, exist_ok=True)
    print("kernel começa com:", k[:8].hex(), "(1f8b = gzip; 'UNCOMPRE' = Image crua da Qualcomm)")
    img, resto = k, b""
    if k[:16] == b"UNCOMPRESSED_IMG":
        n = struct.unpack("<I", k[16:20])[0]
        img, resto = k[20:20 + n], k[20 + n:]
        print(f"cabeçalho UNCOMPRESSED_IMG: Image de {n} bytes | sobra (DTBs anexados): {len(resto)} bytes")
    elif k[:2] == b"\x1f\x8b":
        z = zlib.decompressobj(16 + zlib.MAX_WBITS)
        img = z.decompress(k)
        resto = z.unused_data
        print(f"Image descomprimida: {len(img)} bytes | sobra após o gzip (DTBs anexados): {len(resto)} bytes")
    open(os.path.join(outdir, "Image"), "wb").write(img)
    m = re.search(rb"Linux version [\x20-\x7e]+", img)
    print("banner:", m.group().decode() if m else "?")
    if img[56:60] == b"ARMd":
        text_off, isz, flags = struct.unpack("<QQQ", img[8:32])
        print(f"cabeçalho arm64 Image: text_offset=0x{text_off:x} image_size={isz} flags=0x{flags:x}"
              f" (page size bits={(flags >> 1) & 3}: 1=4K 2=16K 3=64K)")
    ik = img.find(b"IKCFG_ST")
    if ik >= 0:
        fim = img.find(b"IKCFG_ED", ik)
        cfg = gzip.decompress(img[ik + 8:fim])
        open(os.path.join(outdir, "kernel.config"), "wb").write(cfg)
        print(f"IKCONFIG embutido: {len(cfg)} bytes -> kernel.config")
    else:
        print("IKCONFIG: não embutido (sem /proc/config.gz)")
    n = 0
    fonte = resto if resto else k
    for mm in re.finditer(b"\xd0\x0d\xfe\xed", fonte):
        o = mm.start()
        total = struct.unpack(">I", fonte[o + 4:o + 8])[0]
        if total < 64 or o + total > len(fonte):
            continue
        blob = fonte[o:o + total]
        try:
            pr = fdt_props(blob)
        except Exception:
            continue
        if not pr:
            continue
        open(os.path.join(outdir, f"dtb{n:02d}.dtb"), "wb").write(blob)
        print(f"DTB {n:02d}: {total} bytes | model={pr.get('model')} | compatible={pr.get('compatible')}"
              f" | msm-id={pr.get('qcom,msm-id')} | board-id={pr.get('qcom,board-id')}")
        n += 1
    print(f"DTBs anexados: {n}")


if __name__ == "__main__":
    a = sys.argv[1:]
    if not a:
        raise SystemExit(__doc__)
    if a[0] == "censo":
        censo(a[1], a[2])
    elif a[0] == "deps":
        deps(a[1], a[2:])
    elif a[0] == "syms":
        syms(a[1], a[2] if len(a) > 2 else "undef")
    elif a[0] == "strs":
        strs(a[1], a[2])
    elif a[0] == "bootimg":
        bootimg(a[1], a[2])
    else:
        raise SystemExit(__doc__)
