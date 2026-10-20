import sys,struct
f=open(sys.argv[1],'rb'); d=f.read()
assert d[:4]==b'\x7fELF'
is64=d[4]==2; le='<' if d[5]==1 else '>'
# ELF64 header
e_shoff=struct.unpack_from(le+'Q',d,0x28)[0]
e_shentsize=struct.unpack_from(le+'H',d,0x3a)[0]
e_shnum=struct.unpack_from(le+'H',d,0x3c)[0]
e_shstrndx=struct.unpack_from(le+'H',d,0x3e)[0]
secs=[]
for i in range(e_shnum):
    off=e_shoff+i*e_shentsize
    name,typ,flags,addr,offset,size,link,info,align,entsize=struct.unpack_from(le+'IIQQQQIIQQ',d,off)
    secs.append((name,typ,offset,size,link,entsize))
shstr_off=secs[e_shstrndx][2]
def sname(n): 
    e=d.index(b'\0',shstr_off+n); return d[shstr_off+n:e].decode('latin1')
dynsym=dynstr=None
for s in secs:
    nm=sname(s[0])
    if nm=='.dynsym': dynsym=s
    if nm=='.dynstr': dynstr=s
if not dynsym: print("sem .dynsym"); sys.exit()
_,_,off,size,link,entsize=dynsym
stroff=secs[link][2]
def strn(n):
    e=d.index(b'\0',stroff+n); return d[stroff+n:e].decode('latin1')
pats=[p.lower() for p in sys.argv[2:]] or ['jni_onload','java_','initspi','spinative']
cnt=0
for i in range(size//entsize):
    o=off+i*entsize
    st_name,st_info,st_other,st_shndx,st_value,st_size=struct.unpack_from(le+'IBBHQQ',d,o)
    nm=strn(st_name)
    if nm and any(p in nm.lower() for p in pats):
        typ=st_info&0xf; defined = st_shndx!=0
        print(("DEF " if defined else "UND ")+nm)
        cnt+=1
print(f"[{cnt} símbolos casaram]")
