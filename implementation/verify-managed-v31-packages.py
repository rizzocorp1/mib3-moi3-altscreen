import json,hashlib,zipfile,re,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
folder=Path(sys.argv[1]).resolve()
def digest(b):return hashlib.sha256(b).hexdigest()
def render(name,values):
 s=(root/'recovery'/name).read_text(encoding='utf-8').replace('@ROOT@','')
 for k,v in values.items():s=s.replace('@'+k+'@',v)
 assert not re.search(r'@[A-Z_]+@',s)
 return s.encode()
carrier=root/'firmware-audit/LG-wipe-FAZIT.zip'
assert digest(carrier.read_bytes())=='763a85698771ef2bb69fb29eb6a1e8b0a32b063aee1910194865f70ea6c1005d'
reports=[];pair=[]
with zipfile.ZipFile(carrier) as vendor:
 for label,mode in [('INSTALL_ALTSCREEN','install'),('RESTORE_STOCK','restore')]:
  archive=folder/(label+'.zip')
  with zipfile.ZipFile(archive) as z:
   names=z.namelist();assert len(names)==len(set(names))
   assert all(not n.startswith('/') and '..' not in Path(n).parts and '\\' not in n for n in names)
   m=json.loads(z.read('build-manifest.json'));values=m['factoryHashes'];pair.append(m)
   assert m['format']=='moi3-altscreen-managed-v31' and m['operation']==mode and values['MODE']==mode
   for name,sha in m['files'].items():assert digest(z.read(name))==sha,name
   for e in vendor.infolist():
    if e.filename=='fix_hw_partnumber.sh':continue
    assert z.read(e.filename)==vendor.read(e.filename),e.filename
   assert set(names)==(set(vendor.namelist())-{'fix_hw_partnumber.sh'})|set(m['files'])|{'build-manifest.json'}
   assert z.read('fix_hw_partnumber.sh')==render('managed-v31-entry.sh.in',values)
   assert digest(z.read('payload/file-guard'))==values['GUARD_SHA']
   assert digest(z.read('payload/factory-carplay'))==values['CARPLAY_SHA']
   assert digest(z.read('payload/factory-carplay.sh'))==values['STARTUP_SHA']
   markers=[n for n in names if n.startswith('ALTSCREEN_')]
   assert markers==['ALTSCREEN_MANAGED_'+('INSTALL' if mode=='install' else 'RESTORE')+'.txt']
   if mode=='install':
    for template,name,sha in [('managed-v31-controller.sh.in','managed-controller.sh','CONTROLLER_SHA'),('managed-v31-boot.sh.in','managed-boot.sh','BOOT_SHA')]:
     assert z.read('payload/'+name)==render(template,values)
     assert digest(z.read('payload/'+name))==values[sha]
    for name,sha in [('carplay-managed','PATCH_SHA'),('altscreen_handler.so','HANDLER_SHA')]:assert digest(z.read('payload/'+name))==values[sha]
   for name,sha in m['sourceHashes'].items():
    candidates=[root/'firmware-audit/v31-managed/source'/name,root/'recovery'/name,root/'implementation'/name]
    existing=[p for p in candidates if p.is_file()];assert existing,name
    assert digest(existing[0].read_bytes())==sha,name
   assert m['layoutPolicy']=={'safeX':464,'safeY':56,'safeWidth':352,'safeHeight':368,'showETA':True,'factoryGuidanceSuppression':False}
   assert m['runtimePolicy']=={'leaseSeconds':12,'watchdogSeconds':20,'fixedSessionCutoff':False,'logBytes':524288,'continuousPacketAudit':False,'videoCapture':False,'factoryExecutableOverwritten':False}
   assert m['packetBurstPacing']=={'bitsPerSecond':40000000,'maximumBatchDatagrams':4,'maximumAccessUnitSendMicroseconds':75000}
   assert m['transportBaseline']=='V29' and m['completeVehicleRollback']==False and m['hardwareValidated']==False
   for name in names:
    if name.endswith('/'):continue
    assert (folder/label/name).read_bytes()==z.read(name),'Extracted folder differs: '+name
   for name in ['payload/file-guard']+(['payload/carplay-managed','payload/altscreen_handler.so'] if mode=='install' else []):
    info=json.loads(subprocess.check_output(['node',str(root/'firmware-audit/inspect-elf.mjs'),str(folder/label/name)],text=True))
    assert info['architecture']=='AArch64'
    if name.endswith('file-guard'):assert not info['needed']
    if name.endswith('carplay-managed'):assert info['needed'][0]=='/lge/app_rw/smartlink/carplay/.moi3-altscreen-managed-v31/altscreen_handler.so'
    if name.endswith('altscreen_handler.so'):
     assert sorted(x['name'] for x in info['exports'])==sorted(['AES_CBCFrame_Init','AirPlayCopyServerInfo','AirPlayReceiverSessionSetHomeKitSecurityContext','AirPlayReceiverSessionSetSecurityInfo','AirPlayReceiverSessionSetup','AirPlayReceiverSessionTearDown','ScreenStreamCreate','ScreenStreamSetWidthHeight','ScreenStreamStop'])
   reports.append({'package':label,'sha256':digest(archive.read_bytes()),'allPayloadHashesVerified':True,'factoryCarrierUnchanged':True,'templatesMatch':True,'sourceHashesMatch':True,'extractedFolderVerified':True,'targetElfVerified':True})
assert {k:v for k,v in pair[0]['factoryHashes'].items() if k!='MODE'}=={k:v for k,v in pair[1]['factoryHashes'].items() if k!='MODE'}
assert pair[0]['sourceHashes']==pair[1]['sourceHashes']
result={'packages':reports,'pairedBaselineIdentical':True,'hardwareValidated':False}
(folder/'verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result,indent=2))
