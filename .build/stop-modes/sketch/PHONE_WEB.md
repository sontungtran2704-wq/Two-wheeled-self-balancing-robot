#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32_S3\\balancebot\\PHONE_WEB.md"
# Web điều khiển trực tiếp trên điện thoại

## Đăng ký/đăng nhập và quyền điều khiển — cập nhật mới nhất 14/11/2026

Địa chỉ **http://192.168.4.1/** mở trang Đăng nhập cùng bố cục của web máy tính, không QR. Bấm **Đăng ký**, nhập tên, mật khẩu và nhập lại mật khẩu. Chỉ sau khi ESP32 ghi/đọc lại thành công mới báo đăng ký thành công; sau đó đăng nhập để mở dashboard. Lưu tối đa 8 tài khoản trên chính xe, dùng lại sau tắt nguồn/nạp firmware nếu không xóa NVS. Tài khoản xe và máy tính là hai nơi lưu riêng. Đăng ký thêm khi xe đang chạy: đăng nhập tài khoản có sẵn để Dừng trước, rồi đăng xuất/đăng ký; ghi tài khoản chỉ khi motor tắt và không hiệu chuẩn.

Mật khẩu được băm PBKDF2-HMAC-SHA256 với salt ngẫu nhiên 16 byte, 20.000 vòng và hash 32 byte, lưu có CRC/readback trong NVS `phone-auth/users-v1`. Hash chạy trên network task; chỉ control task kiểm tra motor tắt rồi commit. Bản ghi độc lập gyro/PID/hash firmware. Lỗi lưu không báo đã lưu; bản ghi hỏng không tự ghi đè. API yêu cầu phiên thật trên ESP32, cookie HttpOnly/SameSite Strict, token cùng nguồn, hết hạn 8 giờ; giới hạn chung 20 lần thử đăng ký/đăng nhập mỗi phút. Tên 3–32 chữ/số/_, mật khẩu 8–128 byte. Đây là web HTTP trong Wi-Fi xe.

Đăng nhập, nối lại hoặc mở màn hình lái không tự cho phép di chuyển. Phải bấm **Bắt đầu**, kể cả xe đã tự cân bằng. Nếu xe đã Run, thao tác nhận quyền giao diện, không START lại motor; nếu đang dừng, vẫn qua điều kiện hiệu chuẩn/START cũ. Sau ACK, nút Bắt đầu tối cố định; Dừng trả nút xanh bình thường. Lỗi không tự làm sáng nút. Mất kết nối/đăng xuất khóa lại quyền; không giữ thao tác nhấn cũ. Thanh trạng thái lớn giữa phía trên: xanh khi cân bằng, đỏ kèm mã lỗi khi Fault/mất kết nối. Đã bỏ chữ hướng dẫn hai tay, chữ nhắc xoay, mục Cấu hình sẵn và dòng miêu tả dưới Reset trên bản điện thoại. Node Reset được giữ ẩn để dashboard dùng chung vẫn chạy.

## Màn hình lái riêng (cập nhật 14/11/2026)

Trang điện thoại đã bỏ nút QR và hộp QR; bản web máy tính giữ nguyên QR. Màn hình bình thường tiếp tục có chỉnh thông số, hiệu chuẩn, PID/LQR, xuất dữ liệu và ba biểu đồ qua các thẻ cũ.

Bấm **🎮 ĐIỀU KHIỂN XE** để mở màn hình lái ngang: bên trái Tiến/Lùi, bên phải Trái/Phải; giữ một nút mỗi bên bằng hai ngón để kết hợp tiến/lùi với rẽ. Hai ngón độc lập: thả bên trái chỉ đưa vận tốc tiến/lùi về 0, còn giữ bên phải vẫn xoay; thả bên phải chỉ đưa xoay về 0. Có thanh tốc độ, nút Bắt đầu và Dừng ngay trên màn hình lái.

Giữa màn hình là chính các biểu đồ đang chạy: góc và vận tốc lớn bằng nhau, gia tốc nhỏ hơn. **Quay lại** đưa mục tiêu tiến/lùi và xoay về 0, trả các biểu đồ cùng dữ liệu về màn hình chỉnh thông số, không chủ động tắt chức năng cân bằng. Mở màn hình không tự gửi START hoặc nhận quyền; phải bấm Bắt đầu, không phục hồi thao tác giữ cũ.

