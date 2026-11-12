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
  return Object.freeze({...d, read_only: true});
}
module.exports = {loadDesign};
