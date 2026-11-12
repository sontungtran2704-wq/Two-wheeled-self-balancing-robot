function [metrics,data] = run_robot_case(model,c,K,scenario,folder)
dt=c.timing.lqr_s; t=(0:dt:scenario.duration)'; n=numel(t);
p=c.model;if isfield(c,'plant_model'),p=c.plant_model;end
reference=zeros(n,5); reference(:,4)=1;
if scenario.motion
    reference(:,3)=1;
    reference(t>=3 & t<9,1)=.15;
    reference(t>=9 & t<15,1)=-.15;
    reference(t>=15 & t<21,2)=.6;
    reference(t>=21 & t<27,2)=-.6;
    reference(t>=27 & t<39,1)=.15;
    reference(t>=27 & t<39,2)=.4;
    reference(t>=33 & t<39,4)=0;
    reference(t>=39 & t<43,3)=0;
    reference(t>=43 & t<49,1)=-.1;
end
if scenario.push, reference(t>=3 & t<3.1,5)=.025; end
ws=get_param(model,'ModelWorkspace');
assignin(ws,'cfg',c); assignin(ws,'gain',K);
assignin(ws,'initial_state',[scenario.pitch;0;scenario.speed;0;0;0]);
assignin(ws,'reference_input',[t reference]);
set_param(model,'StopTime',num2str(scenario.duration,17));
res=sim(model); s=res.plant_log.Data; u=res.controller_log.Data;
% Independent explicit loop checks Simulink scheduling and S-function state.
sr=zeros(n,6); ur=zeros(n,12); sr(1,:)=[scenario.pitch 0 scenario.speed 0 0 0]; z=[];
for k=1:n
    in=[sr(k,1:2),sr(k,3)+c.model.r*sr(k,2),sr(k,5),reference(k,1:4)];
    [uk,z]=robot_controller_step(c,K,z,in); ur(k,:)=uk';
    if k<n
        sr(k+1,1:4)=robot_rk4(sr(k,1:4)',uk(1),p,dt,reference(k,5))';
        sr(k+1,5)=sr(k,5)+dt*uk(2);
        sr(k+1,6)=sr(k,6)+dt*sr(k,5)+.5*dt^2*uk(2);
    end
end
state_error=max(abs(sr-s),[],'all'); control_error=max(abs(ur-u),[],'all');
assert(state_error<1e-9 && control_error<1e-9,'Simulink versus explicit loop mismatch');
assert(all(isfinite(s),'all') && all(isfinite(u),'all'),'Nonfinite trajectory');
assert(max(abs(u(:,1)))<=c.drive.max_relative_accel+1e-9,'Acceleration limit');
assert(max(abs(u(:,3:4)),[],'all')<=c.drive.max_relative_speed+1e-9,'Wheel speed limit');
v=s(:,3)+p.r*s(:,2); torque=zeros(n,1);
[M,~]=robot_coefficients(p); ml=p.mb*p.l; r=p.r;
for k=1:n
    ds=robot_dynamics(s(k,1:4)',u(k,1),p,reference(k,5));
    torque(k)=r*(M*(u(k,1)+r*ds(2))+ml*cos(s(k,1))*ds(2)-ml*sin(s(k,1))*s(k,2)^2);
end
data=array2table([t s v u reference torque],'VariableNames',...
    {'time','theta','omega','q','x','yaw_rate','yaw_angle','velocity',...
    'u','alpha','left','right','theta_ref','aref','vref','wref','saturated','motion_active','ub','um',...
    'velocity_target','yaw_target','motion_requested','fresh','push_Nm','total_motor_torque_Nm'});
writetable(data,fullfile(folder,[scenario.name '.csv']));
metrics=struct('scenario',scenario.name,'max_pitch_deg',max(abs(s(:,1)))*180/pi,...
    'final_pitch_deg',s(end,1)*180/pi,'final_velocity',v(end),'final_position',s(end,4),...
    'max_u',max(abs(u(:,1))),'max_wheel_speed',max(abs(u(:,3:4)),[],'all'),...
    'saturated_fraction',mean(u(:,9)),'max_total_torque_Nm',max(abs(torque)),...
    'simulink_state_error',state_error,'simulink_controller_error',control_error,...
    'fell',max(abs(s(:,1)))>=c.safety.fall_rad);
bad=find(abs(s(:,1))>deg2rad(.5) | abs(s(:,2))>.05,1,'last');
metrics.settling_s=0;
if ~isempty(bad)
    if bad<n, metrics.settling_s=t(bad+1); else, metrics.settling_s=NaN; end
end
metrics.saturation_s=sum(u(:,9))*dt;
metrics.control_rms=sqrt(mean(u(:,1).^2));
metrics.config_id=c.config_id;
metrics.config_sha256=jsondecode(fileread(fullfile(fileparts(fileparts(mfilename('fullpath'))),'shared','design.json'))).config_sha256;
if scenario.motion
    checkpoints=[8.9 14.9 20.9 26.9 32.9 38.9 48.9];
    errors=zeros(numel(checkpoints),4);
    for j=1:numel(checkpoints)
        [~,idx]=min(abs(t-checkpoints(j)));
        errors(j,:)=[t(idx),v(idx)-u(idx,7),s(idx,5)-u(idx,8),rad2deg(s(idx,1))];
    end
    metrics.plateau_errors=errors;
end
f=figure('Visible','off','Position',[50 50 1000 800]);
tiledlayout(4,1);
nexttile; plot(t,s(:,1)*180/pi,t,u(:,5)*180/pi); ylabel('Pitch (deg)'); legend('actual','reference'); grid on;
nexttile; plot(t,v,t,u(:,7)); ylabel('Velocity (m/s)'); legend('actual','reference'); grid on;
nexttile; plot(t,s(:,5),t,u(:,8)); ylabel('Yaw (rad/s)'); legend('actual','reference'); grid on;
nexttile; yyaxis left; plot(t,u(:,1)); ylabel('u (m/s^2)'); yyaxis right; plot(t,u(:,3),t,u(:,4)); ylabel('Relative wheel speed (m/s)'); xlabel('Time (s)'); legend('u','left','right'); grid on;
sgtitle(strrep(scenario.name,'_',' ')); exportgraphics(f,fullfile(folder,[scenario.name '.png'])); close(f);
fprintf('%s: pitch_max=%.4f deg, final_v=%.6f m/s, final_x=%.6f m, simulation error=%.3g\n',...
    scenario.name,metrics.max_pitch_deg,metrics.final_velocity,metrics.final_position,state_error);
end
