'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const path = require('node:path');
const fs=require('node:fs'),os=require('node:os');
async function login(port){
 const args={method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({username:'testuser',password:'testpass123'})};
 assert.equal((await fetch(`http://127.0.0.1:${port}/api/auth/register`,args)).status,201);
 const r=await fetch(`http://127.0.0.1:${port}/api/auth/login`,args);assert.equal(r.status,200);return r.headers.get('set-cookie').split(';')[0];
}
const delay = ms => new Promise(r => setTimeout(r, ms));

test('CALGYRO distinguishes no link, sent, ACK, rejection and silence', async t => {
  const net = require('node:net');
  const {EventEmitter} = require('node:events');
  const {createBridge} = require('../web_server');
  const bridge = createBridge({port:13009,authFile:path.join(fs.mkdtempSync(path.join(os.tmpdir(),'robot-auth-')),'users.json')});
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
  const cookie=await login(13009);
  const send=()=>socket.emit('robotControl',{command:'calgyro'});
  send(); assert.match(notices.at(-1).msg,/Không gửi được/);
  const response=await fetch('http://127.0.0.1:13009/api/connect',{
    method:'POST',headers:{'Content-Type':'application/json',Cookie:cookie},
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
  const status=await (await fetch('http://127.0.0.1:13009/api/status',{headers:{Cookie:cookie}})).json();
  assert.equal(status.connected,true); assert.equal(status.fresh,false);
  assert.equal(status.telemetry,null,'ACK must never manufacture telemetry');
  send(); await fetch('http://127.0.0.1:13009/api/disconnect',{method:'POST',headers:{Cookie:cookie}});
  assert.match(notices.at(-1).msg,/đã đóng trước/);
});

test('web authenticates and calculates the current two-state LQR', async t => {
  const child = spawn(process.execPath, ['web_server.js'], { cwd: path.join(__dirname, '..'), env: {...process.env, PORT:'13007',ROBOT_AUTH_FILE:path.join(fs.mkdtempSync(path.join(os.tmpdir(),'robot-auth-')),'users.json')}, stdio:'pipe' });
  let output = ''; child.stdout.on('data', b => output += b); child.stderr.on('data', b => output += b);
  t.after(() => child.kill());
  for (let i=0; i<100; i++) {
    try { const r = await fetch('http://127.0.0.1:13007/api/health'); if (r.ok) break; } catch {}
    await delay(50);
    if (child.exitCode !== null) assert.fail(output);
  }
  assert.equal((await fetch('http://127.0.0.1:13007/api/lqr')).status,401);
  const cookie=await login(13007);
  const reply = await fetch('http://127.0.0.1:13007/api/lqr', {method:'POST',headers:{'Content-Type':'application/json',Cookie:cookie},body:JSON.stringify({Q:[100,1,10],R:1,send:false})});
  assert.equal(reply.status,400,'legacy three-state Q must be rejected');
  const calculated=await (await fetch('http://127.0.0.1:13007/api/lqr',{method:'POST',headers:{'Content-Type':'application/json',Cookie:cookie},body:JSON.stringify({Q:[400,4],R:1})})).json();
  assert(Math.abs(calculated.K[0]+23.793396834761598)<1e-6);
  assert(Math.abs(calculated.K[1]+2.7542335398052704)<1e-6);
  const design = await (await fetch('http://127.0.0.1:13007/api/lqr',{headers:{Cookie:cookie}})).json();
  assert.equal(design.config_id,'nema17-relative-v1');
  assert.deepEqual(design.state_order,['theta_rad','omega_rad_s']);
  const forbidden = await fetch('http://127.0.0.1:13007/api/lqr',{headers:{Origin:'https://evil.invalid'}});
  assert.equal(forbidden.status,403);
});

test('permission failure is reported once and does not retry', async t => {
  const net=require('node:net');
  const {EventEmitter}=require('node:events');
  const {createBridge}=require('../web_server');
  const bridge=createBridge({port:13018});
  const socket=new EventEmitter();socket.id='permission-test';
  bridge.io.listeners('connection')[0](socket);
  const errors=[];socket.on('serialError',e=>errors.push(e));
  let attempts=0;
  t.mock.method(net,'createConnection',()=>{
    attempts++;
    const s=new EventEmitter();s.setNoDelay=()=>{};s.destroy=()=>s.emit('close');
    setImmediate(()=>{const e=new Error('connect EACCES');e.code='EACCES';s.emit('error',e);s.emit('close');});
    return s;
  });
  const broadcasts=[];t.mock.method(bridge.io,'emit',(name,...args)=>broadcasts.push([name,...args]));
  t.after(()=>bridge.close());
  socket.emit('connectSerial',{port:'tcp:192.168.4.1:8080'});
  await delay(1200);
  assert.equal(attempts,1);
  assert.equal(errors.length,1);
  assert.match(errors[0],/Mo_Web.cmd/);
  assert.equal(broadcasts.filter(x=>x[0]==='serialError').length,0);
});

test('accounts hash passwords, reject duplicates and invalidate logout sessions',async t=>{
 const {createBridge}=require('../web_server');
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'robot-account-')),file=path.join(dir,'users.json');
 let bridge=createBridge({port:13019,authFile:file});await bridge.listen();t.after(()=>bridge.close());
 const request=(route,body,cookie)=>fetch('http://127.0.0.1:13019'+route,{method:body?'POST':'GET',headers:{'Content-Type':'application/json',Connection:'close',...(cookie?{Cookie:cookie}:{})},...(body?{body:JSON.stringify(body)}:{})});
 const data={username:'member',password:'Password123'};
 const r=await request('/api/auth/register',data);assert.equal(r.status,201);
 assert.match(r.headers.get('set-cookie'),/Max-Age=0/);
 assert.equal((await request('/api/status')).status,401,'registration must not grant access');
 const logged=await request('/api/auth/login',data);
 const cookie=logged.headers.get('set-cookie').split(';')[0];assert.match(logged.headers.get('set-cookie'),/HttpOnly/);
 assert(!fs.readFileSync(file,'utf8').includes(data.password));
 assert.equal((await request('/api/auth/register',data)).status,409);
 assert.equal((await request('/api/auth/login',{...data,password:'wrongpassword'})).status,401);
 assert.equal((await request('/api/status',null,cookie)).status,200);
 await request('/api/auth/logout',{},cookie);
 assert.equal((await request('/api/status',null,cookie)).status,401);
 await bridge.close();bridge=createBridge({port:13019,authFile:file});await bridge.listen();
 assert.equal((await request('/api/auth/login',data)).status,200,'account must survive server restart');
});
