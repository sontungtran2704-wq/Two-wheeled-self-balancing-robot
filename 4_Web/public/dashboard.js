'use strict';

/* ============================================================
   CSS VARIABLE HELPER
   ============================================================ */
function cssVar(name, alpha) {
    const v = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
    return alpha === undefined ? v : `rgba(${v}, ${alpha})`;
}

/* ============================================================
   UTILITY — debounce & throttle
   ============================================================ */
function debounce(fn, ms) {
    let t;
    return (...args) => { clearTimeout(t); t = setTimeout(() => fn(...args), ms); };
}

/* ============================================================
   STATE
   ============================================================ */
const MAX_POINTS = 200;

const chartData = { timestamps: [], angles: [], speeds: [], outputs: [] };

let lqrParams = { Q: [100, 1, 10], R: 1, K: null };

// Trạng thái từ firmware (trường st=)
const ROBOT_STATES = [
    { name: 'IDLE',   label: 'Chờ',          type: 'info'    },
    { name: 'ARMING', label: 'Đang arm…',    type: 'warning' },
    { name: 'RUN',    label: 'Đang cân bằng', type: 'success' },
    { name: 'FALLEN', label: 'Đã đổ',        type: 'error'   },
    { name: 'FAULT',  label: 'LỖI',          type: 'error'   }
];

let robotState = {
    angle: 0, angularVel: 0, position: 0, velocity: 0, output: 0, st: -1,
    isConnected: false
};

let startTime  = Date.now();
let dataPoints = 0;

// RAF dirty flags — gộp việc vẽ chart & DOM theo tần số màn hình
let needsChartUpdate   = false;
let needsDisplayUpdate = false;

/* ============================================================
   SOCKET
   ============================================================ */
const socket = io();

/* ============================================================
   CHART INSTANCES
   ============================================================ */
let angleChart, speedChart, outputChart;

/* ============================================================
   PRESETS (gợi ý ban đầu cho cơ khí ngày 08/11/2026 - hãy tinh chỉnh theo xe thật)
   ============================================================ */
const SIM_MODEL = { mb: 1.141, mw: 0.033, r: 0.033, l: 0.035, h: 0.14, Ir: 3.6e-6 };
const SIM_PID = { kp: 0.5, ki: 0, kd: 0.2 };
const PRESETS = {
    sim0: { Q: [100, 1, 10], R: 1, label: 'Mô phỏng 0°', angle: 0 },
    sim6: { Q: [100, 1, 10], R: 1, label: 'Mô phỏng 6°', angle: 6 },
    sim10: { Q: [100, 1, 10], R: 1, label: 'Mô phỏng 10°', angle: 10 },
    stable: { Q: [100, 1, 10],      R: 1, label: 'Ổn Định' },
    fast:   { Q: [300, 3, 30],     R: 1, label: 'Phản ứng góc nhanh'   },
    smooth: { Q: [50, 1, 1],       R: 3, label: 'Ưu tiên lực nhỏ'    }
};

/* ============================================================
   DOM READY
   ============================================================ */
document.addEventListener('DOMContentLoaded', () => {
    initCharts();
    attachEventListeners();
    initMobileNav();
    initQrModal();
    loadSavedLQR();
    refreshPorts();
    startRenderLoop();
});

/* ============================================================
   CHARTS — INITIALIZATION
   ============================================================ */
