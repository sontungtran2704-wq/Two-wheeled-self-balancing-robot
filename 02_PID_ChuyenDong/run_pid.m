function run_pid()
folder=fileparts(mfilename('fullpath'));root=fileparts(folder);addpath(fullfile(root,'shared'));
out=fullfile(folder,'results');if ~isfolder(out),mkdir(out);end
diary(fullfile(out,'matlab_run.log'));cleanup=onCleanup(@()diary('off'));
fprintf('MATLAB %s, run %s\n',version,datestr(now,31));
[c,d,K]=robot_parameters();
model=build_robot_model(folder,'PID_ChuyenDong',c,K);
closeModel=onCleanup(@()close_system(model,0));
scenario=struct('name','motion_52s','duration',52,'motion',true,'push',false,'pitch',0,'speed',0);
[metrics,data]=run_robot_case(model,c,K,scenario,out);
v=verify_controller_vectors(c,K,out);
summary=struct('config_id',c.config_id,'config_sha256',d.config_sha256,'MATLAB',version,...
 'pid',c.pid,'motion',metrics,'vectors',v);
assert(~metrics.fell,'Motion scenario fell');
assert(metrics.max_pitch_deg<8,'Motion pitch criterion');
assert(max(abs(metrics.plateau_errors(:,2)))<.03,'Motion velocity plateau criterion');
assert(max(abs(metrics.plateau_errors(:,3)))<.08,'Motion yaw plateau criterion');
fid=fopen(fullfile(out,'evidence.json'),'w');fprintf(fid,'%s',jsonencode(summary,PrettyPrint=true));fclose(fid);
save(fullfile(out,'motion.mat'),'c','K','metrics','data');
save_system(model);
fprintf('PID MATLAB/Simulink and controller vector assertions passed. Hardware unverified.\n');
end
