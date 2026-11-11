from pathlib import Path
import hashlib,os,shutil

def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def render(text,values):
 for k,v in values.items():text=text.replace('@'+k+'@',str(v))
 return text

def fixture(r,guard,templates):
 base=r/'lge/app_rw/smartlink/carplay';base.mkdir(parents=True);stage=base/'.moi3-altscreen-managed-v31';stage.mkdir();ram=r/'run/moi3-altscreen-map-v31';ram.mkdir(parents=True);data=r/'data/lge/smartlink/carplay';data.mkdir(parents=True)
 (r/'etc').mkdir();(r/'etc/os-release').write_text('fixture-os');(base/'carplay').write_text('factory executable');(base/'carplay.sh').write_text('factory startup');(base/'carplay.sh').chmod(0o755);os.setxattr(base/'carplay.sh','user.factory',b'original')
 shutil.copy2(base/'carplay.sh',stage/'factory-carplay.sh');shutil.copy2(base/'carplay',stage/'factory-carplay');shutil.copyfile(guard,stage/'file-guard');(stage/'file-guard').chmod(0o755)
 (stage/'carplay-managed').write_text('managed executable');(stage/'altscreen_handler.so').write_text('managed library');(stage/'ENABLED').write_text('MOI3_MANAGED_V31\n');(stage/'boot-status.txt').write_text('fixture\n');(r/'native-active').write_text('yes')
 values={'ROOT':str(r),'STARTUP_SHA':digest(base/'carplay.sh'),'CARPLAY_SHA':digest(base/'carplay'),'PATCH_SHA':digest(stage/'carplay-managed'),'HANDLER_SHA':digest(stage/'altscreen_handler.so'),'GUARD_SHA':digest(guard),'OS_SHA':digest(r/'etc/os-release')}
 ctl=render((templates/'managed-v31-controller.sh.in').read_text(),values);(stage/'managed-controller.sh').write_text(ctl);values['CONTROLLER_SHA']=digest(stage/'managed-controller.sh')
 boot=render((templates/'managed-v31-boot.sh.in').read_text(),values);(stage/'managed-boot.sh').write_text(boot);values['BOOT_SHA']=digest(stage/'managed-boot.sh')
 return base,stage,ram,data,values
