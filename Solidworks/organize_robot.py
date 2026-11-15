from pathlib import Path
import win32com.client as w,pythoncom as c
p=Path('Solidworks/part_robot').resolve()
s=w.Dispatch('SldWorks.Application')
def g(o,n):
 v=getattr(o,n)
 return v() if callable(v) else v
docs=[m for m in (g(s,'GetDocuments') or []) if Path(g(m,'GetPathName') or '.').parent==p]
for m in docs:
 if g(m,'GetSaveFlag'):
  def r():return w.VARIANT(c.VT_BYREF|c.VT_I4,0)
  assert m.Save3(1,r(),r())
titles=[g(m,'GetTitle') for m in docs if str(g(m,'GetPathName')).lower().endswith('.sldasm')]
titles += [g(m,'GetTitle') for m in docs if not str(g(m,'GetPathName')).lower().endswith('.sldasm')]
for t in titles:s.CloseDoc(t)
a=p/'Lap_rap'; b=p/'Chi_tiet'
for d in (a,b): d.mkdir(exist_ok=True)
files=[f for f in p.iterdir() if f.is_file() and not f.name.startswith('~$') and f.suffix.lower() in ('.sldprt','.step')]
asm=p/'Robot_lap_so_bo.SLDASM'
target=a/asm.name
assert asm.exists() and not target.exists()
import shutil
for f in files:
 if not (b/f.name).exists():
  try:f.rename(b/f.name)
  except PermissionError:shutil.copy2(f,b/f.name)
files=list(b.glob("*.SLDPRT"))
asm.rename(target)
for f in files:
 if f.suffix.lower()=='.sldprt':s.ReplaceReferencedDocument(str(target),str(p/f.name),str(f))
def r():return w.VARIANT(c.VT_BYREF|c.VT_I4,0)
m=s.OpenDoc6(str(target),2,1,'',r(),r())
assert m is not None
parts=m.GetComponents(True)
assert len(parts)==46 and all(Path(g(k,'GetPathName')).parent==b and Path(g(k,'GetPathName')).is_file() for k in parts)
assert m.Save3(1,r(),r())
print('Verified',len(parts),'components',len(files),'files',target)

