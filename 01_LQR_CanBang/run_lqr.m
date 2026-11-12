function run_lqr()
folder=fileparts(mfilename('fullpath'));root=fileparts(folder);addpath(fullfile(root,'shared'));
out=fullfile(folder,'results');if ~isfolder(out),mkdir(out);end
diary(fullfile(out,'matlab_run.log'));cleanup=onCleanup(@()diary('off'));
fprintf('MATLAB %s, run %s\n',version,datestr(now,31));
[c,d,K,A,B,Ad,Bd]=robot_parameters();
reference_info=inspect_reference_model(out);
assert(rank(ctrb(Ad,Bd))==2); assert(rank(obsv(Ad,eye(2)))==2);
assert(all(abs(eig(Ad-Bd*K))<1)); assert(B(2)<0 && -K*[.05;0]>0);
Kbase=dlqr(Ad,Bd,diag(c.lqr.baseline_Q),c.lqr.baseline_R);
model=build_robot_model(folder,'LQR_CanBang',c,K);
closeModel=onCleanup(@()close_system(model,0));
base=struct('name','','duration',12,'motion',false,'push',false,'pitch',deg2rad(5),'speed',0);
cases={}; metrics={};
s=base;s.name='baseline_5deg';cases{end+1}={s,c,Kbase};
s=base;s.name='tuned_5deg';cases{end+1}={s,c,K};
s=base;s.name='tuned_10deg';s.pitch=deg2rad(10);cases{end+1}={s,c,K};
s=base;s.name='push';s.pitch=0;s.push=true;cases{end+1}={s,c,K};
for key={'mb','l'}
    name=key{1}; vals=c.provisional_ranges.(name);
    for j=1:numel(vals)
        changed=c;changed.model.(name)=vals(j);s=base;s.name=sprintf('sensitivity_%s_%d',name,j);
        % Fixed nominal gain and controller parameters; only plant changes.
        % run_robot_case currently shares cfg: use plant_model override consumed only by plant.
        changed=c;changed.plant_model=c.model;changed.plant_model.(name)=vals(j);
        cases{end+1}={s,changed,K};
    end
end
for i=1:numel(cases)
    args=cases{i};metrics{i}=run_robot_case(model,args{2},args{3},args{1},out);
end
summary=struct('config_id',c.config_id,'config_sha256',d.config_sha256,'MATLAB',version,...
 'toolboxes',{ver},'A',A,'B',B,'C',eye(2),'D',zeros(2,1),'Ad',Ad,'Bd',Bd,'K',K,...
 'K_baseline',Kbase,'poles',eig(Ad-Bd*K),'controllability_rank',rank(ctrb(Ad,Bd)),...
 'observability_rank',rank(obsv(Ad,eye(2))),'gain_python_error',max(abs(K-d.K(:).')), 'reference_model',reference_info,'scenarios',{metrics});
fid=fopen(fullfile(out,'evidence.json'),'w');fprintf(fid,'%s',jsonencode(summary,PrettyPrint=true));fclose(fid);
save(fullfile(out,'design.mat'),'c','d','A','B','Ad','Bd','K','Kbase','metrics');
% Q,R sweep is independently solved in MATLAB; no change to exported production gains.
rows=[];
for qt=[25 100 400 900]
 for qw=[1 4 10]
  for rr=[.5 1 2]
   kk=dlqr(Ad,Bd,diag([qt qw]),rr);pp=eig(Ad-Bd*kk);
   rows(end+1,:)=[qt qw rr kk max(abs(pp))]; %#ok<AGROW>
  end
 end
end
writetable(array2table(rows,'VariableNames',{'Qtheta','Qomega','R','Ktheta','Komega','spectral_radius'}),fullfile(out,'qr_sweep.csv'));
% Save a useful baseline when opening the SLX interactively.
ws=get_param(model,'ModelWorkspace');assignin(ws,'cfg',c);assignin(ws,'gain',K);
assignin(ws,'initial_state',[deg2rad(5);0;0;0;0;0]);
assignin(ws,'reference_input',[0 0 0 0 1 0;12 0 0 0 1 0]);
set_param(model,'StopTime','12');save_system(model);
fprintf('LQR MATLAB/Simulink assertions passed. Hardware unverified.\n');
end
