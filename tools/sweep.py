"""Discrete Q/R candidates on identical nonlinear initial-condition scenario."""
from simulate import *

rows=[]
for q in (25,100,400):
    for w in (1,4):
        for r in (1,2):
            c=load_config();c['lqr']['Q']=[q,w];c['lqr']['R']=r
            a,m=simulate(f'Q{q}_{w}_R{r}',initial=math.radians(5),config=c,save=False)
            _,_,Ad,Bd,K,_=design_lqr(c)
            rows.append(dict(Qtheta=q,Qomega=w,R=r,Ktheta=K[0,0],Komega=K[0,1],pole_radius=max(abs(np.linalg.eigvals(Ad-Bd@K))),settling_s=m['settling_s'],drift_m=m['drift_m'],omega_peak=m['omega_peak_rad_s'],u_peak=m['control_peak_m_s2'],u_rms=m['control_rms_m_s2'],saturation_s=m['saturation_s'],fell=m['fell']))
with (OUT/'qr_sweep.csv').open('w',newline='',encoding='utf-8') as f:
    w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
print(json.dumps(rows,indent=2))