function initCharts() {
    if (typeof Chart === 'undefined') {
        showNotification('Không tải được Chart.js (cần Internet hoặc chạy npm install)', 'error');
        return;
    }
    Chart.defaults.color = cssVar('--gray');

    const gridColor  = 'rgba(255,255,255,0.07)';
    const ticksLimit = { x: 8, y: 6 };

    const baseOptions = {
        responsive:          true,
        maintainAspectRatio: false,
        animation:           false,          // disable built-in animation (RAF handles smoothness)
        plugins: {
            legend: { display: false },
            tooltip: {
                mode:            'index',
                intersect:       false,
                backgroundColor: 'rgba(22,28,40,0.94)',
                titleColor:      cssVar('--gray'),
                bodyColor:       cssVar('--text'),
                borderColor:     cssVar('--border'),
                borderWidth:     1,
                padding:         10,
                cornerRadius:    8,
                displayColors:   false
            }
        },
        interaction: { mode: 'nearest', axis: 'x', intersect: false },
        scales: {
            x: {
                grid:  { color: gridColor },
                ticks: { maxTicksLimit: ticksLimit.x, maxRotation: 0 }
            },
            y: {
                grid:  { color: gridColor },
                ticks: { maxTicksLimit: ticksLimit.y }
            }
        }
    };

    angleChart = new Chart(
        document.getElementById('angleChart').getContext('2d'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label:           'Góc (rad)',
                    data:            [],
                    borderColor:     cssVar('--primary'),
                    backgroundColor: cssVar('--primary-rgb', 0.14),
                    borderWidth:     2,
                    fill:            true,
                    tension:         0.4,
                    pointRadius:     0,
                    pointHoverRadius: 5
                }]
            },
            options: baseOptions
        }
    );

    speedChart = new Chart(
        document.getElementById('speedChart').getContext('2d'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label:           'Tốc Độ (m/s)',
                    data:            [],
                    borderColor:     cssVar('--success'),
                    backgroundColor: cssVar('--success-rgb', 0.14),
                    borderWidth:     2,
                    fill:            true,
                    tension:         0.4,
                    pointRadius:     0,
                    pointHoverRadius: 5
                }]
            },
            options: baseOptions
        }
    );

    $('outputChart').closest('.chart-card').querySelector('h3').textContent = '⚡ Gia tốc điều khiển tương đối';
    outputChart = new Chart(
        document.getElementById('outputChart').getContext('2d'),
        {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label:           'Gia tốc tương đối (m/s²)',
                    data:            [],
                    borderColor:     cssVar('--warning'),
                    backgroundColor: cssVar('--warning-rgb', 0.14),
                    borderWidth:     2,
                    fill:            true,
                    tension:         0.4,
                    pointRadius:     0,
                    pointHoverRadius: 5
                }]
            },
            options: baseOptions
        }
    );
}

/* ============================================================
   RENDER LOOP — requestAnimationFrame
   Coalesces all DOM/chart writes into one frame per screen refresh
   ============================================================ */
function startRenderLoop() {
    function loop() {
        if (needsChartUpdate && angleChart) {
            [angleChart, speedChart, outputChart].forEach((chart, i) => {
                chart.data.labels          = chartData.timestamps;
                chart.data.datasets[0].data = [chartData.angles, chartData.speeds, chartData.outputs][i];
                chart.update('none');
            });
            needsChartUpdate = false;
        }

        if (needsDisplayUpdate) {
            flushDisplay();
            needsDisplayUpdate = false;
        }

        requestAnimationFrame(loop);
    }
    requestAnimationFrame(loop);
}

/* ============================================================
   EVENT LISTENERS
   ============================================================ */
const $ = id => document.getElementById(id);

function attachEventListeners() {
    $('btn-connect').addEventListener('click', connectSerial);
    $('btn-disconnect').addEventListener('click', disconnectSerial);
    $('btn-refresh-ports').addEventListener('click', refreshPorts);
    $('btn-start').addEventListener('click', startRobot);
    $('btn-stop').addEventListener('click', stopRobot);
    $('btn-lqr').addEventListener('click', updateLQRParameters);
    $('btn-wifi-check').addEventListener('click', () => socket.emit('robotControl', { command: 'get' }));
    $('btn-wifi-save').addEventListener('click', () => {
        const ssid = $('wifi-ssid').value, pass = $('wifi-pass').value;
        socket.emit('setWifi', { ssid, pass });
        // Wait for firmware/server feedback; never claim credentials were saved locally.
    });

    const bindSlider = (inputId, labelId, event) => {
        const input = $(inputId), label = $(labelId);
        input.addEventListener('input', e => { label.textContent = e.target.value; });
        input.addEventListener('input', debounce(e => socket.emit(event, parseFloat(e.target.value)), 150));
    };
    bindSlider('speed-input', 'speed-value', 'setSpeed');
    bindSlider('turn-input',  'turn-value',  'setTurn');
    // Selecting a speed never sends a movement command by itself.
    const driveSpeed = () => Math.max(1, Math.min(20, Number($('drive-speed-limit').value) || 5));
    $('drive-speed-limit').addEventListener('input', () => {
        $('drive-speed-selected').textContent = driveSpeed();
    });
    let drivePointer = null;
    const setButtons = (v, w) => {
        $('speed-input').value = v; $('turn-input').value = w;
        $('speed-value').textContent = v; $('turn-value').textContent = w;
        if (socket.connected) { socket.emit('setSpeed', v); socket.emit('setTurn', w); }
    };
    const releaseButtons = () => {
        if (drivePointer === null) return;
        drivePointer = null;
        setButtons(0, 0);
        if (socket.connected) socket.emit('release');
    };
    document.querySelectorAll('[data-drive-v]').forEach(button => {
        button.style.touchAction = 'none';
        button.addEventListener('contextmenu', e => e.preventDefault());
        button.addEventListener('pointerdown', e => {
            if (drivePointer !== null || !socket.connected || robotState.st !== 2) return;
            e.preventDefault(); drivePointer = e.pointerId;
            button.setPointerCapture(e.pointerId);
            if (navigator.vibrate) navigator.vibrate(25);
            setButtons(Math.sign(Number(button.dataset.driveV)) * driveSpeed(), Number(button.dataset.driveW));
        });
        for (const event of ['pointerup', 'pointercancel', 'lostpointercapture'])
            button.addEventListener(event, e => { if (e.pointerId === drivePointer) releaseButtons(); });
    });
    window.addEventListener('blur', releaseButtons);
    document.addEventListener('visibilitychange', () => { if (document.hidden) releaseButtons(); });
    socket.on('disconnect', releaseButtons);


    // Heartbeat: firmware tự về 0 nếu quá 400 ms không nhận SPEED/TURN, nên khi đang giữ
    // tốc độ khác 0 phải gửi lại định kỳ. Đóng tab / mất kết nối server -> xe tự dừng.
    setInterval(() => {
        if (!socket.connected) return;
        const sp = parseFloat($('speed-input').value) || 0;
        const tn = parseFloat($('turn-input').value)  || 0;
        if (sp !== 0 || tn !== 0) { socket.emit('setSpeed', sp); socket.emit('setTurn', tn); }
    }, 150);
}

