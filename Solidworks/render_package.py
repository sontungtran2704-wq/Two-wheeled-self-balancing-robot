from pathlib import Path
import struct, json, hashlib, zipfile
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

base=Path(__file__).parent/'Bo_part_robot_2026-11-13'
review=base/'Kiem_tra'
paths=sorted(review.glob('*.stl'))
rows=(len(paths)+1)//2
fig=plt.figure(figsize=(12,5*rows),facecolor='white')
titles=['01 - Tam nhom tren','02 - Tam nhom duoi','03 - Banh xe (THAM KHAO)','04 - Dong co (THAM KHAO)','05 - Vong dem M3','06 - Dai oc M3x0.5','07 - Vit M3x5','08 - Vit M3x10','09 - Tru luc giac M3 - 42 mm']
for i,path in enumerate(sorted(review.glob('*.stl'))):
    data=path.read_bytes()
    n=struct.unpack_from('<I',data,80)[0]
    if len(data)==84+50*n:
        dtype=np.dtype([('normal','<f4',(3,)),('vertices','<f4',(3,3)),('attr','<u2')])
        verts=np.frombuffer(data, dtype=dtype,count=n,offset=84)['vertices'].astype(float)
    else:
        pts=[[float(x) for x in line.split()[1:]] for line in data.decode().splitlines() if line.strip().startswith('vertex ')]
        verts=np.array(pts).reshape(-1,3,3)
    assert len(verts)>10 and np.isfinite(verts).all()
    ax=fig.add_subplot(rows,2,i+1,projection='3d')
    normals=np.cross(verts[:,1]-verts[:,0],verts[:,2]-verts[:,0])
    normals/=np.maximum(np.linalg.norm(normals,axis=1)[:,None],1e-12)
    light=np.array([.3,-.5,1]); light/=np.linalg.norm(light)
    shades=.5+.4*np.abs(normals@light)
    colors=np.column_stack([shades*.70,shades*.80,shades*.92,np.ones(len(shades))])
    ax.add_collection3d(Poly3DCollection(verts,facecolors=colors,edgecolors='none',linewidths=0))
    lo=verts.min(axis=(0,1)); hi=verts.max(axis=(0,1)); center=(lo+hi)/2; span=max(hi-lo)*.56
    ax.set_xlim(center[0]-span,center[0]+span); ax.set_ylim(center[1]-span,center[1]+span); ax.set_zlim(center[2]-span,center[2]+span)
    ax.set_box_aspect((1,1,1)); ax.view_init(elev=65 if i<2 else 25,azim=-65)
    ax.set_axis_off(); ax.set_title(titles[i] if i<len(titles) else '10 - Ga L (CHO LO CHAN)',fontsize=14)
fig.suptitle('Mo hinh tu luoi STL xuat tu cac part SolidWorks',fontsize=16)
fig.tight_layout(rect=(0,0,1,.975))
fig.savefig(base/'00_Xem_truoc.png',dpi=150,bbox_inches='tight')
print('RENDERED',base/'00_Xem_truoc.png')
