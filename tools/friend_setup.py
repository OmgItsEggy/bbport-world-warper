"""Portable first-run setup, using the Python already embedded in Bloodborne.exe."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
SAVE_URL = 'https://www.nexusmods.com/bloodborne/mods/619?tab=files&file_id=3156'
SLOT = Path('1/CUSA00207/SPRJ0005')
SAVE_HASHES = {
    'userdata0000': 'f76c06be69d007b8f857c7ab1272d3745bf45547b3d9eddc5d199f11bd957a35',
    'userdata0010': '986e3758d9d752e59f815215c532554e0ca3e199a4b6745780c6aca5de990f16',
}


def import_save(archive, seed):
    """Import only the verified fixture; no arbitrary ZIP extraction or live-save writes."""
    payload, metadata = {}, {}
    with zipfile.ZipFile(archive) as bundle:
        for name, expected in SAVE_HASHES.items():
            matches = [item for item in bundle.infolist()
                       if item.filename.replace('\\', '/').endswith('/SPRJ0005/' + name)
                       or item.filename.replace('\\', '/') == 'SPRJ0005/' + name]
            if len(matches) != 1 or matches[0].file_size > 2 * 1024 * 1024:
                raise ValueError('This is not the supported unmodified test-save ZIP: ' + name)
            data = bundle.read(matches[0])
            if hashlib.sha256(data).hexdigest() != expected:
                raise ValueError('The test save differs from the verified original: ' + name)
            payload[name] = data
        for name in ('param.bin', 'param.sfo'):
            suffix = 'SPRJ0005.sce_sys/' + name
            matches = [item for item in bundle.infolist()
                       if item.filename.replace('\\', '/') == suffix
                       or item.filename.replace('\\', '/').endswith('/' + suffix)]
            if matches:
                if len(matches) != 1 or matches[0].file_size > 16384:
                    raise ValueError('Invalid test-save metadata: ' + name)
                metadata[name] = bundle.read(matches[0])
    slot = Path(seed) / SLOT
    # No existing seed is silently replaced, and verification finishes before writing.
    if slot.exists():
        raise ValueError('A seed already exists; use the existing seed or move it aside first.')
    slot.mkdir(parents=True)
    for name, data in payload.items():
        (slot / name).write_bytes(data)
    if metadata:
        meta = slot.parent / 'SPRJ0005.sce_sys'
        meta.mkdir()
        for name, data in metadata.items():
            (meta / name).write_bytes(data)


def check_seed(seed):
    return all((Path(seed) / SLOT / name).is_file() and
               hashlib.sha256((Path(seed) / SLOT / name).read_bytes()).hexdigest() == digest
               for name, digest in SAVE_HASHES.items())


def ask_path(label):
    return Path(input(label + '\n> ').strip().strip('"')).resolve()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', type=Path, required=True)
    parser.add_argument('--game', type=Path)
    parser.add_argument('--save-zip', type=Path)
    parser.add_argument('--mode', choices=('full', 'entry-check', 'all-lamps', 'bosses'))
    parser.add_argument('--check', action='store_true', help='Validate setup without launching the game')
    args = parser.parse_args()
    settings_path = ROOT / 'settings.json'
    settings = json.loads(settings_path.read_text()) if settings_path.exists() else {}
    install = args.install.resolve()
    game = args.game or (Path(settings['game']) if settings.get('game') else None)
    if not game:
        game = ask_path('Paste your Bloodborne game-data folder (contains eboot.bin and dvdroot_ps4)')
    game = game.resolve()
    required = [install / 'Bloodborne.exe', install / 'out/boot-linked.bin',
                install / 'out/content.bin', install / 'out/patches.bin', game / 'eboot.bin',
                ROOT / 'bin/bb-probe.exe']
    for path in required:
        if not path.is_file():
            raise ValueError('Required file not found: ' + str(path))
    seed = ROOT / 'build/save-seed/savedata'
    if not check_seed(seed):
        print('Download EonaCatNL\'s original 100-percent save ZIP once:\n' + SAVE_URL)
        print('Its author forbids reuploads, so the save is imported locally, not redistributed.')
        archive = args.save_zip or ask_path('Paste the downloaded ZIP path (do not install it over your own save)')
        import_save(archive, seed)
    settings_path.write_text(json.dumps({'install': str(install), 'game': str(game)}, indent=2))
    print('Setup verified. Your live save is not used; a clean disposable copy is reset for every launch.')
    print('Shaders/pipelines are written to your existing bbport GPU cache. Online is disabled for the tour.')
    if args.check:
        return 0
    mode = args.mode
    if not mode:
        print('\n1: FULL warm-up: equipment + 44 lanterns + boss/phase batch\n2: Four-boss entrance diagnostic\n3: Boss/phase batch only\n4: Equipment + lanterns only')
        choice = input('Choose 1, 2, 3 or 4 [1]: ').strip() or '1'
        if choice not in ('1', '2', '3', '4'):
            raise ValueError('Choose 1, 2, 3 or 4; no game was launched.')
        mode = {'1': 'full', '2': 'entry-check', '3': 'bosses', '4': 'all-lamps'}[choice]
    print('Leave the game controls alone. The tour loads Continue and exits automatically.')
    print('Close the game window to stop early. Next launch starts with a clean test save.')
    arguments = ['--install', str(install), '--game', str(game),
                 '--probe', str(ROOT / 'bin/bb-probe.exe'), '--runtime-bin', str(install / 'bin')]
    if mode == 'full':
        from tools.full_run import run_full
        return run_full(arguments)
    from tools.run_smoke import main as run
    return run([*arguments, '--' + mode])


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        print('Setup error:', error, file=sys.stderr)
        raise SystemExit(1)
