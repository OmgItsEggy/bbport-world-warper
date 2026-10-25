import tempfile
import unittest
from pathlib import Path
from tools.run_smoke import recover_gear_checkpoint
TMP = Path(__file__).resolve().parents[1] / 'build/test-tmp'
TMP.mkdir(parents=True, exist_ok=True)

class GearCheckpointTests(unittest.TestCase):
    def test_recover_completed_pass_even_after_restart(self):
        with tempfile.TemporaryDirectory(dir=TMP) as folder:
            root = Path(folder)
            log, checkpoint = root / 'log', root / 'gear'
            log.write_text('gear pass finished: 0 rows failed equip verification; starting map coverage\n'
                           'combined gear pass starting before lamp tour\n')
            recover_gear_checkpoint(log, checkpoint)
            self.assertEqual(checkpoint.read_text(), 'attempted 34 failures 0\n')
            log.write_text('gear pass finished: 2 rows failed equip verification\n')
            recover_gear_checkpoint(log, checkpoint)
            self.assertEqual(checkpoint.read_text(), 'attempted 34 failures 0\n')

    def test_partial_pass_is_not_claimed_complete(self):
        with tempfile.TemporaryDirectory(dir=TMP) as folder:
            root = Path(folder)
            log, checkpoint = root / 'log', root / 'gear'
            log.write_text('gear row 1/34 equip=verified\n')
            recover_gear_checkpoint(log, checkpoint)
            self.assertFalse(checkpoint.exists())
