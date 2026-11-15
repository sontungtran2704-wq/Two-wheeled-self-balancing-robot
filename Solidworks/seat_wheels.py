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
from win32com.client import VARIANT
for k in a.GetComponents(True):
 prefix=Path(get_com_member(k,'GetPathName')).name[:3]
 if prefix != '16_':continue
 v=list(get_com_member(k.Transform2,'ArrayData'))
 d=1 if v[10]>0 else -1
 y=97.5
 v[10]=d*y/1000
 a.ClearSelection2(True)
 assert k.Select4(False,VARIANT(c.VT_DISPATCH,None),False)
 get_com_member(a,'UnfixComponent')
 k.Transform2=math.CreateTransform(VARIANT(c.VT_ARRAY|c.VT_R8,v))
 assert max(abs(x-y) for x,y in zip(v,get_com_member(k.Transform2,'ArrayData')))<1e-8
 a.ClearSelection2(True)
 assert k.Select4(False,VARIANT(c.VT_DISPATCH,None),False)
 get_com_member(a,'FixComponent')
 print(prefix,d,y,flush=True)
a.ClearSelection2(True)
assert a.ForceRebuild3(False)
assert s.save(a,str(p/'Robot_lap_so_bo.SLDASM'))

