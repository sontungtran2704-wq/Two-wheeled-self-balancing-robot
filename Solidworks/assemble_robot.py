import sys
from pathlib import Path
import win32com.client as w,pythoncom as c
sys.path.insert(0,r'C:\Users\USER\.agents\skills\solidworks-automation\scripts')
from sw_session import SolidWorksSession
from sw_connect import get_com_member
s=SolidWorksSession()
p=Path('Solidworks/part_robot').resolve()
s.max_documents=40
a=s.new_assembly()
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
 print(prefix,xyz,flush=True)
add('02_',(0,0,0))
add('01_',(0,0,44))
for x in (-30,30):
 for y in (-55,55):add('09_',(x,y,42))
add('15_',(0,0,62))
for i,x in enumerate((-59/3,0,59/3)):
 add(str(12+i)+'_',(x,-32.5,53.5),[1,0,0,0,0,1,0,-1,0])
for d in (-1,1):
 add('10_',(0,d*72.5,-2),[-d,0,0,0,0,-1,0,d,0])
 rot=[d,0,0,0,0,-1,0,d,0]
 add('04_',(0,d*70.5,-37),rot)
 add('17_',(0,d*83.5,-37),rot)
 add('16_',(0,d*89.5,-37),rot)
assert a.ForceRebuild3(False)
a.ShowNamedView2('*Isometric',7)
get_com_member(a,'ViewZoomtofit2')
assert s.save(a,str(p/'Robot_lap_so_bo.SLDASM'))



