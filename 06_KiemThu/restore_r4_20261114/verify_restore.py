"""Readback check for the requested r4 restoration; no device/network access."""
import hashlib
import json
import pathlib
import re
import sys
import zipfile

root = pathlib.Path(__file__).resolve().parents[2]
qa = pathlib.Path(__file__).resolve().parent
source = root / '03_Firmware_ESP32S3/balancebot'
build = pathlib.Path(sys.argv[1])
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(qa / 'balancebot-r4-source.zip') == 'ca6dd9174fff4be768ca08107cb4abc67c3cd1d7f07687bb21573c8ad5d10a4a'
paths = []
with zipfile.ZipFile(qa / 'balancebot-r4-source.zip') as z:
    for n in z.namelist():
        p = source / pathlib.PurePosixPath(n).name
        assert p.read_bytes() == z.read(n), n
        paths.append(p)
        if p.suffix in ('.cpp', '.h'):
            compiled = (build / 'sketch' / p.name).read_text(encoding='utf-8-sig')
            compiled = re.sub(r'^#line[^\n]*\n', '', compiled, count=1)
            assert compiled == p.read_text(encoding='utf-8-sig'), 'Compiled source: ' + n
    assert len(paths) == 15 and len(list(source.iterdir())) == 15
with zipfile.ZipFile(qa / 'verification-inputs.zip') as z:
    for n in z.namelist():
        p = root / n
        assert p.read_bytes() == z.read(n), n
        paths.append(p)
binary = build / 'balancebot.ino.bin'
assert digest(binary) == 'b398397584f41ec657b774c1a4ee8e368ab524ad4346d362edc91f08c6144ead'
(qa / 'balancebot.ino.bin').write_bytes(binary.read_bytes())
assert digest(qa / 'balancebot.ino.bin') == digest(binary)
paths += [p for p in qa.iterdir() if p.is_file() and p.name not in ('SOURCE_SHA256.json', 'artifact-check.log')]
manifest = [{'path': p.relative_to(root).as_posix(), 'sha256': digest(p)} for p in sorted(set(paths))]
(qa / 'SOURCE_SHA256.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('PASS: all 15 sketch files match frozen r4 ZIP; compiled C++/headers match restored source.')
print('PASS: verification-input ZIP readback; newly built binary matches r4 SHA256 and copied readback.')
print('PASS: manifest written. No COM, upload, calibration command, START or motor operation.')
