"""Read sources without modifying originals; extract only relevant reference code."""
from pathlib import Path
import zipfile, hashlib, json, openpyxl
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'00_Nguon'; OUT.mkdir(exist_ok=True)
sources=[Path(r'C:\Users\USER\Downloads\Bang_do_thong_so_thiet_ke_lai_xe_NEMA17.xlsx'),Path(r'C:\Users\USER\Downloads\Two--wheeled-self-balancing-robot-main.zip'),Path(r'D:\TLHT\HTDKTD_Xe_Can_Banh.zip')]
manifest=[]
for i,p in enumerate(sources):
    manifest.append(dict(path=str(p),bytes=p.stat().st_size,sha256=hashlib.file_digest(p.open('rb'),'sha256').hexdigest()))
    if p.suffix=='.zip':
        with zipfile.ZipFile(p) as z:
            names=z.namelist()
            (OUT/f'zip_{i}_inventory.txt').write_text('\n'.join(names),encoding='utf-8')
            for n in names:
                rel='/'.join(n.split('/')[1:])
                keep=(i==1 and rel.startswith('LQR/') and n.endswith(('.m','.ino','.slx'))) or (i==2 and (rel.startswith('3_Firmware_ESP32S3/') or rel.startswith('4_Web/')) and '/node_modules/' not in rel and n.endswith(('.cpp','.h','.ino','.js','.json','.html','.css','.md','.bat')))
                if keep:
                    dest=OUT/('lqr_reference' if i==1 else 'integration_reference')/rel
                    assert dest.resolve().is_relative_to(OUT.resolve())
                    dest.parent.mkdir(parents=True,exist_ok=True); dest.write_bytes(z.read(n))
w=openpyxl.load_workbook(sources[0],data_only=False)
cells={s.title:{c.coordinate:c.value for row in s for c in row if c.value is not None} for s in w}
(OUT/'workbook_cells.json').write_text(json.dumps(cells,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'source_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
print('Extracted relevant sources; originals unchanged.')
