from pathlib import Path
import hashlib, json, zipfile, shutil

out = Path(__file__).resolve().parent
root = out.parents[1]
source = root / '03_Firmware_ESP32_S3' / 'balancebot'
build = root / '.build' / 'phone-web'
sha = lambda b: hashlib.sha256(b).hexdigest()
with zipfile.ZipFile(out / 'before-source.zip') as z:
    changed = [n for n in z.namelist() if not n.endswith('/') and z.read(n) != (source / Path(n).name).read_bytes()]
    assert changed == ['Runtime.cpp'], changed
print('PASS: all pre-existing sketch files unchanged except Runtime.cpp (HTTP integration)')
for file in source.iterdir():
    if file.suffix in ('.cpp', '.h'):
        compiled = build / 'sketch' / file.name
        text = compiled.read_text(encoding='utf-8').splitlines()
        if text and text[0].startswith('#line '): text = text[1:]
        assert '\n'.join(text).strip() == file.read_text(encoding='utf-8').strip(), file.name
print('PASS: build/sketch matches final C++/header sources')
files = sorted(p for p in source.iterdir() if p.is_file())
with zipfile.ZipFile(out / 'balancebot-phone-source.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for file in files: z.write(file, 'balancebot/' + file.name)
with zipfile.ZipFile(out / 'balancebot-phone-source.zip') as z:
    for file in files: assert z.read('balancebot/' + file.name) == file.read_bytes()
inputs = ['tests/cpp/phone_web_tests.cpp', 'tests/cpp/standalone_tests.cpp',
          'tests/cpp/imu_calibration_tests.cpp', 'tests/cpp/imu_storage_tests.cpp',
          'tools/build_phone_host.ps1', '4_Web/tests/phone_direct.test.js']
inputs += [str(p.relative_to(root)).replace('\\','/') for p in (root/'tests/mocks').rglob('*') if p.is_file()]
with zipfile.ZipFile(out / 'verification-inputs.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for name in inputs: z.write(root / name, name)
with zipfile.ZipFile(out / 'verification-inputs.zip') as z:
    for name in inputs: assert z.read(name) == (root / name).read_bytes()
shutil.copyfile(build/'balancebot.ino.bin', out/'balancebot.ino.bin')
assert (build/'balancebot.ino.bin').read_bytes() == (out/'balancebot.ino.bin').read_bytes()
manifest = {'source':{p.name:sha(p.read_bytes()) for p in files},
            'verification_inputs':{n:sha((root/n).read_bytes()) for n in inputs},
            'artifacts':{p.name:sha(p.read_bytes()) for p in out.iterdir()
                         if p.is_file() and p.name not in ('SHA256.json','bundle-check.log')}}
(out/'SHA256.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('PASS: source/verification ZIP readback and binary copy; hashes saved in SHA256.json')
