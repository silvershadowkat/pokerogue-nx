"""Stage a copy-ready client installation without bundling any game assets."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--nro', type=Path, default=root / 'runtime/build-switch/pokerogue-nx.nro')
args = parser.parse_args()
with args.nro.open('rb') as f:
    header = f.read(32)
if header[16:20] != b'NRO0':
    raise SystemExit('Input is not a Switch NRO')
target = args.output / 'switch/pokerogue-nx'
target.mkdir(parents=True, exist_ok=True)
nro = target / 'pokerogue-nx.nro'
shutil.copy2(args.nro, nro)
config = target / 'config.txt'
# A staging directory may be reused; never overwrite a user's settings or saves.
if not config.exists():
    config.write_text('audio=1\nlog=1\nosk=1\ndirecttouch=1\ntouchpad=0\n', encoding='utf8')
(args.output / 'READ-ME.txt').write_text('''PokeRogue NX 0.2.0-rc2 - Switch candidate
Copy the switch folder to the SD root. Do not delete existing saves.
Use full application mode (hold R while launching a game to enter hbmenu).
Launch switch/pokerogue-nx/pokerogue-nx.nro. Applet mode is not supported.

No game files ship here. Connect Wi-Fi for the initial menu assets.
Online loads the live official game; use your normal PokeRogue account.
Offline downloads game.zip on demand and reads it without extracting it.
Debug is a visible placeholder; cheats/custom modes are not in this candidate.

Tap menus and Touch pad can be enabled independently. Controllers always work.
Close and reopen to return to mode selection. Each mode has separate storage.
Do not copy old offline/debug saves into the online profile.

Manual checks needed: login, server save/reload, Daily Run, repeated offline
save/reopen, touch/controller transitions and performance on your Switch.
Report log.txt and log-prev.txt from switch/pokerogue-nx after a problem.
Review logs before sharing; do not share credentials/cookies/profile files.

Runtime foundation: https://github.com/krappaU/pokerogue-vita
Official game: https://github.com/pagefaultgames/pokerogue
Offline distribution: https://github.com/Admiral-Billy/pokerogue
AI assisted coding/testing. No AI-generated game assets were added.
''',encoding='utf8')
with nro.open('rb') as f:
    digest = hashlib.file_digest(f,'sha256').hexdigest()
manifest = {'version':'0.2.0-rc2','sourceCommit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
            'upstreamRuntime':subprocess.check_output(['git','rev-parse','upstream/main'],cwd=root,text=True).strip(),
            'files':[{'path':str(nro.relative_to(args.output)).replace('\\','/'),'bytes':nro.stat().st_size,'sha256':digest}],
            'bundledGame':False,'switchHardwareTested':False}
(args.output/'build-info.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print(args.output.resolve())
print('Copy its switch folder to the SD root. Preserve existing save files.')
