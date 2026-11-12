function [c,d,K,A,B,Ad,Bd] = robot_parameters()
% Independent MATLAB design from the shared physical and timing contract.
root=fileparts(fileparts(mfilename('fullpath')));
c=jsondecode(fileread(fullfile(root,'shared','robot.json')));
d=jsondecode(fileread(fullfile(root,'shared','design.json')));
p=c.model; [~,~,H,b]=robot_coefficients(p);
A=[0 1;p.mb*p.g*p.l/H 0]; B=[0;-b/H];
sys=c2d(ss(A,B,eye(2),zeros(2,1)),c.timing.lqr_s,'zoh');
Ad=sys.A; Bd=sys.B;
K=dlqr(Ad,Bd,diag(c.lqr.Q),c.lqr.R);
assert(max(abs(K-d.K(:).'))<1e-8,'MATLAB/Python LQR gain mismatch');
assert(max(abs(Ad-d.Ad),[],'all')<1e-12,'Discrete A mismatch');
assert(max(abs(Bd-d.Bd),[],'all')<1e-12,'Discrete B mismatch');
end
