# Trình tự đưa xe lên bàn thử

Cập nhật 12/11/2026: đã nhận telemetry và thử hiệu chuẩn qua web trên COM17,
115200 baud; ACK nhận được, IMU chuyển đang hiệu chuẩn → đạt. Firmware đang
báo hardware_confirmed=1, st=0, left=right=0. Chưa thử motor/cân bằng trong phiên
này; không thay cờ phần cứng để vượt lỗi. Xem phần bàn giao mới ở cuối
TOM_TAT_TOAN_BO_DE_TAI.md để phân biệt bằng chứng mới và lịch sử.

1. **Xác nhận khi ngắt nguồn:** mã module ESP32-S3, GPIO thực tế, jumper
   MS1/MS2/MS3, mã motor và dòng pha. Đo đường kính bánh, track, cân thân và bánh.
   Bộ số tạm đang dùng r33mm, track228mm,200 bước×16.
2. **Nguồn và driver:** xác nhận12V vào VMOT, nguồn logic đúng theo module
   A4988, mass chung, EN có mức mặc định tắt, giới hạn dòng theo Rsense thực tế.
   5V cấp board không đồng nghĩa GPIO/I2C chịu5V. Không rút dây motor khi có điện.
3. **IMU, motor chưa bật:** đọc telemetry, đặt xe thẳng và nghiêng trước chậm;
   θ phải tăng dương, gyro pitch cùng dấu đạo hàm θ. Đo bias/nhiễu và thử rút
   IMU để xác minh lỗi chốt. Hiệu chuẩn gyro khi xe đứng yên, driver tắt.
4. **Thử từng bánh trên giá đỡ:** sau khi có cấu hình pin đã duyệt và chương
   trình thử riêng có timeout, kiểm tra dấu DIR, số xung một vòng, tốc độ và
   gia tốc không mất bước. Firmware hiện không cung cấp JOG tùy ý qua web.
   Không dùng START cân bằng để thay cho phép thử từng bánh.
5. **Thử cân bằng có dây giữ:** vị trí dễ cắt nguồn, vùng trống, bắt đầu gần
   thẳng đứng. START chỉ qua arm gate; ghi θ,ω,v xung,u,sat,jitter,overruns.
   LQR riêng có thể trôi; không coi việc dựng đứng là đã kiểm chứng giữ vị trí.
6. **PID đặt0 rồi lệnh nhỏ:** sau khi LQR ổn định, bật motion với đặt0 trước;
   thử tiến/lùi nhỏ, dừng, quay nhỏ và mất kết nối. Khi mất lệnh phải ramp về0
   và vẫn cân bằng nếu đủ điều kiện; STOP/ESTOP là vô hiệu driver.

Mỗi lần chỉ thay một yếu tố: dấu, cơ khí, bộ lọc hoặc gain. Giữ log trước/sau.
Không tinh chỉnh K từ dashboard; thay JSON, sinh lại gain và chạy đối chiếu
trước khi build. Dùng encoder hoặc đo chuyển động ngoài để đánh giá mất bước:
đồ thị vận tốc từ xung không đủ chứng minh tốc độ thật.
