# Đăng ký/đăng nhập, trạng thái và khóa Bắt đầu — 14/11/2026

## Cách dùng

Tự nạp `03_Firmware_ESP32_S3/balancebot/balancebot.ino` cùng đầy đủ file cùng thư mục; không bật Erase All Flash nếu muốn giữ các bản lưu. Điện thoại nối **BalanceBot-Bench / BalanceBot13**, mở **http://192.168.4.1/** như cũ.

Trang Đăng nhập dùng chính HTML/CSS/JS của web máy tính được nhúng, bỏ QR và đổi thông tin nơi lưu sang ESP32. Bấm **Đăng ký**, nhập tên, mật khẩu và nhập lại. Đăng ký chỉ báo thành công sau khi bộ nhớ ESP32 được ghi/đọc lại thành công, sau đó đăng nhập. Tài khoản lưu trên xe, không tự đồng bộ tài khoản máy tính; tối đa 8 tài khoản, dùng lại qua bật nguồn/nạp firmware mới nếu NVS được giữ. Đăng ký thêm khi motor đang chạy bị từ chối; đăng nhập tài khoản có sẵn để Dừng trước rồi đăng xuất/đăng ký.

Firmware mới vẫn yêu cầu hiệu chuẩn gyro thực tế lần đầu đạt/lưu thành công trước khi được phép chạy. Tài khoản và hiệu chuẩn là hai bản lưu độc lập; đăng ký không giả hiệu chuẩn đạt.

## Thay đổi theo yêu cầu

- Trạng thái lớn kiểu terminal, giữa trên cùng màn hình lái: xanh “ĐANG CÂN BẰNG”, đỏ “LỖI: <mã lỗi>” hoặc mất kết nối.
- Phải chủ động bấm Bắt đầu sau đăng nhập/kết nối, kể cả xe đã tự cân bằng. Mở view không tự nhận quyền. ESP32 cũng chặn CMD/MOTION/SPEED/TURN trước START, không chỉ vô hiệu hóa nút.
- Sau ACK Bắt đầu, cả nút ngoài và nút trong view giữ tối/khóa; lỗi không làm sáng lại. Dừng trả nút xanh. Mất kết nối/đăng xuất yêu cầu Bắt đầu lại.
- Bỏ chữ hướng dẫn giữ hai tay, chữ nhắc xoay, Cấu hình sẵn và miêu tả dưới Reset trên bản phone. Node reset-status giữ ẩn cho mã dashboard dùng chung; PC public files không thay đổi.
- Còn nguyên điều khiển hai ngón, các đồ thị, xuất CSV/JSON, PID/zero/gyro và LQR trước đó.

## Hợp đồng lưu/xác thực

`PhoneAuth.h` lưu tối đa 8 user với username chuẩn hóa chữ thường, PBKDF2-HMAC-SHA256 (20.000 vòng, salt 16 byte, hash 32 byte), CRC/readback NVS `phone-auth/users-v1`. Không lưu mật khẩu thô. Bản ghi chứa rounds để dùng lại độc lập image firmware. Slot đã công bố không sửa; count atomic release/acquire để network đọc an toàn khi control task append user. Corrupt database bị khóa ghi đè.

Hash chạy trên network task. Dòng WEBUSER chỉ mang username/salt/hash, được đưa vào hàng đợi nội bộ. Control task kiểm tra không Run/Arming/đang hiệu chuẩn, giữ motor off rồi ghi/readback; không thể đua với task khác bật motor. WEBUSER bị chặn từ HTTP command thông thường và USB/TCP ngoài. Thành công trả 201 thật sau commit; lỗi không giả đã lưu. Đăng ký có thể bị từ chối nếu xe tự vào Run trong lúc người dùng đang nhập/đang hash.

HTTP cookie phiên HttpOnly/SameSite Strict, token cùng nguồn, TTL 8 giờ, logout thu hồi; login mới thay phiên trước. Tối đa 20 lần thử đăng ký/login mỗi phút chung trên xe. Cookie/token/Host/Origin được kiểm tra trước API trạng thái/điều khiển. `/session` không còn phát token cho người chưa đăng nhập. HTTP body max192 byte để hỗ trợ password128 byte; command vẫn max160. Auth exchange timeout8s, trao đổi khác1.5s; frame/parser/TX vẫn bounded. Đây là HTTP nội bộ, không TLS; truy cập trong Wi-Fi xe. Không thay auth/accounts của Node trên máy tính.

## Bằng chứng thực thi

- `host.log`: **6 executable PASS**. Auth dùng BCrypt PBKDF2 SHA256 ở host qua cùng signature mbedTLS, khớp vector Python; tạo nhiều tài khoản, chuẩn hóa tên/duplicate, giới hạn8, CRC, write failure, power-cycle đọc lại không ghi, không plaintext. HTTP kiểm tra chưa login bị401, chưa START bị403, private command bị chặn, token/cookie/Origin/Host, logout/expiry, gzip/partial/EAGAIN/timeout. LQR2 guards, r4 Run giữ vận tốc0, ngưỡng/dwell/STOP latch và IMU/NVS cũ đều PASS.
- `web-tests.log`: **8/8 PASS**, gồm API và browser PC cũ. Phone test thực thi đăng ký/nhập lại sai, lưu rồi login, sai password, logout/API401/relogin dùng user đã lưu; đổi session khi START, chặn trước START và sau reconnect, latch tối/Stop sáng, terminal centered/green/red + SPEED_RAIL, bỏ QR/preset/hints, hai ngón đủ bốn tổ hợp, biểu đồ và CSV/JSON/PID/LQR/ACK/timeout giữ nguyên.
- Target SDK dùng mbedTLS thật, đã đối chiếu chữ ký hàm với SDK 3.3.12 và compile warnings all: **1,080,035/1,310,720 byte flash**, **56,372/327,680 byte global RAM**. Host crypto/NVS là bằng chứng phần mềm; chưa đo thời gian PBKDF2/flash/jitter trên ESP32 thật.
- `bundle-check.log`/`SHA256.json`: compiled source và header khớp nguồn cuối, file ngoài phạm vi không đổi byte, model/shared assets/gzip dashboard + welcome/ZIP/binary đọc lại khớp.

Gói `balancebot-phone-auth-source.zip` là toàn bộ sketch; `verification-inputs.zip` đủ nguồn/test/mock/tool/thư viện/license; `balancebot.ino.bin` là app binary, không phải ảnh flash gộp; `before-source.zip` là snapshot trước thay đổi này. Ảnh: `phone-login.png`, `forward-right-two-fingers.png`, `fault-terminal.png` (dữ liệu/credentials test giả).

Chưa nạp, mở COM, ghi tài khoản thật hay gửi lệnh motor thật. Không thay logic/ngưỡng tự cân bằng, hiệu chuẩn gyro, pin, chiều motor, LQR/PID mặc định hay driver. Chưa xác nhận RF, độ trễ hoặc NVS khi mất nguồn trên xe thật.
