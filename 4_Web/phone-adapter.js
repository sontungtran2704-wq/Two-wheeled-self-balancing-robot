'use strict';
// Local transport adapter for the unchanged desktop dashboard. No Socket.IO
// server, Node.js, external network, control calculations or motor timing here.
(() => {
    const nativeFetch = window.fetch.bind(window), design = window.PHONE_DESIGN;
    const handlers = new Map(), queue = [];
    let token = '', wanted = true, busy = false, linkUp = false, latest = null, seen = 0;
    let sequence = 0, replyId = -1, pending = null, lastStatus = 0, lastDrive = 0;
    let v = 0, w = 0, motion = false, zeroPending = false, owner = false;
    let lastInput = 0, appliedK = false, disconnecting = false, reportedK=null,controlEpoch=0;
    const event = (name, value) => { for (const fn of handlers.get(name) || []) fn(value); };
    const notice = (msg, type = 'info') => event('notice', {msg, type});
    const fresh = () => latest && performance.now() - seen <= 250;
    const matching = () => fresh() && latest.hash === design.config_sha256 && latest.config === design.config_id;
    function permission(started,pending=false){
        socket.controlStarted=started;socket.controlPending=pending;
        for(const id of ['btn-start','drive-start']){const b=document.getElementById(id);if(b){b.classList.toggle('is-started',started);b.setAttribute('aria-pressed',String(started));b.disabled=started||pending||!latest||latest.imu_calibrated!==1||![0,2].includes(latest.st);}}
        event('controlPermission',{started,pending});
    }
    function release(force=true) { const moving=!!(v||w);if (moving) zeroPending = true; v = w = 0; lastInput = 0;if(force||moving)document.dispatchEvent(new Event('balancebot:release')); }
    function drop(error) {
        release(); queue.length = 0; motion = false; owner = false; seen = 0;
        permission(false);
        if (pending) notice('Mất đường truyền trước khi nhận ACK: chưa xác nhận lệnh ' + pending.line.split(',')[0], 'warning');
        pending = null;
        if (linkUp) { linkUp = false; socket.connected = false; event('disconnect'); event('serialDisconnected'); }
        event('linkStatus', {connected:false, fresh:false, matching:false});
        if (error) notice('Mất liên lạc với xe: ' + error.message, 'error');
    }
    async function request(path, body) {
        const controller = new AbortController(), timeout = setTimeout(() => controller.abort(), 900);
        try {
            const response = await nativeFetch(path, {method:body === undefined ? 'GET' : 'POST',
                cache:'no-store', headers:{'X-Robot-Token':token, 'Content-Type':'text/plain'},
                body, signal:controller.signal});
            if (!response.ok) { if(response.status === 403) token = '';const e=Error('HTTP '+response.status);e.status=response.status;throw e; }
            return await response.text();
        } finally { clearTimeout(timeout); }
    }
    function enqueue(line, done) {
        if (line === 'STOP' || line === 'ESTOP' || line==='CONTROL_STOP') {
            ++controlEpoch;release(); queue.length = 0; pending = null; owner = false; motion = false;permission(false);
            queue.push({line,done}); return true;
        }
        if (!wanted || !socket.connected) { notice('Chưa gửi lệnh: hãy kết nối với xe.', 'error'); return false; }
        if (queue.length >= 6) { notice('Hàng đợi lệnh đang bận.', 'warning'); return false; }
        queue.push({line,done});return true;
    }
    function lineMessage(line) {
        if (line.startsWith('PID:V,')) {
            const values = line.split(',').slice(1).map(Number);
            if (values.length === 3 && values.every(Number.isFinite)) event('pidUpdated',{axis:'V',kp:values[0],ki:values[1],kd:values[2]});
        } else if (line.startsWith('LQR:')) {
            const K = line.slice(4).split(',').map(Number);
            if (K.length === 2 && K.every(Number.isFinite) && (!reportedK||K.some((x,i)=>Math.abs(x-reportedK[i])>1e-9))) {
                reportedK=K;const readback={...design,K,fromFirmware:true,sentToRobot:true};
                // GET after a PID edit must not overwrite unsent Q/R form edits.
                delete readback.Q;delete readback.R;delete readback.model;event('lqrUpdated',readback);
            }
        } else if (/^(ACK:|ERR:)/.test(line) && line!=='ACK:CMD' && line!=='ACK:MOTION') event('serialMessage',line);
    }
    function parse(text) {
        const d = {};
        for (const part of text.split(',')) {
            const pos = part.indexOf('='); if(pos <= 0) throw Error('Telemetry không hợp lệ');
            const key = part.slice(0,pos), raw = part.slice(pos+1);
            if (Object.hasOwn(d,key) || !raw) throw Error('Telemetry trùng/thiếu trường');
            d[key] = ['config','hash','fault'].includes(key) ? raw : Number(raw);
        }
        if (!['t','session','seq','theta','omega','v','yaw','u','st'].every(k=>Number.isFinite(d[k])) ||
            ![0,1,2,3].includes(d.st) || !Number.isInteger(d.session) || d.session < 0 || d.session > 0xffffffff || !/^[a-f0-9]{64}$/.test(d.hash || '')) throw Error('Telemetry không hợp lệ');
        return d;
    }
    async function status() {
        const lines = (await request('/status')).split('\n'), d = parse(lines.shift());
        if (!wanted) return;
        const marker = lines[0]?.startsWith('reply_id=') ? Number(lines.shift().slice(9)) : -1;
        const reply = lines.join('\n');
        if (latest && latest.session !== d.session) {
            release();motion=false;owner=socket.controlStarted;queue.length=0;sequence=0;
            // START itself creates a new firmware session. Keep its ACK waiter;
            // recovery retains explicit permission but never retains held input.
            if(pending?.line!=='START')pending=null;
        }
        latest = d; seen = performance.now();
        if (d.st !== 2) { release(false); motion = false; }
        if (!linkUp) {
            linkUp = true; socket.connected = true; event('connect'); event('serialConnected');
            queue.unshift({line:'GET'});
        }
        event('serialData',{...d,t:d.t/1000,thetaDot:d.omega,xDot:d.v,x:null,
            relativeAcceleration:d.u,st:d.st===3?(d.fault==='FALL'?3:4):d.st});
        event('linkStatus',{connected:true,fresh:true,matching:matching()});
        if (Number.isFinite(d.phone_vkp)) event('pidUpdated',{kp:d.phone_vkp,ki:d.phone_vki,kd:d.phone_vkd,axis:'V'});
        if (!appliedK && matching() && Number.isFinite(d.phone_k0) && Number.isFinite(d.phone_k1)) {
            appliedK = true;reportedK=[d.phone_k0,d.phone_k1]; event('lqrUpdated',{...design,K:reportedK,fromFirmware:true,sentToRobot:true});
        }
        document.querySelectorAll('[data-drive-v]').forEach(b=>{b.disabled=d.st!==2||!matching()||!socket.controlStarted;});
        permission(socket.controlStarted,socket.controlPending);
        if (marker !== replyId) {
            replyId = marker;
            for (const line of lines) lineMessage(line);
            if (pending && marker !== pending.before) {
                const cmd = pending.line.split(',')[0];
                const final = cmd==='GET' ? reply.includes('PID:V,') || reply.startsWith('ERR:') :
                    reply.startsWith('ERR:') || reply.startsWith('ACK:' + (cmd==='$P'?'$P':cmd));
                if (final) { const item=pending;pending=null;item.done?.(reply); }
            }
        }
        if (pending && performance.now()-pending.at > 2000) {
            if(pending.line==='START')permission(false);
            notice('Đã gửi '+pending.line.split(',')[0]+' nhưng chưa nhận ACK; chưa xác nhận thành công.','warning'); pending=null;
        }
    }
    async function send(item) {
        const before = replyId;
        const result = await request('/command',item.line);
        if (result.startsWith('ERR:') || result.startsWith('ACK:STOP') || result.startsWith('ACK:ESTOP')) {
            lineMessage(result); item.done?.(result); return;
        }
        if (result !== 'QUEUED') { lineMessage(result);item.done?.(result);return; }
        if (wanted) pending = {...item,before,at:performance.now()};
    }
    async function cycle() {
        if (busy || !wanted) return;
        busy = true;
        try {
            if (!token) token = (await request('/session')).trim();
            const now = performance.now();
            if (now-lastStatus>=80 || pending) { await status();lastStatus=performance.now(); }
            if (!wanted) return;
            if (['STOP','ESTOP','CONTROL_STOP'].includes(queue[0]?.line)) { await send(queue.shift());return; }
            if (!pending && queue.length) { await send(queue.shift()); return; }
            if (pending) return;
            if (!matching() || latest.st!==2 || !owner) { release(false);return; }
            if (lastInput && now-lastInput>250) release();
            if ((v || w || zeroPending) && now-lastDrive>=100) {
                if ((v || w) && !motion) { await send({line:'MOTION,1',done:r=>{motion=r.startsWith('ACK:');}}); return; }
                const dv=v,dw=w;zeroPending=false;lastDrive=now;sequence=(sequence+1)>>>0;
                // A response to CMD is consumed before sending the next command;
                // no stale request is replayed after any connection failure.
                await send({line:`CMD,${latest.session},${sequence},${dv.toFixed(6)},${dw.toFixed(6)}`});
            }
        } catch (error) { drop(linkUp?error:null);if(error.status===401){wanted=false;location.replace('/welcome');} }
        finally { busy=false; }
    }
    async function disconnect() {
        if (disconnecting) return;disconnecting=true;
        release();queue.length=0;pending=null;wanted=false;
        try {
            if (matching() && latest.st===2 && owner) { sequence=(sequence+1)>>>0;await request('/command',`CMD,${latest.session},${sequence},0,0`); }
        } catch (_) {} finally { drop();disconnecting=false; }
    }
    const socket = {
        connected:false,controlStarted:false,controlPending:false,
        on(name,fn){if(!handlers.has(name))handlers.set(name,[]);handlers.get(name).push(fn);return this;},
        emit(name,p){
            if(name==='connectSerial'){wanted=true;cycle();return;}
            if(name==='disconnectSerial'){disconnect();return;}
            if(name==='release'){release();return;}
            if(name==='pauseControl'){enqueue('CONTROL_STOP',r=>{if(r==='ACK:CONTROL_STOP')notice(latest?.st===2?'Đã dừng điều khiển web. Xe vẫn cân bằng.':'Đã khóa điều khiển web.');});return;}
            if(name==='drive'){
                if(!matching()||latest.st!==2||!owner)return false;
                if(!p||!Number.isFinite(p.v)||!Number.isFinite(p.w)||Math.abs(p.v)>.2||Math.abs(p.w)>.8)return false;
                if((v||w)&&p.v===0&&p.w===0)zeroPending=true;
                v=p.v;w=p.w;lastInput=performance.now();return true;
            }
            if(name==='robotControl'){
                const cmd=String(p?.command||'').toUpperCase();
                if(!['GET','START','STOP','ESTOP','RESET','CALGYRO','SAVE','ZPHERE','ZP+','ZP-'].includes(cmd))return notice('Lệnh không hợp lệ','error');
                release();
                if(cmd==='START'){
                    if(socket.controlStarted||socket.controlPending)return;
                    if(!matching()||latest.imu_calibrated!==1)return notice('Cần telemetry mới, đúng cấu hình và hiệu chuẩn đạt trước START.','error');
                    permission(false,true);
                    const epoch=controlEpoch;
                    if(!enqueue(cmd,r=>{if(epoch!==controlEpoch)return;owner=r.startsWith('ACK:START')||r==='ACK:CONTROL_GRANTED';permission(owner);})){permission(false);}return;
                }
                enqueue(cmd);return;
            }
            if(name==='setSpeed'||name==='setTurn'){
                if(!matching()||latest.st!==2||!owner)return;
                if(!Number.isFinite(p)||Math.abs(p)>(name==='setSpeed'?20:45.83662361046586))return notice('Lệnh vượt giới hạn','error');
                if(p===0&&(v||w))zeroPending=true;
                if(name==='setSpeed')v=p/100;else w=p*Math.PI/180;
                lastInput=performance.now();return;
            }
            if(name==='setPid'){
                if(!matching()||![0,3].includes(latest.st))return notice('Chỉ gửi PID khi motor tắt và telemetry mới.','error');
                if(!p||![p.kp,p.ki,p.kd].every((x,i)=>Number.isFinite(x)&&x>=0&&x<=[5,2,.2][i]))return notice('PID: Kp 0..5, Ki 0..2, Kd 0..0.2.','error');
                enqueue(`$P,V,${p.kp},${p.ki},${p.kd}`,()=>enqueue('GET'));return;
            }
            if(name==='updateLQR'){
                try {
                    const result=window.phoneCalculate(p.Q,p.R);
                    if(!p.send){event('lqrUpdated',result);return;}
                    if(!matching()||latest.st!==0)throw Error('Chỉ gửi LQR khi xe đang Chờ, telemetry mới và cấu hình khớp.');
                    if(latest.lqr2!==1)throw Error('Firmware hiện tại chưa hỗ trợ chỉnh LQR2. Bộ điều khiển vẫn giữ K hiện tại.');
                    enqueue(`LQR2,${design.config_sha256},${result.K[0]},${result.K[1]}`,reply=>{
                        const match=/^ACK:LQR2,([^,]+),([^,]+)/.exec(reply);
                        if(match&&match.slice(1).every((x,i)=>Math.abs(Number(x)-result.K[i])<1e-7)){reportedK=result.K.slice();event('lqrUpdated',{...result,sentToRobot:true});}
                        else event('lqrError','Chưa xác nhận áp dụng K: '+reply);
                    });
                } catch(e){event('lqrError',e.message);}return;
            }
            if(name==='setWifi')notice('Firmware hiện tại chưa hỗ trợ lưu Wi-Fi nhà.','error');
        }
    };
    window.io = () => socket;
    // The desktop UI asks these read-only discovery endpoints. Here there is
    // exactly one device: the ESP32 serving this page, never a computer port.
    window.fetch = (input, options) => {
        const path = typeof input==='string'?input:'';
        if(path==='/api/ports')return Promise.resolve(new Response(JSON.stringify([{path:`tcp:${location.hostname}:8080`,manufacturer:'Wi-Fi BalanceBot-Bench'}]),{headers:{'Content-Type':'application/json'}}));
        if(path==='/api/health')return Promise.resolve(new Response(JSON.stringify({service:'balancebot-direct',urls:[location.origin]}),{headers:{'Content-Type':'application/json'}}));
        return nativeFetch(input,options);
    };
    document.addEventListener('DOMContentLoaded',()=>{
        document.getElementById('logout').onclick=async()=>{await disconnect();try{await request('/api/auth/logout','');location.assign('/welcome');}catch(e){if(e.status===401)location.assign('/welcome');else notice('Chưa đăng xuất được. Kiểm tra kết nối rồi thử lại.','error');}};
        setInterval(cycle,40);cycle();
    });
    window.addEventListener('blur',release);
    window.addEventListener('pagehide',release);
    document.addEventListener('visibilitychange',()=>{if(document.hidden)release();});
})();
