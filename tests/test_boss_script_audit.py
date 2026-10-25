from pathlib import Path
import re
import struct
import unittest
from tools.audit_boss_scripts import profiles, events,gate_profiles
from tools.inspect_map_regions import regions

ROOT=Path(__file__).resolve().parents[1]
PORT=ROOT/'ports/supermedo/gpu/shim'


class ScriptAuditTests(unittest.TestCase):
    def test_profiles_are_explicit_and_use_redirected_health(self):
        data={p['boss']:p for p in profiles()}
        self.assertEqual(len(data),19)
        self.assertNotIn(2600850,data)
        for first,second in [(2410810,2410811),(2420810,2420811),(3400800,3400801),(3600800,3600801)]:
            self.assertEqual(data[first]['owner'],second)
            self.assertEqual(data[first]['second'],second)
            self.assertGreater(data[first]['swap'],0)
        self.assertEqual(data[2800800]['owner'],2800803)
        self.assertEqual(data[2600800]['owner'],2600802)
        self.assertEqual(data[3500800]['region'],0) # Maria needs interaction, not a cinematic spawn point

    def test_unsafe_position_warp_is_not_available_to_encounters(self):
        for file in ('encounter','observe','walk'):
            source=(PORT/f'bbport_world_warper_{file}.h').read_text()
            self.assertNotIn('ApproachBoss(',source)
            self.assertNotIn('table+0x5f8',source)
            self.assertNotIn('reinterpret_cast<Warp>',source)
        source=(PORT/'bbport_world_warper_events.h').read_text()
        self.assertNotIn('0x13cfcc0',source) # SetEventFlag is not bound
        self.assertIn('BbWorldWarperImage::Matches',source)

    def test_readers_reject_bad_headers_and_cyclic_lists(self):
        with self.assertRaises(ValueError): list(regions(b'MSB '+bytes(200)))
        with self.assertRaises(ValueError): events(bytes(200))
        data=bytearray(100)
        data[:16]=struct.pack('<4sII4B',b'MSB ',1,16,0,0,1,255)
        struct.pack_into('<iiq',data,16,3,1,48)
        struct.pack_into('<q',data,32,16)
        data[48:62]='OTHER\0'.encode('utf-16-le')
        with self.assertRaisesRegex(ValueError,'Cyclic'): list(regions(data))

    def test_boss_progress_version_does_not_reuse_failed_v2_attempts(self):
        source=(ROOT/'tools/run_smoke.py').read_text()
        self.assertIn('bosses-v4-progress.txt',source)
        self.assertIn('entry-check-v4-progress.txt',source)
        self.assertNotIn('bosses-v2-progress.txt',source)

    def test_gate_metadata_covers_reported_entry_failures(self):
        gates={g['boss']:g for g in gate_profiles()}
        self.assertEqual(gates[2410800]['first'],12411702)
        self.assertEqual(gates[2410810]['fog_event'],12414810)
        self.assertEqual(gates[2400800]['fog_event'],12404840)
        source=(PORT/'bbport_world_warper_entry_placement.cpp').read_text()
        self.assertIn('GatePoint(*gate,-3.f)',source)
        self.assertNotIn('boss[1]',source)
        self.assertNotIn('target+',source)
        self.assertIn('distance',source.lower())

    def test_phase_cinematic_reload_is_not_treated_as_death(self):
        source=(PORT/'bbport_world_warper_boot.cpp').read_text()
        self.assertIn('if(playing) cinematic_load_seen=true',source)
        self.assertIn('!cinematic_load_seen && !cinematic_wait',source)
        self.assertIn('cinematic-reload-timeout',source)
