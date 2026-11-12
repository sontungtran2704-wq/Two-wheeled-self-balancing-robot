"""Pure numerical oracle. Mirrors the documented discrete update order, not HAL."""
import math
from design import load_config,design_lqr,coefficients

def clip(v,lo,hi): return max(lo,min(hi,v))
def ramp(v,target,step): return v+clip(target-v,-step,step)
class PID:
    def __init__(self,g): self.g=g.copy(); self.reset()
    def reset(self): self.i=0.;self.d=0.;self.prev=0.;self.ready=False
    def update(self,ref,y,dt,tau,limit,saturated):
        e=ref-y
        raw=(y-self.prev)/dt if self.ready else 0.
        self.ready=True;self.prev=y;self.d+=dt/(tau+dt)*(raw-self.d)
        candidate=clip(self.i+e*dt,-1.,1.)
        z=self.g['kp']*e+self.g['ki']*candidate-self.g['kd']*self.d
        if not saturated and (abs(z)<=limit or z*e<0): self.i=candidate
        return clip(self.g['kp']*e+self.g['ki']*self.i-self.g['kd']*self.d,-limit,limit)
class Controller:
    def __init__(self,c=None):
        self.c=c or load_config();self.K=design_lqr(self.c)[4][0];self.p=self.c['pid'];self.dt=self.c['timing']['lqr_s']
        self.vpid=PID(self.p['velocity']);self.ypid=PID(self.p['yaw'])
        self.q=self.d=self.vref=self.wref=self.aref=self.yacc=0.
        self.n=0;self.sat=False;self.stable=0;self.active=False;self.stopping=False
    def tick(self,th,omega,v,yaw,vtarget,wtarget,motion=True,fresh=True):
        dt=self.dt;p=self.p;c=self.c; drive=c['drive'];safety=c['safety']
        if abs(th)<safety['stable_rad'] and abs(omega)<safety['stable_rate']: self.stable+=dt
        else: self.stable=0
        if motion and self.stable>=c['timing']['stable_s']: self.active=True;self.stopping=False
        if not motion and self.active:self.stopping=True
        target_v=clip(vtarget,-p['velocity_max'],p['velocity_max']) if fresh and motion else 0.
        target_w=clip(wtarget,-p['yaw_rate_max'],p['yaw_rate_max']) if fresh and motion else 0.
        if self.n%4==0:
            h=c['timing']['pid_s']
            self.vref=ramp(self.vref,target_v if self.active else 0,p['velocity_slew']*h)
            self.wref=ramp(self.wref,target_w if self.active else 0,p['yaw_slew']*h)
            desired=self.vpid.update(self.vref,v,h,p['derivative_tau_s'],p['accel_max'],self.sat) if self.active else 0.
            self.yacc=self.ypid.update(self.wref,yaw,h,p['derivative_tau_s'],p['yaw_accel_max'],self.sat) if self.active else 0.
            self.aref+=h/(p['accel_filter_s']+h)*(desired-self.aref)
            if self.stopping and abs(v)<.01 and abs(yaw)<.03 and abs(self.aref)<.01:
                self.active=False;self.stopping=False;self.vpid.reset();self.ypid.reset()
        m=c['model']; _,_,_,b=coefficients(m)
        # Exact constant-acceleration equilibrium, with principal small-angle branch.
        tr=math.atan2(self.aref,m['g'])+math.asin(clip(m['r']*coefficients(m)[0]*self.aref/(m['mb']*m['l']*math.hypot(m['g'],self.aref)),-1,1))
        ub=-self.K[0]*th-self.K[1]*omega
        um=self.K[0]*tr+self.aref
        A=drive['max_relative_accel'];V=drive['max_relative_speed']
        bal=clip(ub,-A,A);u=bal+clip(um,-A-bal,A-bal)
        # Feasible per-wheel next velocities enforce both limits at a corner.
        left,right=self.q-self.d,self.q+self.d
        ll,lh=max(-V,left-A*dt),min(V,left+A*dt)
        rl,rh=max(-V,right-A*dt),min(V,right+A*dt)
        qnew=clip(self.q+dt*u,(ll+rl)/2,(lh+rh)/2);uactual=(qnew-self.q)/dt
        dnew=clip(self.d+dt*m['W']/2*self.yacc,max(qnew-lh,rl-qnew),min(qnew-ll,rh-qnew))
        daactual=(dnew-self.d)/dt
        self.sat=abs(u-ub-um)>1e-9 or abs(uactual-u)>1e-9 or abs(daactual-m['W']/2*self.yacc)>1e-9
        self.q=qnew;self.d=dnew;self.n+=1
        return dict(u=uactual,alpha=2*daactual/m['W'],left=self.q-self.d,right=self.q+self.d,theta_ref=tr,aref=self.aref,vref=self.vref,wref=self.wref,saturated=int(self.sat),motion_active=int(self.active),ub=ub,um=um)
