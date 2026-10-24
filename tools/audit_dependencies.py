"""Audit the pinned Visy world-warper source before porting it to Supermedo.

This intentionally uses only the Python standard library so it runs in a clean checkout.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "reference/visy/gpu/shim/party/world_warper.cpp"
CONTENT = ROOT / "reference/visy/gpu/shim/party/world_warper_content.inc"

INCLUDE_RE = re.compile(r'^#include\s+"([^"]+)"', re.MULTILINE)
ARRAY_RE = re.compile(r"WARPER_ARRAY\((\w+),\s*(\d+)\)")

PARTY_ONLY = {
    "party_async_io.h",
    "party_balance.h",
    "party_director.h",
    "party_enemy_census.h",
}
GENERIC_GAME_ACCESS = {
    "coop_hooks.h",
    "game_state.h",
    "lamp_travel.h",
    "party_ids.h",
    "seamless_rules.h",
}
PORT_NATIVE = {
    "bbport_compile_progress.h",
    "../bbport_game_menu.h",
}
SELF_CONTAINED = {"world_warper.h", "world_warper_rules.h"}


def audit() -> dict:
    source = SOURCE.read_text(encoding="utf-8")
    includes = INCLUDE_RE.findall(source)
    equipment = {
        name: int(count)
        for name, count in ARRAY_RE.findall(CONTENT.read_text(encoding="utf-8"))
    }
    known = PARTY_ONLY | GENERIC_GAME_ACCESS | PORT_NATIVE | SELF_CONTAINED
    project = [name for name in includes if not name.startswith("<")]
    return {
        "source": str(SOURCE.relative_to(ROOT)),
        "project_includes": project,
        "generic_game_access": sorted(set(project) & GENERIC_GAME_ACCESS),
        "party_only_to_replace": sorted(set(project) & PARTY_ONLY),
        "port_native_or_equivalent": sorted(set(project) & PORT_NATIVE),
        "self_contained": sorted(set(project) & SELF_CONTAINED),
        "unclassified": sorted(set(project) - known),
        "equipment": equipment,
    }


if __name__ == "__main__":
    print(json.dumps(audit(), indent=2))

