import sys,math
from pathlib import Path
import pythoncom as c,win32com.client as w
sys.path.insert(0,r'C:\Users\USER\.agents\skills\solidworks-automation\scripts')
from sw_session import SolidWorksSession
from sw_part import sketch,sketch_circle,sketch_polygon,sketch_rectangle,extrude_boss,extrude_cut
from sw_connect import mm
from sw_sketch_quality import fully_define_sketch
s=SolidWorksSession()
out=Path('Solidworks/part_robot').resolve()
def profile(m,plane,fn):
 with sketch(m,plane) as sk:
  m.SketchManager.AddToDB=True
  assert fn() is not None
  m.SketchManager.AddToDB=False
 return sk
def save(m,n,rgb,bodies=1):
 assert m.ForceRebuild3(False)
 assert len(m.GetBodies2(0,False))==bodies
 m.MaterialPropertyValues=w.VARIANT(c.VT_ARRAY|c.VT_R8,[*rgb,.45,.75,.18,.35,0.,0.])
 m.ViewDisplayShaded
 m.ViewZoomtofit2
 assert s.save(m,str(out/(n+'.SLDPRT')))
 assert s.export(m,str(out/(n+'.STEP')))
 s.close(m)
s.max_documents=40
from sw_connect import get_com_member as g
for n in ('Mica_1','Mica_2'):
 p=out/(n+'.SLDPRT')
 m=s.open(str(p),silent=True)
 m.Parameter('D1@Boss-Extrude1').SystemValue=mm(1)
 assert m.ForceRebuild3(False)
 b=m.GetBodies2(0,False)[0]
 lo=b.GetExtremePoint(0.,0.,-1.,0.,0.,0.)
 hi=b.GetExtremePoint(0.,0.,1.,0.,0.,0.)
 assert abs(hi[3]-lo[3]-.001)<.000001
 m.MaterialPropertyValues=w.VARIANT(c.VT_ARRAY|c.VT_R8,[.36,.69,.77,.45,.75,.22,.35,.55,0.])
 try:
  m.ActiveView.DisplayMode=4
 except Exception:
  g(m,'ViewDisplayShaded')
 assert abs(m.MaterialPropertyValues[7]-.55)<1e-6
 assert s.save(m,str(p))
 assert s.export(m,str(p.with_suffix('.STEP')))
 s.close(m)
 print(n,'verified 1 mm and transparency',flush=True)
