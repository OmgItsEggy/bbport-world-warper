import copy
import json
from pathlib import Path
import re
import unittest
from tools.inspect_return_points import audit_manifest, binder, dcx, param_rows

ROOT=Path(__file__).resolve().parents[1]
# Independently transcribed from the local ReturnPointParam boss-defeat row audit.
EXPECTED={
    'ClericBeast':2412952,'Gascoigne':2412953,'BloodStarvedBeast':2302951,
    'Amelia':2402951,'Witches':2202951,'Paarl':2302952,'Shadows':2702951,
    'Rom':3202952,'OneReborn':2802951,'Amygdala':3302951,'Micolash':2602952,
    'WetNurse':2602951,'Logarius':2502952,'Emissary':2422952,'Ebrietas':2422951,
    'Gehrman':0,'MoonPresence':0,'Ludwig':3402952,'Laurence':3402953,
    'LivingFailures':3502952,'Maria':3502951,'Orphan':3602952}

class BossDestinationTests(unittest.TestCase):
    def setUp(self):
        self.manifest=json.loads((ROOT/'src/boss_destinations.json').read_text())
        self.rows={}
        for item in self.manifest['destinations']:
            if item['lamp']:
                aab=item['lamp']//10000
                self.rows[item['lamp']]=('test boss 撃破',bytes([aab//10,0,aab%10,0]))

    def test_exact_boss_only_mapping_and_native_header(self):
        self.assertEqual({x['boss']:x['lamp'] for x in self.manifest['destinations']},EXPECTED)
        header=(ROOT/'ports/supermedo/gpu/shim/bbport_world_warper_route.h').read_text()
        block=header.split('boss_stops{{',1)[1].split('}};',1)[0]
        native=[]
        for line in block.splitlines():
            match=re.search(r'Lamp\((\d+),(\d+),"[^"]+",(\d+)\)',line)
            if match:
                aab,n,entity=map(int,match.groups()); native.append((aab*10000+2950+n,entity))
            else:
                match=re.search(r'\{0,0x15000000,"[^"]+",(\d+)\}',line)
                if match: native.append((0,int(match[1])))
        self.assertEqual(native,[(x['lamp'],x['entity']) for x in self.manifest['destinations']])
        self.assertEqual(len(audit_manifest(self.rows,self.manifest)),20)

    def test_normal_lantern_or_dream_substitution_is_rejected(self):
        bad=copy.deepcopy(self.manifest)
        bad['destinations'][0]['lamp']=2412950
        self.rows[2412950]=('normal start',bytes([24,0,1,0]))
        with self.assertRaisesRegex(ValueError,'Not a boss-defeat'): audit_manifest(self.rows,bad)
        bad=copy.deepcopy(self.manifest)
        bad['destinations'][15]['lamp']=2102950
        self.rows[2102950]=('normal Dream',bytes([21,0,0,0]))
        with self.assertRaisesRegex(ValueError,'Not a boss-defeat'): audit_manifest(self.rows,bad)

    def test_invalid_map_or_unexplained_block_is_rejected(self):
        self.rows[2412952]=('boss 撃破',bytes([29,0,0,0]))
        with self.assertRaisesRegex(ValueError,'Wrong map'): audit_manifest(self.rows,self.manifest)
        bad=copy.deepcopy(self.manifest); bad['destinations'][15].pop('blocked')
        self.rows[2412952]=('boss 撃破',bytes([24,0,1,0]))
        with self.assertRaisesRegex(ValueError,'Unexplained missing route'): audit_manifest(self.rows,bad)

    def test_readers_refuse_unsupported_input(self):
        with self.assertRaises(ValueError): list(binder(b'not a binder'))
        with self.assertRaises(ValueError): list(param_rows(b'bad'))
        with self.assertRaises(ValueError): dcx(b'DCX\0'+bytes(100))
