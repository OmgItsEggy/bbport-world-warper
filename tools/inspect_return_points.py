"""Read-only local DCX/BND4/PARAM audit. No game files are modified or exported.

Format references: soulsmods/SoulsFormats BND4, BinderFileHeader and PARAM readers.
This intentionally accepts only the DFLT/BND4 variants actually validated here.
"""
import argparse
from pathlib import Path
import struct
import zlib
import sys
import json

def dcx(data):
    if data[:4]!=b'DCX\0':
        return data
    if data[0x28:0x2c]!=b'DFLT' or data[0x44:0x48]!=b'DCA\0':
        raise ValueError('Unsupported DCX format')
    result=zlib.decompress(data[0x4c:])
    if len(result)!=struct.unpack_from('>I',data,0x1c)[0]:
        raise ValueError('DCX length mismatch')
    return result

def reverse_bits(value):
    return int(f'{value:08b}'[::-1],2)

def cstring(data,offset,unicode=False):
    if not 0<=offset<len(data):
        raise ValueError('Invalid string offset')
    if unicode:
        end=offset
        while end+1<len(data) and data[end:end+2]!=b'\0\0': end+=2
        return data[offset:end].decode('utf-16le')
    return data[offset:data.index(b'\0',offset)].decode('shift_jis',errors='replace')

def binder(data):
    data=dcx(data)
    if len(data)<64 or data[:4]!=b'BND4' or data[9]!=0:
        raise ValueError('Only little-endian BND4 supported')
    count=struct.unpack_from('<I',data,12)[0]
    stride=struct.unpack_from('<Q',data,32)[0]
    if count>10000 or stride<20 or 64+count*stride>len(data):
        raise ValueError('Invalid BND4 table')
    raw=data[0x31]
    bit_big=not data[10]
    fmt=raw if bit_big or (raw&1 and not raw&128) else reverse_bits(raw)
    for i in range(count):
        off=64+i*stride
        flags=data[off] if bit_big else reverse_bits(data[off])
        size=struct.unpack_from('<Q',data,off+8)[0]
        cursor=off+16
        if fmt&32: cursor+=8
        width=8 if fmt&16 else 4
        start=struct.unpack_from('<Q' if width==8 else '<I',data,cursor)[0]
        cursor+=width
        if fmt&2: cursor+=4
        if not fmt&12: raise ValueError('Unnamed binder entry')
        name=cstring(data,struct.unpack_from('<I',data,cursor)[0],bool(data[0x30]))
        if start+size>len(data): raise ValueError('Invalid BND4 data range')
        payload=data[start:start+size]
        if flags&1: payload=dcx(payload)
        yield name,payload

def param_rows(data):
    if len(data)<0x30 or data[0x2c]!=0:
        raise ValueError('Unsupported PARAM endian/header')
    flags=data[0x2d]
    count=struct.unpack_from('<H',data,10)[0]
    # PARAM flags: Flag01=1, IntDataOffset=2, LongDataOffset=4.
    long_offsets=bool(flags&4)
    start=0x40 if long_offsets or flags&3==3 else 0x30
    stride=24 if long_offsets else 12
    if start+count*stride>len(data): raise ValueError('Invalid PARAM rows')
    rows=[]
    for i in range(count):
        pos=start+i*stride
        row_id=struct.unpack_from('<i',data,pos)[0]
        offset=struct.unpack_from('<Q' if long_offsets else '<I',data,pos+(8 if long_offsets else 4))[0]
        name_offset=struct.unpack_from('<Q' if long_offsets else '<I',data,pos+(16 if long_offsets else 8))[0]
        if not 0<=offset<len(data): raise ValueError('Invalid PARAM data offset')
        name=cstring(data,name_offset,bool(data[0x2e]&1)) if name_offset else ''
        rows.append((row_id,offset,name))
    for i,(row_id,offset,name) in enumerate(rows):
        end=rows[i+1][1] if i+1<len(rows) else len(data)
        yield row_id,name,data[offset:end]

def audit_manifest(rows,manifest):
    verified=[]
    names=set()
    if manifest.get('version')!='bosses-v2' or len(manifest['destinations'])!=22:
        raise ValueError('Wrong boss route schema/count')
    for stop in manifest['destinations']:
        if stop['boss'] in names: raise ValueError('Duplicate boss route')
        names.add(stop['boss'])
        if not stop['lamp']:
            if stop['boss'] not in ('Gehrman','MoonPresence') or not stop.get('blocked'):
                raise ValueError('Unexplained missing route')
            continue
        label,row=rows[stop['lamp']]
        if '撃破' not in label: raise ValueError(f'Not a boss-defeat destination: {stop}')
        aab=stop['lamp']//10000
        if len(row)<4 or (row[0],row[2])!=(aab//10,aab%10): raise ValueError(f'Wrong map: {stop}')
        if aab//10==29: raise ValueError('Chalice destination forbidden')
        verified.append((stop['boss'],stop['lamp'],label))
    if len(verified)!=20: raise ValueError('Expected exactly 20 verified boss-room endpoints')
    return verified

def audit_game_routes(gameparam,manifest_path):
    manifest=json.loads(Path(manifest_path).read_text(encoding='utf-8'))
    for name,data in binder(Path(gameparam).read_bytes()):
        if name.replace('\\','/').split('/')[-1]=='ReturnPointParam.param':
            rows={row_id:(row_name,row) for row_id,row_name,row in param_rows(data)}
            return audit_manifest(rows,manifest)
    raise ValueError('ReturnPointParam missing from game binder')

def main():
    sys.stdout.reconfigure(encoding='utf-8')
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binder',type=Path)
    parser.add_argument('--manifest',type=Path)
    args=parser.parse_args()
    if args.manifest:
        for boss,lamp,label in audit_game_routes(args.binder,args.manifest): print('VERIFIED',boss,lamp,label)
        print('PASS: 20 boss-defeat destinations; ending entries explicitly blocked, not ordinary Dream travel.')
        return
    for name,data in binder(args.binder.read_bytes()):
        if 'ReturnPoint' in name:
            print('PARAM',name,'header',data[:0x40].hex())
            for row_id,row_name,row in param_rows(data):
                if row_id%10000>=2950 and row_id%10000<2970:
                    print(row_id,row_name,'bytes',row[:96].hex())

if __name__=='__main__': main()
