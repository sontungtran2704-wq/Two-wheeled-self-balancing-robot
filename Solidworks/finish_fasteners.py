import sys,math
from pathlib import Path
import pythoncom as c,win32com.client as w
sys.path.insert(0,r'C:\Users\USER\.agents\skills\solidworks-automation\scripts')
from sw_session import SolidWorksSession
from sw_part import sketch,sketch_circle,sketch_polygon,sketch_rectangle,extrude_boss,extrude_cut
from sw_connect import mm
from sw_sketch_quality import fully_define_sketch
s=SolidWorksSession()
s.max_documents=40
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
for name,radius,length in [('21_Vit_ham_M3_THAM_KHAO',1.5,3),('22_Vit_M4x8',2,8)]:
 if (out/(name+'.SLDPRT')).exists(): continue
 m=s.new_part()
 assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_circle(m,0,0,mm(radius))),mm(length))
 if radius==2:
  assert extrude_boss(m,profile(m,'Front Plane',lambda:sketch_circle(m,0,0,mm(3.5))),mm(2.5),direction=False)
 save(m,name,(.25,.25,.27) if radius<2 else (.65,.67,.70))
import sys
from pathlib import Path
import win32com.client as w,pythoncom as c
sys.path.insert(0,r'C:\Users\USER\.agents\skills\solidworks-automation\scripts')
from sw_session import SolidWorksSession
from sw_connect import get_com_member
s=SolidWorksSession()
s.max_documents=40
p=Path('Solidworks/part_robot').resolve()
s.max_documents=40
a=s.open(str(p/'Robot_lap_so_bo.SLDASM'),silent=True)
math=get_com_member(s.sw,'GetMathUtility')
math._FlagAsMethod("CreateTransform")
I=[1,0,0,0,1,0,0,0,1]
def add(prefix,xyz,R=I):
 f=next(f for f in p.glob(prefix+'*.SLDPRT') if '.step.' not in f.name.lower())
 m=s.open(str(f),silent=True)
 from win32com.client import VARIANT
 err=VARIANT(c.VT_BYREF|c.VT_I4,0)
 title=get_com_member(a,'GetTitle')
 s.sw.ActivateDoc3(title,False,0,err)
 k=a.AddComponent5(str(f),0,'',False,'',0,0,0)
 assert k is not None,f
 a.ClearSelection2(True)
 k.Select4(False,VARIANT(c.VT_DISPATCH,None),False)
 get_com_member(a,"UnfixComponent")
 k.Transform2=get_com_member(math,"CreateTransform",VARIANT(c.VT_ARRAY|c.VT_R8,[*R,*(v/1000 for v in xyz),1,0,0,0]))
 a.ClearSelection2(True)
 k.Select4(False,VARIANT(c.VT_DISPATCH,None),False)
 get_com_member(a,"FixComponent")
assert len(a.GetComponents(True))==38
for d in (-1,1):
 for x in (-12.5,12.5):
  add('06_',(x,d*42.5,-4))
 add('22_',(0,d*126.5,-37),[d,0,0,0,0,-1,0,d,0])
 add('21_',(0,d*104.5,-32),I)
a.ClearSelection2(True)
assert a.ForceRebuild3(False)
assert len(a.GetComponents(True))==46
assert s.save(a,str(p/'Robot_lap_so_bo.SLDASM'))



