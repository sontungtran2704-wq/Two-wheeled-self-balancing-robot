function next = robot_rk4(s,u,p,dt,push)
if nargin<5, push=0; end
k1=robot_dynamics(s,u,p,push);
k2=robot_dynamics(s+dt*k1/2,u,p,push);
k3=robot_dynamics(s+dt*k2/2,u,p,push);
k4=robot_dynamics(s+dt*k3,u,p,push);
next=s+dt*(k1+2*k2+2*k3+k4)/6;
end
