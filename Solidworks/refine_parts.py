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
from sw_part import sketch_corner_rectangle
sys.path.insert(0,r'C:\Users\USER\.agents\skills\solidworks-automation\subskills\solidworks-threaded-holes\scripts')
from create_threaded_hole_template import create_offset_plane
def openpart(n): return s.open(str(out/(n+'.SLDPRT')),silent=True)
m=openpart('16_Banh_xe')
assert extrude_cut(m,profile(m,'Front Plane',lambda:sketch_polygon(m,0,0,mm(12.1/math.sqrt(3)),6)),mm(6),direction=False)
save(m,'16_Banh_xe',(.12,.30,.34))
m=openpart('04_Dong_co')
assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_circle(m,0,0,mm(11))),mm(2),direction=False)
p=create_offset_plane(m,'Mat_cat_truc','Front Plane',23)
assert extrude_cut(m,profile(m,p,lambda:sketch_corner_rectangle(m,mm(2),mm(-3),mm(3),mm(3))),mm(15),direction=False)
save(m,'04_Dong_co',(.12,.12,.14))
m=s.new_part()
assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_rectangle(m,0,0,mm(15),mm(10))),mm(5))
save(m,'20_Giac_motor',(.88,.86,.78))
