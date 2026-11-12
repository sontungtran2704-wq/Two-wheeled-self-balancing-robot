# Dashboard xe cân bằng

`public/index.html` và `public/style.css` giữ nguyên từng byte từ `4_Web (2).zip`. Máy chủ chuyển đổi giao thức. JavaScript của mẫu chỉ đổi phần đọc và xuất dữ liệu.

Chạy `npm ci` và `npm start` trong thư mục này, hoặc mở `start_server.bat` trên Windows. Mở http://localhost:3000 và chọn COM13 nếu đó là cổng ESP32 hiện tại. Đóng Serial Monitor trước khi kết nối. Có thể nhập `tcp:<IP>:8080` nếu Wi-Fi đã cấu hình trong firmware.

Dashboard kiểm tra phiên bản cấu hình qua telemetry trước START. Khi firmware báo RUN, giá trị khác 0 hợp lệ trên thanh trượt bật PID chuyển động và được đổi từ cm/s, độ/s sang m/s, rad/s. Giới hạn firmware là ±20 cm/s và khoảng ±45,8 độ/s. Giá trị vượt giới hạn đưa yêu cầu của trục tương ứng về 0. STOP gửi lệnh tắt driver. Firmware khóa motor sẽ từ chối START.

Gain LQR lấy từ `shared/design.json`. Ba ô PID trên mẫu áp dụng cho vòng vận tốc dọc khi motor tắt. CSV và JSON xuất góc pitch, vận tốc suy từ STEP, yaw và gia tốc tương đối theo đơn vị SI. Đồ thị thứ ba hiển thị gia tốc tương đối (m/s²). Ô vị trí để trống. Lấy góc đặt pitch hiện tại làm zero, ZP+/ZP- chỉnh 0,1 độ mỗi lần. Chỉ nhận khi IDLE, IMU hợp lệ và xe đứng yên gần thẳng đứng. Điểm zero lưu trong RAM. Hiệu chuẩn gyro gửi CALGYRO. Gửi gain LQR, lưu Wi-Fi và lưu vào xe vẫn chưa được hỗ trợ.

`npm test` kiểm tra API. `npm run test:browser` dùng Playwright và Edge với thiết bị TCP giả lập. Có thể đặt `PLAYWRIGHT_MODULE` tới bản Playwright đã cài.

