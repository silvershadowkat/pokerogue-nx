#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Exercise production mode selection/storage/VFS across separate native processes.

Linux/Xvfb test with synthetic cheat/save markers, never real account data. Online
uses a loopback HTTP fixture. No alternate production mode or network permission
is introduced. Pass a built native runtime with --runtime.
"""
import argparse
import functools
import http.server
import json
import os
from pathlib import Path
import signal
import subprocess
import threading
import time
import zipfile

PROBE = r'''
const N=globalThis.__native, rt=globalThis.__rt;
const assert=(value,why)=>{if(!value)throw new Error(why);};
async function check(){
 const mode=N.clientMode;
 assert(N.dataDir.endsWith('/profiles/'+mode),'wrong native profile');
 if(mode==='debug'){
  await import('./debug-only.js');
  const old=localStorage.getItem('debugSettings');
  if(old)assert(JSON.parse(old).infiniteHP===1,'Debug settings lost on return');
  localStorage.setItem('debugSettings',JSON.stringify({infiniteHP:1,ohko:1}));
  localStorage.setItem('sessionData_Guest','DEBUG_EDITED_PARTY');
  document.cookie='debug_cookie=DEBUG_COOKIE';
  assert(globalThis.__cheatSentinel===true,'Debug fixture did not execute');
  assert(globalThis.__debugBootstrapSentinel===true,'Debug-owned bootstrap not exercised');
  let denied=false;
  try{await fetch('http://127.0.0.1:1/savedata/updateall',{method:'POST',body:'DEBUG_EDITED_PARTY'});}
  catch(e){denied=/disabled in offline mode/.test(String(e));}
  assert(denied,'Debug could reach an account endpoint');
 }else{
  assert(globalThis.__cheatSentinel===undefined,'Debug global leaked');
  assert(globalThis.__debugBootstrapSentinel===undefined,'Debug bootstrap leaked');
  assert(localStorage.getItem('debugSettings')===null,'Debug settings leaked');
  assert(localStorage.getItem('sessionData_Guest')===null,'Debug run leaked');
  assert(!document.cookie.includes('DEBUG_COOKIE'),'Debug cookie leaked');
  assert((await fetch('./debug-only.js')).status===404,'Debug code leaked through VFS/cache');
  if(mode==='online')assert(N.allowSaveImport===false,'Online import was enabled');
 }
 rt.flushStorage();
 console.log('[boundary] PASS '+mode);
}
check().catch(e=>console.error('[boundary] FAIL '+e));
'''


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *_):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path,
                        help='A new test-owned output directory; never use real player data.')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    app = out/'app'
    (app/'games').mkdir(parents=True)
    www = out/'online'
    www.mkdir()
    entry = '<html><body><script type="module" src="mode-probe.js"></script></body></html>'
    (www/'index.html').write_text(entry)
    (www/'mode-probe.js').write_text(PROBE)
    for debug, path in ((False, app/'games/test-base.zip'), (True, app/'debug-game.zip')):
        with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
            z.writestr('index.html', entry)
            z.writestr('mode-probe.js', PROBE)
            if debug:
                z.writestr('debug-only.js', 'globalThis.__cheatSentinel=true;')
                z.writestr('pokerogue-debug.json', json.dumps({
                    'schema':'pokerogue-debug-v1', 'version':'test-debug', 'baseVersion':'test-base'}))
    (app/'game-current.txt').write_text('test-base')
    profile = app/'profiles/debug'
    profile.mkdir(parents=True)
    (profile/'debug.js').write_text('globalThis.__debugBootstrapSentinel=true;')
    (app/'config.txt').write_text('profile=0\nlog=1\n')
    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0),
        functools.partial(QuietHandler, directory=str(www)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    results = []
    try:
        for index, mode in enumerate(('debug', 'online', 'offline', 'debug')):
            (app/'relaunch.txt').write_text(mode)
            control = out/'control.txt'
            control.write_text('')
            log = app/'log.txt'
            if log.exists():
                log.unlink()
            command = ['xvfb-run','-a','-s','-screen 0 960x544x24',str(args.runtime.resolve()),
                       '--js',str(root/'runtime/js'),'--data',str(app),'--control',str(control),
                       '--url',f'http://127.0.0.1:{server.server_port}/']
            env = {**os.environ, 'SDL_AUDIODRIVER':'dummy', 'LIBGL_ALWAYS_SOFTWARE':'1',
                   'LP_NUM_THREADS':'2', 'OMP_NUM_THREADS':'2'}
            with (out/f'{index}-{mode}-stdout.log').open('wb') as output:
                proc = subprocess.Popen(command, env=env, stdout=output, stderr=subprocess.STDOUT,
                                        start_new_session=True)
                try:
                    deadline = time.monotonic()+30
                    content = ''
                    while proc.poll() is None and time.monotonic()<deadline:
                        content = log.read_text(errors='replace') if log.exists() else ''
                        if '[boundary] PASS' in content or '[boundary] FAIL' in content:
                            break
                        time.sleep(.1)
                    assert f'[boundary] PASS {mode}' in content, content[-4000:]
                    assert '[boundary] FAIL' not in content and '[js] unhandled' not in content, content[-4000:]
                    results.append(mode)
                    print(f'PASS native {mode}: separate save/settings/cookies, JS realm and code source', flush=True)
                finally:
                    control.write_text('quit\n')
                    try:
                        proc.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        os.killpg(proc.pid, signal.SIGTERM)
                        proc.wait(timeout=5)
                (out/f'{index}-{mode}-runtime.log').write_text(content)
        debug = json.loads((profile/'localStorage-offline.json').read_text())
        assert json.loads(debug['debugSettings']) == {'infiniteHP':1,'ohko':1}
        assert debug['sessionData_Guest'] == 'DEBUG_EDITED_PARTY'
        (out/'results.json').write_text(json.dumps({'passed':results,'officialAccountsUsed':False},indent=2))
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


if __name__ == '__main__':
    main()
