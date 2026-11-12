function [M,J,H,b] = robot_coefficients(p)
Jc=p.mb*p.h^2/12-2*p.Ir;
if isfield(p,'Jc_scale'), Jc=Jc*p.Jc_scale; end
M=p.mb+2*p.mw+2*(0.5*p.mw*p.r^2+p.Ir)/p.r^2;
J=Jc+p.mb*p.l^2;
H=J+2*p.r*p.mb*p.l+p.r^2*M;
b=p.mb*p.l+p.r*M;
end
