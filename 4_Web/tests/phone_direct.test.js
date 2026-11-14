'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),http=require('node:http'),zlib=require('node:zlib'),crypto=require('node:crypto');
const {calculate}=require('../lqr');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'C:/Users/DELL/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
test('phone cockpit: two-finger combined driving, landscape, return, charts/exports/settings and lost Wi-Fi',async t=>{
 const root=path.join(__dirname,'../..'),out=path.join(root,'06_Kiem_tra/phone_auth_20261114');
 const html=zlib.gunzipSync(fs.readFileSync(path.join(root,'.build/phone-page.html.gz'))).toString(),design=require('../../shared/design.json');
 const welcome=zlib.gunzipSync(fs.readFileSync(path.join(root,'.build/phone-welcome.html.gz'))).toString(),users=new Map();let logged=false,granted=false,session=41;
 const hash=password=>crypto.pbkdf2Sync(password,'test-salt',20000,32,'sha256').toString('hex');
 for(const f of ['style.css','dashboard.js','button-feedback.js'])assert(html.includes(fs.readFileSync(path.join(root,'4_Web/public',f),'utf8')),f+' must be shared exactly');
 let st=0,cal=0,offline=false,delayStatus=0,delayDrive=0,holdLqr=false,reply='',replyId=0,tick=0,K=design.K.slice(),pid=[3,1,.2];
 const commands=[],errors=[],external=[],say=line=>{reply=line;replyId++;};
 const server=http.createServer((req,res)=>{
  res.setHeader('Cache-Control','no-store');
  const authenticated=logged&&(req.headers.cookie||'').includes('bb_phone=test-cookie');
  if(req.url==='/'||req.url==='/welcome'){res.setHeader('Content-Type','text/html; charset=utf-8');res.end(req.url==='/'&&authenticated?html:welcome);return;}
  if(req.url==='/api/auth/me'){res.setHeader('Content-Type','application/json');res.end(JSON.stringify({username:authenticated?'phoneuser':null}));return;}
  if(req.url==='/api/auth/register'||req.url==='/api/auth/login'){
   let body='';req.on('data',b=>body+=b);req.on('end',()=>{const [name,password]=body.split('\n'),username=(name||'').toLowerCase();res.setHeader('Content-Type','application/json');
    if(req.url.endsWith('register')){if(st===2){res.writeHead(409).end(JSON.stringify({error:'Dừng motor trước khi lưu tài khoản.'}));return;}if(users.has(username)){res.writeHead(409).end(JSON.stringify({error:'Tên tài khoản đã tồn tại.'}));return;}users.set(username,hash(password));logged=false;res.setHeader('Set-Cookie','bb_phone=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0');res.writeHead(201).end(JSON.stringify({registered:true}));}
    else if(users.get(username)!==hash(password)){res.writeHead(401).end(JSON.stringify({error:'Sai tài khoản hoặc mật khẩu.'}));}
    else{logged=true;granted=false;res.setHeader('Set-Cookie','bb_phone=test-cookie; HttpOnly; SameSite=Strict; Path=/');res.end(JSON.stringify({username}));}
   });return;
  }
  if(req.url==='/icons/icon.svg'){res.setHeader('Content-Type','image/svg+xml');res.end('<svg xmlns="http://www.w3.org/2000/svg"/>');return;}
  if(req.url==='/manifest.json'){res.setHeader('Content-Type','application/json');res.end('{}');return;}
  if(!authenticated){res.writeHead(401).end(JSON.stringify({error:'Vui lòng đăng nhập.'}));return;}
  if(req.url==='/session'){res.end('test-token');return;}
  if(req.headers['x-robot-token']!=='test-token'){res.writeHead(403).end();return;}
  if(req.url==='/api/auth/logout'){logged=false;granted=false;res.setHeader('Content-Type','application/json');res.setHeader('Set-Cookie','bb_phone=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0');res.end('{"success":true}');return;}
  if(req.url==='/status'){
   if(offline){req.socket.destroy();return;}
   const body=`t=${++tick*80},st=${st},session=${session},seq=${tick},theta=0.01,omega=0,v=0.01,yaw=0,u=0.1,left=0,right=0,motion=1,fresh=1,sat=0,auto_mode=1,auto_wait=0,auto_ms=0,config=${design.config_id},hash=${design.config_sha256},fault=${st===3?'SPEED_RAIL':'NONE'},imu_calibrated=${cal},imu_diag=${cal?1:7},imu_cal_saved=${cal},imu_sample_fresh=1,hardware_confirmed=1,lqr2=1,phone_vkp=${pid[0]},phone_vki=${pid[1]},phone_vkd=${pid[2]},phone_k0=${K[0]},phone_k1=${K[1]}\nreply_id=${replyId}\n${reply}`;
   setTimeout(()=>res.end(body),delayStatus);return;
  }
  if(req.url==='/command'){
   let body='';req.on('data',d=>body+=d);req.on('end',()=>{const p=body.split(',');
    if(['CMD','MOTION','SPEED','TURN'].includes(p[0])&&!granted){res.writeHead(403).end('ERR:PRESS_START');return;}
    commands.push(body);
    if(body==='START'&&st===2){granted=true;res.end('ACK:CONTROL_GRANTED');return;}
    setTimeout(()=>{
     if(body==='START'){if(cal){session++;st=2;granted=true;say('ACK:START');}else say('ERR:IMU');}
     else if(body==='STOP'||body==='ESTOP'){st=0;granted=false;say('ACK:'+body);}
     else if(body==='GET')say(`t=1\nPID:V,${pid.join(',')}\nPID:Y,3,0.8,0.02\nLQR:${K.join(',')}`);
     else if(body==='CALGYRO')say('ACK:CALGYRO_PENDING');
     else if(p[0]==='$P'){pid=p.slice(2).map(Number);say('ACK:$P');}
     else if(p[0]==='LQR2'){if(!holdLqr){K=p.slice(2).map(Number);say('ACK:LQR2,'+K.join(','));}}
     else say('ACK:'+p[0]);
    },15);setTimeout(()=>res.writeHead(202).end('QUEUED'),p[0]==='CMD'?delayDrive:0);
   });return;
  }res.writeHead(404).end();
 });
 await new Promise(r=>server.listen(0,'127.0.0.1',r));const url=`http://127.0.0.1:${server.address().port}`;
 const browser=await chromium.launch({channel:'msedge',headless:true});t.after(async()=>{await browser.close();await new Promise(r=>server.close(r));});
 const page=await browser.newPage({viewport:{width:390,height:844},isMobile:true,hasTouch:true});
 await page.addInitScript(()=>{Element.prototype.requestFullscreen=()=>Promise.reject(Error('Local HTTP fullscreen not supported'));});
 page.setDefaultTimeout(10000);
 page.on('pageerror',e=>errors.push(e.message));page.on('request',r=>{if(!r.url().startsWith(url))external.push(r.url());});
 await page.goto(url);assert.equal(await page.locator('#form-title').textContent(),'Đăng nhập');assert.equal(await page.locator('#phone-connect-box').count(),0);
 assert.equal((await page.request.get(url+'/session')).status(),401);
 await page.click('#toggle');await page.fill('#username','phoneuser');await page.fill('#password','password123');await page.fill('#confirm','notmatching');await page.click('#submit');assert.match(await page.locator('#message').textContent(),/chưa khớp/);assert.equal(users.size,0);
 await page.fill('#confirm','password123');await page.click('#submit');await page.waitForFunction(()=>document.querySelector('#message').textContent.includes('Đăng ký thành công'));assert.equal(users.size,1);assert.notEqual(users.get('phoneuser'),'password123');
 await page.fill('#username','phoneuser');await page.fill('#password','wrongpass');await page.click('#submit');await page.waitForFunction(()=>document.querySelector('#message').textContent.includes('Sai tài khoản'));
 await page.screenshot({path:path.join(out,'phone-login.png'),fullPage:true});await page.fill('#password','password123');await page.click('#submit');await page.waitForFunction(()=>document.querySelector('#state-display')?.textContent.includes('hiệu chuẩn'));
 assert.equal((await page.request.post(url+'/command',{headers:{'X-Robot-Token':'test-token'},data:'CMD,41,1,0.05,0'})).status(),403);
 assert(await page.locator('#reset-status').isHidden());assert.equal(await page.locator('.preset-buttons').count(),0);
 assert.equal(await page.locator('.drive-hands-hint').count(),0);assert.equal(await page.locator('#drive-orientation-hint').count(),0);
 assert(await page.locator('#btn-start').isDisabled());await page.waitForTimeout(150);assert(!commands.includes('START'));
 await page.locator('[onclick="robotCmd(\'calgyro\')"]').click();await page.waitForFunction(()=>[...document.querySelectorAll('.notification')].some(n=>n.textContent.includes('CALGYRO_PENDING')));assert.equal(st,0);
 cal=1;await page.waitForFunction(()=>robotState.imuCalibrated===1);await page.click('#btn-start');await page.waitForFunction(()=>robotState.st===2);await page.waitForTimeout(150);
 await page.waitForFunction(()=>socket.controlStarted===true);assert(await page.locator('#btn-start').isDisabled());assert(await page.locator('#btn-start').evaluate(e=>e.classList.contains('is-started')));
 assert.equal(await page.locator('#btn-phone-qr').count(),0);assert.equal(await page.locator('#modal-qr').count(),0);
 await page.click('#btn-open-drive');await page.waitForFunction(()=>document.querySelector('#drive-screen').classList.contains('force-landscape'));
 assert.notEqual(await page.locator('#drive-screen').evaluate(e=>getComputedStyle(e).transform),'none');
 await page.screenshot({path:path.join(out,'portrait-landscape-fallback.png'),fullPage:false});
 await page.setViewportSize({width:844,height:390});await page.waitForFunction(()=>!document.querySelector('#drive-screen').classList.contains('force-landscape'));await page.waitForTimeout(150);
 const terminal=await page.locator('#drive-live-status').boundingBox();assert(Math.abs(terminal.x+terminal.width/2-422)<8);assert(terminal.height>=34);assert(await page.locator('#drive-live-status').evaluate(e=>e.classList.contains('status-balanced')));
 assert(await page.locator('#drive-start').isDisabled());assert(await page.locator('#drive-start').evaluate(e=>e.classList.contains('is-started')));
 const up=page.locator('#drive-scene-forward');await up.scrollIntoViewIfNeeded();let box=await up.boundingBox();
 const down=async()=>{await page.evaluate(()=>document.querySelectorAll('.notification').forEach(n=>n.remove()));await up.scrollIntoViewIfNeeded();await page.waitForTimeout(150);box=await up.boundingBox();await page.mouse.move(box.x+box.width/2,box.y+box.height/2);await page.mouse.down();};
 await down();await page.waitForFunction(()=>document.querySelector('#drive-scene-forward').classList.contains('is-pressed'));assert.notEqual(await up.evaluate(e=>getComputedStyle(e).transform),'none');
 await page.waitForTimeout(600);await page.screenshot({path:path.join(out,'phone-held.png'),fullPage:false});await page.mouse.up();await page.waitForFunction(()=>!document.querySelector('#drive-scene-forward').classList.contains('is-pressed'));await page.waitForTimeout(400);
 assert(commands.includes('MOTION,1'));assert(commands.some(c=>/^CMD,42,\d+,0\.050000,0\.000000$/.test(c)));assert(commands.some(c=>/^CMD,42,\d+,0\.000000,0\.000000$/.test(c)));
 delayDrive=180;const before=commands.length;await down();for(let i=0;i<40&&!commands.slice(before).some(c=>/^CMD,.*0\.050000/.test(c));i++)await page.waitForTimeout(25);await page.mouse.up();await page.waitForTimeout(700);assert(commands.slice(before).some(c=>/^CMD,42,\d+,0\.000000,0\.000000$/.test(c)));delayDrive=0;
 // Real browser touch events with independent pointer IDs, not JS-dispatched
 // events: v and w must be combined and released independently.
 const cdp=await page.context().newCDPSession(page);
 const point=async(id,n)=>{const b=await page.locator(id).boundingBox();return {x:b.x+b.width/2,y:b.y+b.height/2,id:n,radiusX:4,radiusY:4,force:1};};
 const fwd=await point('#drive-scene-forward',1),right=await point('#drive-scene-right',2);
 const combinedStart=commands.length;await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[fwd,right]});await page.waitForTimeout(450);
 assert(commands.slice(combinedStart).some(c=>/^CMD,42,\d+,0\.050000,-0\.349066$/.test(c)),'forward + right must combine');
 await page.screenshot({path:path.join(out,'forward-right-two-fingers.png'),fullPage:false});
 await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[fwd]});const afterOne=commands.length;await page.waitForTimeout(350);
 assert(commands.slice(afterOne).some(c=>/^CMD,42,\d+,0\.000000,-0\.349066$/.test(c)),'releasing forward keeps right: '+JSON.stringify(commands.slice(afterOne)));
 assert.equal(await page.locator('#drive-scene-right').getAttribute('aria-pressed'),'true');
 await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});await page.waitForTimeout(350);
 const back=await point('#drive-scene-back',3),left=await point('#drive-scene-left',4),reverseStart=commands.length;
 await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[back,left]});await page.waitForTimeout(450);
 assert(commands.slice(reverseStart).some(c=>/^CMD,42,\d+,-0\.050000,0\.349066$/.test(c)),'reverse + left must combine');
 await cdp.send('Input.dispatchTouchEvent',{type:'touchCancel',touchPoints:[]});await page.waitForTimeout(350);assert.equal(await page.locator('#drive-scene-back').getAttribute('aria-pressed'),'false');
 for(const [a,b,expected]of [
  ['#drive-scene-forward','#drive-scene-left',/0\.050000,0\.349066$/],
  ['#drive-scene-back','#drive-scene-right',/-0\.050000,-0\.349066$/]]){
  const at=commands.length;await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[await point(a,5),await point(b,6)]});await page.waitForTimeout(350);
  assert(commands.slice(at).some(c=>c.startsWith('CMD,')&&expected.test(c)),'all four direction combinations');
  await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});await page.waitForTimeout(250);
 }
 const chartBoxes=await page.locator('#drive-scene-charts .chart-card').evaluateAll(es=>es.map(e=>({h:e.getBoundingClientRect().height,w:e.getBoundingClientRect().width})));
 assert(chartBoxes[0].h>chartBoxes[2].h&&chartBoxes[1].h>chartBoxes[2].h);
 st=3;await page.waitForFunction(()=>document.querySelector('#drive-live-status').textContent.includes('SPEED_RAIL'));assert(await page.locator('#drive-live-status').evaluate(e=>e.classList.contains('status-fault')));assert(await page.locator('#drive-start').evaluate(e=>e.classList.contains('is-started')));
 await page.screenshot({path:path.join(out,'fault-terminal.png'),fullPage:false});st=2;await page.waitForFunction(()=>robotState.st===2);
 // Leaving while both hands are held must cancel them, return the same charts
 // to the original panel, and never stop balancing just to close a view.
 await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[fwd,right]});await page.waitForTimeout(200);
 const exitAt=commands.length;await page.evaluate(()=>document.querySelector('#drive-exit').click());await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});await page.waitForTimeout(350);
 assert(await page.locator('#drive-screen').isHidden());assert.equal(await page.locator('#panel-charts .charts-container').count(),1);
 assert(commands.slice(exitAt).some(c=>/^CMD,42,\d+,0\.000000,0\.000000$/.test(c)));assert(!commands.slice(exitAt).includes('STOP'));
 await page.setViewportSize({width:390,height:844});
 delayStatus=200;await page.click('#btn-stop');await page.waitForFunction(()=>robotState.st===0);delayStatus=0;
 assert(!await page.locator('#btn-start').evaluate(e=>e.classList.contains('is-started')));assert(!await page.locator('#btn-start').isDisabled());
 await page.fill('#pid-kp','2.5');await page.fill('#pid-ki','0.9');await page.fill('#pid-kd','0.15');await page.locator('[onclick="sendPid()"]').click();await page.waitForFunction(()=>document.querySelector('#current-kp').textContent==='2.500');assert(commands.includes('$P,V,2.5,0.9,0.15'));
 for(const [Q,R] of [[[400,4],1],[[100,1],1],[[300,3],1],[[50,1],3]]){const actual=await page.evaluate(({Q,R})=>window.phoneCalculate(Q,R).K,{Q,R}),expected=calculate(Q,R).K;actual.forEach((x,i)=>assert(Math.abs(x-expected[i])<1e-12));}
 await page.fill('#q0-input','300');await page.fill('#q1-input','3');await page.fill('#r-input','1');holdLqr=true;await page.click('#btn-lqr');await page.waitForTimeout(2300);assert(!await page.evaluate(()=>lqrParams.Q?.[0]===300&&lqrParams.sentToRobot),'missing ACK must not claim new K');
 holdLqr=false;await page.click('#btn-lqr');await page.waitForFunction(()=>lqrParams.Q?.[0]===300&&lqrParams.sentToRobot===true);assert(commands.some(c=>c.startsWith('LQR2,')));
 await page.locator('[onclick="robotCmd(\'save\')"]').click();await page.waitForTimeout(200);assert(commands.includes('SAVE'));
 await page.locator('.mobile-nav-btn[data-panel="charts"]').click();await page.waitForFunction(()=>angleChart.data.labels.length>2);await page.evaluate(()=>document.querySelectorAll('.notification').forEach(n=>n.remove()));await page.waitForTimeout(100);await page.screenshot({path:path.join(out,'phone-charts.png'),fullPage:true});
 await page.locator('.mobile-nav-btn[data-panel="data"]').click();await page.waitForTimeout(100);assert(await page.locator('#panel-data').isVisible());await page.screenshot({path:path.join(out,'phone-data.png'),fullPage:true});
 await page.locator('.mobile-nav-btn[data-panel="controls"]').click();let download=page.waitForEvent('download');await page.locator('[onclick="exportCSV()"]').click();let file=await download;assert(file.suggestedFilename().endsWith('.csv'));assert(fs.readFileSync(await file.path(),'utf8').includes('theta_rad'));
 download=page.waitForEvent('download');await page.locator('[onclick="downloadJSON()"]').click();file=await download;const exported=JSON.parse(fs.readFileSync(await file.path(),'utf8'));assert(exported.data.length>2&&exported.units.theta==='rad');
 await page.locator('[onclick="clearHistory()"]').click();await page.waitForTimeout(50);assert(await page.evaluate(()=>lastFaultCapture===null));st=3;await page.waitForFunction(()=>robotState.st===4);assert(await page.evaluate(()=>lastFaultCapture?.fault==='SPEED_RAIL'));
 st=0;await page.waitForFunction(()=>robotState.st===0);const startsAtOpen=commands.filter(c=>c==='START').length;
 await page.click('#btn-open-drive');await page.waitForTimeout(150);assert.equal(commands.filter(c=>c==='START').length,startsAtOpen,'opening the view must not arm');
 st=2;await page.waitForFunction(()=>robotState.st===2);await page.setViewportSize({width:844,height:390});await page.waitForTimeout(150);assert(await up.isDisabled());await page.click('#drive-start');await page.waitForFunction(()=>socket.controlStarted);
 await down();await page.waitForTimeout(200);await page.evaluate(()=>document.querySelector('#drive-stop').click());await page.mouse.up();await page.waitForFunction(()=>robotState.st===0);
 assert.equal(await page.locator('#drive-scene-forward').getAttribute('aria-pressed'),'false');await page.click('#drive-start');await page.waitForFunction(()=>robotState.st===2);await page.waitForTimeout(150);
 await down();await page.waitForTimeout(200);offline=true;await page.waitForFunction(()=>!socket.connected);await page.mouse.up();const count=commands.length;await page.waitForTimeout(400);assert.equal(commands.length,count);assert(!await up.evaluate(e=>e.classList.contains('is-pressed')));
 offline=false;await page.waitForFunction(()=>socket.connected);await page.waitForTimeout(300);assert(!commands.slice(count).some(c=>/^CMD,.*0\.050000/.test(c)));
 assert(await up.isDisabled());assert.equal(await page.evaluate(()=>socket.controlStarted),false);
 await page.click('#drive-exit');assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);await page.setViewportSize({width:1440,height:1000});await page.waitForTimeout(100);for(const id of ['panel-controls','panel-charts','panel-data'])assert(await page.locator('#'+id).isVisible());await page.screenshot({path:path.join(out,'desktop-direct.png'),fullPage:false});
 await page.click('#btn-stop');await page.waitForFunction(()=>robotState.st===0);await page.click('#logout');await page.waitForFunction(()=>document.querySelector('#auth-form'));
 assert.equal((await page.request.get(url+'/status',{headers:{'X-Robot-Token':'test-token'}})).status(),401);
 await page.fill('#username','phoneuser');await page.fill('#password','password123');await page.click('#submit');await page.waitForFunction(()=>document.querySelector('#btn-open-drive'));
 assert.equal(users.size,1);assert.equal(await page.evaluate(()=>socket.controlStarted),false);
 assert.deepEqual(errors,[]);assert.deepEqual(external,[]);
});
