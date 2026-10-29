"""SDK-only APK packaging for the native runtime. Outputs stay in build-android
until hardware smoke tests pass; this is not a release or an online client.
"""
from pathlib import Path
import os, subprocess, zipfile, shutil, json, hashlib, xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get('ANDROID_HOME',str(Path.home()/'AppData/Local/Android/Sdk')))
if not os.environ.get('JAVA_HOME'): raise SystemExit('Set JAVA_HOME to your JDK 21 installation.')
java = Path(os.environ['JAVA_HOME'])
tools = sdk/'build-tools/36.0.0'
build = root/'build-android/apk'
build.mkdir(parents=True,exist_ok=True)
official = Path(os.environ.get('POKEROGUE_GAME_DIR', str(root.parent/'game')))
version = ET.parse(root/'android/AndroidManifest.xml').getroot().attrib['{http://schemas.android.com/apk/res/android}versionName'].removesuffix('-native-test')
env = os.environ.copy();env['JAVA_HOME']=str(java);env['PATH']=str(java/'bin')+os.pathsep+env['PATH']

def run(name,args):
    with (build/(name+'.log')).open('w',encoding='utf8') as log:
        p=subprocess.run([str(a) for a in args],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180,
            creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name=='nt' else 0)
    if p.returncode:raise RuntimeError((build/(name+'.log')).read_text(errors='replace')[-5000:])
    print(name,'OK',flush=True)

res=build/'res/drawable';res.mkdir(parents=True,exist_ok=True)
shutil.copyfile(official/'assets/logo128.png',res/'icon.png')
run('resources',[tools/'aapt2.exe','compile','--dir',build/'res','-o',build/'resources.zip'])
androidjar=sdk/'platforms/android-35/android.jar'
run('link',[tools/'aapt2.exe','link','-o',build/'base.apk','--manifest',root/'android/AndroidManifest.xml','-I',androidjar,'-R',build/'resources.zip','--auto-add-overlay'])
classes=build/'classes';classes.mkdir(exist_ok=True)
# Keep the upstream SDL Java sources intact. Apply the native parser's required
# stack size to a generated copy, with an exact-match assertion.
sources=[]
for path in (root/'third_party/SDL/android-project/app/src/main/java/org/libsdl/app').glob('*.java'):
    target=build/'java/org/libsdl/app'/path.name;target.parent.mkdir(parents=True,exist_ok=True)
    text=path.read_text(encoding='utf8')
    if path.name=='SDLActivity.java':
        old='new Thread(new SDLMain(), "SDLThread")'
        assert text.count(old)==1
        text=text.replace(old,'new Thread(null, new SDLMain(), "SDLThread", 8 * 1024 * 1024)')
    target.write_text(text,encoding='utf8');sources.append(target)
sources.extend((root/'android').glob('*.java'))
run('java',[java/'bin/javac.exe','-J-Xmx512m','-source','17','-target','17','-encoding','UTF-8','-classpath',androidjar,'-d',classes,*sources])
with zipfile.ZipFile(build/'classes.jar','w') as z:
    for p in classes.rglob('*.class'):z.write(p,p.relative_to(classes).as_posix())
run('dex',[java/'bin/java.exe','-Xmx512m','-cp',tools/'lib/d8.jar','com.android.tools.r8.D8','--min-api','26','--lib',androidjar,'--output',build,build/'classes.jar'])
shutil.copyfile(build/'base.apk',build/'unsigned.apk')
with zipfile.ZipFile(build/'unsigned.apk','a',compression=zipfile.ZIP_DEFLATED) as z:
    z.write(build/'classes.dex','classes.dex')
    z.write(root/'build-android/runtime/libmain.so','lib/arm64-v8a/libmain.so')
    z.write(root/'build-android/runtime/third_party/SDL/libSDL2.so','lib/arm64-v8a/libSDL2.so')
    z.write(root/'.cache/android/game.zip','assets/game.zip',compress_type=zipfile.ZIP_STORED)
    for path in (root/'js').glob('*.js'):z.write(path,'assets/runtime/js/'+path.name)
    z.write(root/'third_party/cacert.pem','assets/cacert.pem')
    z.writestr('assets/config.txt','audio=1\nlog=1\nprofile=1\nosk=1\n')
run('align',[tools/'zipalign.exe','-f','-p','4',build/'unsigned.apk',build/'aligned.apk'])
key=Path(os.environ.get('ANDROID_TEST_KEYSTORE', str(official/'handheld/build/handheld-test.keystore')))
key.parent.mkdir(parents=True,exist_ok=True)
if not key.exists():run('key',[java/'bin/keytool.exe','-genkeypair','-keystore',key,'-storepass','android','-keypass','android','-alias','androiddebugkey','-keyalg','RSA','-keysize','2048','-validity','10000','-dname','CN=PokeRogue NX Local Test'])
apk=build/f'PokeRogue-Native-Test-{version}.apk'
run('sign',[java/'bin/java.exe','-Xmx512m','-jar',tools/'lib/apksigner.jar','sign','--ks',key,'--ks-key-alias','androiddebugkey','--ks-pass','pass:android','--key-pass','pass:android','--out',apk,build/'aligned.apk'])
run('verify',[java/'bin/java.exe','-jar',tools/'lib/apksigner.jar','verify','--verbose',apk])
with apk.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
print(json.dumps({'apk':str(apk),'bytes':apk.stat().st_size,'sha256':digest,'hardwareTested':False}),flush=True)
