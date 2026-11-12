# Balancebot stepper – ESP32-S3 – PID ngoài + LQR trong

Cập nhật 08/11/2026: xem [kết quả đối chiếu](../../5_Bao_cao/KET_QUA_CO_KHI_20261108.md). Firmware mặc định dùng K = [-32.36122708, -2.51018007, -3.16227766], A41 = -6.33185948, B4 = 1.31993760, Fref = 12.22925950; PID = 0.5 / 0 / 0.2. Bán kính 0.033 m, track 0.228 m, 3200 bước/vòng (15433.2066 bước/m). Gain NVS từ cơ khí cũ được bỏ qua; WiFi và zeropoint được giữ. Sau nạp gửi `GET`, kiểm tra dấu IMU/chiều bánh, `ZPHERE` ở điểm cân bằng, rồi `SAVE` khi IDLE.


## Phần cứng
- Chỉ hỗ trợ ESP32-S3 (ví dụ ESP32-S3-DevKitC-1). Chọn board `ESP32S3 Dev Module`, bật `USB CDC On Boot`.
- Nối dây: STEP_L=GPIO4, DIR_L=GPIO5, STEP_R=GPIO6, DIR_R=GPIO7, EN=GPIO15, SDA=GPIO8, SCL=GPIO9 (MPU6050 cấp 3V3). LED RGB onboard GPIO38 (board v1.0 là GPIO48, sửa `PIN_RGB`).
- Module có OPI PSRAM (N8R8/N16R8) chiếm GPIO26–37; các chân trên không trùng. `Config.h` có `static_assert` chặn chân bị cấm và `#error` nếu build cho chip khác.

## Kiến trúc điều khiển
- PID vị trí vòng ngoài: `e_x = x_ref - x_hat` -> `theta_ref`.
- LQR cân bằng vòng trong: `z = [theta - theta_ref, theta_dot, v_hat]` -> lực điều khiển `F`.
- `x_hat` và `v_hat` được ước lượng từ số xung STEP đã phát. Không có encoder nên firmware không phát hiện được mất bước hoặc trượt bánh.
- `F` được đổi qua mô hình `xdd = A41*theta + B4*F`, tích phân thành tốc độ đặt cho 2 động cơ bước.

## Thông số cần hiệu chỉnh
1. Mở `1_MATLAB/bb_pid_lqr_design.m`, nhập khối lượng thân, khối lượng bánh, bán kính bánh, chiều cao trọng tâm và mô-men động cơ thực tế.
2. Tính lại 3 hệ số LQR vòng trong và `A41`, `B4`; nạp bằng lệnh `$K,k_theta,k_thetaDot,k_velocity,A41,B4,FrefGain`.
3. `POS_KP=0.5`, `POS_KI=0`, `POS_KD=0.2`, `THETA_REF_MAX` trong `Config.h` là gain PID ngoài ban đầu để mô phỏng/tinh chỉnh. Không coi là gain đã được xác nhận trên xe thật.
4. Bắt đầu với `POS_KI=0`; chỉnh `POS_KP`, `POS_KD` khi thử nghiệm an toàn. Chỉ tăng `POS_KI` khi đã giảm được sai lệch tĩnh và có anti-windup.

## Lệnh
`$P,Kp,Ki,Kd` (chỉnh PID vị trí khi đang chạy, giới hạn Kp≤3, Ki≤1, Kd≤2), `JOG,<L cm/s>,<R cm/s>,<ms>` (quay thử bánh khi IDLE, tối đa 15 cm/s và 2 s), `START`, `STOP`, `SPEED,<cm/s>`, `TURN,<deg/s>`, `ZP,<deg>`, `ZP+`, `ZP-`, `ZPHERE`, `CALGYRO`, `$K,k1,k2,k3,A41,B4,FrefGain`, `SAVE`, `DEFAULTS`, `GET`, `WIFI,<ssid>,<pass>`, `REBOOT`.

## Cảnh báo
- Hệ số mặc định đã đối chiếu MATLAB/Python theo cơ khí cung cấp ngày 08/11/2026; chưa xác nhận trên xe thật.
- Kiểm tra dấu góc, chiều quay bánh, giới hạn nghiêng và driver khi nhấc bánh khỏi mặt đất trước khi thử cân bằng.
- Thử không tải trước; tăng tải từng bước. Dừng ngay nếu động cơ mất bước, nóng bất thường hoặc xe dao động tăng dần.
