# LQR cân bằng pitch

Từ thư mục gốc dự án, chạy:

```powershell
python tools/design.py
matlab -batch "run('01_LQR_CanBang/run_all.m')"
```

Yêu cầu: MATLAB R2024b, Control System Toolbox, Simulink. Không cần Symbolic Math
Toolbox hay Simulink Coder. `run_all.m` dựng lại `LQR_CanBang.slx`, tính `c2d`
ZOH và `dlqr` độc lập rồi kiểm tra với `shared/design.json`. Cấu hình duy nhất là
`shared/robot.json`; chỉ `tools/design.py` xuất gain triển khai/header C++.

Mở `LQR_CanBang.slx` sau khi chạy để xem vòng phản hồi có khối điều khiển và
khối động học phi tuyến riêng. Các khối Level-2 MATLAB S-function lấy mã ở
`shared/`. Mô hình chứa cấu hình trong Model Workspace; không phải video hay
phát lại dữ liệu CSV. Khi thay JSON, chạy lại script để cập nhật cấu hình SLX.

Kịch bản: lệch ban đầu 5° (gain cơ sở và tinh chỉnh), 10°, nhiễu mô-men đẩy
0.025 N·m trong 0.1 s ở giây 3, khối lượng thân 0.9/1.4 kg, COM 0.025/0.06 m.
Gain và thông số điều khiển giữ danh định khi khảo sát tham số **plant**.
Chế độ chỉ LQR tắt PID chuyển động; vận tốc và vị trí không được điều chỉnh.
Đồ thị và CSV thể hiện độ trôi còn lại, không gọi đó là giữ vị trí.

Đầu ra `results/`: `evidence.json`, `design.mat`, `qr_sweep.csv` (36 bộ Q/R),
CSV/PNG từng kịch bản và `matlab_run.log`. CSV ghi SI, đồ thị pitch dùng độ;
`evidence.json` gắn config ID/hash và phiên bản toolbox.

Kiểm tra tự động: hạng điều khiển được/quan sát được phần pitch, cực rời rạc,
dấu phản hồi, gain MATLAB–Python, vòng lặp MATLAB–Simulink, hữu hạn và giới hạn
truyền động. Mô hình plant giả định bánh không trượt/mất bước. Hai trạng thái
pitch có quan sát được không đồng nghĩa vận tốc tuyệt đối có thể đo từ IMU.
