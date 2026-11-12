'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const path = require('node:path');
const delay = ms => new Promise(r => setTimeout(r, ms));

test('CALGYRO distinguishes no link, sent, ACK, rejection and silence', async t => {
  const net = require('node:net');
  const {EventEmitter} = require('node:events');
  const {createBridge} = require('../web_server');
  const bridge = createBridge({port:13009});
  const socket = new EventEmitter(); socket.id = 'calibration-test';
  const notices=[], errors=[], commands=[]; let stream;
  socket.on('notice', n => notices.push(n));
  socket.on('serialError', e => errors.push(e));
  // Exercise the actual socket command handler without a physical serial port.
  bridge.io.listeners('connection')[0](socket);
  const fake = net.createServer(s => {
    stream=s; let buffer='';
    s.on('data', b => {buffer+=b; let n; while((n=buffer.indexOf('\n'))>=0){commands.push(buffer.slice(0,n));buffer=buffer.slice(n+1);}});
  });
  await new Promise(r=>fake.listen(0,'127.0.0.1',r));
  await bridge.listen();
  t.after(async()=>{stream?.destroy();await bridge.close();await new Promise(r=>fake.close(r));});
  const send=()=>socket.emit('robotControl',{command:'calgyro'});
  send(); assert.match(notices.at(-1).msg,/Không gửi được/);
  const response=await fetch('http://127.0.0.1:13009/api/connect',{
    method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({port:`tcp:127.0.0.1:${fake.address().port}`})});
  assert.equal(response.status,200);
  send(); assert.match(notices.at(-1).msg,/Bridge đã gửi/);
  send(); assert.match(errors.at(-1),/không gửi lặp/);
  await delay(50); assert.equal(commands.filter(c=>c==='CALGYRO').length,1);
  stream.write('ACK:CALGYRO_PENDING\n'); await delay(50);
  assert.match(notices.at(-1).msg,/ACK chưa có nghĩa hiệu chuẩn đạt/);
  send(); stream.write('ERR:MOTORS_ON\n'); await delay(50);
  assert.match(notices.at(-1).msg,/từ chối/);
  send(); await delay(2200);
  assert.match(notices.at(-1).msg,/chưa nhận ACK/);
  assert.equal(notices.at(-1).type,'error');
  const status=await (await fetch('http://127.0.0.1:13009/api/status')).json();
  assert.equal(status.connected,true); assert.equal(status.fresh,false);
  assert.equal(status.telemetry,null,'ACK must never manufacture telemetry');
  send(); await fetch('http://127.0.0.1:13009/api/disconnect',{method:'POST'});
  assert.match(notices.at(-1).msg,/đã đóng trước/);
});

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
