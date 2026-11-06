"""Package/sign the native client without game files or debug scripts.
Outputs stay in build-android until staged with device-test evidence.
"""
from pathlib import Path
import os, subprocess, zipfile, shutil, json, hashlib, argparse, xml.etree.ElementTree as ET

parser=argparse.ArgumentParser()
parser.add_argument('--test-signing',action='store_true',help='Local test key only; not an upgrade for public APKs.')
parser.add_argument('--test-staging',action='store_true',help='Temporary debuggable package for test content installation.')
args=parser.parse_args()

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get('ANDROID_HOME',str(Path.home()/'AppData/Local/Android/Sdk')))
if not os.environ.get('JAVA_HOME'): raise SystemExit('Set JAVA_HOME to your JDK 21 installation.')
java = Path(os.environ['JAVA_HOME'])
tools = sdk/'build-tools/36.0.0'
native_build = Path(os.environ.get('PKR_ANDROID_BUILD_DIR', str(root/'build-android'))).resolve()
build = native_build/'apk'
build.mkdir(parents=True,exist_ok=True)
version = ET.parse(root/'android/AndroidManifest.xml').getroot().attrib['{http://schemas.android.com/apk/res/android}versionName'].removesuffix('-native-test')
signing=Path(os.environ.get('PKR_SIGNING_DIR',str(Path.home()/'Documents/Codex/Secrets/PokeRogue')))
if args.test_signing:signing=Path(os.environ.get('LOCALAPPDATA',str(Path.home())))/'PokeRogueNX/signing/test-20261105'
key=signing/('pokerogue-nx.jks' if args.test_signing else 'pokerogue-android-release.jks')
password_file=signing/'keystore-password.txt'
if not key.is_file() or not password_file.is_file():
    raise SystemExit('Signing key/password missing. Restore the existing key; this script never creates a replacement.')
manifest=root/'android/AndroidManifest.xml'
if args.test_staging:
    text=manifest.read_text(encoding='utf8')
    assert text.count('android:debuggable="false"')==1
    manifest=build/'AndroidManifest-staging.xml'
    manifest.write_text(text.replace('android:debuggable="false"','android:debuggable="true"'),encoding='utf8')
env = os.environ.copy();env['JAVA_HOME']=str(java);env['PATH']=str(java/'bin')+os.pathsep+env['PATH']
env['PKR_SIGNING_PASSWORD']=password_file.read_text(encoding='utf8').strip()
expected_cert=json.loads((root/'android/release-signing.json').read_text(encoding='utf8'))['certificateSha256']
if not args.test_signing:
    cert=subprocess.check_output([str(java/'bin/keytool.exe'),'-exportcert','-keystore',str(key),'-storepass:env','PKR_SIGNING_PASSWORD','-alias','pokeroguenx'],env=env,timeout=30)
    if hashlib.sha256(cert).hexdigest()!=expected_cert:raise SystemExit('Wrong release signing key. Packaging stopped before changing any APK.')

def run(name,args):
    with (build/(name+'.log')).open('w',encoding='utf8') as log:
        p=subprocess.run([str(a) for a in args],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180,
            creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name=='nt' else 0)
    if p.returncode:raise RuntimeError((build/(name+'.log')).read_text(errors='replace')[-5000:])
    print(name,'OK',flush=True)

res=build/'res/drawable';res.mkdir(parents=True,exist_ok=True)
# Reuse the maintainer's existing P/R/SS icon without generating artwork.
legacy_icon=res/'icon.xml'
if legacy_icon.exists():legacy_icon.unlink()
shutil.copyfile(root/'branding/icon.png',res/'icon.png')
run('resources',[tools/'aapt2.exe','compile','--dir',build/'res','-o',build/'resources.zip'])
androidjar=sdk/'platforms/android-35/android.jar'
run('link',[tools/'aapt2.exe','link','-o',build/'base.apk','--manifest',manifest,'-I',androidjar,'-R',build/'resources.zip','--auto-add-overlay'])
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
    z.write(native_build/'runtime/libmain.so','lib/arm64-v8a/libmain.so')
    z.write(native_build/'runtime/third_party/SDL/libSDL2.so','lib/arm64-v8a/libSDL2.so')
    for path in (root/'js').glob('*.js'):
        if path.name!='debug.js':z.write(path,'assets/runtime/js/'+path.name)
    z.write(root/'third_party/cacert.pem','assets/cacert.pem')
    z.writestr('assets/config.txt','audio=1\nlog=1\nosk=1\ndirecttouch=0\ntouchpad=1\n')
run('align',[tools/'zipalign.exe','-f','-p','4',build/'unsigned.apk',build/'aligned.apk'])
env['PKR_SIGNING_PASSWORD']=password_file.read_text(encoding='utf8').strip()
suffix=('-test-signed' if args.test_signing else '')+('-staging' if args.test_staging else '')
apk=build/f'PokeRogue-{version}-android-arm64{suffix}.apk'
run('sign',[java/'bin/java.exe','-Xmx512m','-jar',tools/'lib/apksigner.jar','sign','--ks',key,'--ks-key-alias','pokeroguenx','--ks-pass','env:PKR_SIGNING_PASSWORD','--key-pass','env:PKR_SIGNING_PASSWORD','--out',apk,build/'aligned.apk'])
run('verify',[java/'bin/java.exe','-jar',tools/'lib/apksigner.jar','verify','--verbose',apk])
certs=subprocess.check_output([str(java/'bin/java.exe'),'-jar',str(tools/'lib/apksigner.jar'),'verify','--print-certs',str(apk)]).decode('utf8')
if not args.test_signing and expected_cert not in certs:
    raise SystemExit('Certificate differs from the pinned release identity. Do not distribute this APK.')
with zipfile.ZipFile(apk) as z:
    assert 'assets/game.zip' not in z.namelist()
    assert 'assets/runtime/js/debug.js' not in z.namelist()
with apk.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
print(json.dumps({'apk':str(apk),'bytes':apk.stat().st_size,'sha256':digest,'hardwareTested':False,'releaseSigning':not args.test_signing,'stagingOnly':args.test_staging}),flush=True)
