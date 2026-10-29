"""Reproducible SDK-only Android build. No cloud builds or Gradle daemons.
Requires Python 3, Node/pnpm, Android SDK 35+36 build tools, and JDK 21.
"""
from pathlib import Path
import os, subprocess, shutil, zipfile, hashlib, json, zlib, argparse
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'handheld/build'; BUILD.mkdir(parents=True,exist_ok=True)
SDK=Path(os.environ.get('ANDROID_HOME',str(Path.home()/'AppData/Local/Android/Sdk')))
if not os.environ.get('JAVA_HOME'): raise SystemExit('Set JAVA_HOME to your JDK 21 installation.')
JAVA=Path(os.environ['JAVA_HOME'])
TOOLS=SDK/'build-tools/36.0.0'
env=os.environ.copy();env['JAVA_HOME']=str(JAVA);env['PATH']=str(JAVA/'bin')+os.pathsep+env['PATH']
def run(name,args,timeout=180):
    with (BUILD/(name+'.log')).open('w',encoding='utf-8') as log:
        p=subprocess.Popen([str(x) for x in args],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,
            creationflags=(subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS) if os.name=='nt' else 0)
        print(name,'PID',p.pid,flush=True)
        try: code=p.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            if os.name=='nt':subprocess.run(['taskkill','/PID',str(p.pid),'/T','/F'],timeout=10,stdout=subprocess.DEVNULL)
            else:p.kill()
            raise
        if code: raise RuntimeError(name+': '+(BUILD/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-4000:])
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()
def pack():
    pin={'assets':git('-C','assets','rev-parse','HEAD'),'locales':git('-C','locales','rev-parse','HEAD'),'format':1}
    receipt=BUILD/'pack.json'
    if receipt.exists() and json.loads(receipt.read_text())['pin']==pin and (BUILD/'game.pak').exists():return json.loads(receipt.read_text())
    entries={}
    # Include every tracked runtime asset and every locale, excluding only
    # repository tooling, tests and CI. Licenses remain in the package.
    for folder,prefix in [('assets',''),('locales','locales/')]:
        for name in git('-C',folder,'ls-files').splitlines():
            if name.startswith(('.', 'test/', 'scripts/')) or name in {'package.json','pnpm-lock.yaml','biome.jsonc','lefthook.yml','tsconfig.json'}:continue
            p=ROOT/folder/name
            if p.is_file():entries[prefix+name]=p
    raw_total=0
    with (BUILD/'game.pak').open('wb') as out,(BUILD/'game.index').open('w',encoding='utf-8',newline='\n') as idx:
        for name,p in sorted(entries.items()):
            data=p.read_bytes();raw=len(data);raw_total+=raw
            compressed=p.suffix.lower() in {'.json','.txt','.md','.xml','.svg','.csv'}
            encoded=zlib.compress(data,6) if compressed else data
            if len(encoded)>=raw:compressed=False;encoded=data
            offset=out.tell();out.write(encoded)
            idx.write(f'{name}\t{offset}\t{len(encoded)}\t{raw}\t{int(compressed)}\n')
    result={'pin':pin,'entries':len(entries),'unpackedBytes':raw_total,'packedBytes':(BUILD/'game.pak').stat().st_size}
    receipt.write_text(json.dumps(result,indent=2));print('pack',result,flush=True);return result
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--skip-web',action='store_true');parser.add_argument('--output',type=Path,default=ROOT/'handheld/release');args=parser.parse_args()
    if not args.skip_web:run('web',['cmd','/c','pnpm','exec','vite','build','--mode','handheld','--config','handheld/vite.config.ts'],600)
    metadata=pack()
    res=BUILD/'res/drawable';res.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/'assets/logo128.png',res/'icon.png')
    run('resources',[TOOLS/'aapt2.exe','compile','--dir',BUILD/'res','-o',BUILD/'resources.zip'])
    androidjar=SDK/'platforms/android-35/android.jar'
    run('link',[TOOLS/'aapt2.exe','link','-o',BUILD/'base.apk','--manifest',ROOT/'handheld/android/AndroidManifest.xml','-I',androidjar,'-R',BUILD/'resources.zip','--auto-add-overlay'])
    classes=BUILD/'classes';classes.mkdir(exist_ok=True)
    for p in classes.rglob('*.class'):p.unlink()
    run('java',[JAVA/'bin/javac.exe','-J-Xmx512m','-source','17','-target','17','-encoding','UTF-8','-classpath',androidjar,'-d',classes,ROOT/'handheld/android/MainActivity.java'])
    with zipfile.ZipFile(BUILD/'classes.jar','w') as z:
        for p in classes.rglob('*.class'):z.write(p,p.relative_to(classes).as_posix())
    run('dex',[JAVA/'bin/java.exe','-Xmx512m','-cp',TOOLS/'lib/d8.jar','com.android.tools.r8.D8','--min-api','26','--lib',androidjar,'--output',BUILD,BUILD/'classes.jar'])
    shutil.copy2(BUILD/'base.apk',BUILD/'unsigned.apk')
    with zipfile.ZipFile(BUILD/'unsigned.apk','a',compression=zipfile.ZIP_DEFLATED) as z:
        z.write(BUILD/'classes.dex','classes.dex');z.write(BUILD/'game.pak','assets/game.pak',compress_type=zipfile.ZIP_STORED);z.write(BUILD/'game.index','assets/game.index')
        # Vite's JSON plugin also writes asset copies to dist. Those already
        # exist in the indexed pack; only generated web output is layered on top.
        packed={line.split('\t')[0] for line in (BUILD/'game.index').read_text(encoding='utf-8').splitlines()}
        for p in (ROOT/'dist').rglob('*'):
            if not p.is_file():continue
            name=p.relative_to(ROOT/'dist').as_posix()
            if name in packed or name.endswith('.map'):continue
            z.write(p,'assets/web/'+name)
    run('align',[TOOLS/'zipalign.exe','-f','-p','4',BUILD/'unsigned.apk',BUILD/'aligned.apk'])
    key=BUILD/'handheld-test.keystore'
    if not key.exists():run('key',[JAVA/'bin/keytool.exe','-genkeypair','-keystore',key,'-storepass','android','-keypass','android','-alias','androiddebugkey','-keyalg','RSA','-keysize','2048','-validity','10000','-dname','CN=PokeRogue Handheld Local Test'])
    args.output.mkdir(parents=True,exist_ok=True);apk=args.output/'PokeRogue-Handheld-0.1.0.apk'
    run('sign',[JAVA/'bin/java.exe','-Xmx512m','-jar',TOOLS/'lib/apksigner.jar','sign','--ks',key,'--ks-key-alias','androiddebugkey','--ks-pass','pass:android','--key-pass','pass:android','--out',apk,BUILD/'aligned.apk'])
    run('verify',[JAVA/'bin/java.exe','-jar',TOOLS/'lib/apksigner.jar','verify','--verbose',apk])
    sha=hashlib.file_digest(apk.open('rb'),'sha256').hexdigest()
    receipt={'applicationId':'com.silvershadow.pokerogue.handheld','version':'0.1.0','upstream':git('rev-parse','HEAD'),'assetPack':metadata,'sha256':sha,'bytes':apk.stat().st_size,'internetPermission':False}
    (args.output/'build-receipt.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt,indent=2),flush=True)
if __name__=='__main__':main()
