"""Reset the disposable character from a pristine local seed before each test."""
from pathlib import Path
import shutil
import time

def reset_test_save(seed: Path, destination: Path, history: Path) -> None:
    seed, destination, history = seed.resolve(), destination.resolve(), history.resolve()
    if seed == destination or seed.is_relative_to(destination) or destination.is_relative_to(seed):
        raise ValueError('Seed and working saves must be separate trees')
    slot = Path('1/CUSA00207/SPRJ0005')
    for name in ('userdata0000', 'userdata0010'):
        if not (seed / slot / name).is_file():
            raise FileNotFoundError(f'Pristine save is incomplete: {name}')
    if destination.exists():
        history.mkdir(parents=True, exist_ok=True)
        destination.rename(history / str(time.time_ns()))
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(seed, destination)
