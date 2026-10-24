import tempfile
import unittest
from pathlib import Path

from src.launcher_warper import (
    prepare_warper_saves,
    read_warper_progress,
    warper_environment,
    warper_paths,
)

ROOT = Path(__file__).resolve().parents[1]
TMP_ROOT = ROOT / "build" / "test-tmp"
TMP_ROOT.mkdir(parents=True, exist_ok=True)


class LauncherWarperTests(unittest.TestCase):
    def test_save_copy_and_resume_never_change_source(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as folder:
            user = Path(folder) / "user"
            original = user / "savedata" / "SPRJ0005" / "userdata0000"
            original.parent.mkdir(parents=True)
            original.write_bytes(b"original-save")
            paths = warper_paths(user)

            self.assertEqual(prepare_warper_saves(paths, fresh=True), 1)
            copied = paths["saves"] / "SPRJ0005" / "userdata0000"
            copied.write_bytes(b"warper-mutated-save")
            self.assertEqual(original.read_bytes(), b"original-save")

            self.assertEqual(prepare_warper_saves(paths, fresh=False), 0)
            self.assertEqual(copied.read_bytes(), b"warper-mutated-save")
            self.assertEqual(prepare_warper_saves(paths, fresh=True), 1)
            self.assertEqual(copied.read_bytes(), b"original-save")

    def test_environment_uses_real_cache_and_disables_online(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as folder:
            root = Path(folder)
            user = root / "user"
            (user / "savedata").mkdir(parents=True)
            cache = root / "gpu-cache"
            paths = warper_paths(user, cache)
            source = {
                "KEEP": "yes",
                "BB_ONLINE": "1",
                "BB_NET_BACKEND": "thd",
                "BB_PARTY": "1",
                "BB_SHADNET_TOKEN": "secret",
                "BB_COMMUNITY": "1",
            }
            env = warper_environment(source, paths, "measure,enemies=8,bosses")

            self.assertEqual(source["BB_ONLINE"], "1")
            self.assertEqual(env["KEEP"], "yes")
            self.assertFalse(any(key.startswith(("BB_PARTY", "BB_SHADNET", "BB_THD", "BB_COMMUNITY")) for key in env))
            self.assertNotIn("BB_ONLINE", env)
            self.assertNotIn("BB_NET_BACKEND", env)
            self.assertEqual(env["BB_USER_DIR"], str(paths["base"]))
            self.assertEqual(env["BB_GPU_USER_DIR"], str(cache.resolve()))
            self.assertEqual(env["BB_WORLD_WARPER"], "measure,enemies=8,bosses")

    def test_invalid_mode_is_rejected(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as folder:
            paths = warper_paths(Path(folder) / "user")
            with self.assertRaises(ValueError):
                warper_environment({}, paths, "tour;BAD=1")

    def test_progress_summary_deduplicates_rows(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as folder:
            state = Path(folder) / "state.txt"
            state.write_text(
                "start 2102950\n"
                "start 2102950\n"
                "done 2102950 12\n"
                "start 2412950\n"
                "skip 2202950 crash\n"
                "finished\n",
                encoding="utf-8",
            )
            self.assertEqual(
                read_warper_progress(state),
                {"done": 1, "skipped": 1, "started": 1, "finished": True, "new_pipelines": 12},
            )


if __name__ == "__main__":
    unittest.main()

