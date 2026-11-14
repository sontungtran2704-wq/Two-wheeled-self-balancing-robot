# Web điện thoại không cần máy tính — phương án r1

Ngày: 13/11/2026. Trạng thái: đã rà soát kiến trúc, chờ xác nhận trước triển khai firmware.

## Mục tiêu và hiện trạng đã kiểm tra
- Điện thoại mở trình duyệt, kết nối xe và điều khiển mà không chạy Node.js/máy tính; không cần Internet khi dùng Wi-Fi xe.
- ESP32-S3 N16R8 theo bàn giao; Arduino ESP32 core 3.3.12; nguồn build tham chiếu flash 16 MB, OPI PSRAM. Chưa đo heap/stack/jitter của web nhúng trên xe thật.
- Runtime.cpp hiện đọc trong workspace có TCP dòng lệnh cổng 8080, control task core 1 priority 5 mỗi 5 ms, IMU core 1 priority 4, network core 0 priority 1. Dashboard 4_Web cần Express/Socket.IO trên máy tính. Cổng 8080 hiện không phải HTTP; không thể chỉ gõ IP đó trong trình duyệt để dùng dashboard.
- WebServer.h trong bộ Arduino đã cài hỗ trợ GET/POST, ghi rõ chỉ một client đồng thời. Không coi việc có header là bằng chứng hiệu năng đáp ứng vòng cân bằng.
- Không giả định các bản build hoặc trao đổi trước đồng nhất với cây nguồn hiện tại. Khi triển khai phải chụp snapshot và rà diff lại; không ghi đè công việc hiện có.

## Các phương án
| Phương án | Hiệu quả tài nguyên | Độ phức tạp | Bảo trì | Thời gian thực | Khả năng phục hồi |
|---|---|---|---|---|---|
| Giữ Node.js trên PC, điện thoại truy cập PC | Good: ESP không thêm tải web | Good: ít thay đổi | Good: tái dùng web | Good: giữ cách phân chia hiện tại | Problem: không đạt yêu cầu bỏ PC |
| Web nhúng trên ESP32 qua HTTP/API | Trade-off: thêm flash, socket và RAM | Trade-off: thay bridge và xác thực bằng giao tiếp HTTP | Good: tái dùng bộ điều khiển và lệnh hiện có | Unknown: phải đo trên xe với tải HTTP | Good: không phụ thuộc PC/Internet; cần giới hạn client và kiểm thử mất mạng |
| Web cloud/ứng dụng điện thoại làm bridge TCP | Problem: thêm phụ thuộc mạng hoặc cài ứng dụng | Problem: nhiều thành phần | Trade-off: thêm công nghệ | Unknown: độ trễ và mất kết nối | Problem: không phù hợp mục tiêu web offline đơn giản |

Quyết định kỹ thuật: **Proceed với web nhúng**, điều kiện kiểm chứng dưới đây. Chưa phê duyệt triển khai r1.

## Phân chia và hợp đồng giao tiếp đề xuất
Điện thoại → Wi-Fi xe hoặc cùng LAN → HTTP cổng 80 trên ESP32 → hàng đợi lệnh hiện có → task điều khiển hiện có → STEP/DIR.

