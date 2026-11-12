"""Reproducible nonlinear, uncertainty, actuator and same-input experiments."""
from design import *
from control_reference import Controller
import copy, platform
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

OUT=ROOT/'06_KiemThu/results';OUT.mkdir(parents=True,exist_ok=True)
FIELDS=['t_s','theta_rad','omega_rad_s','v_true_m_s','v_pulse_est_m_s','x_m','yaw_rad_s','vtarget_m_s','wtarget_rad_s','vref_m_s','wref_rad_s','u_cmd_m_s2','u_actual_m_s2','left_m_s','right_m_s','theta_ref_rad','torque_each_Nm','saturated','motion_active','fresh','fall']

def simulate(name,initial=0.0,duration=12,baseline=False,plant=None,delay=0,tracking=1.,motion=False,push=False,config=None,save=True):
    c=config or load_config();p=plant or c['model'];dt=c['timing']['lqr_s']
    ctrl=Controller(c)
    if baseline:ctrl.K=design_lqr(c,True)[4][0]
    s=np.array([initial,0.,0.,0.]);yr=0.;hist=[]; fifo=[0.]*int(round(delay/dt))
    fell=False
    for n in range(int(round(duration/dt))):
        t=n*dt
        # Long plateaux distinguish tracking error from command ramp transient.
        vt=0.;wt=0.;fresh=True;on=motion
        if motion:
            if 3<=t<9:vt=.15
            elif 9<=t<15:vt=-.15
            elif 15<=t<21:wt=.6
            elif 21<=t<27:wt=-.6
            elif 27<=t<33:vt=.15;wt=.4
            elif 33<=t<39:vt=.15;wt=.4;fresh=False
            elif 39<=t<43:on=False
            elif 43<=t<49:vt=-.10
        measured_v=ctrl.q+c['model']['r']*s[1]  # emitted pulse estimate, not encoder
        z=ctrl.tick(s[0],s[1],measured_v,yr,vt,wt,on,fresh)
        fifo.append(z['u']);u=tracking*p['r']/c['model']['r']*fifo.pop(0)
        moment=.025 if push and 3<=t<3.1 else 0.
        # Push is externally applied body torque, distinct from input u.
        tau=torque(s,u,p,moment)/2
        hist.append([t,s[0],s[1],s[2]+p['r']*s[1],measured_v,s[3],yr,vt,wt,z['vref'],z['wref'],z['u'],u,z['left'],z['right'],z['theta_ref'],tau,z['saturated'],z['motion_active'],fresh,fell])
        if abs(s[0])>c['safety']['fall_rad']:
            fell=True;break
        # Integrate with 1 ms substeps to check nonlinear dynamics independently of ZOH.
        for _ in range(5):s=rk4(s,u,p,dt/5,moment)
        yr+=tracking*z['alpha']*dt*c['model']['W']/p['W']*p['r']/c['model']['r']
    a=np.array(hist,dtype=float)
    if save:
        with (OUT/f'{name}.csv').open('w',newline='',encoding='utf-8') as f:
            w=csv.writer(f);w.writerow(FIELDS);w.writerows(a)
    after=(abs(a[:,1])>.5*math.pi/180)|(abs(a[:,2])>.05)
    bad=np.flatnonzero(after)
    settle=float(a[bad[-1]+1,0]) if len(bad) and bad[-1]<len(a)-1 else (None if len(bad) else 0.)
    metrics=dict(scenario=name,baseline=baseline,duration_s=float(a[-1,0]+dt),fell=fell,theta_peak_deg=float(max(abs(a[:,1]))*180/math.pi),settling_s=settle,drift_m=float(a[-1,5]),final_v_m_s=float(a[-1,3]),control_peak_m_s2=float(max(abs(a[:,11]))),control_rms_m_s2=float(np.sqrt(np.mean(a[:,11]**2))),saturation_s=float(sum(a[:,17])*dt),torque_each_peak_Nm=float(max(abs(a[:,16]))),velocity_estimate_error_peak_m_s=float(max(abs(a[:,3]-a[:,4]))),delay_s=delay,tracking_gain=tracking,plant=p)
    metrics['omega_peak_rad_s']=float(max(abs(a[:,2])))
    metrics['config_id']=c['config_id']
    metrics['config_sha256']=hashlib.sha256((ROOT/'shared/robot.json').read_bytes()).hexdigest()
    metrics['source_manifest']='00_Nguon/source_manifest.json'
    metrics['effective_step_accel_scale']=tracking*p['r']/c['model']['r']
    metrics['fixed_seed']=0
    if motion:
        end_errors=[]
        for t in [8.9,14.9,20.9,26.9,32.9,38.9,48.9]:
            k=np.argmin(abs(a[:,0]-t));end_errors.append(dict(t_s=float(a[k,0]),v_error=float(a[k,3]-a[k,9]),yaw_error=float(a[k,6]-a[k,10]),theta_deg=float(a[k,1]*180/math.pi)))
        metrics['plateau_errors']=end_errors
    return a,metrics

