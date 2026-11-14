# Dừng điều khiển / Dừng khẩn cấp — 14/11/2026

## Hành vi bàn giao

- **Dừng điều khiển**: ở màn hình lái và dashboard ngoài. Đưa mục tiêu tiến/lùi/xoay về0, thu hồi quyền lệnh web, xe đang Run tiếp tục cân bằng. Bắt đầu sáng lại; phải nhấn Bắt đầu để nhận quyền mới, không khôi phục thao tác giữ cũ.
- **Dừng khẩn cấp**: chỉ ở dashboard ngoài, không có trong màn hình lái. Gửi ESTOP tới đường dừng ưu tiên của firmware, tắt motor/chốt lỗi; cần Reset lỗi rồi Bắt đầu mới chạy lại.
- FALL hiển thị **XE ĐÃ ĐỔ**, SPEED_RAIL hiển thị **QUÁ TỐC** ở terminal và trạng thái dữ liệu. Raw code giữ nguyên trong log; SPEED_RAIL vẫn là giới hạn lệnh bánh, không phải đo tốc độ encoder. Lỗi khác vẫn có mã lỗi.
- Footer web máy tính và bản ESP32 đều **2026–2027**.

Địa chỉ/đăng ký/đăng nhập/hiệu chuẩn như bản trước. Tự nạp `03_Firmware_ESP32_S3/balancebot/balancebot.ino` cùng đầy đủ file, giữ NVS, hiệu chuẩn thực tế lần đầu cho image mới rồi mở **http://192.168.4.1/** khi nối BalanceBot-Bench/BalanceBot13.

## Triển khai

Phone gửi `CONTROL_STOP` như lệnh quản lý quyền tại `PhoneWeb.h`, không đưa command này vào ControlCore. ESP32 xóa controlGranted/startPending rồi xếp SPEED0/TURN0 vào đường lệnh cũ và trả ACK. Các lệnh di chuyển tiếp theo bị403 đến khi START/nhận quyền lại. Không gọi STOP/ESTOP/disable cho tạm dừng. Adapter ưu tiên pause, xóa held/targets/queue, epoch chống ACK START cũ cấp quyền trở lại sau pause.

Web máy tính có `pauseControl`: gửi CMD0, thu hồi owner của dashboard và chặn drive đến lần Bắt đầu sau; không gửi STOP/ESTOP. Nút Dừng khẩn cấp ngoài trang dùng ESTOP, không dùng chức năng pause. Dòng mô tả Reset của PC được chỉnh để không nhầm hai loại dừng (trên phone vẫn ẩn).

Chỉ sửa Web/Page và tài liệu trong sketch; Runtime/ControlCore/PhoneAuth/PhoneHttp/IMU/pin/config/driver giữ nguyên từng byte so với trước lần sửa này. Web PC đổi index/dashboard và handler pause của Node theo yêu cầu; CSS/feedback/welcome/auth PC giữ nguyên. Không đổi các ngưỡng, PID/LQR mặc định, điều kiện gyro, tự cân bằng/phục hồi.

## Kiểm chứng

- `host.log`: 7 executable PASS. Test mới chạy Device thật trên host: SPEED0/TURN0 vẫn Run/enabled, ESTOP tắt/latch và cần Reset; Web pause chỉ chuyển SPEED0/TURN0, khóa CMD; hồi quy auth/HTTP/LQR/standalone r4/IMU-NVS tiếp tục đạt.
- `web-tests.log`: 8/8 API/browser tests. Node pause gửi CMD0 không STOP/ESTOP, từ chối drive sau pause, Bắt đầu nhận lại quyền. Phone pause khi còn giữ nút vẫn Run, khóa nút/permission rồi Bắt đầu hoạt động lại; panic chỉ ở ngoài và cần Reset để chỉnh thông số; đủ hai nhãn lỗi và footer2026–2027. Đăng nhập, hai ngón, biểu đồ, CSV/JSON, PID/LQR và kết nối lại giữ nguyên.
- `firmware-build.log`: compile ESP32-S3 core/N16R8 bằng CLI cài tại `C:/Users/DELL/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe`; đường Program Files cũ không còn executable. Xem log cho dung lượng nguồn cuối.
- Build đầu jobs2 bị lỗi Windows ar khi tạo core.a, lưu trong `firmware-first-build.log`. Lần hiệu chỉnh jobs1 dùng cache hợp lệ `.build/phone-auth` đã PASS với nguồn cuối: flash **1,080,451/1,310,720 byte**, RAM global **56,372/327,680 byte**. Binary bàn giao chỉ lấy từ build cuối; script từ chối đóng gói nếu log build chưa hoàn tất.
- `bundle-check.log`/`SHA256.json`: compiled source khớp, file ngoài phạm vi unchanged, page/welcome/model/input hashes, ZIP/binary đọc lại khớp.

`balancebot-stop-modes-source.zip` chứa toàn sketch; `verification-inputs.zip` đủ đầu vào kiểm thử; `balancebot.ino.bin` là app binary, không phải ảnh gộp; `before-source.zip` snapshot trước sửa.

Chưa nạp/COM/motor hay thử điện thoại/xe thật. Chưa đo RF/độ trễ/jitter. Không giả kết quả hiệu chuẩn/tài khoản đã được ghi trên xe thật.
