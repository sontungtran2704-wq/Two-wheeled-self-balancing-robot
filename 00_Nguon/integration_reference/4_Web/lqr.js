'use strict';
/* ============================================================
   lqr.js - Mô hình xe tự cân bằng + bộ giải LQR (không phụ thuộc thư viện)
   Mô hình: con lắc ngược trên xe (cùng mô hình với bb_stepper_design.m)
   Trạng thái z = [theta, theta_dot, x_dot], đầu vào F (N, lực tại mặt đất)
   Quy ước: theta > 0 = nghiêng ra trước.
   ============================================================ */

const G = 9.81;

// ---------- đại số ma trận nhỏ ----------
const zeros = (n, m) => Array.from({ length: n }, () => new Array(m).fill(0));
const eye = n => { const I = zeros(n, n); for (let i = 0; i < n; i++) I[i][i] = 1; return I; };
const T = A => A[0].map((_, j) => A.map(r => r[j]));
const mul = (A, B) => A.map(r => B[0].map((_, j) => r.reduce((s, v, k) => s + v * B[k][j], 0)));
const add = (A, B) => A.map((r, i) => r.map((v, j) => v + B[i][j]));
const sub = (A, B) => A.map((r, i) => r.map((v, j) => v - B[i][j]));
const scale = (A, c) => A.map(r => r.map(v => v * c));

// ---------- mô hình ----------
const DEFAULT_MODEL = { mb: 1.141, mw: 0.033, r: 0.033, l: 0.035, h: 0.14, Ir: 3.6e-6 };

function buildModel(p) {
    const { mb, mw, r, l, h } = p;
    const Ir = p.Ir == null ? 3.6e-6 : p.Ir;
    const Iw = 0.5 * mw * r * r;
    const M  = 2 * mw + 2 * (Iw + Ir) / (r * r);
    const I  = mb * h * h / 12;
    const J  = I + mb * l * l;
    const Mt = M + mb;
    const D  = J - (mb * l) * (mb * l) / Mt;
    const a  = mb * G * l / D;
    const b  = -(mb * l / Mt) / D;
    const c  = -(mb * l / Mt) * a;
    const d  = (1 - mb * l * b) / Mt;
    const A = [[0, 1, 0], [a, 0, 0], [c, 0, 0]];
    const B = [[0], [b], [d]];
    const A41 = c, B4 = d, FrefGain = -a / b;
    return { A, B, A41, B4, FrefGain, unstablePole: Math.sqrt(a), M, Mt, J, D };
}

// ---------- LQR: tích phân phương trình Riccati ngược thời gian tới trạng thái dừng ----------
function riccatiRHS(P, A, B, Q, Rinv) {
    const At = T(A), Bt = T(B);
    const PBR = mul(mul(P, B), scale(Bt, Rinv));            // P B R^-1 B'
    return add(sub(add(mul(At, P), mul(P, A)), mul(PBR, P)), Q);
}

function solveLQR(A, B, Qdiag, R) {
    const n = A.length;
    if (!Array.isArray(Qdiag) || Qdiag.length !== n) throw new Error(`Q phải gồm ${n} số`);
    const Q = zeros(n, n); for (let i = 0; i < n; i++) Q[i][i] = Qdiag[i];
    const Rinv = 1 / R;
    let P = zeros(n, n);
    const dt = 1e-3, Tmax = 200;
    let converged = false, resid = Infinity;
    for (let k = 0; k < Tmax / dt; k++) {
        const k1 = riccatiRHS(P, A, B, Q, Rinv);
        const k2 = riccatiRHS(add(P, scale(k1, dt / 2)), A, B, Q, Rinv);
        const k3 = riccatiRHS(add(P, scale(k2, dt / 2)), A, B, Q, Rinv);
        const k4 = riccatiRHS(add(P, scale(k3, dt)), A, B, Q, Rinv);
        P = add(P, scale(add(add(k1, scale(k2, 2)), add(scale(k3, 2), k4)), dt / 6));
        if (k % 200 === 0) {
            resid = Math.max(...riccatiRHS(P, A, B, Q, Rinv).flat().map(Math.abs));
            if (!isFinite(resid)) break;
            if (resid < 1e-9) { converged = true; break; }
        }
    }
    const Kmat = scale(mul(T(B), P), Rinv);                  // 1x3
    return { K: Kmat[0], P, converged, resid };
}

// ---------- ổn định vòng kín: hệ số đa thức đặc trưng + Routh-Hurwitz ----------
function charPoly(M) {                                       // Faddeev-LeVerrier, trả về các hệ số đa thức đặc trưng
    const n = M.length; const c = [1]; let Mk = zeros(n, n);
    for (let k = 1; k <= n; k++) {
        Mk = add(mul(M, Mk), scale(eye(n), c[k - 1]));
        const AM = mul(M, Mk);
        let tr = 0; for (let i = 0; i < n; i++) tr += AM[i][i];
        c.push(-tr / k);
    }
    return c;
}

function isHurwitz(c) {                                      // Routh array, mọi phần tử cột đầu > 0
    let r1 = [], r2 = [];
    for (let i = 0; i < c.length; i += 2) r1.push(c[i]);
    for (let i = 1; i < c.length; i += 2) r2.push(c[i]);
    const first = [r1[0], r2[0]];
    while (r1.length > 1 || r2.length > 0) {
        const next = [];
        for (let j = 0; j < r1.length - 1; j++) {
            const b = r2[j] === undefined ? 0 : r2[j];
            const b1 = r2[j + 1] === undefined ? 0 : r2[j + 1];
            next.push((r2[0] * r1[j + 1] - r1[0] * b1) / r2[0]);
        }
        if (next.length === 0) break;
        first.push(next[0]); r1 = r2; r2 = next;
        if (first.length >= c.length) break;
    }
    return first.every(v => v > 1e-12);
}

// ---------- hàm tổng hợp ----------
function design(model, Qdiag, R) {
    const m = buildModel(model);
    if (!Qdiag.every(isFinite) || !isFinite(R) || R <= 0) throw new Error('Q, R không hợp lệ');
    const { K, converged, resid } = solveLQR(m.A, m.B, Qdiag, R);
    if (!converged) throw new Error(`Riccati không hội tụ (dư ${resid.toExponential(2)})`);
    if (!K.every(isFinite)) throw new Error('K không hữu hạn');
    // kiểm tra ổn định vòng kín
    const BK = mul(m.B, [K]);
    const Acl = sub(m.A, BK);
    const stable = isHurwitz(charPoly(Acl));
    if (!stable) throw new Error('Vòng kín không ổn định - kiểm tra lại mô hình/Q/R');
    return { K, A41: m.A41, B4: m.B4, FrefGain: m.FrefGain, unstablePole: m.unstablePole, stable, resid };
}

module.exports = { design, buildModel, solveLQR, charPoly, isHurwitz, DEFAULT_MODEL };
