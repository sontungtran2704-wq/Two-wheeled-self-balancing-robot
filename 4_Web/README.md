# Dashboard xe cân bằng

### Mở trên điện thoại qua máy tính (Cập nhật 14/11/2026)

- Mở `Mo_Web.cmd` trên máy tính. Launcher bật `ROBOT_LAN=1`; server tự động hiển thị **Mã QR Code** trực tiếp trên màn hình console và trên giao diện web máy tính.
- **Cách vào nhanh nhất**: Bật camera điện thoại quét mã QR hiển thị trên cửa sổ console hoặc bấm nút `📱 Điện thoại` trên thanh tiêu đề web để quét. Không cần tự gõ IP thủ công.
- Hỗ trợ **PWA (Progressive Web App)**: Trên trình duyệt Safari (iOS) chọn *Chia sẻ → Thêm vào Màn hình chính*, hoặc trên Chrome (Android) chọn *Cài đặt ứng dụng* để mở toàn màn hình như App native, có hỗ trợ rung phản hồi haptic khi lái.
- Nếu muốn **chạy riêng web độc lập trên điện thoại 100% không cần máy tính**, xem hướng dẫn chi tiết tại [HUONG_DAN_CHAY_WEB_DOC_LAP_TREN_ESP32.md](file:///d:/TLHT/Two-wheeled-self-balancing-robot/4_Web/HUONG_DAN_CHAY_WEB_DOC_LAP_TREN_ESP32.md).
- Nếu Windows chặn kết nối đến, mở `Cho_Phep_Dien_Thoai.cmd` và chấp nhận hộp UAC. Script chỉ cho phép đúng Node.exe nhận TCP 3000 từ LocalSubnet trên profile Private. Không tắt tường lửa.
- Máy tính và server tiếp tục chạy ngầm. Cửa sổ console có thể thu nhỏ xuống thanh tác vụ.
- Kiểm chứng: API 6/6, browser kiểm thử máy tính và mobile browser 390×844 đều đạt 100%.

`public/index.html` và `public/style.css` giữ nguyên từng byte từ `4_Web (2).zip`. Máy chủ chuyển đổi giao thức. JavaScript của mẫu chỉ đổi phần đọc và xuất dữ liệu.

Chạy `npm ci` và `npm start` trong thư mục này, hoặc mở `start_server.bat` trên Windows. Mở http://localhost:3000 và chọn COM13 nếu đó là cổng ESP32 hiện tại. Đóng Serial Monitor trước khi kết nối. Có thể nhập `tcp:<IP>:8080` nếu Wi-Fi đã cấu hình trong firmware.

Dashboard kiểm tra phiên bản cấu hình qua telemetry trước START. Khi firmware báo RUN, giá trị khác 0 hợp lệ trên thanh trượt bật PID chuyển động và được đổi từ cm/s, độ/s sang m/s, rad/s. Giới hạn firmware là ±20 cm/s và khoảng ±45,8 độ/s. Giá trị vượt giới hạn đưa yêu cầu của trục tương ứng về 0. STOP gửi lệnh tắt driver. Firmware khóa motor sẽ từ chối START.

Gain LQR lấy từ `shared/design.json`. Ba ô PID trên mẫu áp dụng cho vòng vận tốc dọc khi motor tắt. CSV và JSON xuất góc pitch, vận tốc suy từ STEP, yaw và gia tốc tương đối theo đơn vị SI. Đồ thị thứ ba hiển thị gia tốc tương đối (m/s²). Ô vị trí để trống. Lấy góc đặt pitch hiện tại làm zero, ZP+/ZP- chỉnh 0,1 độ mỗi lần. Chỉ nhận khi IDLE, IMU hợp lệ và xe đứng yên gần thẳng đứng. Điểm zero lưu trong RAM. Hiệu chuẩn gyro gửi CALGYRO. Gửi gain LQR, lưu Wi-Fi và lưu vào xe vẫn chưa được hỗ trợ.

`npm test` kiểm tra API. `npm run test:browser` dùng Playwright và Edge với thiết bị TCP giả lập. Có thể đặt `PLAYWRIGHT_MODULE` tới bản Playwright đã cài.



### Tài khoản, gửi K và Wi-Fi (13/11/2026)
- Đăng ký tạo tài khoản lưu trên máy chạy web, sau đó phải đăng nhập riêng. Đăng xuất xóa phiên và để trống form; tài khoản vẫn còn. Không cần đăng ký lại khi khởi động lại server.
- Nút Tính K & gửi xe dùng LQR pitch hai trạng thái (Q góc, Q tốc độ góc, R). Vận tốc do PID xử lý. Firmware chính cần hỗ trợ LQR2; chỉ gửi khi Chờ, kiểm tra gain ổn định trên mô hình rời rạc và đợi ACK trùng gain. Gain gửi tạm thời đến lần khởi động lại; SAVE hiện lưu PID/zero, không lưu K.
- Firmware mới luôn bật AP BalanceBot-Bench / BalanceBot13 / 192.168.4.1:8080, đồng thời thử Wi-Fi nhà đã lưu. AP/STA đồng thời theo API Espressif: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html . Chưa kiểm chứng sóng/kết nối trên phần cứng trong bước sửa này.
- Sau khi tự nạp sketch chính 03_Firmware_ESP32S3/balancebot/balancebot.ino: kết nối USB hoặc AP xe, motor tắt và RESET về Chờ, nhập SSID Wi-Fi nhà 2,4 GHz + mật khẩu (trống cho mạng mở, hoặc 8–63 byte), bấm Lưu WiFi và khởi động lại xe. Đợi xác nhận, nối lại rồi bấm Kiểm tra WiFi xe: sẽ hiện IP mạng nhà và IP AP riêng. Máy tính vào cùng mạng nhà, nhập IP_mạng_nhà:8080 để điều khiển và có Internet. Muốn kết nối trực tiếp: vào BalanceBot-Bench và dùng 192.168.4.1:8080. Wi-Fi nhà sai/mất sóng vẫn có AP riêng để sửa lại. Khi chuyển mạng, ngắt đường truyền cũ trước; không tự START.
- Wi-Fi được lưu riêng vào NVS wifi-v1 bằng giao dịch có ACK từng bước. Firmware chặn lưu khi motor hoạt động; START bị chặn từ lúc lưu thành công tới khi reboot. Không ghi đè tuning PID/zero.