- Cổng TCP 8080/USB và web PC được giữ tương thích. HTTP chạy ở task mạng/core 0; không parse HTTP, render UI hoặc ghi tài khoản trong ISR/vòng LQR 5 ms.
- Trang responsive nhúng trong flash, không dùng CDN, Node.js, Socket.IO hoặc tài nguyên chỉ có trên PC. Mở trực tiếp http://192.168.4.1 khi dùng AP; khi dùng mạng nhà mở IP LAN thực tế của xe.
- Giao tiếp dự kiến: GET /api/v1/status lấy snapshot; POST /api/v1/command với tập lệnh được phép; POST /api/v1/drive gồm session/seq/v/yaw. Phản hồi phân biệt đã xếp hàng, ACK firmware, bị từ chối, quá hạn. Mọi thành công cần căn cứ phản hồi/trạng thái thật.
- UI lấy telemetry mỗi 100 ms; gửi heartbeat điều khiển 150 ms lúc nhấn giữ. Đây là mục tiêu thiết kế, chưa phải đo đạc. Giữ ngưỡng freshness và lease 400 ms hiện có; không tự START sau đăng nhập/reconnect/tải lại trang. Thả nút, hủy chạm, ẩn tab gửi mục tiêu 0; lease firmware xử lý khi không gửi được.
- Một phiên giữ quyền điều khiển, tối đa hai trình duyệt ban đầu. USB/TCP/HTTP không được tranh quyền bằng START; STOP/ESTOP vẫn có đường ưu tiên. Request phải giới hạn kích thước, hàng đợi không chặn và chống tràn; không truyền lệnh thô tùy ý từ URL.
- Giữ nguyên LQR/PID/gain, cơ khí, giới hạn tốc độ/gia tốc/arming/fall. Chức năng di chuyển dùng đúng giới hạn hiện có. Không quảng bá các chức năng chưa có trong firmware.
- Giữ đăng nhập/đăng ký tối giản: tài khoản mới lưu cục bộ trên xe, không lưu mật khẩu thô; phiên có hết hạn và vô hiệu khi logout/reset. Không tự chuyển dữ liệu tài khoản PC sang ESP32; không xóa tài khoản PC. Chỉ tạo tài khoản/ghi NVS khi motor tắt, dùng đường sở hữu trạng thái như SAVE hiện tại. Đề xuất tối đa 8 tài khoản, 2 phiên; chi phí hash và thời gian ghi NVS phải kiểm tra.
- STA/AP: điện thoại có thể dùng Wi-Fi riêng của xe hoặc IP mạng nhà; cần rà lại code Wi-Fi thực tế trước khi ghép web. Mạng riêng của xe không tự cung cấp Internet cho điện thoại.
- Tài nguyên: UI nén mục tiêu ≤100 KiB trong flash, HTTP thêm ngân sách internal RAM mục tiêu ≤48 KiB. Đây là trần đề xuất cần đo, không phải kết quả. Không đổi partition nếu chưa đánh giá lại. Chọn HTTP server tích hợp SDK có timeout/kích thước body hữu hạn; khả năng SDK và cấu hình thực tế phải qua compile probe trước khi chốt implementation.

## Thứ tự triển khai và điều kiện đạt
1. M1 giao thức/session/owner và API queue độc lập HTTP. Host test validate đầu vào, ACK, stale/lease/sequence, tranh quyền, STOP ưu tiên, không tự START.
2. M2 HTTP + asset nhúng + xác thực. Build đúng ESP32-S3/core; kiểm tra RAM/flash thực tế; host/mock test và hai trình duyệt, login/register/logout/mất phiên. Không đụng tài khoản PC.
3. M3 UI điện thoại: telemetry, nhấn giữ Tiến/Lùi/Xoay, thanh tốc độ, Dừng/Reset/hiệu chuẩn/PID theo firmware hỗ trợ. Browser test ở 390 px/desktop, thả/mất focus/ẩn tab/mất mạng. Mọi tín hiệu xe trong test là giả lập và được ghi rõ.
4. M4 build bàn giao + checklist thử thực tế người dùng: nạp bằng tay khi motor tắt, mở AP từ điện thoại không có PC, telemetry thật, stale/lease/STOP, sau đó mới thử motor có kiểm soát. Đo jitter/overrun/heap/stack khi HTTP tải lại, client chậm và hai điện thoại. Không lấy build/mô phỏng làm bằng chứng xe cân bằng thật.

Giới hạn: chưa viết module firmware hoặc nạp/chạy phần cứng cho web nhúng. Cần người dùng đồng ý r1; có thể đồng ý cho làm liền mạch M1–M4 phần code/kiểm thử, không hỏi lại từng module. Nạp/START/motor vẫn do người dùng thao tác như giới hạn hiện tại.
