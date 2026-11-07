// Static symbol inventory only. Presence does not verify factory function ABI.
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const root=process.argv[2];
if(!root)throw new Error('Usage: node check-sysroot.mjs EXTRACTED_ROOT');
const required={
 'libAirPlay': ['AirPlayCopyServerInfo','AirPlayReceiverSessionSetup','AirPlayReceiverSessionTearDown','AirPlay_DeriveAESKeySHA512ForScreen'],
 'libCoreUtils': ['AES_CBCFrame_Init','AES_CTR_Init','AES_CTR_Update'],
 'libAirPlaySupport': ['ScreenStreamCreate','ScreenStreamStop','ScreenStreamSetWidthHeight'],
};
const files=[];
function visit(dir){for(const item of fs.readdirSync(dir,{withFileTypes:true})){const full=path.join(dir,item.name);if(item.isDirectory())visit(full);else if(item.isFile()&&Object.keys(required).some(prefix=>item.name.startsWith(prefix)&&item.name.includes('.so')))files.push(full);}}
visit(fs.realpathSync(root));
const inspector=fileURLToPath(new URL('./inspect-elf.mjs',import.meta.url));
const libraries=files.map(file=>JSON.parse(execFileSync(process.execPath,[inspector,file],{encoding:'utf8',maxBuffer:16*1024*1024})));
const exportIndex=new Map();
for(const lib of libraries){for(const symbol of lib.exports){const list=exportIndex.get(symbol.name)||[];list.push(lib.file);exportIndex.set(symbol.name,list);}}
const symbols=Object.entries(required).flatMap(([expectedLibrary,names])=>names.map(name=>({name,expectedLibrary,providers:exportIndex.get(name)||[],found:exportIndex.has(name)})));
const report={root:fs.realpathSync(root),libraries:libraries.map(({file,sha256,architecture,needed,versions})=>({file,sha256,architecture,needed,versions})),symbols,missing:symbols.filter(s=>!s.found).map(s=>s.name),abiSignaturesVerified:false,cryptoContextLayoutVerified:false,vehicleCompatibilityVerified:false};
console.log(JSON.stringify(report,null,2));
if(report.missing.length||libraries.some(l=>l.architecture!=='AArch64'))process.exitCode=1;
