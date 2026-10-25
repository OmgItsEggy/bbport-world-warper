import hashlib
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch
import zipfile
from tools.friend_setup import import_save, check_seed, SLOT
from tools.full_run import run_full


class FriendSetupTests(unittest.TestCase):
    def test_full_run_includes_equipment_lanterns_and_bosses_without_user_checkpoint(self):
        with patch('tools.full_run.run_tour', side_effect=[0, 0]) as tour:
            self.assertEqual(run_full(['--install', 'example']), 0)
            self.assertEqual(tour.call_args_list[0].args[0], ['--install', 'example', '--all-lamps'])
            self.assertEqual(tour.call_args_list[1].args[0], ['--install', 'example', '--bosses'])

    def test_zip_import_is_verified_before_writing_and_has_no_path_traversal(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / 'save.zip'
            with zipfile.ZipFile(archive, 'w') as bundle:
                bundle.writestr('download/CUSA00207/SPRJ0005/userdata0000', b'character')
                bundle.writestr('../../outside.txt', b'not extracted')
            digest = {'userdata0000': hashlib.sha256(b'character').hexdigest()}
            with patch('tools.friend_setup.SAVE_HASHES', digest):
                import_save(archive, root / 'seed')
                self.assertTrue(check_seed(root / 'seed'))
                self.assertEqual(list((root / 'seed' / SLOT).iterdir()), [root / 'seed' / SLOT / 'userdata0000'])
                with self.assertRaisesRegex(ValueError, 'already exists'):
                    import_save(archive, root / 'seed')
            self.assertFalse((root / 'outside.txt').exists())

    def test_bad_save_is_rejected_without_partial_import(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / 'save.zip'
            with zipfile.ZipFile(archive, 'w') as bundle:
                bundle.writestr('SPRJ0005/userdata0000', b'wrong')
            with self.assertRaises(ValueError):
                import_save(archive, root / 'seed')
            self.assertFalse((root / 'seed').exists())


if __name__ == '__main__':
    unittest.main()
