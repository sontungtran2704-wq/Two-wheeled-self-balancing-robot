import sys
from pathlib import Path
import win32com.client as w,pythoncom as c
sys.path.insert(0,r'C:\Users\USER\.agents\skills\solidworks-automation\scripts')
from sw_session import SolidWorksSession
from sw_connect import get_com_member
s=SolidWorksSession()
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
for x in (-30,30):
 for y in (-55,55):
  add('07_',(x,y,44))
  add('07_',(x,y,-2),[1,0,0,0,-1,0,0,0,-1])
for d in (-1,1):
 for x in (-12.5,12.5):
  add('07_',(x,d*42.5,0))
 for x in (-15.5,15.5):
  for z in (-52.5,-21.5):
   add('07_',(x,d*72.5,z),[d,0,0,0,0,-1,0,d,0])
a.ClearSelection2(True)
assert a.ForceRebuild3(False)
assert len(a.GetComponents(True))==38
assert s.save(a,str(p/'Robot_lap_so_bo.SLDASM'))
