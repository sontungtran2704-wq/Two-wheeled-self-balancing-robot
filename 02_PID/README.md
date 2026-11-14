# PID vận tốc dọc và yaw ngoài LQR

Từ thư mục gốc:

```powershell
python tools/design.py
python tools/simulate.py
matlab -batch "run('02_PID_ChuyenDong/run_all.m')"
```

MATLAB R2024b + Control System Toolbox + Simulink. Script dựng và chạy
`PID_ChuyenDong.slx` với khối PID/LQR rời rạc riêng khỏi plant phi tuyến RK4.
LQR 200 Hz, PID 50 Hz, cùng JSON/gain với firmware. Khi thay cấu hình, phải
chạy lại để cập nhật Model Workspace trong SLX. Không chỉnh tay gain trong SLX.

Lệnh 52 s: nghỉ, tiến, lùi/đổi chiều, quay trái, quay phải, tiến và quay đồng
thời, hết hạn lệnh, tắt PID chuyển động êm, bật lại và lùi, rồi dừng. PID chỉ
được kích hoạt sau 1 s liên tục đủ điều kiện pitch. `fresh=0` mô phỏng lệnh đã
hết lease; phép kiểm tra timeout giao thức thực nằm trong kiểm thử C++/web.

Đầu ra `results/`: `motion_52s.csv/png`, `motion.mat`, `evidence.json`, nhật ký;
`controller_matlab.csv` và `controller_simulink.csv` đối chiếu cùng 2400 mẫu
đầu vào từ `shared/controller_vectors.csv`. Đây là mô phỏng điều khiển số và
plant, không kiểm chứng IMU, GPIO, jitter FreeRTOS hoặc mất bước vật lý.

PID vận tốc sinh gia tốc đặt; góc tham chiếu giải cân bằng có phản lực mô-men
động cơ. LQR phản hồi `[theta,omega]`, không phản hồi thêm sai số vận tốc.
PID yaw sinh gia tốc yaw; bộ trộn ưu tiên phần chung cân bằng, áp giới hạn
tốc độ và gia tốc trên từng bánh trước khi dành phần còn lại cho yaw.

Các gain PID trong JSON là cấu hình đã đánh giá mô phỏng, chưa xác nhận xe.
Không có encoder: vận tốc dọc phản hồi từ xung cộng thành phần pitch `r*omega`,
còn yaw phản hồi gyro. Sai số STEP/mất bước làm sai vận tốc thật dù đồ thị
ước lượng có thể đẹp. Điều kiện đạt mô phỏng không phải giấy phép chạy xe.
