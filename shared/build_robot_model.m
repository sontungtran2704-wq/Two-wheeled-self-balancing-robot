function model = build_robot_model(folder,model,c,K)
% Generates a runnable feedback model. Only command schedules are prerecorded.
if bdIsLoaded(model), close_system(model,0); end
new_system(model); load_system('simulink');
set_param(model,'Solver','FixedStepDiscrete','FixedStep',num2str(c.timing.lqr_s,17),...
    'StopTime','12','ReturnWorkspaceOutputs','on','SaveOutput','off',...
    'SaveTime','on','TimeSaveName','tout','SignalLogging','off');
set_param(model,'InitFcn',"addpath(fullfile(fileparts(fileparts(get_param(bdroot,'FileName'))),'shared'));");
ws=get_param(model,'ModelWorkspace'); assignin(ws,'cfg',c); assignin(ws,'gain',K);
assignin(ws,'initial_state',zeros(6,1));
assignin(ws,'reference_input',[0 0 0 0 1 0;12 0 0 0 1 0]);
add_block('simulink/Sources/From Workspace',[model '/Command_and_disturbance'],...
    'VariableName','reference_input','Interpolate','off','OutputAfterFinalValue','Holding final value','Position',[30 40 185 80]);
add_block('simulink/User-Defined Functions/Level-2 MATLAB S-Function',[model '/LQR_and_discrete_PID'],...
    'FunctionName','robot_controller_sfun','Parameters','cfg,gain','Position',[270 150 465 230]);
add_block('simulink/User-Defined Functions/Level-2 MATLAB S-Function',[model '/Nonlinear_RK4_plant'],...
    'FunctionName','robot_plant_sfun','Parameters','cfg,initial_state','Position',[560 150 745 230]);
add_block('simulink/Sinks/To Workspace',[model '/Plant_log'],...
    'VariableName','plant_log','SaveFormat','Timeseries','Position',[810 145 920 175]);
add_block('simulink/Sinks/To Workspace',[model '/Controller_log'],...
    'VariableName','controller_log','SaveFormat','Timeseries','Position',[480 300 600 330]);
add_line(model,'Command_and_disturbance/1','LQR_and_discrete_PID/2','autorouting','on');
add_line(model,'Command_and_disturbance/1','Nonlinear_RK4_plant/2','autorouting','on');
add_line(model,'LQR_and_discrete_PID/1','Nonlinear_RK4_plant/1','autorouting','on');
add_line(model,'LQR_and_discrete_PID/1','Controller_log/1','autorouting','on');
add_line(model,'Nonlinear_RK4_plant/1','LQR_and_discrete_PID/1','autorouting','on');
add_line(model,'Nonlinear_RK4_plant/1','Plant_log/1','autorouting','on');
note=Simulink.Annotation(model,sprintf(['Plant: theta, omega, q, x, yaw rate, yaw angle\n' ...
    'u = r * relative wheel acceleration; Ts = %.3f s\n' ...
    'Separate controller / nonlinear plant, no prerecorded plant outputs'],c.timing.lqr_s));
note.Position=[280 30];
save_system(model,fullfile(folder,[model '.slx']));
end