/* ============================================================
   MOBILE NAV — tab-based panel switching
   ============================================================ */
function initMobileNav() {
    const navBtns = document.querySelectorAll('.mobile-nav-btn');
    const panels  = {
        controls: document.getElementById('panel-controls'),
        charts:   document.getElementById('panel-charts'),
        data:     document.getElementById('panel-data')
    };

    function switchPanel(key) {
        navBtns.forEach(b => b.classList.toggle('active', b.dataset.panel === key));
        Object.entries(panels).forEach(([k, el]) => el.classList.toggle('mobile-active', k === key));

        // Charts need a resize tick when they become visible
        if (key === 'charts') {
            setTimeout(() => {
                [angleChart, speedChart, outputChart].filter(Boolean).forEach(c => c.resize());
            }, 60);
        }
    }

    navBtns.forEach(btn => btn.addEventListener('click', () => switchPanel(btn.dataset.panel)));

    // Set initial state on mobile
    const mq = window.matchMedia('(max-width: 1024px)');
    const applyMobile = (matches) => {
        if (matches) {
            const hasActive = [...Object.values(panels)].some(el => el.classList.contains('mobile-active'));
            if (!hasActive) switchPanel('controls');
        } else {
            // Desktop — clear mobile-active so CSS grid shows all panels
            Object.values(panels).forEach(el => el.classList.remove('mobile-active'));
        }
    };
    applyMobile(mq.matches);
    mq.addEventListener('change', (e) => applyMobile(e.matches));
}

/* ============================================================
   PHONE QR MODAL
   ============================================================ */
function initQrModal() {
    const btn = $('btn-phone-qr');
    const modal = $('modal-qr');
    const closeBtn = $('btn-close-qr');
    const copyBtn = $('btn-copy-phone-url');
    const urlInput = $('dashboard-phone-url');
    const qrContainer = $('dashboard-qr-container');
    if (!btn || !modal) return;
    
    let cachedUrl = null;
    function openModal() {
        modal.hidden = false;
        if (cachedUrl) return;
        fetch('/api/health').then(r => r.json()).then(h => {
            const url = (h.urls && h.urls[0]) || window.location.origin;
            cachedUrl = url;
            if (urlInput) urlInput.value = url;
            if (qrContainer && typeof qrcode === 'function') {
                const qr = qrcode(0, 'M');
                qr.addData(url);
                qr.make();
                qrContainer.innerHTML = qr.createSvgTag({ scalable: true, cellSize: 4, margin: 1 });
            }
        }).catch(() => {
            if (urlInput) urlInput.value = window.location.origin;
        });
    }
    btn.addEventListener('click', openModal);
    if (closeBtn) closeBtn.addEventListener('click', () => { modal.hidden = true; });
    modal.addEventListener('click', e => { if (e.target === modal) modal.hidden = true; });
    if (copyBtn && urlInput) {
        copyBtn.addEventListener('click', () => {
            navigator.clipboard?.writeText(urlInput.value).then(() => {
                copyBtn.textContent = 'Đã chép!';
                setTimeout(() => { copyBtn.textContent = 'Sao chép'; }, 2000);
            });
        });
    }
}

/* ============================================================
   SOCKET EVENTS
   ============================================================ */
