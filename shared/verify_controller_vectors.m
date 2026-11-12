function evidence=verify_controller_vectors(c,K,outfolder)
% Same frozen measurements/references are consumed independently by MATLAB,
% Simulink and the host C++ CLI. Expected values originate in Python.
root=fileparts(fileparts(mfilename('fullpath')));
v=readtable(fullfile(root,'shared','controller_vectors.csv'));
inp=table2array(v(:,1:8)); expected=table2array(v(:,9:20)); n=height(v);
actual=zeros(n,12); z=[];
for i=1:n
    [a,z]=robot_controller_step(c,K,z,inp(i,:)); actual(i,:)=a';
end
matlab_error=max(abs(actual-expected),[],'all');
assert(matlab_error<1e-8,'MATLAB numeric controller differs from Python vectors');
tab=array2table(actual,'VariableNames',v.Properties.VariableNames(9:20));
writetable(tab,fullfile(outfolder,'controller_matlab.csv'));
model='Verify_Controller_Vectors';
if bdIsLoaded(model),close_system(model,0);end
new_system(model);load_system('simulink');
set_param(model,'Solver','FixedStepDiscrete','FixedStep',num2str(c.timing.lqr_s,17),...
 'StopTime',num2str((n-1)*c.timing.lqr_s,17),'ReturnWorkspaceOutputs','on');
t=(0:n-1)'*c.timing.lqr_s;
sense=[inp(:,1:2),inp(:,3)-c.model.r*inp(:,2),zeros(n,1),inp(:,4),zeros(n,1)];
refs=[inp(:,5:8),zeros(n,1)];
ws=get_param(model,'ModelWorkspace');assignin(ws,'cfg',c);assignin(ws,'gain',K);
assignin(ws,'sensors',[t sense]);assignin(ws,'commands',[t refs]);
add_block('simulink/Sources/From Workspace',[model '/Sensors'],'VariableName','sensors','Interpolate','off','OutputAfterFinalValue','Holding final value');
add_block('simulink/Sources/From Workspace',[model '/Commands'],'VariableName','commands','Interpolate','off','OutputAfterFinalValue','Holding final value');
add_block('simulink/User-Defined Functions/Level-2 MATLAB S-Function',[model '/Controller'],...
 'FunctionName','robot_controller_sfun','Parameters','cfg,gain');
add_block('simulink/Sinks/To Workspace',[model '/Log'],'VariableName','vector_output','SaveFormat','Timeseries');
add_line(model,'Sensors/1','Controller/1');add_line(model,'Commands/1','Controller/2');add_line(model,'Controller/1','Log/1');
out=sim(model); sl=out.vector_output.Data;
simulink_error=max(abs(sl-expected),[],'all');
assert(simulink_error<1e-8,'Simulink numeric controller differs from Python vectors');
writetable(array2table(sl,'VariableNames',tab.Properties.VariableNames),fullfile(outfolder,'controller_simulink.csv'));
close_system(model,0);
evidence=struct('rows',n,'matlab_python_max_abs_error',matlab_error,...
 'simulink_python_max_abs_error',simulink_error,'tolerance',1e-8,...
 'scope','numerical controller only; not sensor HAL, task timing or hardware');
fid=fopen(fullfile(outfolder,'controller_vector_evidence.json'),'w');fprintf(fid,'%s',jsonencode(evidence,PrettyPrint=true));fclose(fid);
fprintf('Controller vectors (%d): MATLAB %.3g; Simulink %.3g max absolute error.\n',n,matlab_error,simulink_error);
end
