function ds = robot_dynamics(s,u,p,push)
% s=[pitch; pitch_rate; relative_tangential_speed; position].
if nargin<4, push=0; end
[M,J]=robot_coefficients(p); th=s(1); w=s(2); ml=p.mb*p.l;
b=ml*cos(th)+p.r*M; H=J+2*p.r*ml*cos(th)+p.r^2*M;
wd=(ml*p.g*sin(th)-b*u+p.r*ml*sin(th)*w^2+push)/H;
ds=[w;wd;u;s(3)+p.r*w];
end