socket.on('connect',            () => { updateStatus(true);  showNotification('Đã kết nối máy chủ', 'success'); });
socket.on('disconnect',         () => { updateStatus(false); showNotification('Mất kết nối máy chủ', 'error'); });
socket.on('serialData',         d  => updateRobotData(d));
socket.on('serialConnected',    () => onSerialConnected());
socket.on('serialDisconnected', () => onSerialDisconnected());
socket.on('pidUpdated', p => {
    ['kp', 'ki', 'kd'].forEach(k => { $(`current-${k}`).textContent = p[k].toFixed(3); });
    $('pid-status').textContent = 'PID đọc từ xe (GET).';
});
socket.on('robotWifi', w => {
    const why = { 1: 'không thấy tên WiFi này (sai tên, hoặc router chỉ phát 5 GHz)', 4: 'sai mật khẩu hoặc router không chấp nhận (WPA3-only?)', 6: 'router từ chối/ngắt kết nối' };
    let t;
    if(w.mode==='AP_STA') {
        const home=w.staIp!=='0.0.0.0';
        t=home ? `Xe đã nối mạng nhà. IP: ${w.staIp}:8080. Máy tính nối cùng mạng rồi dùng IP này để điều khiển. `
            : w.configured ? 'Đã có cấu hình mạng nhà nhưng chưa kết nối được. Kiểm tra tên/mật khẩu và mạng 2,4 GHz. ' : 'Chưa cấu hình Wi-Fi nhà. ';
        t+=`Wi-Fi riêng vẫn bật: BalanceBot-Bench, mật khẩu BalanceBot13, IP ${w.apIp}:8080.`;
    } else if (w.mode === 'STA') {
        t = `Xe đang nối WiFi nhà "${w.ssid}", IP ${w.ip}, sóng ${w.rssi} dBm. Máy tính phải cùng WiFi này. Nhập "${w.ip}" vào ô IP/tên xe rồi bấm Kết nối.`;
        const h = $('wifi-host');
        if (!h.value.trim() || h.value.trim() === $('wifi-ssid').value.trim()) h.value = w.ip;
    } else if (w.mode === 'AP') {
        t = `Xe phát WiFi BalanceBot-Bench; mật khẩu BalanceBot13; IP ${w.ip || '192.168.4.1'}, TCP 8080.`;
        if (w.staFail != null) t += ` Không nối được WiFi nhà: ${why[w.staFail] || 'mã ' + w.staFail}.`;
    } else t = `WiFi xe: chế độ ${w.mode}`;
    $('wifi-status').textContent = t;
});
socket.on('lqrUpdated',         p  => onLQRUpdated(p));
socket.on('lqrError',           m  => showNotification(`LQR: ${m}`, 'error'));
socket.on('serialError',        m  => showNotification(`Lỗi: ${m}`, 'error'));
socket.on('notice',             n  => showNotification(n.msg, n.type || 'info'));
socket.on('serialMessage',      m  => onFirmwareMessage(m));
socket.on('linkStatus', s => {
    robotState.isConnected = s.connected;
    $('status-text').textContent = !s.connected ? 'Chưa mở đường truyền' :
        !s.fresh ? 'Đã mở cổng — chưa có telemetry mới' :
        !s.matching ? 'Telemetry khác cấu hình' : '✓ ESP32-S3 — telemetry mới';
    $('status-indicator').className = `status-dot ${s.connected && s.fresh && s.matching ? 'connected' : 'disconnected'}`;
});

// Thông điệp văn bản từ firmware (EVT:..., ERR:..., K UPDATED, SAVED, GYRO OK...)
function onFirmwareMessage(line) {
    if (line === 'ACK:RESET') {
        $('reset-status').textContent = 'Firmware đã xác nhận RESET. Chờ telemetry trạng thái Chờ; motor không tự bật.';
        showNotification('Đã RESET lỗi. Motor không tự bật.', 'success');
        return;
    }
    if (line === 'ERR:RESET_REQUIRED') {
        showNotification('Lỗi đang chốt. Kê xe chắc, xem nguyên nhân rồi bấm RESET lỗi → Chờ.', 'error');
        return;
    }
    if (line === 'ACK:START') {
        showNotification('Firmware đã nhận START. Giữ xe thẳng đứng trong lúc ARM.', 'info');
        return;
    }
    let type = 'info';
    if (line.startsWith('ERR') || line.includes('FAIL')) type = 'error';
    else if (line.startsWith('EVT:FALLEN') || line.startsWith('EVT:FAULT') || line.startsWith('EVT:RUNAWAY') ||
             line.startsWith('EVT:IMU') || line.startsWith('EVT:NAN')) type = 'error';
    else if (line.includes('OK') || line.includes('UPDATED') || line.includes('SAVED') || line === 'EVT:RUN') type = 'success';
    showNotification(line, type);
}

/* ============================================================
   CONNECTION
   ============================================================ */
