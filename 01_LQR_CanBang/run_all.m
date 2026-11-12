% Run from any working directory with run('01_LQR_CanBang/run_all.m').
here=fileparts(mfilename('fullpath')); root=fileparts(here);
addpath(fullfile(root,'shared')); addpath(here); run_lqr();
