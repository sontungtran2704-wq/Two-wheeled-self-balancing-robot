#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32_S3\\balancebot\\PHONE_WEB.md"
# Web điều khiển trực tiếp trên điện thoại

Sau khi nạp sketch `balancebot.ino` cùng tất cả file trong thư mục này:

1. Bật xe. Điện thoại nối Wi-Fi **BalanceBot-Bench**, mật khẩu **BalanceBot13**.
2. Nếu điện thoại báo mạng không có Internet, chọn tiếp tục dùng mạng Wi-Fi đó.
3. Mở Chrome/Safari và nhập **http://192.168.4.1/** (HTTP, không phải HTTPS; không thêm cổng 8080).
4. Máy tính có thể tắt hoàn toàn từ trước. Không cần Internet hay router.
5. Đây là firmware mới: giữ xe đứng yên, motor tắt, mở “Hiệu chuẩn & thông số” rồi bấm “HIỆU CHUẨN GYRO”. Chờ chỉ báo hiệu chuẩn **Đạt / Đã lưu Có**. ACK đang xử lý chưa có nghĩa hiệu chuẩn thành công. Kết quả đạt được tự lưu, những lần bật nguồn sau dùng lại.

Trang điện thoại có trạng thái/góc/vận tốc/hiệu chuẩn, Bắt đầu, Dừng, dừng khẩn, reset lỗi, giữ nút tiến/lùi/xoay, thanh tốc độ 1–20 cm/s, hiệu chuẩn gyro, chỉnh zero, gửi PID vận tốc và lưu thông số. Xoay giữ mức 20°/s. Các thông số mặc định hiển thị trong ô PID chỉ được áp dụng khi bấm Gửi PID.

Thả nút gửi vận tốc 0. Chuyển ứng dụng, mất Wi-Fi hoặc dữ liệu quá hạn hủy thao tác giữ nút; kết nối lại không tự chạy tiếp lệnh cũ. Xe vẫn dùng giới hạn thời hạn lệnh 400 ms và giảm tốc của bộ điều khiển hiện có. Mất web không tắt chức năng tự cân bằng. Sau hiệu chuẩn, việc tự bắt đầu và tự phục hồi vẫn theo điều kiện hiện có; Dừng/Reset tiếp tục giữ hành vi khóa tự bắt đầu hiện có.

Chỉ dùng một giao diện điều khiển tại một thời điểm. Nếu giao diện trước đã bấm Bắt đầu và giữ quyền điều khiển, bấm Dừng trước khi đổi giữa điện thoại và web máy tính. Chức năng web trên máy tính và TCP cổng 8080 vẫn có. Đồ thị, xuất dữ liệu và tính LQR đầy đủ vẫn ở giao diện máy tính; trang điện thoại là trang điều khiển gọn. Không bổ sung captive portal tự bật, PWA hay ứng dụng cài đặt.

## Hợp đồng tích hợp

- Chỉ bổ sung HTTP cổng 80 trong tác vụ network core 0; không thêm công việc vào IMU/control/ISR.
- `PhoneHttp.h`: bộ phân tích cố định, header tối đa 1024 byte, body tối đa 160 byte; từ chối chunked, NUL, length sai/trùng, Host/Origin không hợp lệ.
- `PhoneWeb.h`: một trao đổi HTTP mỗi lần, hết hạn 1500 ms kể cả khi client gửi nhỏ giọt; mỗi lượt tối đa RX 192 byte/TX 1024 byte. Ghi TCP nonblocking giữ offset khi gửi một phần/EAGAIN; lỗi thật đóng kết nối. Không cấp phát heap cho bộ đệm ứng dụng; SDK mạng vẫn quản lý socket của SDK.
- `/` phục vụ HTML/CSS/JS ngay từ flash, không CDN. `/session` cấp token 128 bit đổi khi khởi động; `/status` và `POST /command` yêu cầu token. Không CORS; kiểm tra Host là IP của xe và Origin cùng Host cho lệnh trình duyệt. Đây là điều khiển trong Wi-Fi nội bộ được bảo vệ bằng mật khẩu AP, không có tài khoản người dùng riêng như web máy tính.
- Lệnh mang source 3 đi qua `acceptLine` và hàng đợi hiện có; mọi kiểm tra trạng thái, hiệu chuẩn, gain, session và giới hạn chuyển động vẫn do bộ điều khiển hiện có quyết định. STOP/ESTOP dùng đường dừng ưu tiên hiện có. HTTP 202/QUEUED chỉ là đã xếp lệnh; trạng thái và phản hồi thật đọc qua `/status`.
- Chỉ `Runtime.cpp` được sửa trong các file firmware cũ; thêm ba header và tài liệu này. Giữ nguyên ControlCore, cấu hình, pin, chiều motor, IMU, lưu hiệu chuẩn và driver bước.

Kiểm thử host và trình duyệt là bằng chứng phần mềm. Chưa nạp hoặc xác nhận chất lượng Wi-Fi/độ trễ/jitter trên xe thật. Bản thân chức năng này không chứng minh hay sửa nguyên nhân Wi-Fi cũ bị rớt.