async function refreshPorts() {
    const sel = $('port-select');
    try {
        const res = await fetch('/api/ports');
        const ports = await res.json();
        const prev = sel.value;
        sel.innerHTML = '';
        if (!ports.length) {
            sel.innerHTML = '<option value="">(không thấy cổng)</option>';
        } else {
            ports.forEach(p => {
                const o = document.createElement('option');
                o.value = p.path;
                o.textContent = p.manufacturer ? `${p.path} — ${p.manufacturer}` : p.path;
                sel.appendChild(o);
            });
            if (prev && ports.some(p => p.path === prev)) sel.value = prev;
        }
    } catch (e) {
        showNotification('Không quét được cổng serial', 'error');
    }
}

function connectSerial() {
    const custom = $('wifi-host').value.trim();
    if (custom && custom === $('wifi-ssid').value.trim()) {     // trình duyệt tự điền tên WiFi nhà vào ô địa chỉ xe
        $('wifi-host').value = '';
        showNotification('Ô IP/tên xe đang chứa tên WiFi nhà, không phải địa chỉ xe. Đã xoá; nhập IP xe (bấm "Kiểm tra WiFi xe") hoặc balancebot.local.', 'warning');
        return;
    }
    // ô nhập IP/tên có ưu tiên hơn danh sách; không ghi cổng thì dùng 8080
    const port = custom ? `tcp:${custom.replace(/^tcp:/, '')}${/:\d+$/.test(custom) ? '' : ':8080'}` : $('port-select').value;
    if (!port) { showNotification('Chọn cổng serial hoặc nhập IP xe trước', 'warning'); return; }
    socket.emit('connectSerial', { port, baudRate: 115200 });
}

function disconnectSerial() {
    socket.emit('disconnectSerial');
}

function onSerialConnected() {
    robotState.isConnected = true;
    if (robotState._reconnTimer) { clearTimeout(robotState._reconnTimer); robotState._reconnTimer = null; }
    $('status-text').textContent = 'Đã mở cổng — đang chờ telemetry';
    showNotification('Đã mở đường truyền; đang chờ dữ liệu ESP32-S3.', 'info');
}

function onSerialDisconnected() {
    robotState.isConnected = false;
    $('status-text').textContent = '✗ ESP32-S3';
    // Việc tự nối lại do server đảm nhiệm (1 bộ hẹn giờ, báo qua thông báo "Tự nối lại...").
    // Dashboard không tự gọi connectSerial nữa để nhiều tab/lần bấm không tranh nhau làm xe bỏ client.
    showNotification('Đã ngắt kết nối với xe', 'warning');
}

function updateStatus(connected) {
    $('status-indicator').className = `status-dot ${connected ? 'connected' : 'disconnected'}`;
    $('status-text').textContent = connected ? 'Kết nối' : 'Chưa kết nối';
}

/* ============================================================
   ROBOT CONTROL
   ============================================================ */
function robotCmd(command) {
    if (!socket.connected) {
        showNotification('Chưa gửi lệnh: mất kết nối máy chủ web.', 'error');
        return;
    }
    socket.emit('robotControl', { command });
}

function sendPid() {
    const g = id => Number(document.getElementById(id).value);
    const p = { kp: g('pid-kp'), ki: g('pid-ki'), kd: g('pid-kd') };
    if (!['kp','ki','kd'].every((k,i) => Number.isFinite(p[k]) && p[k] >= 0 && p[k] <= [3,1,2][i])) {
        showNotification('PID: Kp 0..3, Ki 0..1, Kd 0..2', 'error'); return;
    }
    socket.emit('setPid', p);
    $('pid-status').textContent = 'Đang chờ đọc lại PID từ xe…';
}

function startRobot() {
    robotCmd('start');
}

function resetRobot() {
    if (!socket.connected || !robotState.isConnected) {
        showNotification('Chưa gửi RESET: hãy kết nối lại với xe.', 'error');
        return;
    }
    if ([1, 2].includes(robotState.st)) {
        showNotification('Bấm Dừng trước khi RESET; xe đang ARM hoặc chạy.', 'error');
        return;
    }
    ['speed-input', 'turn-input'].forEach(id => { $(id).value = 0; });
    $('speed-value').textContent = '0';
    $('turn-value').textContent = '0';
    $('reset-status').textContent = 'Đã yêu cầu RESET; đang chờ ACK của firmware.';
    robotCmd('reset');
}

function stopRobot() {
    if (navigator.vibrate) navigator.vibrate([40, 30, 40]);
    robotCmd('stop');
    // đưa các thanh trượt về 0 để lần chạy sau không tự đi
    ['speed-input', 'turn-input'].forEach(id => { $(id).value = 0; });
    $('speed-value').textContent = '0';
    $('turn-value').textContent  = '0';
    socket.emit('setSpeed', 0);
    socket.emit('setTurn', 0);
    showNotification('Đã dừng xe', 'warning');
}

