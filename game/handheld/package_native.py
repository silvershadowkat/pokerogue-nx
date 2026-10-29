"""Build the native runtime's indexed ZIP from an official offline web build.

Run Vite with --mode native --config handheld/vite.config.ts first.
Assets/locales remain in one archive at runtime, without extraction.
"""
from pathlib import Path
import argparse
import subprocess
import zipfile
import json
import hashlib

ROOT = Path(__file__).resolve().parents[1]

def tracked(folder):
    data = subprocess.check_output(['git', '-C', str(ROOT / folder), 'ls-files', '-z'])
    return [name.decode('utf8') for name in data.split(b'\0') if name]

def build_provenance():
    def git(*args):
        return subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()
    # Source ZIPs intentionally omit Git internals. Their packager writes this
    # pin record; assets/locales still come from the documented Git checkouts.
    in_repo = subprocess.run(['git', '-C', str(ROOT), 'ls-files', '--error-unmatch', 'package.json'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    snapshot = None if in_repo else json.loads((ROOT / 'handheld/source-pin.json').read_text(encoding='utf8'))
    pins = {}
    for folder in ('assets', 'locales'):
        expected = snapshot['submodules'][folder] if snapshot else git('ls-tree', 'HEAD', folder).split()[2]
        actual = git('-C', str(ROOT / folder), 'rev-parse', 'HEAD')
        if actual != expected:
            raise SystemExit(f'{folder} is {actual}, but the official source pins {expected}; update submodules before packaging.')
        clean = subprocess.run(['git', '-C', str(ROOT / folder), 'diff', '--quiet', 'HEAD', '--']).returncode
        if clean:
            raise SystemExit(f'{folder} has tracked edits; package the pinned official assets/locales.')
        pins[folder] = actual
    overlay = {}
    for name in ('src/main.ts', '.env.native', 'handheld/vite.config.ts'):
        overlay[name] = hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
    source_commit = snapshot.get('sourceCommit', snapshot['officialCommit']) if snapshot else git('rev-parse', 'HEAD')
    upstream_path = ROOT / 'handheld/upstream.json'
    official_commit = json.loads(upstream_path.read_text(encoding='utf8'))['officialCommit'] if upstream_path.exists() else source_commit
    return {'officialCommit': official_commit, 'sourceCommit': source_commit, 'submodules': pins,
            'officialVersion': json.loads((ROOT / 'package.json').read_text())['version'],
            'buildInputSha256': overlay,
            'note': 'Official source with handheld build adapter; source snapshot accompanies the executable.'}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if not (ROOT / 'dist/asset-manifest.json').is_file():
        raise SystemExit('Build the native Vite configuration first.')
    provenance = build_provenance()
    files = {}
    for folder, prefix in [('assets', ''), ('locales', 'locales/')]:
        for name in tracked(folder):
            if name.startswith(('.', 'test/', 'scripts/')) or name in {
                'package.json', 'pnpm-lock.yaml', 'biome.jsonc', 'lefthook.yml', 'tsconfig.json'
            }:
                continue
            path = ROOT / folder / name
            if path.is_file():
                files[prefix + name] = path
    for path in (ROOT / 'dist').rglob('*'):
        if path.is_file() and not path.name.endswith('.map'):
            files[path.relative_to(ROOT / 'dist').as_posix()] = path
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, 'w', allowZip64=True) as archive:
        archive.writestr('handheld-build.json', json.dumps(provenance, indent=2), compress_type=zipfile.ZIP_DEFLATED)
        for name, path in sorted(files.items()):
            method = zipfile.ZIP_DEFLATED if path.suffix.lower() in {
                '.json', '.txt', '.md', '.xml', '.svg', '.csv', '.js', '.css', '.html'
            } else zipfile.ZIP_STORED
            archive.write(path, name, compress_type=method)
    print(f'{len(files)} entries, {args.output.stat().st_size} bytes: {args.output}')

if __name__ == '__main__':
    main()
