import tempfile
import unittest
from pathlib import Path
from src.test_save import reset_test_save
TMP = Path(__file__).resolve().parents[1] / 'build/test-tmp'
TMP.mkdir(parents=True, exist_ok=True)

class SaveResetTests(unittest.TestCase):
    def test_reset_restores_seed_and_preserves_previous_run(self):
        with tempfile.TemporaryDirectory(dir=TMP) as folder:
            root = Path(folder)
            seed, work, history = root / 'seed', root / 'working', root / 'history'
            slot = Path('1/CUSA00207/SPRJ0005')
            (seed / slot).mkdir(parents=True)
            for name in ('userdata0000', 'userdata0010'):
                (seed / slot / name).write_bytes(b'pristine')
            reset_test_save(seed, work, history)
            (work / slot / 'userdata0000').write_bytes(b'played')
            (work / slot / 'extra-slot').write_bytes(b'not in seed')
            reset_test_save(seed, work, history)
            self.assertEqual((work / slot / 'userdata0000').read_bytes(), b'pristine')
            self.assertFalse((work / slot / 'extra-slot').exists())
            self.assertEqual((seed / slot / 'userdata0000').read_bytes(), b'pristine')
            self.assertEqual(next(history.glob('*/1/CUSA00207/SPRJ0005/userdata0000')).read_bytes(), b'played')

    def test_invalid_seed_keeps_working_save(self):
        with tempfile.TemporaryDirectory(dir=TMP) as folder:
            root = Path(folder)
            (root / 'work').mkdir()
            with self.assertRaises(FileNotFoundError):
                reset_test_save(root / 'missing', root / 'work', root / 'history')
            self.assertTrue((root / 'work').exists())