/* ============================================================
   LQR PARAMETERS
   ============================================================ */
function readLQRForm() {
    const num = id => parseFloat($(id).value);
    return {
        Q: [num('q0-input'), num('q1-input')],
        R: num('r-input'),
        model: { mb: num('mb-input'), mw: num('mw-input'), r: num('rw-input'), l: num('l-input'), h: num('h-input'), Ir: num('ir-input') }
    };
}

function writeLQRForm(p) {
    if (Array.isArray(p.Q)) p.Q.forEach((v, i) => { $(`q${i}-input`).value = v; });
    if (p.R != null) $('r-input').value = p.R;
    if (p.model) {
        $('mb-input').value = p.model.mb; $('mw-input').value = p.model.mw;
        $('rw-input').value = p.model.r;  $('l-input').value  = p.model.l;
        $('h-input').value  = p.model.h;
        $('ir-input').value = p.model.Ir == null ? SIM_MODEL.Ir : p.model.Ir;
    }
}

function updateLQRParameters() {
    socket.emit('updateLQR', {...readLQRForm(), send: true});
}

function onLQRUpdated(p) {
    lqrParams = p;
    const f = (v, d = 3) => (v == null ? '—' : Number(v).toFixed(d));
    for (let i = 0; i < 3; i++) $(`current-k${i}`).textContent = f(p.K && p.K[i]);
    $('current-a41').textContent  = f(p.A41);
    $('current-b4').textContent   = f(p.B4, 4);
    $('current-pole').textContent = p.unstablePole == null ? '—' : `+${f(p.unstablePole, 2)} rad/s`;
    $('current-sent').textContent = p.sentToRobot ? 'Có' : 'Chưa';
    writeLQRForm(p);
    showNotification(p.fromFirmware ? 'Đã đọc K hiện tại từ firmware' : p.sentToRobot ? 'Firmware đã xác nhận áp dụng K (đến khi khởi động lại xe)' : 'Đã tính K (chưa gửi xuống xe)',
                     p.sentToRobot ? 'success' : 'info');
}

function loadSavedLQR() {
    try {
        const saved = JSON.parse(localStorage.getItem('lqr_params_20261108'));
        if (saved && Array.isArray(saved.Q) && saved.Q.length === 2) writeLQRForm(saved);
    } catch (_) { /* ignore */ }
}

/* ============================================================
   PRESET BUTTONS
   ============================================================ */
function loadPreset(key) {
    showNotification('Preset mô hình cũ chưa áp dụng cho xe hiện tại. Chỉnh Q góc, Q tốc độ góc và R trong khung LQR.', 'info');
}

/* ============================================================
   DATA PIPELINE
   socket event → state → chartData → RAF flags
   ============================================================ */
let lastSt = -2;
let lastImuDiagnostic;
const firmwareSamples = [];
const MAX_LOG_SAMPLES = 3000;
let lastFaultCapture = null;
let previousSampleFault = false;

