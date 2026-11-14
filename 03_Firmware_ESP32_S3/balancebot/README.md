# Bản chính LQR + PID — lưu gyro và tự cân bằng (14/11/2026)

Mở `balancebot.ino` trong thư mục này; cần giữ tất cả file cùng thư mục. Không dùng các sketch `balancebot_lqr_only` hoặc `balancebot_lqr_60_test` cho chức năng mới này. Không thay gain LQR, PID 3/1/0.2, GPIO, cơ khí, giới hạn STEP/gia tốc/tốc độ.

## Hành vi

- Firmware **không tự hiệu chuẩn gyro khi bật nguồn**. Nó đọc bias đã lưu trong NVS `imu-cal-v1/gyro`, kiểm tra định dạng/CRC/WHO_AM_I/pin/trim và SHA-256 của firmware đang chạy, rồi đối chiếu khoảng 0,5 giây dữ liệu tĩnh với bias đó. Không tính hay ghi lại bias trong bước kiểm tra.
- **Mỗi bản firmware mới phải chờ hiệu chuẩn lần đầu đạt và lưu thành công**, dù NVS còn bias của bản trước. Khi mã firmware không khớp hoặc bản lưu không hợp lệ: `imu_diag=7`, motor tắt; cả START thủ công và tự cân bằng đều bị chặn. Phải chủ động bấm Hiệu chuẩn gyro trên web. Không có số bias đo từ xe thật được điền sẵn trong source này.
- Sau hiệu chuẩn đạt, các lần tắt/bật nguồn của chính firmware đó dùng lại kết quả đã lưu. Theo lựa chọn của người dùng, nạp lại **đúng cùng binary** cũng dùng lại kết quả; không phân biệt với lần bật nguồn. Biên dịch ra image khác phải hiệu chuẩn lại. Bản lưu định dạng cũ cũng yêu cầu hiệu chuẩn mới.
- Bật nguồn với dữ liệu hợp lệ: giữ cả pitch thô và pitch đã trừ zero trong **(-10°, +10°)**, tốc độ góc pitch/yaw nhỏ hơn 0,15 rad/s liên tục **5 giây**, xe tự chạy cân bằng, mục tiêu vận tốc/yaw bằng 0. Ra ngoài vùng, dữ liệu IMU cũ/không hợp lệ, chuyển động hoặc vòng xử lý bị gián đoạn sẽ đếm lại.
- Có thể bấm START thay vì chờ tự khởi động: vẫn cần IMU hợp lệ, pitch trong ±10° và qua trạng thái Arming 1 giây như cơ chế thủ công hiện có.
- Khi vào Run, PID vận tốc giữ mục tiêu 0 ngay, kể cả chưa qua cửa sổ ổn định của điều khiển chuyển động. Lệnh tiến/lùi/xoay vẫn chờ pitch dưới ±3° và tốc độ góc nhỏ liên tục 1 giây. `motion=0` lúc này nghĩa là lệnh chuyển động chưa được mở; PID giữ vận tốc 0 đã hoạt động. Bản r2 từng chờ cả hai, làm chậm việc triệt vận tốc lúc đầu. Không cam kết giữ đúng vị trí tuyệt đối vì xe không có encoder.
- Góc zero-corrected đạt **±40°**: lỗi `FALL`, motor tắt. Dựng lại dưới ±10° và ổn định đủ 5 giây thì tự xóa FALL và cân bằng lại với phiên lệnh mới; không khôi phục lệnh tiến/lùi/xoay cũ.
- `SPEED_RAIL` (lệnh tốc độ bánh giữ ở giới hạn 0,6 m/s liên tục 300 ms) cũng tắt motor rồi tự phục hồi khi dựng dưới ±10° và ổn định liên tục 5 giây. Giới hạn tốc độ/gia tốc vẫn giữ nguyên. Đây là giới hạn lệnh suy từ STEP, không phải đo quá tốc độ bằng encoder.
- **Không cần web, máy tính, kết nối Wi-Fi hoặc lệnh START để tự cân bằng.** Vòng LQR/PID, đọc bộ hiệu chuẩn đã lưu, đếm 5 giây và phục hồi đều chạy trên ESP32. Web là phần bổ trợ để giám sát, điều khiển và hiệu chỉnh.
- Hiệu chuẩn/SAVE/chỉnh PID/zero làm đếm lại 5 giây, không tự khóa chế độ độc lập. Trong khi đang đo hoặc ghi flash, motor vẫn tắt; chỉ bắt đầu đếm khi IMU hợp lệ trở lại. Nếu trước đó chưa bấm STOP, xe tự cân bằng sau khi đủ điều kiện.
- **DỪNG/STOP**, ESTOP và RESET chủ động vẫn hủy tự chạy lại. Các thao tác cấu hình không được phép xóa lệnh dừng của bạn. Sau khi đã chủ động DỪNG, muốn chạy lại thì bấm START hoặc bật nguồn lại. RESET tự nó không bật motor.
- Chỉ FALL và SPEED_RAIL được tự phục hồi. IMU, DEADLINE, STEP_WATCHDOG, ESTOP và lỗi khởi tạo vẫn giữ lỗi, cần kiểm tra nguyên nhân rồi RESET/START riêng.
- Ngưỡng cho phép khởi động không chứng minh xe thật bắt được cân bằng ở 10°. Chưa thử bản này trên xe thật.

