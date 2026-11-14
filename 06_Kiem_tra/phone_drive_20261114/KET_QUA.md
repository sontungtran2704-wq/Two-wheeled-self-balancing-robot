# Màn hình lái ngang hai tay — 14/11/2026

## Cách dùng

Tự nạp `03_Firmware_ESP32_S3/balancebot/balancebot.ino` cùng đầy đủ file cùng thư mục; hiệu chuẩn gyro thật lần đầu cho image mới như cũ. Điện thoại nối **BalanceBot-Bench / BalanceBot13**, mở **http://192.168.4.1/**, không cần máy tính.

Trang điện thoại bỏ nút QR và hộp QR. Trang thường vẫn có các phần chỉnh thông số, hiệu chuẩn, PID/LQR, CSV/JSON và các biểu đồ cũ. Bấm **🎮 ĐIỀU KHIỂN XE** để mở màn hình riêng:

- Bên trái Tiến/Lùi, bên phải Trái/Phải; một ngón mỗi bên, kết hợp thành một cặp v/w nguyên tử.
- Đủ bốn tổ hợp tiến-trái, tiến-phải, lùi-trái, lùi-phải; thả một ngón chỉ bỏ lệnh ở trục đó. Không đổi dấu/giới hạn yaw so với bản cũ.
- Giữa là chính biểu đồ góc và vận tốc lớn bằng nhau, gia tốc nhỏ hơn; không tạo bản sao hay xóa lịch sử.
- Có thanh tốc độ, Bắt đầu, Dừng, Quay lại. Mở màn hình không tự START khi xe đang dừng. Khi xe đã tự cân bằng, giao diện nhận quyền điều khiển theo luồng cũ.
- Quay lại hủy cả hai lệnh di chuyển và trả biểu đồ/dữ liệu về trang chỉnh thông số; xe tiếp tục cân bằng nếu đang Run. Dừng giữ hành vi motor/khóa tự chạy lại cũ.
- Thả/cancel/lost capture, blur/chuyển ứng dụng, đổi kích thước/hướng, lỗi/non-Run, thoát hoặc mất Wi-Fi đều hủy thao tác giữ; không phát lại lệnh cũ khi nối lại.

## Xoay ngang

Click mở màn hình yêu cầu Fullscreen rồi Orientation Lock khi trình duyệt cho phép. Trình duyệt/HTTP nội bộ/iOS có thể không cho khóa hướng bằng API; khi đó dùng bố cục ngang CSS xoay toàn bộ màn hình và hit testing tự nhiên của trình duyệt. Xoay điện thoại sang ngang để nhìn thuận. Không khẳng định có thể ép khóa hướng của hệ điều hành trên mọi điện thoại. Thử nghiệm có mô phỏng API bị từ chối để xác nhận fallback không chặn điều khiển.

## Phạm vi

Thêm `4_Web/phone-drive.html/.css/.js`; generator nhúng riêng cho ESP32, bỏ QR khỏi HTML phone và không nhúng thư viện QR. `4_Web/public/index.html/style.css/dashboard.js` của PC giữ nguyên. `phone-adapter.js` thêm sự kiện drive nguyên tử, giữ giới hạn |v|≤0.2 m/s, |w|≤0.8 rad/s và session/sequence/lease cũ; giảm hủy hiệu ứng nút vô ích khi đang Idle; GET sau chỉnh PID không ghi đè Q/R đang sửa. Không đổi protocol/C++ điều khiển/IMU/driver. Trong thư mục sketch chỉ `PhonePage.h` và tài liệu `PHONE_WEB.md` khác snapshot trước lần sửa này; các file còn lại đối chiếu từng byte.

## Kiểm chứng

- `web-tests.log`: bộ 8 API/browser tests. Ca phone dùng sự kiện chạm thật của Chrome DevTools với hai ID pointer: đủ bốn tổ hợp v/w, thả một ngón vẫn giữ trục kia, touchCancel, thả khi request còn chạy, thoát khi cả hai ngón còn giữ gửi zero và không STOP, kích thước hai đồ thị chính lớn hơn gia tốc, biểu đồ trở về đúng panel, QR không còn, màn hình ngang fallback, mở view không START, Dừng/Bắt đầu trong view, mất Wi-Fi không chạy lại. Cả PID/LQR/ACK/timeout/CSV/JSON và web máy tính cũ đều kiểm tra hồi quy.
- `host.log`: 5 executable PASS, gồm HTTP binary/gzip/buffer/partial/EAGAIN/timeout, LQR2 guards, cân bằng r4 và hiệu chuẩn/NVS. SDK/motor thật không được dùng trong các mocks.
- `firmware-build.log`: biên dịch ESP32-S3/N16R8 với core 3.3.12, warnings all. Không nạp hoặc gửi lệnh tới xe.
- `bundle-check.log`: compiled source khớp nguồn cuối, các file firmware ngoài phạm vi byte-identical, asset/model hash đúng, gzip/ZIP/binary đọc lại khớp. `SHA256.json` lưu hash gói.

Ảnh mô phỏng: `forward-right-two-fingers.png` thể hiện hai nút giữ đồng thời; `portrait-landscape-fallback.png` khi API khóa hướng bị từ chối; các ảnh biểu đồ và dữ liệu ở cùng thư mục.

Gói: `balancebot-phone-drive-source.zip` đầy đủ sketch; `balancebot.ino.bin` app binary (không phải ảnh flash gộp); `verification-inputs.zip` nguồn giao diện/test/mock/tool/mô hình/thư viện/license; `before-source.zip` sketch trước thay đổi.

Chưa thử trên điện thoại/xe thật, chưa đo RF/độ trễ/jitter, không khẳng định sửa lỗi SPEED_RAIL hay Wi-Fi cũ. Firmware mới vẫn cần người dùng tự hiệu chuẩn thật lần đầu trước khi hoạt động.