function updateRobotData(data) {
    robotState.fault = data.fault;
    robotState.autoWait=data.auto_wait===1;
    robotState.autoMs=Number.isFinite(data.auto_ms)?data.auto_ms:0;
    robotState.imuDiagnostic=data.imu_diag;
    robotState.imuCalibrated=data.imu_calibrated;
    robotState.calAccelMg=data.cal_accel_mg;
    if (Number.isInteger(data.imu_diag) && data.imu_diag !== lastImuDiagnostic) {
        lastImuDiagnostic = data.imu_diag;
        const messages = ['Đang kiểm tra IMU', 'Hiệu chuẩn gyro đạt', 'Lỗi đọc I²C',
            'Mã nhận dạng IMU không khớp', 'Cấu hình thanh ghi IMU thất bại',
            'Gia tốc ngoài dải tĩnh', 'Gyro chưa ổn định',
            'Firmware này chưa có hiệu chuẩn hợp lệ: giữ xe yên và bấm Hiệu chuẩn gyro lần đầu',
            'Không lưu được hiệu chuẩn gyro vào bộ nhớ',
            'Đang kiểm tra dữ liệu với gyro bias đã lưu; không hiệu chuẩn lại'];
        const accel = data.imu_diag === 5 && data.cal_accel_mg > 0
            ? ` (${(data.cal_accel_mg / 1000).toFixed(3)} g; trung bình cần 0,90–1,10 g)` : '';
        const message = data.imu_diag===1 && data.imu_cal_restored===1
            ? 'Đang dùng hiệu chuẩn gyro đã lưu; không hiệu chuẩn lại khi bật nguồn'
            : data.imu_diag===1 && data.imu_cal_saved===1
                ? 'Hiệu chuẩn gyro đạt — đã lưu vào ESP32'
                : messages[data.imu_diag] && messages[data.imu_diag] + accel;
        if (message) showNotification(message, data.imu_diag === 1 ? 'success' : [0,9].includes(data.imu_diag) ? 'info' : 'error');
    }
    robotState.angle      = (data.imu_sample_fresh === 0 || (data.imu_sample_fresh == null && data.imu_calibrated === 0)) ? null : Number(data.theta);
    robotState.angularVel = (data.imu_sample_fresh === 0 || (data.imu_sample_fresh == null && data.imu_calibrated === 0)) ? null : Number(data.thetaDot);
    robotState.position   = data.x ?? null;
    robotState.velocity   = parseFloat(data.xDot)     || 0;
    robotState.output     = data.u ?? null;
    robotState.st         = Number.isFinite(data.st) ? data.st : -1;

    if (robotState.st !== lastSt) {
        lastSt = robotState.st;
        const s = ROBOT_STATES[robotState.st];
        if (s) showNotification(`Trạng thái xe: ${s.label}`, s.type);
    }

    const t = Number.isFinite(data.t) ? data.t : (Date.now() - startTime) / 1000;
    firmwareSamples.push(data);
    if (firmwareSamples.length > MAX_LOG_SAMPLES) firmwareSamples.shift();
    const isFault = [3, 4].includes(robotState.st);
    if (isFault && !previousSampleFault) {
        // Freeze the received pre-fault samples; later Idle/fault traffic must not erase them.
        lastFaultCapture = {
            capturedAt: new Date().toISOString(),
            fault: data.fault || 'UNKNOWN',
            data: firmwareSamples.slice()
        };
    }
    previousSampleFault = isFault;
    pushChartPoint(t);

    dataPoints++;
    needsDisplayUpdate = true;
}

function pushChartPoint(t) {
    chartData.timestamps.push(t.toFixed(2));
    chartData.angles.push(robotState.angle);
    chartData.speeds.push(robotState.velocity);
    chartData.outputs.push(robotState.output);

    if (chartData.timestamps.length > MAX_POINTS) {
        chartData.timestamps.shift();
        chartData.angles.shift();
        chartData.speeds.shift();
        chartData.outputs.shift();
    }
    needsChartUpdate = true;
}

/* DOM write — chỉ gọi từ vòng RAF */
function flushDisplay() {
    const { angle, angularVel, position, velocity, output, st } = robotState;

    $('angle-display').textContent = angle?.toFixed(4) ?? '—';
    $('angular-vel').textContent   = angularVel?.toFixed(4) ?? '—';
    $('pos-display').textContent   = position == null ? '—' : position.toFixed(4);
    const s = ROBOT_STATES[st];
    $('state-display').textContent = s ? s.label : '—';

    const qAngle = $('quick-angle');
    if (qAngle) qAngle.textContent = angle == null ? '—' : `${(angle * 180 / Math.PI).toFixed(1)}°`;
    const qSpeed = $('quick-speed');
    if (qSpeed) qSpeed.textContent = `${(velocity * 100).toFixed(0)} cm/s`;
    const qState = $('quick-state');
    if (qState) {
        qState.textContent = s ? s.label : '—';
        qState.className = `qm-badge ${s ? s.name.toLowerCase() : ''}`;
    }
    if ([3, 4].includes(st)) {
        const detail = robotState.fault === 'SPEED_RAIL'
            ? ' Lệnh tốc độ bánh chạm giới hạn liên tục; đây không phải lỗi vượt góc ngã.' : '';
        $('reset-status').textContent = `Lỗi đang chốt: ${robotState.fault || 'chưa có mã'}.${detail} Đã giữ đoạn dữ liệu trước lỗi: tải JSON trước khi đóng trang. Kê xe chắc rồi bấm RESET lỗi → Chờ.`;
    } else if (st === 0) {
        $('reset-status').textContent = 'Xe đang Chờ. RESET không tự bật motor; START là thao tác riêng.';
    }
    if(robotState.autoWait && [0,3,4].includes(st) && robotState.imuCalibrated===1) {
        const remaining=Math.max(0,5-robotState.autoMs/1000).toFixed(1);
        $('reset-status').textContent=`Tự cân bằng: giữ xe trong ±10° và ổn định liên tục 5 giây (còn ${remaining} s). Ra ngoài vùng hoặc rung sẽ đếm lại. Bấm DỪNG để hủy tự chạy lại.`;
    } else if(st===2) $('reset-status').textContent='Xe đang cân bằng. DỪNG sẽ tắt motor và hủy tự chạy lại; START để chạy lại.';
    if(st===0 && robotState.imuCalibrated===0) {
        const reasons=['Đang kiểm tra IMU','', 'Lỗi I²C','Sai mã IMU','Lỗi cấu hình IMU','Gia tốc chưa hợp lệ','Gyro còn rung','Chờ hiệu chuẩn firmware mới','Lỗi lưu hiệu chuẩn','Kiểm tra bias đã lưu'];
        $('state-display').textContent=reasons[robotState.imuDiagnostic] || 'Chờ IMU';
        $('state-display').title=`Motor chưa ARM. Gia tốc hiệu chuẩn: ${robotState.calAccelMg ?? '—'} mg; trung bình 900–1100 mg; góc khi chưa hiệu chuẩn chỉ dùng chẩn đoán.`;
    } else $('state-display').title='';

    $('angle-value').innerHTML    = `${angle?.toFixed(3) ?? '—'} <small>rad</small>`;
    $('speed-display').innerHTML  = `${velocity.toFixed(3)} <small>m/s</small>`;
    $('output-display').innerHTML = output == null ? '— <small>m/s²</small>' : `${output.toFixed(2)} <small>m/s²</small>`;

    $('data-points').textContent = dataPoints;
    $('uptime').textContent      = fmtTime((Date.now() - startTime) / 1000);

    const validAngles = chartData.angles.filter(Number.isFinite);
    if (validAngles.length > 0) {
        const mn = Math.min(...validAngles).toFixed(3);
        const mx = Math.max(...validAngles).toFixed(3);
        $('angle-range').textContent = `${mn} / ${mx}`;
    }
}

