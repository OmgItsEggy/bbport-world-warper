"""Launcher support for a Supermedo world-warper run.

The save and GPU-cache roots must remain separate: the automated run is free to
move the character and consume items in its copied save, while every pipeline it
creates belongs in the same cache normal play will consume.
"""
from __future__ import annotations

import re
import shutil
from pathlib import Path

WARPER_DIR = "world_warper"
WARPER_STATE = "world_warper_state.txt"

_MODE_RE = re.compile(r"^(tour|replay|measure)(?:,[a-z][a-z0-9_-]*(?:=[A-Za-z0-9_+.-]+)?)*$")
_ONLINE_EXACT = {"BB_ONLINE", "BB_NET_BACKEND"}
_ONLINE_PREFIXES = ("BB_PARTY", "BB_SHADNET", "BB_THD", "BB_COMMUNITY")


def warper_paths(save_root: Path, cache_root: Path | None = None) -> dict[str, Path]:
    """Return all paths used by a run.

    ``save_root`` is the player's normal user/save directory. ``cache_root`` may
    differ when the launcher stores GPU caches separately.
    """
    user = Path(save_root).resolve()
    cache = Path(cache_root).resolve() if cache_root is not None else user
    base = user / WARPER_DIR
    return {
        "user": user,
        "cache": cache,
        "base": base,
        "saves": base / "savedata",
        "state": base / WARPER_STATE,
    }


def _assert_safe_paths(paths: dict[str, Path]) -> None:
    user = paths["user"].resolve()
    base = paths["base"].resolve()
    saves = paths["saves"].resolve()
    if base == user or not base.is_relative_to(user):
        raise ValueError("warper base must be a child of the player user directory")
    if saves == user or not saves.is_relative_to(base):
        raise ValueError("warper save copy must be a child of the warper base")


def prepare_warper_saves(paths: dict[str, Path], *, fresh: bool) -> int:
    """Create or resume the disposable save copy.

    Returns the number of copied files. A return value of zero means a valid
    existing copy was retained for resume. The player's ``savedata`` tree is
    only ever read.
    """
    _assert_safe_paths(paths)
    source = paths["user"] / "savedata"
    if not source.is_dir() or not any(path.is_file() for path in source.rglob("*")):
        raise FileNotFoundError(f"no save in {source}")

    base = paths["base"]
    target = paths["saves"]
    if fresh and base.exists():
        shutil.rmtree(base)
    if target.is_dir() and any(path.is_file() for path in target.rglob("*")):
        return 0

    base.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, target, dirs_exist_ok=True)

    addcont = paths["user"] / "addcont"
    if addcont.is_dir() and not (base / "addcont").exists():
        shutil.copytree(addcont, base / "addcont")
    return sum(1 for path in target.rglob("*") if path.is_file())


def warper_environment(environment: dict[str, str], paths: dict[str, Path], mode: str = "tour") -> dict[str, str]:
    """Return a solo-run environment without mutating the caller's mapping."""
    _assert_safe_paths(paths)
    if not _MODE_RE.fullmatch(mode):
        raise ValueError(f"invalid world-warper mode: {mode!r}")

    env = {
        key: value
        for key, value in environment.items()
        if key not in _ONLINE_EXACT and not key.startswith(_ONLINE_PREFIXES)
    }
    env.update(
        BB_USER_DIR=str(paths["base"]),
        BB_GPU_USER_DIR=str(paths["cache"]),
        BB_WORLD_WARPER=mode,
        BB_WORLD_WARPER_STATE=str(paths["state"]),
        BB_TIMEOUT="0",
        BB_LAMP_TRAVEL="1",
    )
    return env


def read_warper_progress(state: Path) -> dict[str, int | bool]:
    """Summarise the append-only progress file written by the native warper."""
    result: dict[str, int | bool] = {
        "done": 0,
        "skipped": 0,
        "started": 0,
        "finished": False,
        "new_pipelines": 0,
    }
    try:
        text = Path(state).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return result

    done: set[str] = set()
    skipped: set[str] = set()
    started: set[str] = set()
    for line in text.splitlines():
        parts = line.split()
        if parts == ["finished"]:
            result["finished"] = True
        elif len(parts) >= 2 and parts[1].isdigit():
            action, row = parts[:2]
            if action == "done":
                done.add(row)
                if len(parts) >= 3 and parts[2].isdigit():
                    result["new_pipelines"] = int(result["new_pipelines"]) + int(parts[2])
            elif action == "skip":
                skipped.add(row)
            elif action == "start":
                started.add(row)

    result["done"] = len(done)
    result["skipped"] = len(skipped)
    result["started"] = len(started - done - skipped)
    return result

