#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32_S3\\balancebot\\PHONE_WEB.md"
# Web điều khiển trực tiếp trên điện thoại

Sau khi nạp sketch `balancebot.ino` cùng tất cả file trong thư mục này:

1. Bật xe. Điện thoại nối Wi-Fi **BalanceBot-Bench**, mật khẩu **BalanceBot13**.
2. Nếu điện thoại báo mạng không có Internet, chọn tiếp tục dùng mạng Wi-Fi đó.
3. Mở Chrome/Safari và nhập **http://192.168.4.1/** (HTTP, không phải HTTPS; không thêm cổng 8080).
4. Máy tính có thể tắt hoàn toàn từ trước. Không cần Internet hay router.
5. Đây là firmware mới: giữ xe đứng yên, motor tắt, mở “Hiệu chuẩn & thông số” rồi bấm “HIỆU CHUẨN GYRO”. Chờ chỉ báo hiệu chuẩn **Đạt / Đã lưu Có**. ACK đang xử lý chưa có nghĩa hiệu chuẩn thành công. Kết quả đạt được tự lưu, những lần bật nguồn sau dùng lại.

Giao diện hiện dùng **chính HTML/CSS/dashboard.js/Chart.js của web máy tính**, nhúng và nén trong flash. Điện thoại có ba thẻ **Điều khiển / Biểu đồ / Dữ liệu** giống bản máy tính ở kích thước điện thoại; màn hình lớn hiển thị ba cột. Có hiệu ứng nút lún/sáng khi chạm/giữ, xóa hiệu ứng khi thả/hủy/chuyển ứng dụng.

Các chức năng gồm trạng thái/góc/vận tốc, Bắt đầu/Dừng/Reset, giữ nút tiến/lùi/xoay, thanh tốc độ 1–20 cm/s, hiệu chuẩn gyro, chỉnh zero, gửi PID, lưu PID/zero, ba biểu đồ, tải CSV/JSON, giữ dữ liệu trước lỗi, tính và gửi LQR hai trạng thái. Xoay giữ mức 20°/s. PID/Q/R nhập trong ô không tự áp dụng cho xe.

LQR được tính trong trình duyệt bằng cùng thuật toán/mô hình của `4_Web/lqr.js`. Chỉ gửi khi xe Chờ và dữ liệu mới/đúng cấu hình. Firmware kiểm tra IMU đã hiệu chuẩn, đúng hash mô hình, số hữu hạn và ổn định rời rạc trước khi nhận; chỉ khi ACK khớp mới hiển thị đã áp dụng. K mặc định giữ nguyên. K chỉnh có hiệu lực trong lần bật nguồn đó, không được lưu bằng SAVE; khởi động lại dùng K mặc định, giống mô tả của web máy tính.

Thả nút gửi vận tốc 0. Chuyển ứng dụng, mất Wi-Fi hoặc dữ liệu quá hạn hủy thao tác giữ nút; kết nối lại không tự chạy tiếp lệnh cũ. Xe vẫn dùng giới hạn thời hạn lệnh 400 ms và giảm tốc của bộ điều khiển hiện có. Mất web không tắt chức năng tự cân bằng. Sau hiệu chuẩn, việc tự bắt đầu và tự phục hồi vẫn theo điều kiện hiện có; Dừng/Reset tiếp tục giữ hành vi khóa tự bắt đầu hiện có.

Chỉ dùng một giao diện điều khiển tại một thời điểm. Nếu giao diện trước đã bấm Bắt đầu và giữ quyền điều khiển, bấm Dừng trước khi đổi giữa điện thoại và web máy tính. Web máy tính và TCP cổng 8080 vẫn có. Bản trực tiếp tự kết nối tới chính xe phục vụ trang; nút Kết nối/Ngắt kết nối và Đăng xuất điều khiển phiên giao diện trực tiếp, không phải mở cổng COM trên điện thoại. Đăng nhập/tài khoản của server máy tính không được chuyển sang ESP32; truy cập trực tiếp được bảo vệ bằng mật khẩu Wi-Fi và token cùng nguồn. Wi-Fi nhà vẫn ẩn như web máy tính hiện tại; preset cũ vẫn chỉ thông báo chưa áp dụng như web máy tính. Không thêm captive portal/app cài đặt hay yêu cầu Internet.

## Hợp đồng tích hợp

- Chỉ bổ sung HTTP cổng 80 trong tác vụ network core 0; không thêm công việc vào IMU/control/ISR.
- `PhoneHttp.h`: bộ phân tích cố định, header tối đa 1024 byte, body tối đa 160 byte; từ chối chunked, NUL, length sai/trùng, Host/Origin không hợp lệ.
- `PhoneWeb.h`: một trao đổi HTTP mỗi lần, hết hạn 1500 ms kể cả khi client gửi nhỏ giọt; mỗi lượt tối đa RX 192 byte/TX 1024 byte. Ghi TCP nonblocking giữ offset khi gửi một phần/EAGAIN; lỗi thật đóng kết nối. Trang đầy đủ nén gzip, không giải nén trên ESP32. Bộ đệm ứng dụng tĩnh; SDK mạng vẫn quản lý socket của SDK. Phản hồi lệnh có số thứ tự và bộ đệm đủ cho GET/PID/LQR, adapter gửi tuần tự và chờ ACK thật.
- `/` phục vụ HTML/CSS/JS ngay từ flash, không CDN. `/session` cấp token 128 bit đổi khi khởi động; `/status` và `POST /command` yêu cầu token. Không CORS; kiểm tra Host là IP của xe và Origin cùng Host cho lệnh trình duyệt. Đây là điều khiển trong Wi-Fi nội bộ được bảo vệ bằng mật khẩu AP, không có tài khoản người dùng riêng như web máy tính.
- Lệnh mang source 3 đi qua `acceptLine` và hàng đợi hiện có; mọi kiểm tra trạng thái, hiệu chuẩn, gain, session và giới hạn chuyển động vẫn do bộ điều khiển hiện có quyết định. STOP/ESTOP dùng đường dừng ưu tiên hiện có. HTTP 202/QUEUED chỉ là đã xếp lệnh; trạng thái và phản hồi thật đọc qua `/status`.
- `ControlCore.cpp/.h` chỉ mở thêm lệnh LQR2 được kiểm tra khi Chờ, K hiện tại trong RAM và thông báo capability/readback. Công thức, K mặc định, PID mặc định, ngưỡng/dwell, giới hạn, tự bắt đầu, lưu hiệu chuẩn và driver không thay đổi. `Runtime.cpp` bổ sung trạng thái đọc và tuyến web; pin/chiều motor/cấu hình/IMU giữ nguyên.
- Chạy `tools/generate_phone_page.py` sau khi sửa giao diện máy tính, adapter hoặc mô hình. Script kiểm tra hash mô hình, nhúng file thật cùng Chart.js/QR/license, tạo `PhonePage.h` và đầu vào kiểm thử trong `.build`. Người nạp từ Arduino IDE chỉ cần tất cả file sketch, không cần chạy Python.

Kiểm thử host và trình duyệt là bằng chứng phần mềm. Chưa nạp hoặc xác nhận chất lượng Wi-Fi/độ trễ/jitter trên xe thật. Bản thân chức năng này không chứng minh hay sửa nguyên nhân Wi-Fi cũ bị rớt.
