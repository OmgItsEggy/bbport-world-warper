"""Launch the local 60-second camera sweep using existing prepared game data."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from src.test_save import reset_test_save

def recover_gear_checkpoint(log_path, checkpoint):
    """Migrate completed gear coverage from an older build's retained log."""
    import re
    if checkpoint.exists() or not log_path.exists():
        return
    matches = re.findall(r'gear pass finished: (\d+) rows failed equip verification',
                         log_path.read_text(encoding='utf-8', errors='replace'))
    if matches:
        checkpoint.write_text(f'attempted 34 failures {matches[-1]}\n', encoding='ascii')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--runtime-bin', type=Path, required=True)
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--record-exit', action='store_true')
    parser.add_argument('--all-lamps', action='store_true')
    parser.add_argument('--encounters', action='store_true')
    args = parser.parse_args()
    install, game, probe = args.install.resolve(), args.game.resolve(), args.probe.resolve()
    required = [probe, game / 'eboot.bin', install / 'out/boot-linked.bin',
                install / 'out/content.bin', install / 'out/patches.bin']
    for path in required:
        if not path.is_file():
            parser.error(f'Missing required file: {path}')
    run = ROOT / 'build/fresh-character'
    user = run / 'user'
    user.mkdir(parents=True, exist_ok=True)
    for name in ('bbport.ini', 'input.ini'):
        if (install / name).is_file():
            shutil.copy2(install / name, run / name)
    env = {key: value for key, value in os.environ.items()
           if key not in {'BB_ONLINE', 'BB_NET_BACKEND'} and
           not key.startswith(('BB_PARTY', 'BB_SHADNET', 'BB_THD', 'BB_COMMUNITY'))}
    env.update(BB_WORLD_WARPER='smoke', BB_USER_DIR=str(user),
               BB_GPU_USER_DIR=str(install / 'user'), BB_CONFIG=str(run / 'bbport.ini'))
    env['BB_WORLD_WARPER_LAMP'] = '2412950'
    if args.encounters and (args.all_lamps or args.record_exit):
        parser.error('--encounters cannot be combined with all-lamps or recording')
    route = 'encounters' if args.encounters else ('all-lamps' if args.all_lamps else 'three-area')
    env['BB_WORLD_WARPER_ROUTE'] = route
    progress = run / f'{route}-progress.txt'
    env['BB_WORLD_WARPER_STATE'] = str(progress)
    gear_progress = progress.with_suffix('.gear.txt')
    env['BB_WORLD_WARPER_GEAR_STATE'] = str(gear_progress)
    env['BB_WORLD_WARPER_AUTO'] = '1'
    env['BB_WORLD_WARPER_GEAR'] = '0' if args.record_exit or args.encounters else '1'
    env.pop('BB_WORLD_WARPER_CATHEDRAL_ROUTE', None)
    if args.record_exit:
        env['BB_WORLD_WARPER_RECORD'] = 'cathedral-exit'
        env['BB_PAD_RECORD'] = str(run / 'cathedral-exit.pad')
    if not (user / 'savedata/1/CUSA00207/SPRJ0005/userdata0000').is_file():
        parser.error('Automatic Continue requires the imported test character.')
    env['PATH'] = str(args.runtime_bin.resolve()) + os.pathsep + str(install / 'bin') + os.pathsep + env.get('PATH', '')
    command = [str(probe), str(install / 'out/boot-linked.bin'),
               '--content-profile', str(install / 'out/content.bin'),
               '--patches', str(install / 'out/patches.bin'),
               '--app0', str(game), '--user', str(user), '--timeout', '0']
    if (user / 'savedata/1/CUSA00207/SPRJ0005/userdata0000').is_file():
        print('Automatic Continue is enabled. Leave the title screen controls untouched.')
    else:
        print('First run: choose New Game and create a disposable character. No progressed save is needed.')
    if not args.encounters:
        print('All-lamp tour selected; some destinations still need in-game validation.' if args.all_lamps
              else 'Three-area tour: Central Yharnam, Cathedral Ward, Old Yharnam.')
    print('Focused encounter observation: two Upper Cathedral destinations and Cainhurst rooftop; no gear repetition.'
          if args.encounters else 'Combined pass: weapon/armour rotation first, then the lamp tour.')
    print('The disposable save is reset from the pristine download before every launch.')
    print('The game closes 10 seconds after the tour finishes. Log:', run / 'smoke.log')
    if args.record_exit:
        print('RECORDING MODE: automatic travel stops at Cathedral Ward. Automatic exit is disabled.')
        print('At the lamp: press F9, walk outside to a useful viewpoint, release movement, then press F9 again.')
        print('Quit normally after recording. Your recorded route stays in the project.')
    if args.prepare_only:
        return 0
    recover_gear_checkpoint(run / 'smoke.log', gear_progress)
    # Preserve the previous session's crash evidence before opening a new log.
    if (run / 'smoke.log').exists():
        import time
        shutil.copy2(run / 'smoke.log', run / f'smoke-{time.time_ns()}.log')
    with (run / 'smoke.log').open('w', encoding='utf-8') as log:
        for attempt in range(5):
            reset_test_save(ROOT / 'build/save-seed/savedata', user / 'savedata', run / 'save-history')
            before = progress.read_text() if progress.exists() else ''
            code = subprocess.run(command, cwd=run, env=env, stdout=log, stderr=subprocess.STDOUT).returncode
            after = progress.read_text() if progress.exists() else ''
            lines = after.splitlines()
            terminal = len(lines) >= (44 if args.all_lamps else 3)
            failed_load = after != before and lines and lines[-1].startswith('skip ')
            if args.record_exit or (code == 0 and (not failed_load or terminal)):
                print('Coverage recorded:', sum(line.startswith('done ') for line in lines),
                      'completed,', sum(line.startswith('skip ') for line in lines), 'skipped.')
                return code
            print('Restarting from recorded progress after failed load/crash:', attempt + 1)
        print('Retry limit reached. Check smoke.log and the done/skip progress file.')
        return 1

if __name__ == '__main__':
    raise SystemExit(main())
