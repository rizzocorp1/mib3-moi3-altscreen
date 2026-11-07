"""Static ELF64 LE function disassembly. Never loads or executes the input ELF."""
import sys, struct, re
from pathlib import Path
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

data = Path(sys.argv[1]).read_bytes()
if data[:6] != b'\x7fELF\x02\x01':
    raise ValueError('Expected ELF64 little endian')
def unpack(fmt, offset):
    return struct.unpack_from('<' + fmt, data, offset)
section_offset = unpack('Q', 40)[0]
stride, count, names_index = unpack('HHH', 58)
sections = [unpack('IIQQQQIIQQ', section_offset + i * stride) for i in range(count)]
def string(section, index):
    if index >= section[5]: raise ValueError('Invalid string index')
    start = section[4] + index
    end = data.find(b'\0', start, section[4] + section[5])
    if end < 0: raise ValueError('Unterminated string')
    return data[start:end].decode('utf8', errors='replace')
named_sections = {string(sections[names_index], s[0]): s for s in sections}
symbols = {}
tables = {}
for index, section in enumerate(sections):
    if section[1] not in (2, 11): continue
    entries = []
    for offset in range(section[4], section[4] + section[5], section[9]):
        name, info, other, ndx, value, size = unpack('IBBHQQ', offset)
        symbol = (string(sections[section[6]], name), value, size, ndx, info)
        entries.append(symbol)
        if symbol[0] and ndx: symbols[symbol[0]] = symbol
    tables[index] = entries
addresses = {s[1]: s[0] for s in symbols.values() if s[4] & 15 == 2}
# AArch64 ELF uses a 32-byte PLT header and 16-byte entries.
if '.plt' in named_sections and '.rela.plt' in named_sections:
    plt = named_sections['.plt']; rel = named_sections['.rela.plt']
    for i, offset in enumerate(range(rel[4], rel[4] + rel[5], rel[9])):
        target, info, addend = unpack('QQq', offset)
        addresses[plt[3] + 32 + i * 16] = tables[rel[6]][info >> 32][0] + '@plt'
decoder = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
decoder.skipdata = True
if len(sys.argv) > 4 and sys.argv[2] == '--range':
    address, size = (int(x,0) for x in sys.argv[3:5])
    section = next(s for s in sections if s[1] != 8 and s[3] <= address < s[3]+s[5])
    if address+size > section[3]+section[5]: raise ValueError('Range exceeds section')
    offset = section[4]+address-section[3]
    for instruction in decoder.disasm(data[offset:offset+size],address):
        annotation = ''
        if instruction.mnemonic in ('b','bl') and instruction.op_str.startswith('#0x'):
            annotation = ' ; ' + addresses.get(int(instruction.op_str[1:],16),'')
        print(f'{instruction.address:08x}: {instruction.mnemonic:9} {instruction.op_str}{annotation}')
    sys.exit(0)
if len(sys.argv) > 3 and sys.argv[2] == '--relocation':
    wanted = int(sys.argv[3],0)
    for section in sections:
        if section[1] != 4 or section[9] != 24: continue
        for offset in range(section[4],section[4]+section[5],24):
            target, info, addend = unpack('QQq',offset)
            if target == wanted:
                index = info >> 32
                name = tables.get(section[6],[])[index][0] if index else ''
                aliases = [s[0] for s in symbols.values() if s[1] == addend]
                print(f'{target:#x}: type={info & 0xffffffff}, symbol={name}, addend={addend:#x}, aliases={aliases}')
    sys.exit(0)
if len(sys.argv) > 3 and sys.argv[2] == '--immediate':
    operand = '#' + hex(int(sys.argv[3],0))
    text = named_sections['.text']
    for instruction in decoder.disasm(data[text[4]:text[4]+text[5]], text[3]):
        if instruction.mnemonic == 'mov' and instruction.op_str.split(', ')[-1] == operand:
            owners = [s[0] for s in symbols.values() if s[1] <= instruction.address < s[1]+s[2] and s[4] & 15 == 2]
            print(f'{instruction.address:#x}: {instruction.mnemonic} {instruction.op_str} in {owners}')
    sys.exit(0)
