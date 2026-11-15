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
m=s.new_part()
assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_polygon(m,0,0,mm(12/math.sqrt(3)),6)),mm(6),direction=False)
assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_circle(m,0,0,mm(5))),mm(12))
assert extrude_cut(m,profile(m,'Front Plane',lambda:sketch_circle(m,0,0,mm(2.5))),mm(12),direction=False)
assert extrude_cut(m,profile(m,'Front Plane',lambda:sketch_circle(m,0,0,mm(1.65))),mm(6),direction=True)
assert extrude_cut(m,profile(m,'Top Plane',lambda:sketch_circle(m,0,mm(9),mm(1.25))),mm(5.1),direction=False)
save(m,'17_Hub_AF12_L18',(.68,.48,.2))
for n,x,y,z,rgb in [('18_Bo_nguon_THAM_KHAO',28,65,1.6,(.1,.28,.55)),('19_Cong_tac_THAM_KHAO',20,14,15,(.18,.18,.19))]:
 m=s.new_part()
 assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_rectangle(m,0,0,mm(x),mm(y))),mm(z))
 save(m,n,rgb)
