"""Create a portable add-on and complete modified port source archive; never include game/save data."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def source_files(root):
    data = subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z', '--recurse-submodules'])
    return [root / name.decode('utf-8') for name in data.split(b'\0') if name]


def project_files():
    data = subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', '-z', '--cached', '--others', '--exclude-standard'])
    result = []
    for name in data.split(b'\0'):
        if not name:
            continue
        relative = Path(name.decode('utf-8'))
        if relative.parts[0] in ('routes', 'bin', 'build', 'out'):
            continue
        path = ROOT / relative
        if path.is_file():
            result.append(path)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream', type=Path, required=True)
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--version', default='v0.1.0-alpha.1')
    args = parser.parse_args()
    upstream, destination = args.upstream.resolve(), args.destination.resolve()
    if destination.exists():
        raise ValueError('Use a new destination; existing packages are never overwritten')
    probe = upstream / 'out/bb-probe.exe'
    if not probe.is_file():
        raise ValueError('Build the modified probe first')
    files = project_files()
    stage = destination / 'bbport-world-warper'
    stage.mkdir(parents=True)
    for path in files:
        relative = path.relative_to(ROOT)
        # Native sources stay in the companion archive, but the preflight headers
        # are needed at runtime. No personal routes, settings or saves are copied.
        if relative.parts[0] in ('.github', 'reference', 'tests'):
            continue
        target = stage / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    (stage / 'bin').mkdir()
    shutil.copy2(probe, stage / 'bin/bb-probe.exe')
    info = {'version': args.version, 'base': 'Supermedo/bloodborne_pc windows-v1.9.4',
            'base_commit': '3fabfa7a7af67a34a6dfc630b6ec2539b68fad4d',
            'probe_sha256': hashlib.sha256(probe.read_bytes()).hexdigest(),
            'runtime': 'Uses the DLLs and embedded Python from your prepared bbport v1.9.4 installation',
            'save': 'Original Nexus mod 619 file 3156 must be downloaded by its user; redistribution prohibited'}
    (stage / 'BUILD.json').write_text(json.dumps(info, indent=2), encoding='utf-8')
    portable = destination / ('WorldWarper-' + args.version + '-windows-x64.zip')
    with zipfile.ZipFile(portable, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(stage.rglob('*')):
            if path.is_file():
                archive.write(path, path.relative_to(destination))
    sources = destination / ('WorldWarper-' + args.version + '-corresponding-source.zip')
    with zipfile.ZipFile(sources, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in files:
            archive.write(path, Path('world-warper') / path.relative_to(ROOT))
        for path in source_files(upstream):
            if path.is_file():
                archive.write(path, Path('modified-supermedo') / path.relative_to(upstream))
        # The integration is a set of new untracked files in the build checkout.
        for path in sorted((upstream / 'gpu/shim').glob('bbport_world_warper*')):
            archive.write(path, Path('modified-supermedo/gpu/shim') / path.name)
        archive.writestr('BUILD.json', json.dumps(info, indent=2))
    checksums = destination / 'SHA256SUMS.txt'
    checksums.write_text(''.join(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + path.name + '\n'
                                 for path in (portable, sources)), encoding='ascii')
    print(portable)
    print(sources)
    print(checksums)


if __name__ == '__main__':
    main()
