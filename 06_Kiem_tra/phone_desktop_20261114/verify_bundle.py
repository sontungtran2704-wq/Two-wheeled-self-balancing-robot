from pathlib import Path
import gzip, hashlib, json, shutil, zipfile

out = Path(__file__).resolve().parent
root = out.parents[1]
source = root/'03_Firmware_ESP32_S3/balancebot'
build = root/'.build/phone-desktop'
sha = lambda b: hashlib.sha256(b).hexdigest()
expected = {'ControlCore.cpp','ControlCore.h','Runtime.cpp','PhoneWeb.h','PhonePage.h','PHONE_WEB.md','README.md'}
with zipfile.ZipFile(out/'before-source.zip') as z:
    changed = {Path(n).name for n in z.namelist() if not n.endswith('/') and z.read(n)!=(source/Path(n).name).read_bytes()}
    assert changed == expected, changed
print('PASS: only declared firmware files changed; generated config/model, pins/directions, IMU/calibration/NVS and stepper files remain byte-identical')
for p in source.iterdir():
    if p.suffix in ('.cpp','.h'):
        lines=(build/'sketch'/p.name).read_text(encoding='utf-8').splitlines()
        if lines and lines[0].startswith('#line '): lines=lines[1:]
        assert '\n'.join(lines).strip()==p.read_text(encoding='utf-8').strip(),p.name
print('PASS: actual target build C++/headers match final source')
page_meta=json.loads((root/'.build/phone-page-inputs.json').read_text())
for name,digest in page_meta['inputs'].items(): assert sha((root/name).read_bytes())==digest,name
page=(root/'.build/phone-page.html.gz').read_bytes()
assert sha(gzip.decompress(page))==page_meta['page_sha256']
shutil.copyfile(root/'.build/phone-page-inputs.json',out/'phone-page-inputs.json')
shutil.copyfile(root/'.build/phone-page.html.gz',out/'phone-page.html.gz')
print('PASS: shared desktop asset/model hashes and gzip readback')
source_files=sorted(p for p in source.iterdir() if p.is_file())
with zipfile.ZipFile(out/'balancebot-phone-desktop-source.zip','w',zipfile.ZIP_DEFLATED) as z:
    for p in source_files:z.write(p,'balancebot/'+p.name)
with zipfile.ZipFile(out/'balancebot-phone-desktop-source.zip') as z:
    for p in source_files:assert z.read('balancebot/'+p.name)==p.read_bytes()
inputs=set(page_meta['inputs']) | {
    'tools/build_phone_host.ps1','tools/generate_phone_page.py',
    'tests/cpp/phone_web_tests.cpp','tests/cpp/phone_lqr_tests.cpp','tests/cpp/standalone_tests.cpp',
    'tests/cpp/imu_calibration_tests.cpp','tests/cpp/imu_storage_tests.cpp',
    '4_Web/tests/api.test.js','4_Web/tests/browser.test.js','4_Web/tests/phone_direct.test.js',
    '4_Web/web_server.js','4_Web/auth.js','4_Web/lqr.js','4_Web/package.json','4_Web/package-lock.json'}
inputs.update(str(p.relative_to(root)).replace('\\','/') for p in (root/'tests/mocks').rglob('*') if p.is_file())
with zipfile.ZipFile(out/'verification-inputs.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name in sorted(inputs):z.write(root/name,name)
with zipfile.ZipFile(out/'verification-inputs.zip') as z:
    for name in inputs:assert z.read(name)==(root/name).read_bytes()
shutil.copyfile(build/'balancebot.ino.bin',out/'balancebot.ino.bin')
assert (build/'balancebot.ino.bin').read_bytes()==(out/'balancebot.ino.bin').read_bytes()
manifest={'source':{p.name:sha(p.read_bytes()) for p in source_files},
          'verification_inputs':{n:sha((root/n).read_bytes()) for n in sorted(inputs)},
          'artifacts':{p.name:sha(p.read_bytes()) for p in out.iterdir() if p.is_file() and p.name not in ('SHA256.json','bundle-check.log')}}
(out/'SHA256.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('PASS: source/test ZIP readback, binary copy and SHA256.json')
