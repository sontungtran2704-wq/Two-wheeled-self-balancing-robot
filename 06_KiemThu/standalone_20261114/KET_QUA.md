# Bàn giao thay đổi độc lập web — 14/11/2026

Yêu cầu cuối: không đo lại gyro khi bật nguồn; hiệu chuẩn thực tế một lần qua web và lưu; giữ CALGYRO khi cần. Bật nguồn có thể tự cân bằng sau 5 giây liên tục trong ±10°, hoặc dùng START thủ công. FALL tại ±40°; chỉ FALL được tự phục hồi. STOP/ESTOP/RESET và thao tác cấu hình hủy tự chạy lại.

## Bằng chứng nguyên nhân và phạm vi

- Source gốc `ImuHal.cpp` gọi `requestCalibration()` ngay khi task khởi động và không có ghi gyro bias vào NVS. Source `Device` bắt đầu Idle, chỉ START chuyển Arming, mọi Fault giữ chốt. Đã giữ bản gốc tại `before/balancebot` cùng robot.json/design.json trước sửa.
- Chưa đọc bias của xe thật, chưa kết nối COM hoặc chạy CALGYRO trên phần cứng trong phiên này. Source không chứa bias giả để sử dụng trên xe. Giá trị mẫu trong tests chỉ thuộc mock, không được đóng gói vào firmware.
- Sửa shared/robot.json đúng arm=10°, fall=40°, auto_rearm_s=5; chạy tools/design.py sinh lại design/header và mô hình giao thức. K vẫn [-23.793396834761598, -2.7542335398052704], PID vẫn 3/1/0.2, cơ khí/GPIO/tốc độ/gia tốc/STEP và hardware_confirmed không đổi. Hash mới f713711d2dd1267aa36c0c2f214783cd0f568f78af534471ced4ef95d0ea5654.
- ControlCore thêm chế độ tự chạy do Runtime bật sau khởi tạo; kiểm tra IMU, độ nghiêng thô/đã zero, omega/yaw và liên tục thời gian. Sau phục hồi khởi tạo lại controller, mục tiêu về 0, đổi session. Lỗi khác FALL không được tự xóa. START thủ công vẫn Arming 1 giây, nay gate 10°. Không sửa thuật toán điều khiển.
- SavedImuCalibration.h mã hóa blob có version, CRC32, WHO_AM_I, địa chỉ/pin/trim, bias và thống kê chất lượng; khóa riêng imu-cal-v1/gyro. ImuHal chỉ ghi sau CALGYRO đo đạt và kiểm tra đọc lại; khi bật nguồn chỉ đọc và kiểm tra 100 mẫu với bias cũ, không tính/ghi bias mới. Thiếu/hỏng/lỗi I2C hoặc bias không phù hợp: không cho IMU valid. Lỗi ghi: diag8, motor không sẵn sàng.
- Runtime chuyển xử lý CALGYRO về task sở hữu trạng thái, kiểm tra motor/busy ở lúc thực thi, hủy tự chạy trước hiệu chuẩn/ghi flash. Không sửa PID/zero khi lưu gyro. Giữ đọc PID/zero từ hash ngay trước lần đổi ngưỡng này, đã xác minh chỉ safety khác; không chấp nhận hash tùy ý.
- Web chỉ thêm phản hồi đã lưu/đang đọc lại, đếm 5 giây và nhận quyền điều khiển khi firmware báo đã tự Run. Không đổi bố cục. Nhận quyền ở Run không gửi START lần nữa.

## Kiểm chứng vừa thực hiện

- Host C++ `tools/build_standalone_host.ps1 -Compiler <zig.exe>`: **3 executable PASS**, log host-final.log. Kiểm tra boot/FALL đủ 5 giây, biên ±10°, FALL40°, ra vùng đếm lại, STOP/RESET/hiệu chuẩn hủy tự chạy, lỗi IMU/DEADLINE/ESTOP/SPEED_RAIL/watchdog, dữ liệu stale/NaN/chuyển động/gap/wrap, mục tiêu0/sessionmới; CRC, truncation, context/version/bias bất thường.
- Mock chạy **ImuHal.cpp thực** với Wire/NVS/clock trong RAM: boot không hiệu chuẩn/không ghi; CALGYRO đo/lưu/readback, reboot đọc lại không thay bias, hiệu chuẩn lại có ghi; dữ liệu hỏng, residual không phù hợp, I2C và ghi thất bại chặn ready. Đây không phải NVS/IMU thật.
- Python cấu hình **5/5 PASS**, configuration-final.log; mô hình **2/2 PASS**. So sánh giá trị K/PID/model với bản trước PASS, chỉ safety nguồn đổi. Không coi kiểm tra mô hình là xe cân bằng.
- Web API **6/6 PASS** trong web-tests.log; kiểm thử browser ban đầu lỗi do fixture gọi nhầm updateDisplay, đã sửa sang flushDisplay và chạy lại browser **1/1 PASS**, browser-test-final.log. Browser dùng TCP giả, không gửi lệnh tới xe thật.
- Arduino CLI ESP32-S3 core3.3.12, N16R8, USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi, jobs1: **BUILD PASS**. Flash 950647/1310720 byte; RAM 49076/327680 byte. firmware-build.log. Lần gọi đầu trong sandbox không truy cập được Arduino15; build ngoài sandbox bằng toolchain đã cài đạt, không nạp.
- Đối chiếu toàn bộ .cpp/.h mà Arduino sao chép để build với source hiện tại: PASS sau bỏ directive #line do Arduino thêm. ZIP được đọc lại và so từng byte với source: PASS. SOURCE_SHA256.json ghi hash các nguồn/check/binary.
- Suite legacy tests/cpp/control_tests.cpp chưa dùng cho kết luận này: checkout đầu phiên đã không có WifiSettings.h/LQR2 firmware mà suite đó tham chiếu. Không khôi phục tính năng ngoài phạm vi để ép suite PASS. Host regression mới dùng code hiện có và có oracle theo yêu cầu mới.

## Artifact và giới hạn

- balancebot-standalone-source.zip: mở balancebot/balancebot.ino với tất cả file cùng thư mục.
- balancebot.ino.bin SHA256 10bc893028dea07b0a27e8cc47939c1ac64f27a01bfe38c7c89ecdb7c66cae9b; chỉ là artifact build, chưa nạp.
- Xem README của sketch cho thao tác nạp/hiệu chuẩn lần đầu và sử dụng. Erase All Flash Disabled để giữ NVS. Gyro chỉ được coi là lưu sau đo thực tế/đọc lại đạt, không suy ra từ build hay ACK pending.
- Web đang chạy chưa được chủ động restart trong phiên này; không biết xe có đang chạy nên không dừng server. Sau khi dừng/ngắt kết nối và tự nạp, phải khởi động lại đúng server dự án để đọc hash mới. Không dừng tất cả node.exe.
- Chưa kiểm chứng: dữ liệu gyro/zero thực tế, giữ NVS qua mất điện, nguồn/nhiệt độ, thời gian vòng điều khiển thật, chiều motor, bắt cân bằng sau dựng10°, hoạt động không web, phần firmware thực sự đang nằm trên board. Source hiện có trước sửa chưa có Wi-Fi nhà/LQR2; không đồng nhất nó với binary đã nạp ở các phiên trước. Các sketch lqr_only/lqr_60_test không được sửa.
