function robot_plant_sfun(block)
block.NumDialogPrms=2;
block.NumInputPorts=2; block.NumOutputPorts=1;
block.SetPreCompInpPortInfoToDynamic; block.SetPreCompOutPortInfoToDynamic;
block.InputPort(1).Dimensions=12; block.InputPort(2).Dimensions=5;
for k=1:2, block.InputPort(k).DirectFeedthrough=false; end
block.OutputPort(1).Dimensions=6;
block.SampleTimes=[block.DialogPrm(1).Data.timing.lqr_s 0];
block.SimStateCompliance='DefaultSimState';
block.RegBlockMethod('PostPropagationSetup',@post);
block.RegBlockMethod('InitializeConditions',@init);
block.RegBlockMethod('Outputs',@outputs);
block.RegBlockMethod('Update',@update);
end
function post(b)
b.NumDworks=1; b.Dwork(1).Name='physical_state'; b.Dwork(1).Dimensions=6;
b.Dwork(1).DatatypeID=0; b.Dwork(1).Complexity='Real'; b.Dwork(1).UsedAsDiscState=true;
end
function init(b)
b.Dwork(1).Data=b.DialogPrm(2).Data(:);
end
function outputs(b)
b.OutputPort(1).Data=b.Dwork(1).Data;
end
function update(b)
c=b.DialogPrm(1).Data; dt=c.timing.lqr_s;
s=b.Dwork(1).Data; u=b.InputPort(1).Data; ref=b.InputPort(2).Data;
p=c.model;if isfield(c,'plant_model'),p=c.plant_model;end
s(1:4)=robot_rk4(s(1:4),u(1),p,dt,ref(5));
s(6)=s(6)+dt*s(5)+.5*dt^2*u(2); s(5)=s(5)+dt*u(2);
b.Dwork(1).Data=s;
end
