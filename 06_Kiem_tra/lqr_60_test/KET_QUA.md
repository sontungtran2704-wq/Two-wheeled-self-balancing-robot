# Bản thử LQR riêng, ngưỡng 60° — 12/11/2026

Theo yêu cầu người dùng: bỏ gate góc ±4° tại START/Arming; thay ngưỡng FALL bằng ±60° trong profile TestSafetyConfig.h. START ngoài ±60° vẫn bị từ chối, vượt ngưỡng trong Arming hoặc Run sẽ tắt motor. Vẫn yêu cầu gyro chậm dưới 0,3 rad/s liên tục 1 giây trước Run. Giữ điều kiện hiệu chuẩn và đặt zero, không tự START sau boot/RESET.

Giữ nguyên gain LQR, thông số cơ khí đang có, GPIO/IMU/stepper và GeneratedConfig.h. PID bị khóa như bản LQR-only. Không xác nhận thông số cơ khí đã khớp xe thật. GET đánh dấu LQR_60_TEST,PID_DISABLED,ARM_ANGLE_GATE_OFF,FALL_DEG_60; hash cấu hình cơ sở không phân biệt các profile.

Đây là thay đổi hành vi theo yêu cầu, KHÔNG phải bản sửa nguyên nhân SPEED_RAIL. Bảo vệ tốc độ 0,6 m/s liên tục 300 ms vẫn còn, nên lỗi này có thể tái xuất hiện. LQR hiện tại chưa được kiểm chứng phục hồi ở góc 60°.

Kiểm thử host C++ đã chạy PASS: START và ARM tại ±20°/±59°, khóa PID, FALL ngoài ±60° trong Run/Arming, chặn START >60°, chặn tốc độ góc lớn, RESET không bật motor, SPEED_RAIL vẫn ngắt. Kiểm tra nguyên byte gain/hardware/IMU/stepper PASS; ZIP được đọc lại và đối chiếu SHA256. Không nạp hoặc chạy motor thật.

## Cách dùng
1. Giải nén balancebot_lqr_60_test.zip, giữ toàn bộ file cạnh nhau. Mở balancebot_lqr_60_test.ino. Không chỉ chép riêng file .ino vì chương trình còn cần các module .cpp/.h.
2. Tắt nguồn motor; ngắt bridge/Serial Monitor đang chiếm COM. Profile ESP32S3 Dev Module, Flash16MB, OPI PSRAM, Hardware CDC/JTAG, USB CDC On Boot Enabled (core build 3.3.12).
3. Người dùng tự Upload. Sau nạp, nối web, GET và kiểm tra marker LQR_60_TEST trước khi thử.
4. Hiệu chuẩn khi đứng yên motor tắt. Thử trên giá/dây chống ngã, không thử bắt đầu nghiêng 60° trên sàn chỉ vì firmware cho phép. Có cách ngắt nguồn ngay.
5. Xuất JSON khi có lỗi. Để quay lại bản cũ, nạp balancebot_lqr_only.ino. NVS zero được dùng chung, tránh SAVE điểm zero thử ngẫu nhiên.

Build ESP32-S3 core 3.3.12 vừa chạy: PASS, exit 0; flash 939703 bytes (71%), RAM tĩnh 49076 bytes (14%). App SHA256: 40B998F027015421BF24B3C5AEC679D18F2B36EFB8A0C69CEF779282EDA71FB6. Binary lưu trong firmware/ để đối chiếu; ưu tiên Arduino IDE Upload từ sketch đầy đủ, không đoán offset để nạp .bin. Build không chứng minh cân bằng thực tế. Build lại bằng tools/build_lqr_60_test.ps1.
