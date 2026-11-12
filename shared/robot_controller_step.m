function [out,z] = robot_controller_step(c,K,z,in)
% Pure discrete controller, independently implemented from the shared contract.
% z: q,d,vref,wref,aref,yacc,n,sat,stable,active,stopping,PIDv(5),PIDyaw(5).
% in: theta,omega,v,yaw_rate,vtarget,wtarget,motion,fresh.
if isempty(z), z=zeros(21,1); end
th=in(1); omega=in(2); v=in(3); yaw=in(4);
motion=logical(in(7)); fresh=logical(in(8));
dt=c.timing.lqr_s; p=c.pid; safe=c.safety; drive=c.drive;
if abs(th)<safe.stable_rad && abs(omega)<safe.stable_rate
    z(9)=z(9)+dt;
else
    z(9)=0;
end
if motion && z(9)>=c.timing.stable_s, z(10)=1; z(11)=0; end
if ~motion && z(10), z(11)=1; end
tv=0; tw=0;
if fresh && motion
    tv=clip(in(5),-p.velocity_max,p.velocity_max);
    tw=clip(in(6),-p.yaw_rate_max,p.yaw_rate_max);
end
if mod(z(7),4)==0
    h=c.timing.pid_s;
    z(3)=z(3)+clip(tv*z(10)-z(3),-p.velocity_slew*h,p.velocity_slew*h);
    z(4)=z(4)+clip(tw*z(10)-z(4),-p.yaw_slew*h,p.yaw_slew*h);
    desired=0; z(6)=0;
    if z(10)
        [desired,z(12:16)]=pid_step(p.velocity,z(12:16),z(3),v,h,p.derivative_tau_s,p.accel_max,z(8));
        [z(6),z(17:21)]=pid_step(p.yaw,z(17:21),z(4),yaw,h,p.derivative_tau_s,p.yaw_accel_max,z(8));
    end
    z(5)=z(5)+h/(p.accel_filter_s+h)*(desired-z(5));
    if z(11) && abs(v)<.01 && abs(yaw)<.03 && abs(z(5))<.01
        z(10:11)=0; z(12:21)=0;
    end
end
m=c.model; M=robot_coefficients(m);
tr=atan2(z(5),m.g)+asin(clip(m.r*M*z(5)/(m.mb*m.l*hypot(m.g,z(5))),-1,1));
ub=-K(1)*th-K(2)*omega; um=K(1)*tr+z(5);
A=drive.max_relative_accel; V=drive.max_relative_speed;
bal=clip(ub,-A,A); u=bal+clip(um,-A-bal,A-bal);
left=z(1)-z(2); right=z(1)+z(2);
llo=max(-V,left-A*dt); lhi=min(V,left+A*dt);
rlo=max(-V,right-A*dt); rhi=min(V,right+A*dt);
qn=clip(z(1)+dt*u,(llo+rlo)/2,(lhi+rhi)/2); ua=(qn-z(1))/dt;
dn=clip(z(2)+dt*m.W/2*z(6),max(qn-lhi,rlo-qn),min(qn-llo,rhi-qn)); daa=(dn-z(2))/dt;
z(8)=abs(u-ub-um)>1e-9 || abs(ua-u)>1e-9 || abs(daa-m.W/2*z(6))>1e-9;
z(1)=qn; z(2)=dn; z(7)=z(7)+1;
out=[ua;2*daa/m.W;qn-dn;qn+dn;tr;z(5);z(3);z(4);z(8);z(10);ub;um];
end
function [out,z]=pid_step(g,z,ref,y,dt,tau,lim,sat)
% z=[integral, derivative, previous_measurement, ready, reserved].
e=ref-y; raw=0;
if z(4), raw=(y-z(3))/dt; end
z(4)=1; z(3)=y; z(2)=z(2)+dt/(tau+dt)*(raw-z(2));
candidate=clip(z(1)+e*dt,-1,1);
val=g.kp*e+g.ki*candidate-g.kd*z(2);
if ~sat && (abs(val)<=lim || val*e<0), z(1)=candidate; end
out=clip(g.kp*e+g.ki*z(1)-g.kd*z(2),-lim,lim);
end
function v=clip(v,lo,hi)
v=max(lo,min(hi,v));
end
