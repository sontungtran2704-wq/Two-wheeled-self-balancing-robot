'use strict';
// Adapted from source 4_Web: Express/Socket.IO, USB Serial and Wi-Fi TCP.
// New: versioned read-only gains, bounded lines, owner/lease and strict telemetry.
const express = require('express');
const http = require('node:http');
const net = require('node:net');
const path = require('node:path');
const {Server} = require('socket.io');
const {SerialPort} = require('serialport');
const {loadDesign} = require('./lqr');
const TCP_RE = /^tcp:([A-Za-z0-9][A-Za-z0-9.-]{0,62}):(\d{1,5})$/;
const MAX_COMMAND = 160, MAX_TELEMETRY = 1024, STALE_MS = 250;
const finite = (n, max) => typeof n === 'number' && Number.isFinite(n) && Math.abs(n) <= max;

function createBridge(options = {}) {
  const port = Number(options.port ?? process.env.PORT ?? 3000);
  const host = options.host ?? process.env.HOST ?? '127.0.0.1';
  const origins = new Set([`http://localhost:${port}`, `http://127.0.0.1:${port}`,
    ...(process.env.ALLOWED_ORIGINS || '').split(',').filter(Boolean)]);
  const hosts = new Set([...origins].map(x => new URL(x).host));
  const allowed = req => hosts.has(req.headers.host) && (!req.headers.origin || origins.has(req.headers.origin));
  const app = express(), server = http.createServer(app);
  const io = new Server(server, {allowRequest: (req, cb) => cb(null, allowed(req)), maxHttpBufferSize: 2048});
  const design = loadDesign();
  let link = null, wanted = null, retry = null, retryCount = 0, generation = 0;
  let latest = null, lastTelemetry = 0, owner = null, sequence = 0;
  let desired = {v: 0, w: 0}, lastDrive = 0, motionRequested = false;
  const pid = {};
  let malformed = 0, shuttingDown = false;
  let calibrationAck = null;
  function finishCalibrationAck(message, type = 'error') {
    if (!calibrationAck) return;
    calibrationAck.socket.emit('notice', {msg: message, type});
    calibrationAck = null;
  }
  function fresh() { return latest && Date.now() - lastTelemetry <= STALE_MS; }
  function matching() { return fresh() && latest.config === design.config_id && latest.hash === design.config_sha256; }
  function write(line) {
    if (typeof line !== 'string' || line.length > MAX_COMMAND || /[\r\n]/.test(line)) return false;
    if (!link?.open || link.stream.destroyed || link.stream.writableLength > 1024) return false;
    link.stream.write(line + '\n'); return true;
  }
  function zero() {
    if (matching() && latest.st === 2 && owner) sendPair(0, 0);
    desired = {v: 0, w: 0}; lastDrive = 0;
  }
  function revoke() { zero(); owner = null; motionRequested = false; io.emit('controlOwner', null); }
  function resetTelemetry() { latest = null; lastTelemetry = 0; sequence = 0; desired = {v:0,w:0}; lastDrive=0; motionRequested=false; }
  function dashboardData(d) {
    return {...d, t:d.t/1000, thetaDot:d.omega, xDot:d.v, x:null,
      u:d.u??null, relativeAcceleration:d.u, st:d.st===3?(d.fault==='FALL'?3:4):d.st};
  }
  function sendPair(v, w) {
    sequence = (sequence + 1) >>> 0;
    return write(`CMD,${latest.session},${sequence},${v.toFixed(6)},${w.toFixed(6)}`);
  }
  function error(socket, msg) { socket.emit('serialError', msg); }
  function claim(socket) {
    if (owner && owner !== socket.id) { error(socket, 'Một dashboard khác đang giữ quyền điều khiển.'); return false; }
    owner = socket.id; io.emit('controlOwner', owner); return true;
  }
  function authority(socket, running = false) {
    if (owner !== socket.id) { error(socket, 'Bấm START để nhận quyền điều khiển.'); return false; }
    if (!matching()) { error(socket, 'Telemetry cũ hoặc cấu hình firmware không khớp.'); return false; }
    if (running && latest.st !== 2) { error(socket, 'Xe chưa ở trạng thái RUN.'); return false; }
    return true;
  }
  function emitState() { io.emit('linkStatus', {connected: !!link?.open, fresh: !!fresh(), matching: !!matching(), malformed, port: link?.name || null}); }
  function processLine(line) {
    if (!line || line.length > MAX_TELEMETRY) { malformed++; return; }
    const wifi=/^EVT:WIFI,(AP|STA),([0-9.]+)$/.exec(line);
    if(wifi){io.emit('robotWifi',{mode:wifi[1],ip:wifi[2]});return;}
    if (/^(ACK:|ERR:|EVT:)/.test(line)) {
      if (line === 'ACK:CALGYRO_PENDING')
        finishCalibrationAck('Firmware đã nhận CALGYRO; giữ xe yên và chờ kết quả IMU. ACK chưa có nghĩa hiệu chuẩn đạt.', 'info');
      else if (['ERR:MOTORS_ON','ERR:PINMAP_UNCONFIRMED'].includes(line))
        finishCalibrationAck(`Firmware từ chối hiệu chuẩn: ${line}`);
      io.emit('serialMessage', line); return;
    }
    const pm = /^PID:([VY]),([^,]+),([^,]+),([^,]+)$/.exec(line);
    if (pm) {
      const a = pm.slice(2).map(Number);
      if (a.every(Number.isFinite)) { pid[pm[1]] = {axis:pm[1],kp:a[0],ki:a[1],kd:a[2]}; if(pm[1]==='V')io.emit('pidUpdated', pid.V); }
      else malformed++;
      return;
    }
    if (!line.startsWith('t=')) { malformed++; return; }
    const d = {};
    for (const part of line.split(',')) {
      const eq = part.indexOf('=');
      if (eq <= 0) { malformed++; return; }
      const key = part.slice(0, eq), value = part.slice(eq + 1);
      if (Object.hasOwn(d,key) || !value) { malformed++; return; }
      d[key] = ['config','hash','fault'].includes(key) ? value : Number(value);
    }
    const required = ['t','theta','omega','v','yaw','left','right','st','motion','session','seq','fresh','sat'];
    if (!required.every(k => Number.isFinite(d[k])) || ![0,1,2,3].includes(d.st) ||
        !Number.isInteger(d.session) || d.session < 0 || d.session > 4294967295 ||
        !Number.isInteger(d.seq) || d.seq < 0 || d.seq > 4294967295 ||
        !/^[a-f0-9]{64}$/.test(d.hash || '') || typeof d.config !== 'string' ||
        ['u','jitter_us','overruns'].some(k => k in d && !Number.isFinite(d[k]))) { malformed++; return; }
    if (latest && d.session === latest.session) {
      const advance = (d.seq - latest.seq) >>> 0;
      if (advance === 0 || advance >= 0x80000000) { malformed++; return; }
    }
    if (latest && d.session !== latest.session) { desired={v:0,w:0};lastDrive=0;sequence=0;motionRequested=false; }
    latest = d; lastTelemetry = Date.now();
    if (d.st === 3) { owner = null; desired={v:0,w:0}; lastDrive=0; io.emit('controlOwner',null); }
    io.emit('serialData', dashboardData(d)); emitState();
  }
  function attach(stream, mine) {
    let buffer = '', dropping = false;
    stream.on('data', chunk => {
      if (link !== mine) return;
      for (const ch of chunk.toString('utf8')) {
        if (ch === '\n') {
          if (!dropping) processLine(buffer.replace(/\r$/, ''));
          buffer='';dropping=false;
        } else if (!dropping) {
          buffer += ch;
          if (buffer.length > MAX_TELEMETRY) { buffer='';dropping=true;malformed++; }
        }
      }
    });
  }
  function closeLink() {
    finishCalibrationAck('Đường truyền đã đóng trước khi nhận ACK hiệu chuẩn; chưa biết firmware có nhận lệnh hay không.');
    const old = link; link = null;
    resetTelemetry();
    if (old) { old.open=false; return old.close(); }
  }
  function reconnect() {
    if (!wanted || retry || shuttingDown || retryCount >= 5) return;
    retry=setTimeout(async()=>{
      retry=null;
      if (!wanted) return;
      try { await open(wanted.port,wanted.baudRate);retryCount=0; }
      catch { reconnect(); }
    },Math.min(1000*2**retryCount++,8000));
  }
  function connectionError(e) {
    if (e.code === 'EACCES' || e.code === 'EPERM') {
      // Permission failures will not improve with a reconnect loop.
      wanted=null;clearTimeout(retry);retry=null;
      return new Error('EACCES/EPERM: tiến trình server bị từ chối quyền mở kết nối. Đóng đúng server dự án rồi mở lại bằng 4_Web/Mo_Web.cmd từ File Explorer. Nếu dùng 192.168.4.1:8080, máy cần nối Wi-Fi BalanceBot-Bench.');
    }
    return e;
  }
  async function open(address, baudRate = 115200) {
    if (typeof address !== 'string' || address.length > 128 || /[\r\n]/.test(address)) throw Error('Cổng không hợp lệ.');
    if (!Number.isInteger(baudRate) || ![115200,230400,460800,921600].includes(baudRate)) throw Error('Baud không hợp lệ.');
    if (link?.name===address && link.baudRate===baudRate && link.open) {
      write('GET'); emitState(); return;
    }
    const token=++generation;
    revoke(); await closeLink();
    if(token!==generation)throw Error('Kết nối đã hủy.');
    const match = TCP_RE.exec(address);
    let stream;
    if (address.startsWith('tcp:')) {
      if (!match || Number(match[2])<1 || Number(match[2])>65535) throw Error('Dùng tcp:host:port.');
      stream=net.createConnection({host:match[1],port:Number(match[2])});stream.setNoDelay(true);
    } else stream=new SerialPort({path:address,baudRate,autoOpen:false});
    const mine={stream,name:address,baudRate,open:false,close:()=>{
      if(match || !stream.isOpen) {stream.destroy();return;}
      return new Promise(resolve=>stream.close(resolve));
    }};
    link=mine;attach(stream,mine);
    return new Promise((resolve,reject)=>{
      let settled=false;
      const timeout=setTimeout(()=>{ stream.destroy(new Error('Hết thời gian kết nối 5 s.')); },5000);
      function fail(e){ if(!settled){settled=true;clearTimeout(timeout);reject(e);} }
      stream.on(match?'connect':'open',()=>{
        if(token!==generation){mine.close();fail(new Error('Kết nối đã hủy.'));return;}
        mine.open=true;settled=true;clearTimeout(timeout);io.emit('serialConnected',{port:address});write('GET');emitState();resolve();
      });
      stream.on('error',e=>{
        const opened=mine.open, cause=connectionError(e);
        fail(cause);
        // Initial open failures are reported by the caller; avoid duplicate toasts.
        if(opened)io.emit('serialError',cause.message);
      });
      stream.on('close',()=>{
        fail(new Error('Kết nối đã đóng.'));
        if(link!==mine)return;
        finishCalibrationAck('Mất đường truyền trước ACK hiệu chuẩn; chưa xác nhận firmware nhận lệnh.');
        owner=null;link=null;resetTelemetry();io.emit('serialDisconnected');io.emit('controlOwner',null);emitState();reconnect();
      });
      if(!match)stream.open();
    });
  }
  async function connectUser(address,baudRate){
    wanted=null;clearTimeout(retry);retry=null;retryCount=0;
    await open(address,baudRate); wanted={port:address,baudRate};
  }
  function disconnect(){wanted=null;clearTimeout(retry);retry=null;generation++;revoke();closeLink();io.emit('serialDisconnected');emitState();}
  app.use((req,res,next)=>allowed(req)?next():res.status(403).json({error:'Forbidden origin/host'}));
  app.use(express.json({limit:'2kb'}));
  app.use(express.static(path.join(__dirname,'public')));
  app.use('/vendor/chart',express.static(path.dirname(require.resolve('chart.js'))));
  app.get('/api/lqr',(req,res)=>res.json(design));
  app.post('/api/lqr',(req,res)=>res.status(409).json({error:'READ_ONLY_GAIN',message:'Tính và kiểm chứng lại gain bằng MATLAB; web chỉ đọc cấu hình đã xuất.'}));
  app.get('/api/status',(req,res)=>res.json({connected:!!link?.open,fresh:!!fresh(),matching:!!matching(),malformed,telemetry:latest,owner}));
  app.get('/api/ports',async(req,res)=>{
    try {res.json([{path:'tcp:192.168.4.1:8080',manufacturer:'Wi-Fi BalanceBot-Bench'},...(await SerialPort.list()).map(p=>({path:p.path,manufacturer:p.manufacturer||''}))]);}
    catch(e){res.status(500).json({error:e.message});}
  });
  app.post('/api/connect',async(req,res)=>{
    try{await connectUser(req.body?.port,req.body?.baudRate??115200);res.json({success:true});}
    catch(e){res.status(400).json({error:e.message});}
  });
  app.post('/api/disconnect',(req,res)=>{disconnect();res.json({success:true});});
  io.on('connection',socket=>{
    socket.emit('lqrUpdated',design);if(pid.V)socket.emit('pidUpdated',pid.V);
    if(latest)socket.emit('serialData',dashboardData(latest));emitState();socket.emit('controlOwner',owner);
    socket.on('connectSerial',async(p={})=>{try{await connectUser(p.port,p.baudRate??115200);}catch(e){error(socket,e.message);}});
    socket.on('disconnectSerial',disconnect);
    socket.on('robotControl',(p={})=>{
      const cmd=({start:'START',stop:'STOP',estop:'ESTOP',reset:'RESET',get:'GET',calgyro:'CALGYRO',
        zphere:'ZPHERE','zp+':'ZP+','zp-':'ZP-',save:'SAVE'})[p.command];
      if(!cmd)return error(socket,'Lệnh không hợp lệ.');
      if(cmd==='CALGYRO') {
        if(owner && owner!==socket.id)return error(socket,'Dashboard khác đang điều khiển.');
        if(fresh() && [1,2].includes(latest.st))return error(socket,'Dừng motor trước khi hiệu chuẩn.');
        if(calibrationAck)return error(socket,'Đang chờ ACK CALGYRO; không gửi lặp.');
        // Register before write: a fast reply must not race pending state.
        calibrationAck={socket, deadline:Date.now()+2000};
        if(!write(cmd))return finishCalibrationAck('Không gửi được CALGYRO: đường truyền chưa mở hoặc đang bận.');
        socket.emit('notice',{msg:'Bridge đã gửi CALGYRO; đang chờ ACK từ firmware.',type:'info'});
        return;
      }
      if(['STOP','ESTOP'].includes(cmd)){write(cmd);owner=null;desired={v:0,w:0};lastDrive=0;motionRequested=false;io.emit('controlOwner',null);return;}
      if(cmd==='SAVE'){
        if(!matching() || ![0,3].includes(latest.st))return error(socket,'Dừng motor trước khi lưu PID và điểm zero.');
        if(owner&&owner!==socket.id)return error(socket,'Dashboard khác đang điều khiển.');
      }
      if(['ZPHERE','ZP+','ZP-'].includes(cmd)){
        if(!matching())return error(socket,'Chưa đặt zero: cần telemetry mới và cấu hình trùng khớp.');
        if(latest.st!==0)return error(socket,'Chưa đặt zero: kê xe chắc, bấm RESET lỗi → Chờ trước. RESET không tự bật motor.');
        if(latest.imu_calibrated!==1)return error(socket,'Chưa đặt zero: giữ xe đứng yên và hiệu chuẩn gyro trước.');
        if(owner&&owner!==socket.id)return error(socket,'Dashboard khác đang điều khiển.');
      }
      if(cmd==='START'){
        if(!matching())return error(socket,'Cần telemetry mới và cấu hình trùng khớp trước START.');
        if(latest.hardware_confirmed===0)
          return error(socket,'Firmware đang khóa motor. Kiểm tra EN của cả hai A4988 trước khi mở khóa.');
        if(latest.imu_calibrated===0)
          return error(socket,'MPU6050 chưa hiệu chuẩn. Giữ xe yên và bấm Hiệu chuẩn gyro.');
        if(!claim(socket))return;
      } else if(['RESET','CALGYRO'].includes(cmd) && owner && owner!==socket.id) return error(socket,'Dashboard khác đang điều khiển.');
      if(!write(cmd))error(socket,'Chưa kết nối tới xe.');
    });
    socket.on('setMotion',value=>{if(typeof value!=='boolean')return error(socket,'MOTION phải boolean.');if(authority(socket)){zero();write(`MOTION,${+value}`);}});
    socket.on('drive',(p={})=>{
      if(!finite(p.v,.2)||!finite(p.w,.8))return error(socket,'Giới hạn |v|≤0.2 m/s, |yaw|≤0.8 rad/s.');
      if(!authority(socket,true))return;
      desired={v:p.v,w:p.w};lastDrive=Date.now();if(!sendPair(p.v,p.w))error(socket,'Đường truyền bận; lệnh không gửi.');
    });
    socket.on('release',()=>{if(owner===socket.id)zero();});
    for(const [event,key,max,scale] of [['setSpeed','v',20,.01],['setTurn','w',.8*180/Math.PI,Math.PI/180]]){
      socket.on(event,value=>{
        if(!authority(socket,true))return;
        if(!finite(value,max)){
          if(desired[key]!==0) { desired[key]=0; sendPair(desired.v,desired.w); }
          return;
        }
        if(Date.now()-lastDrive>STALE_MS)desired={v:0,w:0};
        if(value!==0&&!motionRequested){
          if(!write('MOTION,1'))return error(socket,'Không gửi được lệnh bật PID chuyển động.');
          motionRequested=true;
        }
        desired[key]=value*scale;lastDrive=Date.now();sendPair(desired.v,desired.w);
      });
    }
    socket.on('setPid',(p={})=>{
      const axis=p.axis??'V';
      if(!['V','Y'].includes(axis)||![p.kp,p.ki,p.kd].every((v,i)=>finite(v,[5,2,.2][i])&&v>=0))return error(socket,'PID: axis V/Y, Kp 0..5, Ki 0..2, Kd 0..0.2.');
      if(!matching()||![0,3].includes(latest.st))return error(socket,'Chỉ đổi PID khi động cơ tắt và telemetry mới.');
      if(owner&&owner!==socket.id)return error(socket,'Dashboard khác đang điều khiển.');
      write(`$P,${axis},${p.kp},${p.ki},${p.kd}`);write('GET');
    });
    socket.on('setWifi',()=>error(socket,'Wi-Fi được cấu hình trong firmware hiện tại.'));
    for(const event of ['sendK','updateLQR'])socket.on(event,()=>socket.emit('lqrError','READ_ONLY_GAIN: không gửi $K của mô hình cũ.'));
    socket.on('disconnect',()=>{if(owner===socket.id)revoke();});
  });
  const watchdog=setInterval(()=>{
    if(calibrationAck && Date.now()>=calibrationAck.deadline)
      finishCalibrationAck('Đã gửi CALGYRO nhưng chưa nhận ACK sau 2 giây. Kiểm tra cổng, baud 115200 và firmware/USB CDC; chưa thể kết luận hiệu chuẩn thất bại.');
    if(lastDrive&&Date.now()-lastDrive>STALE_MS){desired={v:0,w:0};lastDrive=0;}
    if(owner&&!fresh()){owner=null;desired={v:0,w:0};lastDrive=0;io.emit('controlOwner',null);}
    emitState();
  },100);
  async function listen(){await new Promise(resolve=>server.listen(port,host,resolve));return server.address();}
  async function close(){shuttingDown=true;clearInterval(watchdog);disconnect();await new Promise(resolve=>io.close(resolve));}
  return {app,server,io,listen,close,processLine,design};
}
if(require.main===module){
  const bridge=createBridge();bridge.listen().then(a=>console.log(`BalanceBot web http://${a.address}:${a.port}`));
  for(const signal of ['SIGINT','SIGTERM'])process.on(signal,()=>bridge.close().then(()=>process.exit(0)));
}
module.exports={createBridge};
