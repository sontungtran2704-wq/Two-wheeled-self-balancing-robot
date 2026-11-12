function robot_controller_sfun(block)
block.NumDialogPrms=2;
block.NumInputPorts=2; block.NumOutputPorts=1;
block.SetPreCompInpPortInfoToDynamic; block.SetPreCompOutPortInfoToDynamic;
block.InputPort(1).Dimensions=6; block.InputPort(2).Dimensions=5;
for k=1:2, block.InputPort(k).DirectFeedthrough=true; end
block.OutputPort(1).Dimensions=12;
block.SampleTimes=[block.DialogPrm(1).Data.timing.lqr_s 0];
block.SimStateCompliance='DefaultSimState';
block.RegBlockMethod('PostPropagationSetup',@post);
block.RegBlockMethod('InitializeConditions',@init);
block.RegBlockMethod('Outputs',@outputs);
block.RegBlockMethod('Update',@update);
end
function post(b)
b.NumDworks=1; b.Dwork(1).Name='controller_state'; b.Dwork(1).Dimensions=21;
b.Dwork(1).DatatypeID=0; b.Dwork(1).Complexity='Real'; b.Dwork(1).UsedAsDiscState=true;
end
function init(b)
b.Dwork(1).Data=zeros(21,1);
end
function [y,z]=step(b)
c=b.DialogPrm(1).Data; s=b.InputPort(1).Data; ref=b.InputPort(2).Data;
in=[s(1);s(2);s(3)+c.model.r*s(2);s(5);ref(1:4)];
[y,z]=robot_controller_step(c,b.DialogPrm(2).Data,b.Dwork(1).Data,in);
end
function outputs(b)
[y,~]=step(b); b.OutputPort(1).Data=y;
end
function update(b)
[~,z]=step(b); b.Dwork(1).Data=z;
end
