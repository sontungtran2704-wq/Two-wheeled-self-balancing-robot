# Web điện thoại độc lập — 14/11/2026

Yêu cầu: bổ sung điều khiển từ điện thoại khi máy tính tắt, giữ nguyên phần còn lại.

## Cách dùng

Mở `03_Firmware_ESP32_S3/balancebot/balancebot.ino`, giữ toàn bộ file cùng thư mục rồi tự nạp. Sau khi nạp firmware mới, hiệu chuẩn gyro thật một lần và chờ chỉ báo đạt/đã lưu. Các lần bật nguồn sau dùng lại kết quả theo cơ chế hiện có.

Điện thoại nối **BalanceBot-Bench** / **BalanceBot13**, chọn tiếp tục dùng Wi-Fi không có Internet, mở **http://192.168.4.1/**. Máy tính không cần bật hoặc kết nối trước đó. Giao diện và mã JavaScript nằm trong flash ESP32, không dùng CDN.

Điện thoại có trạng thái, Bắt đầu/Dừng/ESTOP/Reset, nhấn giữ tiến/lùi/xoay, thanh tốc độ tiến/lùi, hiệu chuẩn gyro, zero, PID vận tốc và lưu. Trang này là giao diện gọn; đồ thị, xuất dữ liệu và tính LQR đầy đủ vẫn ở web máy tính. Không cài app/captive portal trong thay đổi này.

## Phạm vi

- `Runtime.cpp`: thêm HTTP cổng 80 vào network task, source lệnh 3, đọc bản sao trạng thái và đưa lệnh vào hàng đợi cũ. USB/TCP8080 tiếp tục hoạt động.
- Thêm `PhoneHttp.h`, `PhoneWeb.h`, `PhonePage.h`, `PHONE_WEB.md`.
- Thêm kiểm thử `tests/cpp/phone_web_tests.cpp`, `4_Web/tests/phone_direct.test.js`, lệnh chạy `tools/build_phone_host.ps1`.
- Các file cũ ngoài Runtime giữ nguyên từng byte so với `before-source.zip`: ControlCore/LQR/PID, giới hạn ngã/tự phục hồi, điều kiện hiệu chuẩn theo firmware, pin, chiều motor, IMU và driver bước. `verify_bundle.py` thực thi đối chiếu này.
- HTTP dùng bộ đệm cố định, RX/TX và thời gian chờ giới hạn; không sửa tác vụ điều khiển/IMU/ISR. Dùng HTTP polling cùng nguồn để không thêm thư viện hoặc phụ thuộc Node.js/Socket.IO lên ESP32. Xem hợp đồng chi tiết trong `PHONE_WEB.md`.

## Bằng chứng

- `host.log`: PASS parser/HTTP, kiểm tra token/Host/Origin, gói sai/quá dài, ACK thật, gửi từng phần/EAGAIN, lỗi/timeout/wrap; PASS bộ kiểm thử r4 cân bằng/khởi động/phục hồi và gyro/NVS thật qua mocks.
- `browser.log`: PASS Edge headless ở viewport điện thoại 390×844; hiệu chuẩn đang chờ không mở START; giữ/thả và thả khi lệnh đang gửi đều gửi zero; Dừng khi status bận không bị mất; blur/mất mạng hủy giữ, nối lại không phát lại chuyển động cũ; không yêu cầu tài nguyên bên ngoài, không lỗi JavaScript/không tràn ngang. `phone-390.png` là ảnh từ dữ liệu giả lập.
- `firmware-build.log`: kết quả biên dịch ESP32-S3, core 3.3.12, `USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi`, warnings all. Biên dịch nguồn cuối được đối chiếu với `build/sketch` trước khi đóng gói.
- `bundle-check.log`, `SHA256.json`: đối chiếu nguồn cũ, nguồn biên dịch, ZIP đọc lại và bản sao binary.

## Bàn giao

`balancebot-phone-source.zip` chứa toàn bộ sketch. `verification-inputs.zip` chứa đầu vào kiểm thử; `balancebot.ino.bin` là app binary sau build, không phải một ảnh gộp bootloader/partition. `before-source.zip` là sao lưu trước thay đổi. Script `verify_bundle.py` có thể chạy lại sau build để kiểm tra và tạo gói.

Chưa nạp firmware, mở cổng COM, gửi START/CALGYRO tới xe hay thay đổi mạng Windows. Chưa đo Wi-Fi thật, độ trễ/jitter hoặc thử điều khiển trên điện thoại thật. Các kiểm thử mô phỏng không chứng minh đã khắc phục nguyên nhân Wi-Fi cũ bị rớt. Người dùng tự nạp và kiểm tra trong khung chống ngã; bản firmware mới sẽ yêu cầu hiệu chuẩn lần đầu theo chức năng hiện có.
