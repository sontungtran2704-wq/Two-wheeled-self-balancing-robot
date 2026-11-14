# Bản thử chỉ LQR — 12/11/2026

Đây là sketch riêng; bản chính ở ../balancebot được giữ nguyên. Mục đích là
tách tác động vòng ngoài khi chẩn đoán xe chưa cân bằng. Không bảo đảm xe đứng
yên hoặc hết đổ. Gain LQR và thông số cơ khí vẫn là bộ hiện tại, chưa đo lại.

## Khác biệt

- Lõi cưỡng bức motion=false ở mọi tick; PID vận tốc/yaw không được kích hoạt.
- MOTION, SPEED, TURN, CMD và $P trả ERR:LQR_ONLY, kể cả khi web kéo slider.
- GET phát EVT:CONTROL,LQR_ONLY,PID_DISABLED để nhận diện bản thử.
- Giữ START/STOP/ESTOP/RESET, hiệu chuẩn gyro, zero, SAVE, telemetry và Wi-Fi.
- Giữ bảo vệ góc ngã 22°, ARM 4°/1 giây, IMU/deadline/speed rail như bản chính.
- GeneratedConfig.h là bản sao nguyên byte, không sửa hoặc tính gain mới.
- Dùng cùng NVS balancebot: điểm zero đã lưu có thể được đọc lại; các gain PID
  lưu trong NVS không tác động vòng LQR. SAVE/đặt zero có thể ảnh hưởng điểm
  zero khi quay về bản chính. Không dùng SAVE để thử ngẫu nhiên.

## Tự nạp qua Arduino IDE

1. Kê xe chắc, ngắt nguồn motor trước thao tác nạp. Giữ nguồn USB cho board.
2. Bấm Ngắt kết nối trên web, đóng Serial Monitor và mọi công cụ đang giữ COM.
3. Mở balancebot_lqr_only.ino trong thư mục này, không mở sketch balancebot cũ.
4. Chọn ESP32S3 Dev Module (module N16R8), Flash Size 16MB, PSRAM OPI,
   USB Mode Hardware CDC and JTAG, USB CDC On Boot Enabled; core tham chiếu
   3.3.12. Đây là profile build được giữ từ dự án, không kết luận mọi cổng
   WCH/native USB đều dùng chung một đường Serial. Nếu không có telemetry,
   kiểm tra cổng USB/cấu hình trước, không đổi cờ phần cứng để vượt lỗi.
5. Chọn cổng thực tế trong IDE (COM17 là cổng từng quan sát, có thể thay đổi).
   Bạn tự bấm Upload. Không mass erase/eFuse; không cần xóa toàn bộ flash.
6. Đóng Monitor trước nối lại web; 115200 baud hoặc Wi-Fi BalanceBot-Bench,
   mật khẩu BalanceBot13, TCP 192.168.4.1:8080.
7. Bấm Kiểm tra WiFi xe (GET), cần thấy EVT:CONTROL,LQR_ONLY,PID_DISABLED.
   Telemetry hash vẫn giống bản chính; hash không dùng để phân biệt hai bản.

## Trước khi thử cân bằng

- Chưa START: motor tắt; hiệu chuẩn gyro khi xe đứng yên, chờ đạt. Đối chiếu
  góc và dấu pitch khi nghiêng trước/sau; cần kiểm tra riêng chiều hai bánh.
- Chỉ thử trên giá/dây giữ, có thể ngắt nguồn ngay, sau khi các dấu đã đúng.
  Bạn tự quyết định/bấm START; bản thử vẫn phải ARM đủ điều kiện.
- Không kéo slider; bản này cố ý từ chối chuyển động. LQR chỉ giữ pitch,
  không triệt vận tốc trôi. Không tăng giới hạn/gain để che lỗi ngã.
- Nếu đổ: STOP, ghi fault và log trước lỗi, rồi mới RESET về Chờ. Không tự
  khởi động lại sau RESET. Số xung/vận tốc báo không chứng minh motor bám đúng.
- Quay lại bản chính: nạp ../balancebot/balancebot.ino bằng profile cũ.

Build lại: tools/build_lqr_only.ps1 từ gốc dự án. Binary chỉ dành cho profile
trên; không nạp file app .bin tại địa chỉ phỏng đoán. Upload bằng Arduino IDE
hoặc đúng lệnh/offset trong build metadata nếu dùng công cụ nạp riêng.
