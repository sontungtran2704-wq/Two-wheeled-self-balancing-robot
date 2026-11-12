'use strict';
// ============ SERVER CHO XE CÂN BẰNG ĐỘNG CƠ BƯỚC (ESP32-S3 + LQR) ============
// - Tính K từ Q, R và thông số xe (lqr.js) rồi gửi `$K,k_theta,k_thetaDot,k_velocity,A41,B4,FrefGain` xuống ESP32-S3
// - Chuyển lệnh điều khiển (START/STOP/SPEED/TURN/ZP.../CALGYRO/SAVE) xuống serial
// - Đọc telemetry `t=..,theta=..,...,st=..` và các dòng sự kiện `EVT:`/`ERR:` rồi phát cho trình duyệt
const express = require('express');
const http = require('http');
const socketIO = require('socket.io');
const path = require('path');
const net = require('net');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const lqr = require('./lqr');

const PORT = process.env.PORT || 3000;
// Mặc định chỉ nghe trên máy này. Muốn cho thiết bị khác trong mạng dùng (điện thoại...):
//   set HOST=0.0.0.0 và set ALLOWED_ORIGINS=http://192.168.1.50:3000  (địa chỉ IP của máy chạy server)
const HOST = process.env.HOST || '127.0.0.1';

// ============ BẢO MẬT: chỉ nhận trang web do chính server này phục vụ ============
// Chống (1) máy khác trong LAN, (2) trang web lạ trong trình duyệt của bạn gửi START/SPEED tới localhost
// (WebSocket không bị CORS chặn nên phải tự kiểm tra Origin), (3) DNS rebinding (kiểm tra Host).
const ALLOWED_ORIGINS = new Set([
    `http://localhost:${PORT}`, `http://127.0.0.1:${PORT}`,
    ...(process.env.ALLOWED_ORIGINS || '').split(',').map(s => s.trim()).filter(Boolean)
]);
const ALLOWED_HOSTS = new Set([...ALLOWED_ORIGINS].map(o => o.replace(/^https?:\/\//, '')));
// origin undefined = client không phải trình duyệt (curl, script cục bộ) -> cho phép vì đã chỉ nghe localhost
const originOk = o => o === undefined || ALLOWED_ORIGINS.has(o);
const hostOk = h => ALLOWED_HOSTS.has(h);
const requestOk = req => hostOk(req.headers.host) && originOk(req.headers.origin);

const app = express();
const server = http.createServer(app);
const io = socketIO(server, { allowRequest: (req, cb) => cb(null, requestOk(req)) });   // không dùng cors '*'

app.use((req, res, next) => requestOk(req) ? next() : res.status(403).json({ error: 'Forbidden origin/host' }));

// Chỉ phục vụ thư mục public (không lộ mã nguồn server / node_modules)
app.use(express.static(path.join(__dirname, 'public')));
app.use('/vendor/chart', express.static(path.join(__dirname, 'node_modules', 'chart.js', 'dist')));
app.use(express.json());

// ============ ĐƯỜNG TRUYỀN XUỐNG XE: USB-Serial hoặc WiFi (TCP) ============
// Cả hai dùng chung giao thức dòng văn bản (cùng bộ lệnh như firmware), nên phần còn lại của server không đổi.
// link = { kind: 'serial'|'tcp', name, isOpen(), write(str), close() }
let link = null;
let pidConfig = null;
let wifiInfo = null;       // WiFi của xe đọc từ lệnh GET (mode, ip, ssid, lỗi nối WiFi nhà)

function serialWrite(line) {
    if (link && link.isOpen()) { link.write(line + '\n'); return true; }
    return false;
}

// ============ CẤU HÌNH LQR ============
const LIMITS = {
    Q: [0, 10000],            // mỗi phần tử đường chéo
    R: [0.01, 1000],
    mb: [0.1, 20], mw: [0.01, 5], r: [0.01, 0.3], l: [0.02, 1], h: [0.04, 1.5], Ir: [0, 0.01]
};
const inRange = (v, [lo, hi]) => typeof v === 'number' && isFinite(v) && v >= lo && v <= hi;

let lqrConfig = {
    Q: [100, 1, 10],          // [theta error, theta_dot, vehicle velocity]
    R: 1,
    model: { ...lqr.DEFAULT_MODEL },
    K: null, A41: null, B4: null, FrefGain: null, unstablePole: null,
    sentToRobot: false,
    updateTime: new Date()
};

function recompute(Q, R, model) {
    if (!Array.isArray(Q) || Q.length !== 3 || !Q.every(v => inRange(v, LIMITS.Q))) throw new Error('Q phải gồm 3 số trong 0..10000');
    if (!inRange(R, LIMITS.R)) throw new Error('R phải trong 0.01..1000');
    model = { Ir: lqr.DEFAULT_MODEL.Ir, ...model };
    for (const k of ['mb', 'mw', 'r', 'l', 'h', 'Ir']) if (!inRange(model[k], LIMITS[k])) throw new Error(`Thông số ${k} ngoài khoảng cho phép`);
    if (model.l * 2 > model.h * 1.5) throw new Error('l (trọng tâm) quá cao so với h');
    const res = lqr.design(model, Q, R);
    lqrConfig = { Q, R, model: { ...model }, K: res.K, A41: res.A41, B4: res.B4, FrefGain: res.FrefGain,
                  unstablePole: res.unstablePole, sentToRobot: false, updateTime: new Date() };
    return lqrConfig;
}

function sendKToRobot() {
    const c = lqrConfig;
    if (!c.K) return false;
    const f = v => v.toFixed(4);
    const ok = serialWrite(`$K,${c.K.map(f).join(',')},${f(c.A41)},${f(c.B4)},${f(c.FrefGain)}`);
    c.sentToRobot = ok;
    return ok;
}

// Tính sẵn K mặc định lúc khởi động
try { recompute(lqrConfig.Q, lqrConfig.R, lqrConfig.model); }
catch (e) { console.error('Không tính được K mặc định:', e.message); }

function applyLQR(body) {
    // Tương thích ngược: nếu Q là số thì coi là Q_theta, giữ nguyên các Q còn lại
    let Q = body.Q;
    if (typeof Q === 'number') Q = [Q, ...lqrConfig.Q.slice(1)];
    const R = body.R == null ? lqrConfig.R : body.R;
    const model = { ...lqrConfig.model, ...(body.model || {}) };
    return recompute(Q == null ? lqrConfig.Q : Q, R, model);
}

// ============ ROUTES ============
app.get('/api/lqr', (req, res) => res.json(lqrConfig));

app.post('/api/lqr', (req, res) => {
    try {
        const c = applyLQR(req.body || {});
        if (req.body && req.body.send !== false) sendKToRobot();
        io.emit('lqrUpdated', lqrConfig);
        res.json({ success: true, config: c });
    } catch (e) {
        res.status(400).json({ error: e.message });
    }
});

app.get('/api/ports', async (req, res) => {
    try {
        const ports = await SerialPort.list();
        const wifi = [
            { path: `tcp:balancebot.local:${TCP_PORT}`,  manufacturer: 'WiFi (xe nối WiFi nhà, tìm bằng tên)' },
            { path: `tcp:192.168.4.1:${TCP_PORT}`,       manufacturer: 'WiFi riêng của xe (BalanceBot)' }
        ];
        res.json([...wifi, ...ports.map(p => ({ path: p.path, manufacturer: p.manufacturer || '' }))]);
    } catch (err) { res.status(500).json({ error: err.message }); }
});

// ============ MỞ/ĐÓNG ĐƯỜNG TRUYỀN (dùng chung cho REST và socket) ============
// Quy tắc (sửa lỗi "nối được vài giây rồi báo hết thời gian chờ"):
//  - Timeout 10 s CHỈ áp dụng cho giai đoạn đang nối; nối xong thì không còn timeout cố định nào giết socket.
//  - Xe im lặng > 6 s: server gửi thử GET; > 10 s vẫn không có byte nào mới coi là mất sóng và đóng.
//  - Tự nối lại do SERVER giữ (1 bộ hẹn giờ duy nhất, lùi dần 1-2-4-8 s, tối đa 10 lần); dashboard không tự nối lại nữa,
//    nên nhiều tab / nhiều lần bấm không còn tranh nhau làm xe bỏ client cũ.
//  - Người dùng bấm Ngắt kết nối thì không tự nối lại.
const TCP_PORT = 8080;
const TCP_RE = /^tcp:([A-Za-z0-9]([A-Za-z0-9.-]{0,60}[A-Za-z0-9])?):(\d{1,5})$/;
const CONNECT_TIMEOUT_MS = 10000;
const SILENCE_PROBE_MS = 6000;
const SILENCE_DROP_MS = 10000;
const MAX_RETRIES = 10;

let wanted = null;          // { port, baudRate } đường truyền cần giữ; null = người dùng đã ngắt / chưa nối
let retryTimer = null;
let retryCount = 0;
let pending = null;         // { port, promise } lần nối đang chạy

function clearRetry() { if (retryTimer) { clearTimeout(retryTimer); retryTimer = null; } }

function closeLink() { if (link) { const l = link; link = null; l.close(); } }

// Người dùng chủ động ngắt: không tự nối lại
function userClose() {
    const wasOpen = !!(link && link.isOpen());
    wanted = null; retryCount = 0; clearRetry();
    closeLink();
    if (wasOpen) io.emit('serialDisconnected');
    return wasOpen;
}

// Đường truyền đang giữ bị mất ngoài ý muốn
function onLinkLost(reason) {
    io.emit('serialDisconnected');
    if (reason) io.emit('serialError', reason);
    scheduleReconnect(reason || 'Mất kết nối với xe');
}

function scheduleReconnect(reason) {
    if (!wanted || retryTimer) return;
    if (retryCount >= MAX_RETRIES) {
        io.emit('notice', { type: 'error', msg: `Đã thử nối lại ${MAX_RETRIES} lần không được. Kiểm tra xe/WiFi rồi bấm Kết nối.` });
        wanted = null; retryCount = 0;
        return;
    }
    const delay = Math.min(1000 * 2 ** Math.min(retryCount, 3), 8000);
    retryCount++;
    io.emit('notice', { type: 'warning', msg: `Tự nối lại xe sau ${delay / 1000} s (lần ${retryCount}/${MAX_RETRIES})...` });
    retryTimer = setTimeout(async () => {
        retryTimer = null;
        if (!wanted || link) return;
        try { await openLink(wanted.port, wanted.baudRate); retryCount = 0; }
        catch (e) { console.error('Nối lại thất bại:', e.message); scheduleReconnect(e.message); }
    }, delay);
}

function attachParser(stream) {
    stream.pipe(new ReadlineParser({ delimiter: '\n' })).on('data', handleSerialData);
}

function openSerial(port, baudRate = 115200) {
    return new Promise((resolve, reject) => {
        closeLink();
        let opened = false, settled = false;
        const fail = e => { if (!settled) { settled = true; reject(e); } };
        const sp = new SerialPort({ path: port, baudRate }, err => { if (err) { if (link === me) link = null; fail(err); } });
        const me = { kind: 'serial', name: port, isOpen: () => sp.isOpen, write: d => sp.write(d), close: () => { if (sp.isOpen) sp.close(); } };
        link = me;
        attachParser(sp);
        sp.on('open', () => {
            opened = true; settled = true;
            console.log(`Serial mở: ${port} @ ${baudRate}`);
            io.emit('serialConnected', { port }); serialWrite('GET'); resolve();
        });
        sp.on('error', err => { console.error('Serial error:', err.message); io.emit('serialError', err.message); });
        sp.on('close', () => {
            console.log('Serial đóng');
            if (link !== me) return;          // đang chuyển đường khác hoặc người dùng đã ngắt
            link = null;
            if (opened) onLinkLost('Cổng serial bị đóng (rút cáp USB?)');
            else fail(new Error('Không mở được cổng serial'));
        });
    });
}

function connectHint(host) {
    if (host === '192.168.4.1') return 'Máy tính phải đang nối WiFi "BalanceBot" (mật khẩu balance123) và xe đã bật.';
    if (/\.local$/i.test(host)) return 'Không tìm thấy balancebot.local: thử nhập thẳng IP xe (xem trong Serial Monitor, lệnh GET).';
    return 'Kiểm tra xe và máy tính cùng một mạng WiFi, IP đúng (xem Serial Monitor, lệnh GET).';
}

function openTcp(host, port) {
    return new Promise((resolve, reject) => {
        closeLink();
        console.log(`Đang kết nối tới xe qua WiFi: ${host}:${port}...`);
        const sock = net.createConnection({ host, port });
        let opened = false, settled = false, lastErr = null, lastRx = Date.now(), probed = false, watchdog = null;
        const fail = e => { if (!settled) { settled = true; reject(e); } };
        const me = {
            kind: 'tcp',
            name: `${host}:${port}`,
            isOpen: () => opened && !sock.destroyed,
            write: d => sock.write(d),
            close: () => { clearTimeout(connectTimer); clearInterval(watchdog); sock.destroy(); }
        };
        link = me;
        sock.setNoDelay(true);

        // Chỉ canh giai đoạn đang nối. Sau khi nối xong timer này bị xoá.
        const connectTimer = setTimeout(() => {
            if (!opened) sock.destroy(new Error(`Không nối được tới xe trong ${CONNECT_TIMEOUT_MS / 1000} s (${host}:${port}). ${connectHint(host)}`));
        }, CONNECT_TIMEOUT_MS);

        attachParser(sock);
        sock.on('data', () => { lastRx = Date.now(); probed = false; });

        sock.on('connect', () => {
            opened = true; settled = true;
            clearTimeout(connectTimer);
            sock.setKeepAlive(true, 5000);
            lastRx = Date.now();
            console.log(`WiFi/TCP mở thành công: ${host}:${port}`);
            // Canh im lặng: telemetry ~25 Hz nên im lặng thật sự là mất sóng. Thử GET trước khi kết luận.
            watchdog = setInterval(() => {
                if (link !== me) return;
                const idle = Date.now() - lastRx;
                if (idle > SILENCE_DROP_MS) {
                    sock.destroy(new Error(`Mất tín hiệu từ xe (${Math.round(idle / 1000)} s không nhận được dữ liệu). Xe tắt, ra xa sóng, hoặc máy tính đã tự đổi sang WiFi khác.`));
                } else if (idle > SILENCE_PROBE_MS && !probed) {
                    probed = true;
                    sock.write('GET\n');
                }
            }, 1000);
            io.emit('serialConnected', { port: me.name });
            serialWrite('GET');
            resolve();
        });

        sock.on('error', err => {
            lastErr = err.message;
            console.error('TCP error:', err.message);
            if (opened && link === me) io.emit('serialError', `Lỗi kết nối WiFi: ${err.message}`);
            else fail(err);
        });

        sock.on('close', hadError => {
            clearTimeout(connectTimer); clearInterval(watchdog);
            console.log(`WiFi/TCP đóng (lỗi: ${hadError}${lastErr ? ', ' + lastErr : ''})`);
            if (link !== me) { fail(new Error('Đã hủy kết nối')); return; }     // ta tự đóng (chuyển đường / ngắt)
            link = null;
            if (opened) onLinkLost(lastErr ? null : 'Xe đã đóng kết nối (xe khởi động lại hoặc đã nhận client khác)');
            else fail(new Error(lastErr || 'Kết nối bị đóng'));
        });
    });
}

function openLink(port, baudRate) {
    if (typeof port === 'string' && port.startsWith('tcp:')) {
        const m = TCP_RE.exec(port);
        const n = m ? Number(m[3]) : 0;
        if (!m || n < 1 || n > 65535) return Promise.reject(new Error('Địa chỉ WiFi không hợp lệ (dạng tcp:host:port)'));
        return openTcp(m[1], n);
    }
    return openSerial(port, baudRate);
}

// Điểm vào duy nhất cho yêu cầu của người dùng (REST và socket)
async function connectUser(port, baudRate = 115200) {
    if (link && link.isOpen() && link.name === port.replace(/^tcp:/, '')) {   // đã nối đúng đường này
        wanted = { port, baudRate };
        io.emit('serialConnected', { port: link.name });
        return;
    }
    if (pending && pending.port === port) return pending.promise;              // đang nối: dùng chung kết quả
    clearRetry(); retryCount = 0; wanted = null;
    const p = openLink(port, baudRate).then(() => { wanted = { port, baudRate }; });
    pending = { port, promise: p };
    try { await p; } finally { if (pending && pending.promise === p) pending = null; }
}

app.post('/api/connect', async (req, res) => {
    const { port, baudRate = 115200 } = req.body || {};
    if (!port) return res.status(400).json({ error: 'Chưa chọn cổng' });
    try { await connectUser(port, baudRate); res.json({ success: true }); }
    catch (e) { res.status(500).json({ error: e.message }); }
});

app.post('/api/disconnect', (req, res) => {
    if (userClose()) res.json({ success: true });
    else res.json({ success: false, message: 'Chưa kết nối' });
});

// ============ SOCKET.IO ============
const ALLOWED_ROBOT_CMDS = {
    start: 'START', stop: 'STOP', zphere: 'ZPHERE', 'zp+': 'ZP+', 'zp-': 'ZP-',
    calgyro: 'CALGYRO', save: 'SAVE', get: 'GET', defaults: 'DEFAULTS', reboot: 'REBOOT'
};

io.on('connection', socket => {
    console.log(`Client kết nối: ${socket.id}`);
    socket.emit('lqrUpdated', lqrConfig);
    if (pidConfig) socket.emit('pidUpdated', pidConfig);
    if (wifiInfo) socket.emit('robotWifi', wifiInfo);

    socket.on('connectSerial', async ({ port, baudRate = 115200 } = {}) => {
        if (!port) { socket.emit('serialError', 'Chưa chọn cổng'); return; }
        try { await connectUser(port, baudRate); }
        catch (e) { socket.emit('serialError', e.message); }
    });

    socket.on('disconnectSerial', () => userClose());

    socket.on('robotControl', ({ command } = {}) => {
        const line = ALLOWED_ROBOT_CMDS[command];
        if (!line) return socket.emit('serialError', `Lệnh không hợp lệ: ${command}`);
        if (!serialWrite(line)) socket.emit('serialError', 'Chưa kết nối tới xe');
    });

    // tốc độ tiến (cm/s) và quay (độ/s)
    socket.on('setSpeed', v => {
        const n = Number(v);
        if (!isFinite(n)) return;
        serialWrite(`SPEED,${Math.max(-40, Math.min(40, n))}`);
    });
    socket.on('setTurn', v => {
        const n = Number(v);
        if (!isFinite(n)) return;
        serialWrite(`TURN,${Math.max(-90, Math.min(90, n))}`);
    });
    // cấu hình WiFi nhà cho xe (lưu vào NVS của xe, cần REBOOT để áp dụng)
    socket.on('setWifi', ({ ssid, pass } = {}) => {
        const okSsid = typeof ssid === 'string' && ssid.length >= 1 && ssid.length <= 32 && !/[,\r\n]/.test(ssid);
        const okPass = typeof pass === 'string' && pass.length <= 64 && !/[\r\n]/.test(pass) && (pass.length === 0 || pass.length >= 8);
        if (!okSsid || !okPass) return socket.emit('serialError', 'SSID 1..32 ký tự (không dấu phẩy); mật khẩu trống hoặc 8..64 ký tự');
        if (!serialWrite(`WIFI,${ssid},${pass}`)) socket.emit('serialError', 'Chưa kết nối tới xe');
    });
    // PID vị trí vòng ngoài: $P,Kp,Ki,Kd (giới hạn giống firmware)
    socket.on('setPid', ({ kp, ki, kd } = {}) => {
        const v = [Number(kp), Number(ki), Number(kd)], max = [3, 1, 2];
        if (!v.every((x, i) => isFinite(x) && x >= 0 && x <= max[i]))
            return socket.emit('serialError', 'PID: Kp 0..3, Ki 0..1, Kd 0..2');
        if (!serialWrite(`$P,${v.join(',')}`)) socket.emit('serialError', 'Chưa kết nối tới xe');
        else serialWrite('GET');
    });
    socket.on('setZeropoint', v => {
        const n = Number(v);
        if (isFinite(n) && Math.abs(n) <= 15) serialWrite(`ZP,${n}`);
    });

    // cập nhật Q, R, thông số xe -> tính K -> gửi xuống robot
    socket.on('updateLQR', body => {
        try {
            applyLQR(body || {});
            const sent = (body && body.send === false) ? false : sendKToRobot();
            io.emit('lqrUpdated', lqrConfig);
            if (!sent && !(body && body.send === false)) socket.emit('notice', { type: 'warning', msg: 'Đã tính K nhưng CHƯA gửi xuống xe (chưa kết nối).' });
        } catch (e) {
            socket.emit('lqrError', e.message);
        }
    });

    socket.on('sendK', () => {
        if (!sendKToRobot()) socket.emit('serialError', 'Chưa kết nối tới xe hoặc chưa có K');
        else io.emit('lqrUpdated', lqrConfig);
    });

    socket.on('disconnect', () => console.log(`Client rời: ${socket.id}`));
});

// ============ XỬ LÝ DỮ LIỆU SERIAL / TCP ============
function handleSerialData(raw) {
    const line = String(raw).trim();
    if (!line) return;

    // dòng thông điệp từ firmware: EVT:..., ERR:..., "K UPDATED", "SAVED", "GYRO OK", "K=[...]"...
    const pid = /PIDpos=\[([^\]]+)\]/.exec(line);
    if (pid) {
        const values = pid[1].split(',').map(Number);
        if (values.length === 3 && values.every(Number.isFinite)) {
            pidConfig = { kp: values[0], ki: values[1], kd: values[2] };
            io.emit('pidUpdated', pidConfig);
        }
    }
    const w = /^WIFI mode=(\w+) ip=(\S+) host=(\S+) port=(\d+) rssi=(-?\d+) ssid=(.*)$/.exec(line);
    if (w) { wifiInfo = { ...(wifiInfo || {}), mode: w[1], ip: w[2], host: w[3], port: Number(w[4]), rssi: Number(w[5]), ssid: w[6] }; io.emit('robotWifi', wifiInfo); }
    const wf = /^WIFI_STA_FAIL status=(-?\d+)/.exec(line);
    if (wf) { wifiInfo = { ...(wifiInfo || {}), staFail: Number(wf[1]) }; io.emit('robotWifi', wifiInfo); }
    if (!line.startsWith('t=')) {
        io.emit('serialMessage', line);
        return;
    }
    const d = {};
    for (const part of line.split(',')) {
        const i = part.indexOf('=');
        if (i > 0) d[part.slice(0, i).trim()] = parseFloat(part.slice(i + 1));
    }
    const n = v => (typeof v === 'number' && isFinite(v) ? v : 0);
    io.emit('serialData', {
        t: n(d.t), theta: n(d.theta), thetaDot: n(d.theta_dot),
        x: n(d.x), xDot: n(d.x_dot), u: n(d.u), st: n(d.st)
    });
}

// ============ KHỞI ĐỘNG ============
server.listen(PORT, HOST, () => {
    console.log('\n==============================================');
    console.log('  XE CAN BANG DONG CO BUOC - ESP32-S3 - LQR');
    console.log(`  Web: http://localhost:${PORT}   (nghe tren ${HOST})`);
    if (lqrConfig.K) console.log(`  K mac dinh = [${lqrConfig.K.map(v => v.toFixed(3)).join(', ')}]`);
    console.log('==============================================\n');
});

process.on('SIGINT', () => {
    console.log('\nĐang tắt...');
    // Dừng xe trước khi thoát
    serialWrite('STOP');
    setTimeout(() => { userClose(); process.exit(0); }, 100);
});

