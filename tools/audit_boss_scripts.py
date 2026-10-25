"""Read-only local MSB/EMEVD cross-check for the compiled boss script profiles.

This verifies references and coordinates, NOT that a walking route is unobstructed
or that an AI phase/particular attack was rendered. No game assets are exported.
"""
import argparse
from pathlib import Path
import re
import struct
from tools.inspect_map_regions import regions,objects
from tools.inspect_return_points import dcx

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT/'ports/supermedo/gpu/shim/bbport_world_warper_boss_scripts.h'
GATES = ROOT/'ports/supermedo/gpu/shim/bbport_world_warper_entry.h'


def gate_profiles():
    pattern=r'\{(\d+),(\d+),(\d+),(\d+),\{([^}]+)\},(-?[\d.]+)f\}'
    result=[]
    for m in re.finditer(pattern,GATES.read_text(encoding='utf-8')):
        result.append(dict(zip(('boss','first','object','fog_event'),map(int,m.groups()[:4])),
                           position=[float(v.strip().removesuffix('f')) for v in m[5].split(',')],yaw=float(m[6])))
    if len(result)!=6: raise ValueError('Expected six authored gate profiles')
    return result


def profiles(path=HEADER):
    result=[]
    pattern=r'\{(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),\{([^}]*)\}([^}]*)\}'
    for match in re.finditer(pattern,Path(path).read_text(encoding='utf-8')):
        numbers=list(map(int,match.groups()[:7]))
        pos=[float(v.strip().removesuffix('f')) for v in match[8].split(',') if v.strip()]
        extra=[int(v) for v in match[9].split(',') if v.strip()]
        if pos and len(pos)!=3:
            raise ValueError('Invalid compiled entry position')
        result.append(dict(zip(('boss','region','battle','dead','owner','swap','second'),numbers),
                           position=pos, phases=extra[:2]))
    if len(result)!=19 or len({p['boss'] for p in result})!=19:
        raise ValueError('Expected 19 unique compiled encounter profiles')
    return result


def events(raw):
    data=dcx(raw)
    def get(fmt,at):
        if at<0 or at+struct.calcsize(fmt)>len(data):
            raise ValueError('EMEVD offset outside file')
        return struct.unpack_from(fmt,data,at)
    if data[:12]!=b'EVD\0\0\xff\0\0\xcc\0\0\0' or len(data)<144 or get('<I',12)[0]!=len(data):
        raise ValueError('Unsupported Bloodborne EMEVD header')
    head=get('<16Q',16)
    if head[0]>10000 or head[2]>1000000:
        raise ValueError('Invalid EMEVD counts')
    result={}
    for i in range(head[0]):
        eid,count,local,_,_,_,_=get('<QQqQqII',head[1]+48*i)
        if count>head[2] or local<0 or local+32*count>32*head[2] or eid in result:
            raise ValueError('Invalid EMEVD event table')
        instructions=[]
        for j in range(count):
            category,index,size,offset,_=get('<IIQqq',head[3]+local+32*j)
            if size>4096 or (size and (offset<0 or offset+size>head[12])):
                raise ValueError('Invalid instruction arguments')
            args=data[head[13]+offset:head[13]+offset+size] if size else b''
            instructions.append((category,index,args))
        result[eid]=instructions
    return result


def audit(game,profile_list=None):
    root=Path(game)/'dvdroot_ps4'
    cache={}
    result=[]
    for p in profile_list if profile_list is not None else profiles():
        aab=p['boss']//10000
        stem=f'm{aab//10:02}_{aab%10:02}_00_00'
        if stem not in cache:
            rs={r['entity']:r for r in regions((root/f'map/MapStudio/{stem}.msb.dcx').read_bytes())}
            es=events((root/f'event/{stem}.emevd.dcx').read_bytes())
            refs=set(es)
            triggers=set()
            for instructions in es.values():
                for category,index,args in instructions:
                    refs.update(struct.unpack_from('<I',args,i)[0] for i in range(0,len(args)-3,4))
                    # IfCharacterRegionState: condition/state padding, chr, region.
                    if (category,index)==(3,2) and len(args)>=12:
                        character,region=struct.unpack_from('<II',args,4)
                        if character==10000: triggers.add(region)
            cache[stem]=(rs,es,refs,triggers)
        rs,es,refs,triggers=cache[stem]
        if p['battle'] not in es:
            raise ValueError(f'Missing battle event for {p["boss"]}')
        for flag in [p['dead'],p['owner'],p['swap'],p['second']]+p['phases']:
            if flag and flag not in refs:
                raise ValueError(f'Missing script reference {flag} for {p["boss"]}')
        if p['region']:
            region=rs.get(p['region'])
            if not region or p['region'] not in triggers:
                raise ValueError(f'Missing PLAYER entrance condition {p["region"]}')
            if len(p['position'])!=3 or any(abs(a-b)>0.001 for a,b in zip(region['position'],p['position'])):
                raise ValueError(f'Compiled entrance coordinates differ: {p["boss"]}')
        result.append((p['boss'],p['region'],p['battle']))
    if profile_list is None:
        for gate in gate_profiles():
            aab=gate['boss']//10000
            stem=f'm{aab//10:02}_{aab%10:02}_00_00'
            _,es,_,_=cache[stem]
            if gate['first'] not in es or gate['fog_event'] not in es:
                raise ValueError(f'Missing first-entry/fog event for {gate["boss"]}')
            parts=[p for p in objects((root/f'map/MapStudio/{stem}.msb.dcx').read_bytes()) if p['entity']==gate['object']]
            if len(parts)!=1 or any(abs(a-b)>0.001 for a,b in zip(parts[0]['position'],gate['position'])) or abs(parts[0]['rotation'][1]-gate['yaw'])>0.001:
                raise ValueError(f'Authored fog object transform differs: {gate["boss"]}')
            packed=struct.pack('<I',gate['object'])
            if not any(packed in args for _,_,args in es[gate['fog_event']]):
                raise ValueError('Fog event does not reference its authored object')
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game',type=Path)
    for row in audit(parser.parse_args().game): print('VERIFIED',*row)
