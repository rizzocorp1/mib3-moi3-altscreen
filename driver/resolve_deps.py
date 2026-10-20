import subprocess, os, sys, collections
DBG="/opt/homebrew/opt/e2fsprogs/sbin/debugfs"
IMG=sys.argv[1]; SYSROOT=sys.argv[2]
START="/lge/app_rw/smartlink/dcmanager/sdcm"
LIBDIRS=["/lib64","/usr/lib64","/usr/lib","/lib","/lge/app_ro/lib64","/lge/app_rw/smartlink/dcmanager",
         "/lge/hmi/libs","/usr/lib/aarch64-linux-gnu","/lge/app_ro/lib","/lge/secure/lib"]
def dump(path, out):
    os.makedirs(os.path.dirname(out), exist_ok=True)
    r=subprocess.run([DBG,"-R",f"dump {path} {out}",IMG],capture_output=True,text=True)
    return os.path.exists(out) and os.path.getsize(out)>0
def needed(f):
    r=subprocess.run(["/usr/bin/objdump","-p",f],capture_output=True,text=True)
    return [ln.split()[1] for ln in r.stdout.splitlines() if "NEEDED" in ln]
def find_lib(soname):
    # loader tem nome fixo
    cands=[soname]
    for d in LIBDIRS:
        for c in cands:
            out=os.path.join(SYSROOT, (d+"/"+c).lstrip("/"))
            if os.path.exists(out): return (d+"/"+c)
            if dump(d+"/"+c, out): return (d+"/"+c)
    return None
resolved={}; missing=set(); q=collections.deque()
# start
dump(START, os.path.join(SYSROOT, START.lstrip("/")))
q.append(os.path.join(SYSROOT, START.lstrip("/")))
seen=set([START])
while q:
    f=q.popleft()
    for so in needed(f):
        if so in resolved or so in missing: continue
        p=find_lib(so)
        if p:
            resolved[so]=p
            q.append(os.path.join(SYSROOT,p.lstrip("/")))
        else:
            missing.add(so)
print(f"RESOLVIDAS: {len(resolved)}")
for so,p in sorted(resolved.items()): print(f"  {so}  <-  {p}")
print(f"\nFALTANDO: {len(missing)}")
for so in sorted(missing): print(f"  {so}")
