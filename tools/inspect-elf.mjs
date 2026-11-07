// Read-only ELF64 little-endian inventory. Does not execute firmware.
import fs from 'node:fs';
import crypto from 'node:crypto';
const file=process.argv[2];
if(!file) throw new Error('Usage: node inspect-elf.mjs FILE');
const b=fs.readFileSync(file);
function bounds(offset,size){if(!Number.isSafeInteger(offset)||!Number.isSafeInteger(size)||offset<0||size<0||offset+size>b.length)throw new Error('ELF range outside file');}
function u16(o){bounds(o,2);return b.readUInt16LE(o);}
function u32(o){bounds(o,4);return b.readUInt32LE(o);}
function u64(o){bounds(o,8);const n=Number(b.readBigUInt64LE(o));if(!Number.isSafeInteger(n))throw new Error('Oversized ELF offset');return n;}
function string(section,index){if(index>=section.size)throw new Error('String index outside section');const start=section.offset+index;bounds(start,section.size-index);const end=b.indexOf(0,start);if(end<start||end>=section.offset+section.size)throw new Error('Unterminated ELF string');return b.toString('utf8',start,end);}
if(b.length<64||b.subarray(0,4).toString('hex')!=='7f454c46'||b[4]!==2||b[5]!==1)throw new Error('Expected ELF64 little-endian file');
const shoff=u64(40),stride=u16(58),count=u16(60),namesIndex=u16(62);
if(stride<64||!count||namesIndex>=count)throw new Error('Unsupported ELF section table');
bounds(shoff,stride*count);
const sections=Array.from({length:count},(_,i)=>{const p=shoff+i*stride;return {nameIndex:u32(p),type:u32(p+4),offset:u64(p+24),size:u64(p+32),link:u32(p+40),entrySize:u64(p+56)};});
for(const s of sections){if(s.type!==8)bounds(s.offset,s.size);s.name=string(sections[namesIndex],s.nameIndex);}
const needed=[],sonames=[],imports=[],exports=[],versions=[];
for(const s of sections){
 if(s.type===6){if(s.entrySize!==16||s.size%16||s.link>=count)throw new Error('Invalid dynamic table');for(let p=s.offset;p<s.offset+s.size;p+=16){const tag=u64(p),value=u64(p+8);if(tag===1)needed.push(string(sections[s.link],value));if(tag===14)sonames.push(string(sections[s.link],value));}}
 if(s.type===11){if(s.entrySize!==24||s.size%24||s.link>=count)throw new Error('Invalid symbol table');for(let p=s.offset;p<s.offset+s.size;p+=24){const name=string(sections[s.link],u32(p));if(!name)continue;const info=b[p+4],ndx=u16(p+6);const symbol={name,type:info&15,binding:info>>4,size:u64(p+16)};if(ndx===0)imports.push(symbol);else if(symbol.binding===1||symbol.binding===2)exports.push(symbol);}}
 if(s.name==='.gnu.version_r'){
  if(s.link>=count)throw new Error('Invalid version string table');let p=s.offset,records=0;
  while(p<s.offset+s.size){if(++records>65536)throw new Error('Version record loop');bounds(p,16);const n=u16(p+2),library=string(sections[s.link],u32(p+4));let aux=p+u32(p+8);for(let i=0;i<n;i++){bounds(aux,16);versions.push({library,version:string(sections[s.link],u32(aux+8))});const next=u32(aux+12);if(i+1<n&&!next)throw new Error('Truncated version list');aux+=next;}const next=u32(p+12);if(!next)break;p+=next;}
 }
}
console.log(JSON.stringify({file,sha256:crypto.createHash('sha256').update(b).digest('hex'),machine:u16(18),architecture:u16(18)===183?'AArch64':u16(18)===62?'x86_64':'other',needed,sonames,versions,imports,exports},null,2));
