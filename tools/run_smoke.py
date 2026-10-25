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
from src.encounter_report import write_report
from tools.inspect_return_points import audit_game_routes
from tools.audit_boss_scripts import audit as audit_boss_scripts

def recover_gear_checkpoint(log_path, checkpoint):
    """Migrate completed gear coverage from an older build's retained log."""
    import re
    if checkpoint.exists() or not log_path.exists():
        return
    matches = re.findall(r'gear pass finished: (\d+) rows failed equip verification',
                         log_path.read_text(encoding='utf-8', errors='replace'))
    if matches:
        checkpoint.write_text(f'attempted 34 failures {matches[-1]}\n', encoding='ascii')

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--runtime-bin', type=Path, required=True)
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--record-exit', action='store_true')
    parser.add_argument('--all-lamps', action='store_true')
    parser.add_argument('--encounters', action='store_true')
    parser.add_argument('--bosses', action='store_true')
    parser.add_argument('--entry-check', action='store_true')
    args = parser.parse_args(argv)
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
    if sum((args.encounters,args.bosses,args.all_lamps,args.record_exit,args.entry_check))>1:
        parser.error('Select only one tour/recording mode')
    route = 'entry-check' if args.entry_check else 'bosses' if args.bosses else ('encounters' if args.encounters else ('all-lamps' if args.all_lamps else 'three-area'))
    boss_batch=args.encounters or args.bosses or args.entry_check
    if args.bosses or args.entry_check:
        try:
            verified=audit_game_routes(game / 'dvdroot_ps4/param/gameparam/gameparam.parambnd.dcx',ROOT / 'src/boss_destinations.json')
            scripts=audit_boss_scripts(game)
        except (ValueError,KeyError,OSError) as error:
            parser.error(f'Boss-only route verification failed; game was not launched: {error}')
        print(f'Local game-data preflight passed: {len(verified)} boss-room endpoints verified.')
        print(f'Local scripts/maps preflight passed: {len(scripts)} activation profiles verified.')
    env['BB_WORLD_WARPER_ROUTE'] = route
    # v4 adds first-entry versus repeat-fog handling and confirmed wake-up hits.
    # Never reuse the earlier timed attempts as successful entrance/phase evidence.
    progress = run / ('bosses-v4-progress.txt' if args.bosses else 'encounters-v4-progress.txt' if args.encounters else 'entry-check-v4-progress.txt' if args.entry_check else f'{route}-progress.txt')
    env['BB_WORLD_WARPER_STATE'] = str(progress)
    gear_progress = progress.with_suffix('.gear.txt')
    env['BB_WORLD_WARPER_GEAR_STATE'] = str(gear_progress)
    env['BB_WORLD_WARPER_AUTO'] = '1'
    env['BB_WORLD_WARPER_GEAR'] = '0' if args.record_exit or boss_batch else '1'
    env['BB_WORLD_WARPER_BOSS_HEALTH_STAGES'] = '1' if boss_batch and not args.entry_check else '0'
    env['BB_WORLD_WARPER_ENTRY_ONLY'] = '1' if args.entry_check else '0'
    env['BB_WORLD_WARPER_BOSS_RUNS'] = '1' if boss_batch else '0'
    env['BB_WORLD_WARPER_BOSS_STATE'] = str(progress.with_suffix('.boss-attempts.txt'))
    env.pop('BB_WORLD_WARPER_CATHEDRAL_ROUTE', None)
    if args.record_exit:
        env['BB_WORLD_WARPER_RECORD'] = 'cathedral-exit'
        env['BB_PAD_RECORD'] = str(run / 'cathedral-exit.pad')
    if not (ROOT / 'build/save-seed/savedata/1/CUSA00207/SPRJ0005/userdata0000').is_file():
        parser.error('Automatic Continue requires the imported pristine test seed. Run Start-WorldWarper.cmd first.')
    env['PATH'] = str(args.runtime_bin.resolve()) + os.pathsep + str(install / 'bin') + os.pathsep + env.get('PATH', '')
    command = [str(probe), str(install / 'out/boot-linked.bin'),
               '--content-profile', str(install / 'out/content.bin'),
               '--patches', str(install / 'out/patches.bin'),
               '--app0', str(game), '--user', str(user), '--timeout', '0']
    print('Automatic Continue is enabled. Leave the title screen controls untouched.')
    if args.bosses:
        print('Boss-only v4: 19 encounters, Micolash excluded, two ending entries unavailable. No gear repeat.')
        print('One authored outside-gate setup where required, then real entry walking; no fight-time position teleports.')
        print('Activation/known phase events are checked; stalled entrances and falls are reported incomplete.')
    elif args.entry_check:
        print('SHORT ENTRY CHECK: Cleric Beast, Gascoigne, Rom, Ebrietas only. No health phases, gear pass or full tour.')
        print('Each stop ends once the actual battle event is confirmed. Failures are logged automatically.')
    elif not args.encounters:
        print('All-lamp tour selected; some destinations still need in-game validation.' if args.all_lamps
              else 'Three-area tour: Central Yharnam, Cathedral Ward, Old Yharnam.')
    if not args.bosses and not args.entry_check:
        print('Focused encounter observation: two Upper Cathedral destinations and Cainhurst rooftop; no gear repetition.'
              if args.encounters else 'Combined pass: weapon/armour rotation first, then the lamp tour.')
    print('The disposable save is reset from the pristine download before every launch.')
    if boss_batch and not args.entry_check:
        print('Boss batch: ranged then close observation; health requests use current-form and shared-owner maximum HP.')
        print('Known transitions wait for game events, with a bounded failure timeout. Group encounters remain observation-only.')
        print('A completed tour does not certify every phase, attack or shader; see the evidence report.')
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
            log.flush()
            report_path=run / f'{route}-report.json'
            report=write_report(run / 'smoke.log', report_path)
            print('Session evidence report:', report_path)
            if args.entry_check:
                for boss in ('ClericBeast','Gascoigne','Rom','Ebrietas'):
                    result=report['boss_roster'].get(boss,{'status':'not-attempted'})
                    print(boss+':',result['status'],result.get('reason',''))
            after = progress.read_text() if progress.exists() else ''
            lines = after.splitlines()
            terminal = len(lines) >= (4 if args.entry_check else 22 if args.bosses else 44 if args.all_lamps else 3)
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