if len(sys.argv) > 6 and sys.argv[2] == '--switch16':
    table, count, origin, bias = (int(x,0) for x in sys.argv[3:7])
    section = next(s for s in sections if s[1] != 8 and s[3] <= table < s[3]+s[5])
    offset = section[4] + table - section[3]
    for i in range(count):
        destination = origin + unpack('h',offset+i*2)[0]*4
        print(f'case {i+bias} ({i+bias:#x}): {destination:#x}')
    sys.exit(0)
if len(sys.argv) > 3 and sys.argv[2] == '--cstring':
    for argument in sys.argv[3:]:
        address = int(argument, 0)
        section = next(s for s in sections if s[1] != 8 and s[3] <= address < s[3]+s[5])
        offset = section[4] + address - section[3]
        end = data.find(b'\0', offset, section[4]+section[5])
        if end < 0: raise ValueError('Unterminated string')
        print(f'{address:#x}: {data[offset:end].decode("utf8", errors="replace")}')
    sys.exit(0)
if len(sys.argv) > 3 and sys.argv[2] == '--string':
    needle = sys.argv[3].encode()
    offset = data.find(needle)
    if offset < 0: raise ValueError('String not found')
    section = next(s for s in sections if s[1] != 8 and s[4] <= offset < s[4]+s[5])
    target = section[3] + offset - section[4]
    print(f'String VMA: 0x{target:x}')
    text = named_sections['.text']; pages = {}
    for instruction in decoder.disasm(data[text[4]:text[4]+text[5]], text[3]):
        if instruction.mnemonic == 'adrp':
            register, address = instruction.op_str.split(', ')
            pages[register] = (int(address[1:], 16), instruction.address)
        match = re.fullmatch(r'(x\d+), (x\d+), #0x([0-9a-f]+)', instruction.op_str)
        if instruction.mnemonic == 'add' and match and match[2] in pages:
            page, origin = pages[match[2]]
            if instruction.address-origin <= 64 and page+int(match[3],16) == target:
                owners = [s[0] for s in symbols.values() if s[1] <= instruction.address < s[1]+s[2] and s[4] & 15 == 2]
                print(f'Address pair at 0x{origin:x}/0x{instruction.address:x} in {owners}')
    sys.exit(0)
if len(sys.argv) > 3 and sys.argv[2] == '--calls':
    target = sys.argv[3]
    destinations = {address for address, name in addresses.items() if name in (target, target + '@plt')}
    if target in symbols: destinations.add(symbols[target][1])
    text = named_sections['.text']
    matches = []
    for instruction in decoder.disasm(data[text[4]:text[4]+text[5]], text[3]):
        if instruction.mnemonic == 'bl' and instruction.op_str.startswith('#0x') and int(instruction.op_str[1:], 16) in destinations:
            matches.append(instruction.address)
    for address in matches:
        owners = [s[0] for s in symbols.values() if s[1] <= address < s[1]+s[2] and s[4] & 15 == 2]
        print(f'\nCall at 0x{address:x} in {owners}')
        start = max(text[3], address - 64)
        offset = text[4] + start - text[3]
        for instruction in decoder.disasm(data[offset:offset+112], start):
            print(f'{instruction.address:08x}: {instruction.mnemonic:9} {instruction.op_str}')
    sys.exit(0)
for requested in sys.argv[2:]:
    if requested not in symbols: raise ValueError('Symbol not found: ' + requested)
    name, address, size, ndx, info = symbols[requested]
    section = sections[ndx]
    offset = section[4] + address - section[3]
    print(f'\n{name} @ 0x{address:x}, {size} bytes')
    for instruction in decoder.disasm(data[offset:offset + size], address):
        annotation = ''
        if instruction.mnemonic in ('bl', 'b') and instruction.op_str.startswith('#0x'):
            destination = int(instruction.op_str[1:], 16)
            annotation = ' ; ' + addresses.get(destination, '')
        print(f'{instruction.address:08x}: {instruction.mnemonic:9} {instruction.op_str}{annotation}')
