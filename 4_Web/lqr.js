'use strict';
// The old continuous 3-state force solver is preserved under 00_Nguon only.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
function loadDesign() {
  const root = path.join(__dirname, '..', 'shared');
  const d = JSON.parse(fs.readFileSync(path.join(root, 'design.json'), 'utf8'));
  const digest = crypto.createHash('sha256').update(fs.readFileSync(path.join(root, 'robot.json'))).digest('hex');
  if (d.schema_version !== 1 || d.config_sha256 !== digest ||
      JSON.stringify(d.state_order) !== '["theta_rad","omega_rad_s"]' ||
      d.input !== 'relative_tangential_acceleration_m_s2' ||
      !Array.isArray(d.K) || d.K.length !== 2 || !d.K.every(Number.isFinite)) {
    throw new Error('Design/config mismatch: run python tools/design.py then verify MATLAB.');
  }
  return Object.freeze({...d, read_only: false});
}
function calculate(Q,R) {
  if(!Array.isArray(Q)||Q.length!==2||!Q.every(x=>typeof x==='number'&&Number.isFinite(x)&&x>0&&x<=10000)||typeof R!=='number'||!Number.isFinite(R)||R<0.01||R>1000)throw Error('Q gồm hai số >0 đến 10000; R từ 0.01 đến 1000.');
  const d=loadDesign(),A=d.Ad,B=d.Bd.flat();let P=[[Q[0],0],[0,Q[1]]],K;
  for(let n=0;n<20000;n++) {
    const PB=P.map(row=>row[0]*B[0]+row[1]*B[1]),den=R+B[0]*PB[0]+B[1]*PB[1];
    const BPA=[0,1].map(j=>PB[0]*A[0][j]+PB[1]*A[1][j]);K=BPA.map(x=>x/den);
    const N=[0,1].map(i=>[0,1].map(j=>A[0][i]*(P[0][0]*A[0][j]+P[0][1]*A[1][j])+A[1][i]*(P[1][0]*A[0][j]+P[1][1]*A[1][j])-BPA[i]*BPA[j]/den+(i===j?Q[i]:0)));
    const err=Math.max(...N.flat().map((x,i)=>Math.abs(x-P.flat()[i])));P=N;
    if(err<1e-8) {
      const C=A.map((row,i)=>row.map((x,j)=>x-B[i]*K[j])),tr=C[0][0]+C[1][1],det=C[0][0]*C[1][1]-C[0][1]*C[1][0];
      if(!(1-det>0&&1-tr+det>0&&1+tr+det>0))throw Error('Hệ kín rời rạc không ổn định.');
      return {...d,Q,R,K,P,read_only:false,sentToRobot:false};
    }
  }
  throw Error('Bộ giải LQR chưa hội tụ.');
}
module.exports = {loadDesign,calculate};
