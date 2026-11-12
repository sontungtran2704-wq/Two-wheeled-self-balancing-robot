'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const path = require('node:path');
const delay = ms => new Promise(r => setTimeout(r, ms));

test('web rejects live LQR mutation instead of publishing gains', async t => {
  const child = spawn(process.execPath, ['web_server.js'], { cwd: path.join(__dirname, '..'), env: {...process.env, PORT:'13007'}, stdio:'pipe' });
  let output = ''; child.stdout.on('data', b => output += b); child.stderr.on('data', b => output += b);
  t.after(() => child.kill());
  for (let i=0; i<100; i++) {
    try { const r = await fetch('http://127.0.0.1:13007/api/lqr'); if (r.ok) break; } catch {}
    await delay(50);
    if (child.exitCode !== null) assert.fail(output);
  }
  const reply = await fetch('http://127.0.0.1:13007/api/lqr', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({Q:[100,1,10],R:1,send:false})});
  assert.equal(reply.status,409,'LQR profile must be read-only');
  const design = await (await fetch('http://127.0.0.1:13007/api/lqr')).json();
  assert.equal(design.config_id,'nema17-relative-v1');
  assert.deepEqual(design.state_order,['theta_rad','omega_rad_s']);
  const forbidden = await fetch('http://127.0.0.1:13007/api/lqr',{headers:{Origin:'https://evil.invalid'}});
  assert.equal(forbidden.status,403);
});
