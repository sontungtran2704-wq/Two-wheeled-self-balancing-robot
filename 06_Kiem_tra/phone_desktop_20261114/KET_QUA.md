# Giao diện điện thoại dùng chung web máy tính — 14/11/2026

Yêu cầu: giao diện và các chức năng điều khiển giống web máy tính, có hiệu ứng nhấn/giữ nút; dùng trực tiếp khi tắt máy tính, giữ thông số và hành vi cân bằng hiện có.

## Bản bàn giao

Mở `03_Firmware_ESP32_S3/balancebot/balancebot.ino`, giữ đầy đủ file cùng thư mục rồi tự nạp. Điện thoại nối **BalanceBot-Bench / BalanceBot13**, chọn tiếp tục sử dụng Wi-Fi không Internet, mở **http://192.168.4.1/**. Firmware mới yêu cầu hiệu chuẩn gyro thực tế lần đầu đạt/lưu thành công; các lần bật nguồn sau dùng lại kết quả.

- Chính HTML/CSS/dashboard.js/Chart.js/QR của web máy tính được nhúng và nén gzip vào flash; không cần tải CDN hay server máy tính.
- Ba thẻ Điều khiển / Biểu đồ / Dữ liệu trên điện thoại, ba cột trên màn hình lớn, đúng responsive CSS của web máy tính.
- Hiệu ứng lún, sáng và viền khi nhấn/giữ; hủy khi thả, pointercancel, mất capture, blur hoặc mất kết nối. Dùng chung `button-feedback.js` và CSS trên cả hai giao diện. Script chỉ thay hình thức, không tự gửi lệnh.
- START/STOP/Reset, tiến/lùi/xoay, thanh tốc độ, gyro/zero/PID/SAVE, đồ thị, CSV/JSON, giữ dữ liệu trước lỗi đều dùng cùng dashboard.
- Tính LQR trong trình duyệt bằng chính thuật toán `4_Web/lqr.js` và mô hình được kiểm tra hash; gửi LQR2 qua hàng đợi cũ. Firmware kiểm tra Idle, IMU hợp lệ/hiệu chuẩn, config hash, số hữu hạn |K|≤10000 và Jury ổn định rời rạc trước ACK. Không báo áp dụng khi chỉ gửi bytes hoặc hết hạn chờ ACK.
- K mặc định giữ nguyên; thay K chỉ xảy ra khi người dùng bấm gửi hợp lệ, không lưu trong SAVE và trở về mặc định khi khởi động lại.

## Phạm vi và giới hạn tương đương

Giữ nguyên pin/chiều motor, GeneratedConfig/GeneratedLqrModel, PID mặc định, xử lý gyro/NVS, bộ điều khiển bước và toàn bộ ngưỡng/dwell/lease/các lỗi/tự phục hồi. `ControlCore.cpp/.h` chỉ thêm biến K mặc định bằng cfg, handler/readback LQR2 và capability; phần tính tick chỉ thay tham chiếu K cố định bằng K hiện tại, giá trị mặc định như cũ. `Runtime.cpp` thêm trạng thái PID/K/IMU đọc từ bản sao view cho giao diện. `PhoneWeb.h` thêm gzip/binary response và ID phản hồi, bộ đệm đủ GET. Parser HTTP giữ nguyên.

Kết nối trên ESP32 là phiên HTTP trực tiếp tới chính xe; không có cổng COM trên điện thoại. Đăng xuất ngắt phiên giao diện và đưa mục tiêu chuyển động về 0, không phải đăng xuất tài khoản Node. Tài khoản/đăng nhập server máy tính không được chuyển sang NVS ESP32; trang trực tiếp dựa trên mật khẩu AP, token và kiểm tra cùng nguồn. Phần Wi-Fi nhà vẫn ẩn và preset cũ vẫn chỉ thông báo chưa áp dụng như web máy tính hiện tại. Đây là các khác biệt kết nối/khả năng gốc, không giả lập thao tác lưu/áp dụng thành công. Không thêm captive portal hay ứng dụng cài đặt.

## Kiểm chứng thực thi

- `host.log`: PASS HTTP gzip/binary đúng byte, header/body/token/Host/Origin, partial send/EAGAIN, lỗi/timeout/wrap, GET dài; PASS LQR2 không nhận khi chưa hiệu chuẩn/Arming/Fault/IMU cũ, từ chối hash/số/K không ổn định, readback K và reboot trở mặc định. PASS toàn bộ standalone r4 và IMU/NVS mocks: giữ vận tốc 0 ngay Run, 40° đổ, dưới 10° ổn định 5 giây tự phục hồi, STOP latch, hiệu chuẩn theo firmware.
- `web-tests.log`: **8/8 PASS**: 6 API cũ, kiểm thử trình duyệt web máy tính, kiểm thử giao diện trực tiếp. Trình duyệt kiểm tra HTML/CSS/dashboard/feedback chính là file máy tính, góc nhìn 390×844 không tràn ngang, giữ/thả/thả lúc request đang chạy gửi zero, STOP khi status bận, solver K bằng máy tính đến 1e-12 ở bốn bộ Q/R, ACK/timeout LQR thật, PID đọc lại, CSV/JSON có dữ liệu, giữ log trước SPEED_RAIL, ba thẻ/ba cột, mất Wi-Fi không phát lại chuyển động, không tài nguyên bên ngoài/lỗi JS.
- Bộ browser cũ ban đầu gặp panel Wi-Fi nhà đã `hidden` sẵn trong index trước thay đổi. Fixture được sửa để chỉ mở panel trong test mô phỏng, không đổi hiển thị sản phẩm. Sau đó cả suite đạt; `desktop-regression.log` giữ kết quả lần đầu để không che lịch sử.
- `firmware-build.log`: biên dịch ESP32-S3 core 3.3.12, FQBN `esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi`, warnings all. Binary dùng **1,073,619/1,310,720 byte flash**, biến toàn cục **55,124/327,680 byte RAM**.
- `.build/phone-page-inputs.json` được chép vào gói: hash file gốc và kiểm tra gzip đọc lại. `verify_bundle.py` đối chiếu nguồn biên dịch, file cũ không thuộc phạm vi, ZIP đọc lại và binary; xem `bundle-check.log`/`SHA256.json`.

Ảnh từ dữ liệu mô phỏng: `phone-held.png`, `phone-charts.png`, `phone-data.png`, `desktop-direct.png`.

## Gói lưu

`balancebot-phone-desktop-source.zip`: toàn bộ sketch tự chứa; `verification-inputs.zip`: nguồn giao diện/server/mô hình/tests/tool/mock/Chart.js và license cần tái lập; `balancebot.ino.bin`: app binary sau build, không phải ảnh flash gộp; `before-source.zip`: snapshot sketch trước thay đổi lần này.

Chưa nạp, mở COM, thay mạng Windows hoặc phát lệnh tới xe thật. Chưa đo độ trễ/jitter/độ bền Wi-Fi và chưa thử trên điện thoại/xe thật. Không khẳng định giao diện mới xử lý nguyên nhân SPEED_RAIL hay nguyên nhân Wi-Fi rớt trong ảnh cũ.
