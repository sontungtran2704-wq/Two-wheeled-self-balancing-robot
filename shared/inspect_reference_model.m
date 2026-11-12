function info=inspect_reference_model(outfolder)
% Read-only Simulink metadata inspection: does not execute legacy callbacks.
root=fileparts(fileparts(mfilename('fullpath')));
source=fullfile(root,'00_Nguon','lqr_reference','LQR','matlab','Mophong.slx');
meta=Simulink.MDLInfo(source);
info=struct('path',source,'checked_with',version,'status','METADATA_READ_ONLY',...
 'model_version',meta.ModelVersion,'release',meta.ReleaseName,...
 'reason_not_executed','Legacy DC voltage model and inconsistent parameters; no gain transfer to STEP/DIR.');
fid=fopen(fullfile(outfolder,'reference_slx_inspection.txt'),'w');
fprintf(fid,'%s\n',evalc('disp(meta)'));
fprintf(fid,'No original files changed; callbacks not executed.\n');fclose(fid);
end
