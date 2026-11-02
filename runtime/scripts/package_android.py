"""Package/sign the native client without game files or debug scripts.
Outputs stay in build-android until staged with device-test evidence.
"""
from pathlib import Path
import os, subprocess, zipfile, shutil, json, hashlib, secrets, xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get('ANDROID_HOME',str(Path.home()/'AppData/Local/Android/Sdk')))
if not os.environ.get('JAVA_HOME'): raise SystemExit('Set JAVA_HOME to your JDK 21 installation.')
java = Path(os.environ['JAVA_HOME'])
tools = sdk/'build-tools/36.0.0'
build = root/'build-android/apk'
build.mkdir(parents=True,exist_ok=True)
version = ET.parse(root/'android/AndroidManifest.xml').getroot().attrib['{http://schemas.android.com/apk/res/android}versionName'].removesuffix('-native-test')
env = os.environ.copy();env['JAVA_HOME']=str(java);env['PATH']=str(java/'bin')+os.pathsep+env['PATH']

def run(name,args):
    with (build/(name+'.log')).open('w',encoding='utf8') as log:
        p=subprocess.run([str(a) for a in args],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180,
            creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name=='nt' else 0)
    if p.returncode:raise RuntimeError((build/(name+'.log')).read_text(errors='replace')[-5000:])
    print(name,'OK',flush=True)

res=build/'res/drawable';res.mkdir(parents=True,exist_ok=True)
# A code-defined app mark, not a game asset or generated bitmap artwork.
(res/'icon.xml').write_text('''<vector xmlns:android="http://schemas.android.com/apk/res/android" android:width="108dp" android:height="108dp" android:viewportWidth="108" android:viewportHeight="108">
<path android:fillColor="#111827" android:pathData="M0,0h108v108h-108z"/>
<path android:fillColor="#426C91" android:pathData="M22,18h64v72h-64z"/>
<path android:fillColor="#F1F5F9" android:pathData="M38,32h23c13,0 19,7 19,19s-7,19 -19,19h-10v13h-13zM51,44v14h10c4,0 6,-2 6,-7s-2,-7 -6,-7z"/>
</vector>''',encoding='utf8')
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
    for path in (root/'js').glob('*.js'):
        if path.name!='debug.js':z.write(path,'assets/runtime/js/'+path.name)
    z.write(root/'third_party/cacert.pem','assets/cacert.pem')
    z.writestr('assets/config.txt','audio=1\nlog=1\nosk=1\ndirecttouch=1\ntouchpad=1\n')
run('align',[tools/'zipalign.exe','-f','-p','4',build/'unsigned.apk',build/'aligned.apk'])
signing=Path(os.environ.get('LOCALAPPDATA',str(Path.home())))/'PokeRogueNX/signing'
signing.mkdir(parents=True,exist_ok=True)
key=signing/'pokerogue-nx.jks'
password_file=signing/'keystore-password.txt'
if not password_file.exists():password_file.write_text(secrets.token_urlsafe(32),encoding='utf8')
env['PKR_SIGNING_PASSWORD']=password_file.read_text(encoding='utf8').strip()
key.parent.mkdir(parents=True,exist_ok=True)
if not key.exists():run('key',[java/'bin/keytool.exe','-genkeypair','-keystore',key,'-storepass:env','PKR_SIGNING_PASSWORD','-keypass:env','PKR_SIGNING_PASSWORD','-alias','pokeroguenx','-keyalg','RSA','-keysize','3072','-validity','10000','-dname','CN=SilverShadow PokeRogue NX'])
apk=build/f'PokeRogue-NX-{version}-android-arm64.apk'
run('sign',[java/'bin/java.exe','-Xmx512m','-jar',tools/'lib/apksigner.jar','sign','--ks',key,'--ks-key-alias','pokeroguenx','--ks-pass','env:PKR_SIGNING_PASSWORD','--key-pass','env:PKR_SIGNING_PASSWORD','--out',apk,build/'aligned.apk'])
run('verify',[java/'bin/java.exe','-jar',tools/'lib/apksigner.jar','verify','--verbose',apk])
with zipfile.ZipFile(apk) as z:
    assert 'assets/game.zip' not in z.namelist()
    assert 'assets/runtime/js/debug.js' not in z.namelist()
with apk.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
print(json.dumps({'apk':str(apk),'bytes':apk.stat().st_size,'sha256':digest,'hardwareTested':False}),flush=True)