def vectors():
    c=Controller();rows=[]
    # Actual sensed data fixed across languages, including saturation and handover.
    for n in range(2400):
        t=n*.005
        th=.004*math.sin(t*2);om=.008*math.cos(t*2)
        if 7<t<7.4:th=.2;om=.5
        v=.09*math.sin(t*.6);y=.2*math.sin(t)
        vt=.15 if 2<t<6 else -.1
        wt=.6 if 4<t<8 else -.4
        on=not (9<t<10);fresh=not (5<t<5.8)
        inp=[th,om,v,y,vt,wt,int(on),int(fresh)]
        z=c.tick(*inp);rows.append(inp+list(z.values()))
    fields=['theta','omega','v','yaw','vtarget','wtarget','motion','fresh','u','alpha','left','right','theta_ref','aref','vref','wref','saturated','motion_active','ub','um']
    with (ROOT/'shared/controller_vectors.csv').open('w',newline='',encoding='utf-8') as f:
        w=csv.writer(f);w.writerow(fields);w.writerows(rows)

def main():
    results=[];curves={}
    for name,kwargs in [('baseline_5deg',dict(initial=math.radians(5),baseline=True)),('tuned_5deg',dict(initial=math.radians(5))),('tuned_10deg',dict(initial=math.radians(10))),('tuned_15deg',dict(initial=math.radians(15))),('push',dict(push=True)),('motion',dict(duration=52,motion=True))]:
        a,m=simulate(name,**kwargs);results.append(m);curves[name]=a
    c=load_config()
    for key in ['mb','mw','r','l','Jc_scale']:
        for i,val in enumerate(c['provisional_ranges'][key]):
            p=copy.deepcopy(c['model']);p[key]=val
            _,m=simulate(f'sensitivity_{key}_{i}',initial=math.radians(5),plant=p)
            results.append(m)
    for d in c['provisional_ranges']['actuator_delay_s']:
        for gain in c['provisional_ranges']['tracking_gain']:
            _,m=simulate(f'actuator_delay{d}_gain{gain}',initial=math.radians(5),delay=d,tracking=gain)
            results.append(m)
    # Track width changes yaw conversion; no pitch effect in decoupled planar model.
    for W in c['provisional_ranges']['W']:
        p=copy.deepcopy(c['model']);p['W']=W
        a,m=simulate(f'track_{W}',duration=52,motion=True,plant=p)
        m['yaw_geometry_scale']=c['model']['W']/W
        m['note']='Actual yaw uses actual track; gyro feedback closes outer loop. No tire slip model.'
        results.append(m)
    (OUT/'metrics.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
    flat=[{k:v for k,v in r.items() if not isinstance(v,(dict,list))} for r in results]
    fields=sorted(set().union(*(r.keys() for r in flat)))
    with (OUT/'metrics.csv').open('w',newline='',encoding='utf-8') as f:
        w=csv.DictWriter(f,fieldnames=fields);w.writeheader();w.writerows(flat)
    fig,axs=plt.subplots(3,1,figsize=(9,8),sharex=True)
    for name in ['baseline_5deg','tuned_5deg','push']:
        a=curves[name]
        for ax,col,scale in zip(axs,[1,5,11],[180/math.pi,1,1]):ax.plot(a[:,0],a[:,col]*scale,label=name)
    for ax,label in zip(axs,['Pitch (deg)','Position (m)','Relative acceleration (m/s²)']):ax.set_ylabel(label);ax.grid();ax.legend()
    axs[-1].set_xlabel('Time (s)');fig.tight_layout();fig.savefig(OUT/'balance_comparison.png',dpi=160);plt.close(fig)
    a=curves['motion'];fig,axs=plt.subplots(3,1,figsize=(11,8),sharex=True)
    axs[0].plot(a[:,0],a[:,3],label='pulse-tracking plant v');axs[0].plot(a[:,0],a[:,9],'--',label='ramped v ref')
    axs[1].plot(a[:,0],a[:,6],label='yaw rate');axs[1].plot(a[:,0],a[:,10],'--',label='ramped yaw ref')
    axs[2].plot(a[:,0],a[:,1]*180/math.pi,label='pitch');axs[2].plot(a[:,0],a[:,15]*180/math.pi,'--',label='pitch reference')
    for ax,label in zip(axs,['Velocity (m/s)','Yaw rate (rad/s)','Pitch (deg)']):ax.set_ylabel(label);ax.grid();ax.legend()
    axs[-1].set_xlabel('Time (s)');fig.tight_layout();fig.savefig(OUT/'motion.png',dpi=160);plt.close(fig)
    vectors()
    print(json.dumps([{k:r[k] for k in ['scenario','fell','theta_peak_deg','settling_s','drift_m','saturation_s']} for r in results[:6]],indent=2))
if __name__=='__main__':main()
