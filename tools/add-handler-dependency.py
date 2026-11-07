"""Add one explicit dependency to a COPY of an ELF64 Linux executable.

No instructions are patched. One non-property GNU PT_NOTE header is reused for
a new read-only load segment containing the extended dynamic string table.
The source file is never modified. This is not a firmware installer.
"""
import hashlib
from pathlib import Path
import struct
import sys

def patch(source, output, dependency):
    source=Path(source); output=Path(output)
    if output.exists(): raise ValueError('Output already exists')
    original=source.read_bytes(); data=bytearray(original)
    def read(fmt,offset): return struct.unpack_from('<'+fmt,data,offset)
    def write(fmt,offset,*values):struct.pack_into('<'+fmt,data,offset,*values)
    if data[:6]!=b'\x7fELF\x02\x01' or read('H',18)[0] not in (62,183):raise ValueError('Expected x86_64/AArch64 ELF64 LE')
    if read('H',16)[0] not in (2,3):raise ValueError('Not an executable/PIE')
    encoded=dependency.encode('utf8')
    if not dependency.startswith('/') or any(c in encoded for c in (0,10,13)):raise ValueError('Expected absolute dependency path')
    phoff=read('Q',32)[0];stride,count=read('HH',54)
    if stride!=56 or phoff+count*stride>len(data):raise ValueError('Bad program headers')
    headers=[list(read('IIQQQQQQ',phoff+i*stride)) for i in range(count)]
    loads=[h for h in headers if h[0]==1]
    if not loads:raise ValueError('No load segments')
    def file_offset(address,size):
        for h in loads:
            if h[3]<=address and address+size<=h[3]+h[5]:return h[2]+address-h[3]
        raise ValueError('Address not file-backed')
    dynamics=[h for h in headers if h[0]==2]
    if len(dynamics)!=1:raise ValueError('Expected one dynamic segment')
    dynamic=dynamics[0];start=dynamic[2];length=dynamic[5]
    if length%16 or start+length>len(data):raise ValueError('Invalid dynamic segment')
    entries=[read('qQ',i) for i in range(start,start+length,16)]
    null=next(i for i,e in enumerate(entries) if e[0]==0)
    if null+1>=len(entries) or any(e!=(0,0) for e in entries[null:]):raise ValueError('No clean spare dynamic slot')
    tags=dict(entries[:null]);string_address=tags[5];string_size=tags[10]
    if string_size>1024*1024:raise ValueError('Unexpected string table size')
    string_offset=file_offset(string_address,string_size)
    strings=bytes(data[string_offset:string_offset+string_size])
    for tag,value in entries[:null]:
        if tag==1 and strings[value:].split(b'\0',1)[0]==encoded:raise ValueError('Dependency already present')
    note_index=None
    for index,h in enumerate(headers):
        if h[0]!=4:continue
        position=h[2];end=position+h[5];types=[]
        if end>len(data):raise ValueError('Bad note range')
        while position<end:
            namesz,descsz,kind=read('III',position);position+=12
            name=bytes(data[position:position+namesz]).rstrip(b'\0')
            position+=(namesz+3)&~3;position+=(descsz+3)&~3
            if position>end:raise ValueError('Bad note')
            types.append((name,kind))
        if types and all(name==b'GNU' and kind in (1,3) for name,kind in types):note_index=index;break
    if note_index is None:raise ValueError('No reusable build-ID/ABI note; GNU property notes are preserved')
    alignment=max(4096,max(h[7] for h in loads))
    if alignment&(alignment-1):raise ValueError('Invalid alignment')
    align=lambda n:(n+alignment-1)&~(alignment-1)
    new_offset=align(len(data));new_address=align(max(h[3]+h[6] for h in loads))
    new_strings=strings+encoded+b'\0'
    data.extend(b'\0'*(new_offset-len(data)));data.extend(new_strings)
    write('IIQQQQQQ',phoff+note_index*stride,1,4,new_offset,new_address,new_address,len(new_strings),len(new_strings),alignment)
    active=[(1,string_size)]+[(tag,new_address if tag==5 else len(new_strings) if tag==10 else value) for tag,value in entries[:null]]
    for index in range(len(entries)):
        tag,value=active[index] if index<len(active) else (0,0)
        write('qQ',start+index*16,tag,value)
    shoff=read('Q',40)[0];shstride,shcount=read('HH',58)
    matched=0
    if shstride!=64 or shoff+shstride*shcount>len(original):raise ValueError('Bad section headers')
    for index in range(shcount):
        offset=shoff+index*shstride
        if read('I',offset+4)[0]==3 and read('Q',offset+16)[0]==string_address and read('Q',offset+32)[0]==string_size:
            write('QQQ',offset+16,new_address,new_offset,len(new_strings));matched+=1
    if matched!=1:raise ValueError('Expected one matching .dynstr section')
    with output.open('xb') as file:file.write(data)
    if source.read_bytes()!=original:raise ValueError('Source changed during patch')
    return {'sourceSha256':hashlib.sha256(original).hexdigest(),'outputSha256':hashlib.sha256(data).hexdigest(),'dependency':dependency,'newSegmentFlags':'R','instructionPatches':0}

if __name__=='__main__':
    import json
    if len(sys.argv)!=4:raise SystemExit('usage: add-handler-dependency.py SOURCE COPY ABSOLUTE_HANDLER_PATH')
    print(json.dumps(patch(*sys.argv[1:]),indent=2))
