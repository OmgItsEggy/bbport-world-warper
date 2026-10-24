import importlib.util
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load_audit_module():
    path = ROOT / "tools/audit_dependencies.py"
    spec = importlib.util.spec_from_file_location("audit_dependencies", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(module)
    return module


class ProjectContractTests(unittest.TestCase):
    def test_revisions_are_pinned(self):
        project = json.loads((ROOT / "project.json").read_text(encoding="utf-8"))
        self.assertEqual(project["targets"]["supermedo"]["tag"], "windows-v1.9.4")
        self.assertEqual(len(project["targets"]["supermedo"]["commit"]), 40)
        self.assertEqual(project["targets"]["reference"]["tag"], "v0.7.5")
        self.assertEqual(len(project["targets"]["reference"]["commit"]), 40)

    def test_live_cache_and_copied_save_are_separate(self):
        project = json.loads((ROOT / "project.json").read_text(encoding="utf-8"))
        policy = project["cache_policy"]
        self.assertEqual(policy["save_directory"], "isolated-copy")
        self.assertEqual(policy["gpu_cache_directory"], "player-real-cache")

    def test_equipment_tables_match_reference(self):
        audit = load_audit_module().audit()
        self.assertEqual(audit["equipment"]["kWarperRightWeapons"], 26)
        self.assertEqual(audit["equipment"]["kWarperLeftWeapons"], 15)
        self.assertEqual(audit["equipment"]["kWarperArmorSets"], 34)

    def test_every_project_include_is_classified(self):
        audit = load_audit_module().audit()
        self.assertEqual(audit["unclassified"], [])
        self.assertEqual(
            audit["party_only_to_replace"],
            ["party_async_io.h", "party_balance.h", "party_director.h", "party_enemy_census.h"],
        )


if __name__ == "__main__":
    unittest.main()

