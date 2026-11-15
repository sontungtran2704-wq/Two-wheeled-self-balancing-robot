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
for name,width,holes in [('Mica_1',30,[(25,y) for y in (15,85)]),('Mica_2',70,[(x,y) for x in (5,65) for y in (15,85)]+[(x,y) for x in (20,50) for y in (10,40,60,90)])]:
 m=s.new_part()
 assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_rectangle(m,0,0,mm(width),mm(100))),mm(2))
 def circles():
  for x,y in holes:
   assert sketch_circle(m,mm(x-width/2),mm(y-50),mm(1.7)) is not None
  return True
 assert extrude_cut(m,profile(m,'Front Plane',circles),0,both_directions=True)
 from sw_review import collect_geometry_measurements
 g=collect_geometry_measurements(m)
 assert sorted((round(h['position_mm'][0],3),round(h['position_mm'][1],3),round(h['diameter_mm'],3)) for h in g['holes'])==sorted((x-width/2,y-50,3.4) for x,y in holes),g
 save(m,name,(.48,.69,.74))
 print(name,len(holes),flush=True)