## Nạp và hiệu chuẩn thực tế một lần cho mỗi firmware mới

1. Kê xe chắc và tắt nguồn motor khi nạp. Ngắt bridge/Serial Monitor khỏi COM, tự nạp sketch `balancebot.ino`.
2. Arduino: ESP32S3 Dev Module; Flash 16 MB; OPI PSRAM; USB Hardware CDC/JTAG và CDC On Boot Enabled theo cổng USB đã xác minh. **Erase All Flash Before Sketch Upload: Disabled** để giữ NVS.
3. Dừng server web cũ của dự án rồi mở lại `4_Web/Mo_Web.cmd` để bridge đọc hash cấu hình mới. Không dừng toàn bộ node.exe. Đóng Serial Monitor trước khi web mở COM.
4. Kê xe trong khung chống ngã. Kết nối web với xe; khi chưa có hiệu chuẩn hợp lệ, xe đang Chờ và motor đã tắt. Giữ thân xe đứng yên, bấm **Hiệu chuẩn gyro** và đợi “Hiệu chuẩn gyro đạt — đã lưu vào ESP32”. ACK pending chỉ là đã nhận lệnh. Nếu trước đó đã bấm DỪNG/RESET để thao tác, trạng thái dừng chủ động vẫn giữ nguyên.
5. `imu_calibrated=1, imu_cal_saved=1` mới xác nhận có kết quả thực tế đã lưu/đọc lại thành công. Nếu lỗi lưu (`imu_diag=8`) hoặc cửa sổ đo không đạt thì chưa sẵn sàng; không được giả trạng thái đạt.
6. Nếu cần đặt lại điểm zero, giữ đúng tư thế cân bằng, dùng Lấy góc và **Lưu vào xe** một lần. Gyro bias và PID/zero là hai bản lưu riêng. Bộ PID/zero từ hash ngay trước bản này được giữ qua thay đổi ngưỡng an toàn.
7. Kiểm tra góc/dấu motor với khung chống ngã trước. Khi có hiệu chuẩn hợp lệ và không có lệnh dừng chủ động, dựng xe trong ±10° ổn định 5 giây: xe tự cân bằng, không bấm START. Nếu đã chủ động DỪNG/RESET, bật nguồn lại để thử chế độ tự khởi động. START chỉ là lựa chọn thủ công, không bắt buộc khi boot.
8. Thử tắt nguồn, đóng web và tắt máy tính; bật nguồn xe, dựng trong ±10° đủ 5 giây. Xe phải tự cân bằng bằng bộ gyro đã lưu. Sau FALL hoặc SPEED_RAIL, dựng lại dưới ±10° đủ 5 giây để tự phục hồi; không cần mở web hay bấm RESET/START. Đây là bước kiểm chứng trên xe thật, chưa thực hiện trong phiên sửa code.
9. Muốn hiệu chuẩn lại: mở web, DỪNG, RESET nếu cần, giữ yên rồi bấm Hiệu chuẩn gyro. Vì đã DỪNG chủ động nên xe không tự bật motor sau hiệu chuẩn; bật nguồn lại hoặc START khi sẵn sàng. Khi nạp firmware mới, lặp lại hiệu chuẩn thực tế cho bản mới; giữ NVS chỉ bảo toàn dữ liệu, không cho phép dùng bias của bản firmware trước.

## Phạm vi đã kiểm chứng

Kiểm thử host/mocks và build không chứng minh cân bằng, chiều bánh, độ chính xác IMU, độ bền NVS khi mất nguồn hoặc thời gian vòng điều khiển trên xe thật. Gyro bias cần đo thực tế một lần sau khi tự nạp bản này; chưa đo trong phiên sửa code.

Source hiện có tại thời điểm bắt đầu sửa chưa chứa `WifiSettings.h` hoặc xử lý LQR2 trong firmware chính, dù web có các nút tương ứng. Bước này không khôi phục hai tính năng đó và không đồng nhất source này với binary bạn đang chạy trên xe. Kết nối AP/USB hiện có và luồng PID vẫn giữ theo source gốc đã sao lưu.
