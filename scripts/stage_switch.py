"""Stage a copy-ready Switch folder. Existing saves are never copied or removed."""
from pathlib import Path
import argparse
import hashlib
import shutil

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--nro', type=Path, default=root / 'runtime/build-switch/pokerogue-native-test.nro')
parser.add_argument('--game', type=Path, help='Include game.zip for a first installation; omit for NRO-only updates')
args = parser.parse_args()
with args.nro.open('rb') as f:
    header = f.read(32)
if header[16:20] != b'NRO0':
    raise SystemExit('Input is not a Switch NRO')
target = args.output / 'switch/pokerogue-native-test'
target.mkdir(parents=True, exist_ok=True)
shutil.copy2(args.nro, target / 'pokerogue-native-test.nro')
if args.game:
    shutil.copy2(args.game, target / 'game.zip')
    # Exclusive creation preserves settings in an existing staging folder.
    config = target / 'config.txt'
    if not config.exists():
        config.write_text('local=game.zip\naudio=1\nlog=1\nprofile=1\nosk=1\n', encoding='utf8')
    with (target / 'game.zip').open('rb') as f:
        (target / 'game.zip.version').write_text('NX bundled game SHA256 ' + hashlib.file_digest(f, 'sha256').hexdigest() + '\n', encoding='utf8')
print(args.output.resolve())
print('Copy its switch folder to the SD root. Preserve existing save files.')
