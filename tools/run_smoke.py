"""Launch the local 60-second camera sweep using existing prepared game data."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--runtime-bin', type=Path, required=True)
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    install, game, probe = args.install.resolve(), args.game.resolve(), args.probe.resolve()
    required = [probe, game / 'eboot.bin', install / 'out/boot-linked.bin',
                install / 'out/content.bin', install / 'out/patches.bin']
    for path in required:
        if not path.is_file():
            parser.error(f'Missing required file: {path}')
    source = install / 'user/savedata'
    if not source.is_dir():
        parser.error(f'No existing character save at {source}')
    run = ROOT / 'build/smoke'
    user = run / 'user'
    user.mkdir(parents=True, exist_ok=True)
    if not (user / 'savedata').exists():
        shutil.copytree(source, user / 'savedata')
    for name in ('bbport.ini', 'input.ini'):
        if (install / name).is_file():
            shutil.copy2(install / name, run / name)
    env = {key: value for key, value in os.environ.items()
           if key not in {'BB_ONLINE', 'BB_NET_BACKEND'} and
           not key.startswith(('BB_PARTY', 'BB_SHADNET', 'BB_THD', 'BB_COMMUNITY'))}
    env.update(BB_WORLD_WARPER='smoke', BB_USER_DIR=str(user),
               BB_GPU_USER_DIR=str(install / 'user'), BB_CONFIG=str(run / 'bbport.ini'))
    env['PATH'] = str(args.runtime_bin.resolve()) + os.pathsep + str(install / 'bin') + os.pathsep + env.get('PATH', '')
    command = [str(probe), str(install / 'out/boot-linked.bin'),
               '--content-profile', str(install / 'out/content.bin'),
               '--patches', str(install / 'out/patches.bin'),
               '--app0', str(game), '--user', str(user), '--timeout', '0']
    print('Load your copied character. The camera sweep starts in gameplay and ends after 60 seconds.')
    print('Quit normally afterward. Log:', run / 'smoke.log')
    if args.prepare_only:
        return 0
    with (run / 'smoke.log').open('w', encoding='utf-8') as log:
        return subprocess.run(command, cwd=run, env=env, stdout=log, stderr=subprocess.STDOUT).returncode

if __name__ == '__main__':
    raise SystemExit(main())
