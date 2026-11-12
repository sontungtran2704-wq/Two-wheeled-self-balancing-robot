"""Generate report only from stored, observed numerical results."""
from pathlib import Path
import json,hashlib,platform,importlib.metadata
root=Path(__file__).resolve().parents[1]
data=json.loads((root/'06_KiemThu/results/metrics.json').read_text())
rows={x['scenario']:x for x in data}
lines=['# Kết quả mô phỏng Python','',
       'Dữ liệu phi tuyến có giới hạn STEP; không phải kết quả thử xe. Cấu hình `nema17-relative-v1`.','',
       '| Ca | Pitch đỉnh (°) | Xác lập góc (s) | x cuối (m) | u đỉnh (m/s²) | Bão hòa (s) | Ngã |',
       '|---|---:|---:|---:|---:|---:|---|']
for x in data[:6]:
    lines.append(f"|{x['scenario']}|{x['theta_peak_deg']:.4f}|{x['settling_s']}|{x['drift_m']:.4f}|{x['control_peak_m_s2']:.4f}|{x['saturation_s']:.4f}|{x['fell']}|")
lines+=['','Xác lập góc: vào và giữ |θ|≤0.5°, |ω|≤0.05rad/s đến cuối mô phỏng. Với ca chuyển động nhiều đoạn, cột này là thời điểm cuối cùng hết quá độ, không phải thời gian đáp ứng của từng đoạn. x cuối ca chuyển động là dịch chuyển có chủ đích cộng sai số, không gọi toàn bộ là trôi.','',
        'Bộ tinh chỉnh là lựa chọn đánh đổi tốc độ góc, độ trôi và công điều khiển, không tuyên bố tối ưu toàn cục. `qr_sweep.csv` chứa12 ứng viên. LQR riêng giữ góc nhưng còn vận tốc dư; bật PID đặt0 để giảm vận tốc trong giả định bám bước.','',
        '| Cuối đoạn (s) | Sai số v (m/s) | Sai số yaw (rad/s) | Pitch (°) |','|---|---:|---:|---:|']
for x in rows['motion']['plateau_errors']:lines.append(f"|{x['t_s']:.1f}|{x['v_error']:.5f}|{x['yaw_error']:.5f}|{x['theta_deg']:.4f}|")
checks={'pitch_settle_under_2s':rows['tuned_5deg']['settling_s']<2,'motion_pitch_under_8deg':rows['motion']['theta_peak_deg']<8,'motion_plateau_velocity_under_003':all(abs(x['v_error'])<.03 for x in rows['motion']['plateau_errors']),'motion_plateau_yaw_under_008':all(abs(x['yaw_error'])<.08 for x in rows['motion']['plateau_errors'])}
lines+=['','## Kiểm tra tiêu chí','']+[f'- {k}: {"PASS" if v else "FAIL"}' for k,v in checks.items()]
lines+=['','## Giới hạn','',
'Quét một tham số mỗi lần và một lưới trễ/gain bám; không chứng minh ổn định với mọi tổ hợp bất định. Đầu ra torque chỉ là yêu cầu mô hình, chưa có đường cong pull-out motor để kết luận bám được. Mô hình yaw không có trượt ngang/ma sát; IMU lý tưởng trong các ca chính, chưa xác minh bộ lọc trên xe.',
'','![Cân bằng](../06_KiemThu/results/balance_comparison.png)','', '![Chuyển động](../06_KiemThu/results/motion.png)']
(root/'05_BaoCao/08_Ket_qua_Python.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
manifest=dict(python=platform.python_version(),packages={p:importlib.metadata.version(p) for p in ['numpy','scipy','matplotlib','openpyxl']},config_sha256=hashlib.sha256((root/'shared/robot.json').read_bytes()).hexdigest(),checks=checks,physical_status='NOT_RUN')
(root/'06_KiemThu/results/python_verification.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(checks));assert all(checks.values())