/* ============================================================
   UTILITIES
   ============================================================ */
function fmtTime(sec) {
    const h = Math.floor(sec / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = Math.floor(sec % 60);
    return [h, m, s].map(n => String(n).padStart(2, '0')).join(':');
}

/* ============================================================
   NOTIFICATION SYSTEM — stacking, auto-dismiss, click to close
   ============================================================ */
function showNotification(message, type = 'info') {
    const container = document.getElementById('notification-container');
    const el = document.createElement('div');
    el.className = `notification ${type}`;
    el.textContent = message;
    el.addEventListener('click', () => dismissNotification(el));
    container.appendChild(el);

    // Force reflow so CSS transition fires on add
    el.getBoundingClientRect();
    el.classList.add('visible');

    const timer = setTimeout(() => dismissNotification(el), 3500);
    el._timer = timer;
}

function dismissNotification(el) {
    clearTimeout(el._timer);
    el.classList.remove('visible');
    el.classList.add('hiding');
    el.addEventListener('transitionend', () => el.remove(), { once: true });
}

/* ============================================================
   DATA MANAGEMENT
   ============================================================ */
function clearHistory() {
    firmwareSamples.length = 0;
    lastFaultCapture = null;
    chartData.timestamps.length = 0;
    chartData.angles.length     = 0;
    chartData.speeds.length     = 0;
    chartData.outputs.length    = 0;
    dataPoints = 0;
    startTime  = Date.now();

    [angleChart, speedChart, outputChart].filter(Boolean).forEach(c => {
        c.data.labels            = [];
        c.data.datasets[0].data  = [];
        c.update();
    });

    showNotification('Đã xóa lịch sử dữ liệu', 'success');
}

function exportCSV() {
    if (chartData.timestamps.length === 0) {
        showNotification('Không có dữ liệu để xuất', 'warning');
        return;
    }

    const header = 't_s,theta_rad,omega_rad_s,v_pulse_m_s,yaw_gyro_rad_s,u_relative_m_s2,session,seq';
    const rows = firmwareSamples.map(d =>
        [d.t, d.theta, d.thetaDot, d.xDot, d.yaw, d.relativeAcceleration, d.session, d.seq].join(','));

    downloadBlob([header, ...rows].join('\n'), 'text/csv', `robot_${Date.now()}.csv`);
    showNotification('Đã xuất CSV thành công', 'success');
}

function downloadJSON() {
    const payload = {
        exportedAt: new Date().toISOString(),
        lqrParams,
        robotState,
        totalPoints: firmwareSamples.length,
        data: firmwareSamples,
        lastFaultCapture,
        units: { t: 's', theta: 'rad', thetaDot: 'rad/s', xDot: 'm/s (STEP)', yaw: 'rad/s', relativeAcceleration: 'm/s²' }
    };
    downloadBlob(JSON.stringify(payload, null, 2), 'application/json', `robot_${Date.now()}.json`);
    showNotification('Đã tải JSON thành công', 'success');
}

function downloadBlob(content, mime, filename) {
    const url = URL.createObjectURL(new Blob([content], { type: mime }));
    const a   = Object.assign(document.createElement('a'), { href: url, download: filename });
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(() => URL.revokeObjectURL(url), 1000);
}
