# Firmware: bước tiếp theo là kiểm tra cảm biến

GPIO đã được người dùng xác nhận: STEP L4/DIR L5, STEP R6/DIR R7,
EN chung15, SDA8/SCL9. A4988 VDD3V3; MS1–MS3 cao nên1/16; SLP/RST3V3.
Module flash/PSRAM, hướng IMU và chiều quay motor chưa kiểm chứng.

Mặc định `ROBOT_REFERENCE_PINMAP=1`, `ROBOT_HARDWARE_CONFIRMED=1`.
Driver giữ tắt khi khởi động và chỉ bật sau START và ARM thành công.
Đặt `ROBOT_HARDWARE_CONFIRMED=0` để build bản khóa motor.

Từ thư mục gốc:

```powershell
./tools/build_host.ps1
./tools/build_firmware.ps1
```

Build dùng Arduino CLI1.5.1/core ESP32 3.3.12, target tham chiếu ESP32-S3,
USB CDC, flash 16 MB, OPI PSRAM 8 MB theo mã ESP32-S3-N16R8 trên module
thật. Script chỉ compile, không upload. Bin/log ở `.build/esp32s3-n16r8`.

Giữ VMOT ngắt khi nạp qua COM13. Đọc Serial 115200. Giữ xe yên khi
khởi động để hiệu chuẩn gyro500 mẫu và kiểm tra `imu_calibrated=1`,
`imu_who=104`, theta/omega khi nghiêng thân và `hardware_confirmed=1`.
`GET` đọc trạng thái, `CALGYRO` yêu cầu hiệu chuẩn lại khi motor tắt.
Kiểm tra dấu: nghiêng trước chậm → theta dương; omega cùng dấu thay đổi góc.
Không có encoder: v là ước lượng từ xung và gyro, không phát hiện mất bước.

Khi thử EN, ngắt VMOT 12 V và cấp logic bằng USB. Đo GPIO15 và EN ở từng
A4988 so với GND chung. Cả ba điểm phải gần 3,3 V khi IDLE. Với nguồn đã ngắt,
kiểm tra thông mạch từ GPIO15 đến hai EN và dùng điện trở kéo lên 3,3 V tại
đường EN để giữ driver tắt cả khi ESP32 reset.

Bản khóa motor phát `BalanceBot-Bench` nếu SSID router để trống. Gửi
`GET` qua USB để đọc IP. Mật khẩu mặc định cố định là `BalanceBot13`,
có thể đổi tại cấu hình biên dịch `ROBOT_BENCH_AP_PASSWORD`.
Kết nối máy tính vào AP và nhập IP xe vào ô Wi-Fi trên dashboard, TCP 8080.

Dashboard trong `4_Web` sử dụng giao thức sau.
`START`, `STOP`, `ESTOP`, `RESET`, `MOTION,0|1`, `SPEED,cm/s`, `TURN,deg/s`,
`CMD,session,sequence,v_m_s,yaw_rad_s`, `$P,V|Y,kp,ki,kd`, `GET`.
Gain LQR chỉ đọc, `$K` bị từ chối. STOP tắt driver; ESTOP chốt fault.
Mất lệnh400ms đưa chuyển động về0 qua ramp, giữ LQR nếu còn đủ điều kiện.

Xem `THU_XE.md` để thử từng bánh rồi mới cân bằng. Chưa upload/chạy xe trong
phiên làm việc này; thời gian tick và jitter trên board chưa được đo.


