"""Fetch Android-only pins without modifying an existing dependency checkout."""
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[1]/'third_party'
pins=[
 ('SDL','https://github.com/libsdl-org/SDL.git','release-2.32.10','5d249570393f7a37e037abf22cd6012a4cc56a71'),
 ('mbedtls','https://github.com/Mbed-TLS/mbedtls.git','v2.28.10','2fc8413bfcb51354c8e679141b17b3f1a5942561'),
 ('curl','https://github.com/curl/curl.git','curl-8_10_1','7eb8c048470ed2cc14dca75be9c1cdae7ac8498b'),
]
for name,url,tag,commit in pins:
    path=root/name
    if not path.exists():
        subprocess.run(['git','-c','advice.detachedHead=false','clone','--depth','1','--branch',tag,url,str(path)],check=True,timeout=180)
    actual=subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()
    if actual!=commit:raise RuntimeError(f'{name}: existing checkout differs from expected pin; left unchanged')
    print(name,actual)
