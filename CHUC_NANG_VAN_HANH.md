# Chức năng vận hành — cập nhật 11/11/2026

Giữ bố cục giao diện bản cũ. Kế thừa cách thao tác; không nhập thuật toán hoặc gain của bản cũ vào bộ điều khiển mới.

| Chức năng | Triển khai và giới hạn |
|---|---|
| USB | COM thực tế, 115200 baud. Web và Serial Monitor không mở cùng một cổng. Ngắt kết nối web trước khi nạp. |
| Wi-Fi trực tiếp | Khi chưa đặt SSID trong firmware, phát `BalanceBot-Bench`, mật khẩu cố định `BalanceBot13`, TCP `192.168.4.1:8080`, kể cả bản mở khóa motor. |
| GET | Đọc thông tin Wi-Fi và PID hiện tại; xếp sau lệnh cập nhật PID để không trả giá trị cũ. |
| Hiệu chuẩn gyro | Giữ yên; thu 500 mẫu liên tiếp hợp lệ trong tối đa 1500 lần thử, khoảng 7,5 giây cộng chờ ban đầu. Rung nhất thời làm bắt đầu lại cửa sổ thay vì hủy ngay. Gia tốc tĩnh vẫn cần 0,95–1,05 g. |
| Trạng thái IMU | Dashboard nêu đang hiệu chuẩn, lỗi I2C, sai mã, lỗi thanh ghi, gia tốc hoặc gyro không đạt. `cal_accel_mg` hỗ trợ chẩn đoán. Không giả lập góc 0 khi IMU chưa hiệu chuẩn. |
| START | Chỉ chuyển Chờ → Arming → Run khi IMU hợp lệ và tư thế đủ điều kiện; giữ thẳng liên tục 1 giây. Không ép trạng thái Run. |
| STOP | Tắt motor, xóa yêu cầu chuyển động. Lỗi đã chốt cần RESET có chủ đích. |
| Tiến/lùi/quay | Slider gửi heartbeat; giới hạn 0,2 m/s và 0,8 rad/s. Mất lệnh làm vận tốc đặt về 0 theo giới hạn tốc độ thay đổi; cân bằng tiếp tục nếu còn đủ điều kiện. |
| PID | Gửi PID vận tốc V qua nút hiện có, yaw Y qua giao thức. Chỉ đổi khi motor tắt. Kp 0..5, Ki 0..2, Kd 0..0,2. |
| Lấy góc, ZP+/ZP− | Đặt điểm zero khi đang Chờ, IMU hợp lệ và xe yên gần thẳng đứng; mỗi bước 0,1 độ. |
| Lưu vào xe | SAVE lưu PID V/Y và điểm zero vào NVS; chỉ khi motor tắt. Khởi động đọc lại nếu hash cấu hình trùng và các giá trị hợp lệ. Không lưu trạng thái Run hay bỏ qua hiệu chuẩn IMU. |
| Ba đồ thị | Góc rad, vận tốc suy ra xung m/s, gia tốc tương đối điều khiển m/s². Không phải vận tốc encoder hay lực N. |
| CSV | Xuất telemetry có phiên bản cấu hình để đối chiếu. |
| LQR trực tuyến | Chưa hỗ trợ thay gain từ bộ giải web cũ; `/api/lqr` chỉ đọc gain đã kiểm chứng, `$K` bị từ chối. |
| Đổi Wi-Fi nhà trên web | Chưa hỗ trợ lưu SSID từ web. Dùng AP cố định hoặc cấu hình SSID trong firmware; giao diện không thông báo lưu/khởi động lại giả. |

## Kiểm tra tái lập

Tại thư mục dự án:

```powershell
./tools/build_host.ps1
./tools/build_firmware.ps1
cd 4_Web
npm test
npm run test:browser
```

Kiểm thử trình duyệt sử dụng TCP giả lập, kiểm tra dữ liệu ba biểu đồ, SAVE, CALGYRO, lấy góc, START/STOP, PID, chuyển động, CSV, dữ liệu lỗi và hết hạn. Không thay thế thử cảm biến, lưu NVS qua mất nguồn, Wi-Fi thực hoặc motor trên xe.

## Thử trên xe

1. Nạp sketch `03_Firmware_ESP32S3/balancebot` bằng cấu hình ESP32-S3 N16R8 của script build; ngắt kết nối web và đóng Serial Monitor trước khi nạp.
2. Khởi động lại server web sau khi sửa mã (`node web_server.js` trong `4_Web`), tải lại trình duyệt Ctrl+F5.
3. Kết nối USB hoặc Wi-Fi AP; không cần bấm START để hiệu chuẩn và xem góc.
4. Giữ xe yên, bấm Hiệu chuẩn gyro. Chờ thông báo đạt, nghiêng nhẹ kiểm tra góc thay đổi đúng chiều.
5. Nếu gia tốc không đạt, ghi `imu_who`, `imu_diag`, `cal_accel_mg`; kiểm tra cấu hình và cảm biến. Không mở khóa bằng cách giả `imu_calibrated=1`.
6. Kiểm tra chiều hai bánh và dấu pitch trên giá thử trước thử giữ cân bằng. Sau đó giữ xe thẳng, bấm START. Các thử nghiệm vận hành thực tế này chưa được xác nhận bằng bài test máy tính.

Các file thay đổi chính: `Runtime.cpp` (AP, GET, lưu NVS), `ControlCore.h/.cpp` (xuất/khôi phục tham số có kiểm tra), `ImuHal.cpp` (cửa sổ hiệu chuẩn), `web_server.js` (adapter SAVE/Wi-Fi), `dashboard.js` (phản hồi trạng thái). Giao diện HTML/CSS không thay bố cục.

## Sửa hiệu chuẩn IMU — 12/11/2026

Thay mô tả hiệu chuẩn trước đó: đánh giá cửa sổ 500 mẫu (tối đa 3 cửa sổ), trung bình độ lớn gia tốc 0,90–1,10 g, độ lệch chuẩn tối đa trên mỗi trục gia tốc 0,02 g, gyro 0,02 rad/s, tốc độ gyro thô tối đa 0,15 rad/s. Đây là ngưỡng vận hành tạm dùng; không tự chia gia tốc cho độ lớn để che sai số, không thay thế hiệu chuẩn gia tốc nhiều tư thế.

Trong lúc chưa đạt, góc accelerometer và gyro thô được phát để chẩn đoán, với imu_calibrated=0; Sensors.valid vẫn false nên không thể ARM. Mất mẫu quá 20 ms làm web bỏ hiển thị góc cũ. Sau đạt dùng bộ lọc bổ sung và bias gyro như trước.

Telemetry bổ sung: cal_accel_mg là trung bình cửa sổ (không còn là mẫu cuối), cal_accel_std_mg là độ lệch chuẩn lớn nhất của các trục gia tốc, cal_gyro_std_mrad tương tự cho gyro, imu_sample_fresh cho biết dữ liệu chẩn đoán còn mới.

Host tests có ca gia tốc ổn định quanh 1,05 g, rung, quay, thang đo sai, thiếu mẫu và NaN; browser test kiểm tra góc chẩn đoán vẫn hiện nhưng START bị chặn khi chưa hiệu chuẩn. Chưa xác nhận trên phần cứng cho tới khi nạp và lấy telemetry mới.
