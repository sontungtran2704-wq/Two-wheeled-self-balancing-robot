# Khôi phục đúng bản r4 theo ảnh người dùng — 14/11/2026

Người dùng đổi yêu cầu từ sửa kết nối sang khôi phục bản r4 trong ảnh. Dừng hướng sửa TCP; không áp dụng bản vá TCP hay thay đổi SDK. Bản trước khi khôi phục được lưu đủ 16 file trong `before-restore-source.zip`, đã đối chiếu readback trước khi ghi đè.

## Kết quả khôi phục

- Tất cả 15 file trong sketch chính khớp từng byte với `standalone_20261114_r4/balancebot-standalone-r4-source.zip`. ZIP r4 có SHA256 `ca6dd9174fff4be768ca08107cb4abc67c3cd1d7f07687bb21573c8ad5d10a4a`.
- Những file thực sự khác và đã trả về r4: `ControlCore.cpp`, `Runtime.cpp`, `README.md`. Xóa riêng `WifiSettings.h` là file thêm sau r4, sau khi sao lưu và xác minh nó trùng bản Wi-Fi trước khi khôi phục. Không sửa tay GeneratedConfig; các file còn lại đã trùng r4 nên không ghi lại.
- Phát hiện khi đối chiếu: ControlCore đang có `active_?velocity.update(...):0`, khác cơ chế r4 `motion||active_`. Khôi phục r4 trả PID giữ vận tốc 0 ngay khi vào Run. Không quy kết ai hoặc lần sửa nào đã làm thay đổi dòng này.
- Dashboard.js và hai nguồn cấu hình shared khớp bản kiểm chứng r4; không đổi web/server đang chạy, bố cục, tài khoản hoặc thông số cơ khí.
- R4 mặc định SSID biên dịch trống, chạy AP riêng `BalanceBot-Bench`, mật khẩu `BalanceBot13`, TCP `192.168.4.1:8080`. Bản này chưa có lưu Wi-Fi nhà `WIFI_*`, `wifi_cfg=1` và AP+STA của bản Wi-Fi sau đó; các nút Wi-Fi nhà hiện có trên web không được coi là firmware đã hỗ trợ.
- Giữ LQR, PID 3/1/0.2, ngưỡng đổ 40°, tự phục hồi khi đứng ổn định dưới 10° liên tục 5 s và cơ chế hiệu chuẩn thực tế theo từng image. STOP/RESET chủ động vẫn ngăn tự bật motor.

## Kiểm chứng vừa chạy

- `tools/build_standalone_host.ps1`: 3 executable host/mocks PASS. Bổ sung hồi quy trong `tests/cpp/standalone_tests.cpp`: vận tốc ±0.45 m/s ở pitch 5° phải được PID tác động ngược sai lệch ngay nhịp đầu, trong khi gate tiến/lùi/yaw vẫn đóng; không có link vẫn giữ mục tiêu 0; trạng thái chưa chạy không bật PID.
- Arduino CLI compile ESP32-S3 core 3.3.12, `USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi`, jobs 1: PASS. Flash 951099/1310720 byte, RAM 49076/327680 byte. Đây là lệnh build vừa chạy, không lấy log PASS cũ thay thế.
- Binary vừa build có SHA256 `b398397584f41ec657b774c1a4ee8e368ab524ad4346d362edc91f08c6144ead`, trùng binary r4 đã lưu. Source/compiled C++ và header, ZIP readback, binary readback được kiểm tra bằng `verify_restore.py`.
- Không chạy suite Wi-Fi mới vì phần chức năng đó đã được người dùng yêu cầu quay lui. Không gọi nó PASS trên r4. Không đổi firmware sau khi build.

## Dữ liệu kết nối và giới hạn kết luận

Trước khôi phục, Windows có IP 192.168.32.132 và đang nối Kingshouse-4.3_5G; web dùng IP AP 192.168.4.1. Nhật ký WLAN 16:25:38–16:25:42 ghi BalanceBot-Bench không khả dụng rồi driver ngắt trong lúc association; Windows nối mạng khác. Điều này giải thích ENETUNREACH nhưng chưa xác định AP mất do nguồn/reset/radio/STA hay adapter Windows. Không tự đổi cấu hình Wi-Fi Windows, không tắt firewall.

Đoạn firmware cũ tự đóng TCP khi nonblocking send trả ngắn/EAGAIN và đường Serial đồng bộ có khả năng làm chậm network task đã được nhận diện bằng source SDK; **không sửa** các điểm này vì người dùng chuyển sang khôi phục đúng r4. Chưa đo trên xe nên không gọi chúng là nguyên nhân chắc chắn của việc Wi-Fi Windows bị rời mạng.

Chưa nạp firmware, mở COM, gửi CALGYRO/START, khởi động lại server hoặc thử motor. Host/build không chứng minh Wi-Fi ổn định hay xe cân bằng thật.

## Người dùng thực hiện

1. Mở `03_Firmware_ESP32S3/balancebot/balancebot.ino`, giữ đủ 15 file cùng thư mục; kê xe chắc, nguồn motor tắt, ngắt bridge/Serial Monitor khỏi COM rồi tự nạp. Giữ thiết lập N16R8 đã xác minh, Erase All Flash Disabled.
2. Nối Wi-Fi máy tính vào BalanceBot-Bench, mật khẩu BalanceBot13; “không có Internet” là bình thường. Dùng web `http://127.0.0.1:3000`, nhập IP xe `192.168.4.1:8080`, bấm Kết nối. Chỉ hiệu chuẩn khi telemetry mới và đúng phiên bản.
3. Nếu báo Chờ hiệu chuẩn firmware mới, giữ nguồn motor tắt và thân xe đứng yên, bấm Hiệu chuẩn gyro; đợi `imu_calibrated=1` và `imu_cal_saved=1`. ACK pending không chứng minh đã đạt. Cùng binary r4 và bản lưu hợp lệ có thể dùng lại hiệu chuẩn; không gán số bias giả.
4. Nếu Windows vẫn tự rời Wi-Fi xe, chưa thử motor; cần thu log boot/reset và trạng thái AP tại đúng thời điểm mất kết nối. Khôi phục r4 chưa được kiểm chứng giải quyết hiện tượng này.