Trang yêu cầu toàn màn hình và khóa hướng ngang khi trình duyệt hỗ trợ. HTTP nội bộ/iOS hoặc cài đặt điện thoại có thể từ chối API này; khi đó giao diện dùng bố cục xoay ngang bằng CSS, người dùng xoay điện thoại sang ngang để nhìn thuận. Thoát toàn màn hình bằng nút hệ thống sẽ quay về trang chính nếu trang đã vào toàn màn hình. Đổi hướng/kích thước, blur, chuyển ứng dụng, mất Wi-Fi, trạng thái không Run, thoát màn hình đều hủy các thao tác giữ. Mất liên lạc vẫn tuân theo lease 400 ms của firmware.

Các file bổ sung `4_Web/phone-drive.html/.css/.js` chỉ nhúng trên ESP32 qua generator; không thay HTML/CSS/JS web máy tính. `phone-adapter.js` nhận cặp v/w nguyên tử, kiểm tra giới hạn cũ rồi dùng cùng CMD/session/sequence. Các file điều khiển, IMU/NVS, driver, cấu hình và các điều kiện khởi động/phục hồi giữ nguyên từng byte so với bản trước màn hình lái này.

Sau khi nạp sketch `balancebot.ino` cùng tất cả file trong thư mục này:

1. Bật xe. Điện thoại nối Wi-Fi **BalanceBot-Bench**, mật khẩu **BalanceBot13**.
2. Nếu điện thoại báo mạng không có Internet, chọn tiếp tục dùng mạng Wi-Fi đó.
3. Mở Chrome/Safari và nhập **http://192.168.4.1/** (HTTP, không phải HTTPS; không thêm cổng 8080).
4. Máy tính có thể tắt hoàn toàn từ trước. Không cần Internet hay router.
5. Đăng ký nếu chưa có tài khoản trên xe, rồi đăng nhập. Đây là firmware mới: giữ xe đứng yên, motor tắt, vào mục “Zeropoint & Cảm biến” rồi bấm “HIỆU CHUẨN GYRO”. Chờ hiệu chuẩn đạt và đã lưu; ACK pending chưa có nghĩa hiệu chuẩn thành công. Kết quả đạt tự lưu, các lần bật nguồn sau dùng lại.

Giao diện hiện dùng **chính HTML/CSS/dashboard.js/Chart.js của web máy tính**, nhúng và nén trong flash. Điện thoại có ba thẻ **Điều khiển / Biểu đồ / Dữ liệu** giống bản máy tính ở kích thước điện thoại; màn hình lớn hiển thị ba cột. Có hiệu ứng nút lún/sáng khi chạm/giữ, xóa hiệu ứng khi thả/hủy/chuyển ứng dụng.

Các chức năng gồm trạng thái/góc/vận tốc, Bắt đầu/Dừng/Reset, giữ nút tiến/lùi/xoay, thanh tốc độ 1–20 cm/s, hiệu chuẩn gyro, chỉnh zero, gửi PID, lưu PID/zero, ba biểu đồ, tải CSV/JSON, giữ dữ liệu trước lỗi, tính và gửi LQR hai trạng thái. Xoay giữ mức 20°/s. PID/Q/R nhập trong ô không tự áp dụng cho xe.

LQR được tính trong trình duyệt bằng cùng thuật toán/mô hình của `4_Web/lqr.js`. Chỉ gửi khi xe Chờ và dữ liệu mới/đúng cấu hình. Firmware kiểm tra IMU đã hiệu chuẩn, đúng hash mô hình, số hữu hạn và ổn định rời rạc trước khi nhận; chỉ khi ACK khớp mới hiển thị đã áp dụng. K mặc định giữ nguyên. K chỉnh có hiệu lực trong lần bật nguồn đó, không được lưu bằng SAVE; khởi động lại dùng K mặc định, giống mô tả của web máy tính.

Thả nút gửi vận tốc 0. Chuyển ứng dụng, mất Wi-Fi hoặc dữ liệu quá hạn hủy thao tác giữ nút; kết nối lại không tự chạy tiếp lệnh cũ. Xe vẫn dùng giới hạn thời hạn lệnh 400 ms và giảm tốc của bộ điều khiển hiện có. Mất web không tắt chức năng tự cân bằng. Sau hiệu chuẩn, việc tự bắt đầu và tự phục hồi vẫn theo điều kiện hiện có; Dừng/Reset tiếp tục giữ hành vi khóa tự bắt đầu hiện có.

