from pathlib import Path
import json,hashlib,zipfile,shutil
root=Path(__file__).resolve().parent
base=root/'Bo_part_robot_2026-11-13'
logs=root/'Nhat_ky_CAD_bo_sung_2026-11-14'
logs.mkdir(exist_ok=True)
for p in (base/'Kiem_tra').glob('*.bmp'):
    assert p.resolve().is_relative_to(base.resolve())
    dest=logs/p.name
    if not dest.exists(): shutil.move(str(p),str(dest))
files=[p for p in base.rglob('*') if p.is_file() and p.name!='MANIFEST.json' and not p.name.startswith('~$')]
manifest={'native_parts':sum(p.parent==base and p.suffix.upper()=='.SLDPRT' for p in files),'step_files':sum(p.parent==base and p.suffix.upper()=='.STEP' for p in files),'files':[{'path':p.relative_to(base).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(files)]}
assert manifest['native_parts']==manifest['step_files']==10
(base/'MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
out=root/'Bo_part_robot_2026-11-14_bo_sung.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
    for p in files+[base/'MANIFEST.json']: z.write(p,base.name+'/'+p.relative_to(base).as_posix())
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
    assert sum(n.endswith('.SLDPRT') for n in z.namelist())==10
print(json.dumps({'zip':str(out),'parts':10,'bytes':out.stat().st_size}))
