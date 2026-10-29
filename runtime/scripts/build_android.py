"""Build the native Android candidate with the locally installed NDK, at -j2."""
from pathlib import Path
import os, subprocess

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get('ANDROID_HOME', str(Path.home() / 'AppData/Local/Android/Sdk')))
ndk = sdk / 'ndk/29.0.14206865'
cmake = 'cmake'
ninja = sdk / 'cmake/3.22.1/bin/ninja.exe'
build = root / 'build-android'
build.mkdir(exist_ok=True)
deps = build / 'deps'
common = ['-G','Ninja', f'-DCMAKE_MAKE_PROGRAM={ninja.as_posix()}',
          f'-DCMAKE_TOOLCHAIN_FILE={(ndk / "build/cmake/android.toolchain.cmake").as_posix()}',
          '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-26',
          '-DCMAKE_BUILD_TYPE=Release','-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
          '-DCMAKE_POLICY_VERSION_MINIMUM=3.5']

def run(name,args,timeout=600):
    with (build / (name+'.log')).open('w',encoding='utf8') as f:
        p = subprocess.Popen([str(x) for x in args],cwd=root,stdout=f,stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name=='nt' else 0)
        print(name,'PID',p.pid,flush=True)
        try:code=p.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            if os.name=='nt':subprocess.run(['taskkill','/PID',str(p.pid),'/T','/F'],stdout=subprocess.DEVNULL,timeout=15)
            else:p.kill()
            raise
    if code:raise RuntimeError((build/(name+'.log')).read_text(errors='replace')[-5000:])

if not (deps/'lib/libmbedtls.a').exists():
    run('configure-mbedtls',[cmake,'-S',root/'third_party/mbedtls','-B',build/'mbedtls',*common,
         f'-DCMAKE_INSTALL_PREFIX={deps.as_posix()}','-DENABLE_PROGRAMS=OFF','-DENABLE_TESTING=OFF',
         '-DUSE_SHARED_MBEDTLS_LIBRARY=OFF','-DUSE_STATIC_MBEDTLS_LIBRARY=ON'])
    run('build-mbedtls',[cmake,'--build',build/'mbedtls','-j2'])
    run('install-mbedtls',[cmake,'--install',build/'mbedtls'])
run('configure-runtime',[cmake,'-S',root,'-B',build/'runtime',*common,
    f'-DPKR_ANDROID_DEPS={deps.as_posix()}','-DPKR_COMMIT='+subprocess.check_output(['git','rev-parse','--short','HEAD'],cwd=root,text=True).strip()+'-android',
    '-DPKR_DEBUG=ON','-DPKR_OFFLINE_TEST=ON'])
run('build-runtime',[cmake,'--build',build/'runtime','-j2'],900)
print('Native Android libraries built.',flush=True)