Chỉ dùng một giao diện điều khiển tại một thời điểm. Bấm Dừng trước khi đổi giữa điện thoại và web máy tính. Web máy tính/TCP8080 vẫn có. Tài khoản xe đăng ký riêng, không tự sao chép từ máy tính. Đăng xuất thu hồi phiên ESP32 và đưa mục tiêu di chuyển về 0. Wi-Fi nhà vẫn ẩn, Cấu hình sẵn đã bỏ khỏi bản điện thoại. Không thêm captive portal/app cài đặt hay yêu cầu Internet.

## Hợp đồng tích hợp

- HTTP cổng 80 và hash mật khẩu chạy trong network task core 0, không trong IMU/ISR. Control task chỉ thêm job lưu tài khoản, với motor tắt và kiểm tra trạng thái như SAVE; không đưa xử lý HTTP/hash vào vòng điều khiển.
- `PhoneHttp.h`: header tối đa 1024 byte, HTTP body tối đa 192 byte cho đăng nhập; lệnh điều khiển vẫn tối đa 160 byte. Từ chối chunked, NUL, length/cookie sai/trùng, Host/Origin không hợp lệ.
- `PhoneWeb.h`: một trao đổi HTTP mỗi lần, hết hạn 1500 ms kể cả khi client gửi nhỏ giọt; mỗi lượt tối đa RX 192 byte/TX 1024 byte. Ghi TCP nonblocking giữ offset khi gửi một phần/EAGAIN; lỗi thật đóng kết nối. Trang đầy đủ nén gzip, không giải nén trên ESP32. Bộ đệm ứng dụng tĩnh; SDK mạng vẫn quản lý socket của SDK. Phản hồi lệnh có số thứ tự và bộ đệm đủ cho GET/PID/LQR, adapter gửi tuần tự và chờ ACK thật.
- `/` và `/welcome` phục vụ gzip từ flash, không CDN. Chưa đăng nhập chỉ nhận trang Đăng nhập. `/session`, `/status`, `/command` cần cookie phiên hợp lệ; trạng thái/lệnh còn cần token. Đăng nhập mới thay phiên cũ; logout/expiry thu hồi quyền. Auth routes giới hạn chờ 8 giây, các trao đổi khác 1,5 giây. Backend từ chối CMD/MOTION/SPEED/TURN trước khi nhận START/nhận quyền rõ ràng. Không CORS; kiểm tra Host/Origin cùng nguồn.
- Lệnh mang source 3 đi qua `acceptLine` và hàng đợi hiện có; mọi kiểm tra trạng thái, hiệu chuẩn, gain, session và giới hạn chuyển động vẫn do bộ điều khiển hiện có quyết định. STOP/ESTOP dùng đường dừng ưu tiên hiện có. HTTP 202/QUEUED chỉ là đã xếp lệnh; trạng thái và phản hồi thật đọc qua `/status`.
- `ControlCore.cpp/.h` chỉ mở thêm lệnh LQR2 được kiểm tra khi Chờ, K hiện tại trong RAM và thông báo capability/readback. Công thức, K mặc định, PID mặc định, ngưỡng/dwell, giới hạn, tự bắt đầu, lưu hiệu chuẩn và driver không thay đổi. `Runtime.cpp` bổ sung trạng thái đọc và tuyến web; pin/chiều motor/cấu hình/IMU giữ nguyên.
- Chạy `tools/generate_phone_page.py` sau khi sửa giao diện máy tính, adapter hoặc mô hình. Script kiểm tra hash mô hình, nhúng file thật cùng Chart.js/license, trang welcome đăng ký/đăng nhập và view lái (bỏ QR), tạo `PhonePage.h` và đầu vào kiểm thử trong `.build`. Người nạp từ Arduino IDE chỉ cần tất cả file sketch, không cần chạy Python.

Kiểm thử host và trình duyệt là bằng chứng phần mềm. Chưa nạp hoặc xác nhận chất lượng Wi-Fi/độ trễ/jitter trên xe thật. Bản thân chức năng này không chứng minh hay sửa nguyên nhân Wi-Fi cũ bị rớt.
