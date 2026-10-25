"""Read-only Bloodborne MSB region inspection; never rewrites game assets.

Bloodborne's MSB v3 headers/region layout follow Soulstruct bloodborne/maps/
regions.py and SoulsFormats MSBB/Region.cs. Only region metadata is decoded.
No Soulstruct dependency and no execution of decompiled event scripts.
"""
import argparse
import json
import math
from pathlib import Path
import struct
from tools.inspect_return_points import dcx


def regions(raw):
    data = dcx(raw)
    def unpack(fmt, at):
        size = struct.calcsize(fmt)
        if at < 0 or at + size > len(data):
            raise ValueError('MSB offset outside file')
        return struct.unpack_from(fmt, data, at)
    def name(at):
        if at < 0 or at >= len(data) or at % 2:
            raise ValueError('Invalid MSB string offset')
        end = at
        while end + 2 <= len(data) and end - at < 4096:
            if data[end:end+2] == b'\0\0':
                return data[at:end].decode('utf-16-le')
            end += 2
        raise ValueError('Unterminated MSB name')
    if data[:16] != struct.pack('<4sII4B', b'MSB ', 1, 16, 0, 0, 1, 255):
        raise ValueError('Unsupported MSB header')
    at, seen = 16, set()
    while at:
        if at in seen or len(seen) >= 8:
            raise ValueError('Cyclic or excessive MSB lists')
        seen.add(at)
        version, count, title = unpack('<iiq', at)
        if version != 3 or not 1 <= count <= 100000:
            raise ValueError('Unsupported MSB list')
        offsets = unpack(f'<{count}q', at + 16)
        if name(title) == 'POINT_PARAM_ST':
            for entry in offsets[:-1]:
                name_off, subtype, index, shape = unpack('<qiii', entry)
                pos = unpack('<3f', entry+0x14)
                rot = unpack('<3f', entry+0x20)
                shape_off, entity_off = unpack('<2q', entry+0x40)
                entity, = unpack('<i', entry+entity_off)
                if subtype != 0 or shape not in range(6) or not all(map(math.isfinite, pos+rot)):
                    raise ValueError('Invalid Bloodborne region')
                dims = unpack('<'+'f'*(0,1,1,2,2,3)[shape], entry+shape_off)
                if not all(math.isfinite(x) for x in dims):
                    raise ValueError('Invalid region dimensions')
                yield dict(entity=entity, name=name(entry+name_off), position=list(pos),
                           rotation=list(rot), shape=shape, dimensions=list(dims))
            return
        at = offsets[-1]
    raise ValueError('No region list')


def objects(raw):
    """Actual MSB Object parts (not dummy objects) for authored fog anchors."""
    data=dcx(raw)
    def get(fmt,offset):
        if offset<0 or offset+struct.calcsize(fmt)>len(data):
            raise ValueError('MSB object offset outside file')
        return struct.unpack_from(fmt,data,offset)
    if data[:16]!=struct.pack('<4sII4B',b'MSB ',1,16,0,0,1,255):
        raise ValueError('Unsupported MSB header')
    at,seen=16,set()
    while at:
        if at in seen or len(seen)>=8: raise ValueError('Cyclic MSB lists')
        seen.add(at)
        version,count,name=get('<iiq',at)
        if version!=3 or not 1<=count<=100000: raise ValueError('Invalid MSB list')
        offsets=get(f'<{count}q',at+16)
        if data[name:name+28]=='PARTS_PARAM_ST'.encode('utf-16-le'):
            for entry in offsets[:-1]:
                if get('<i',entry+0x14)[0]!=1: continue
                super_offset,=get('<q',entry+0xb0)
                entity,=get('<i',entry+super_offset)
                pos=get('<3f',entry+0x28);rot=get('<3f',entry+0x34)
                if not all(map(math.isfinite,pos+rot)): raise ValueError('Invalid object transform')
                yield dict(entity=entity,position=list(pos),rotation=list(rot))
            return
        at=offsets[-1]
    raise ValueError('No part list')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('path', type=Path)
    p.add_argument('ids', type=int, nargs='*')
    args = p.parse_args()
    print(json.dumps([r for r in regions(args.path.read_bytes())
                      if not args.ids or r['entity'] in args.ids], indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
