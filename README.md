# Xe hai bánh tự cân bằng — LQR pitch và PID chuyển động

Ưu tiên hiện tại: hoàn thiện chạy xe và kiểm thử; **chưa làm báo cáo tổng hợp**
theo yêu cầu cập nhật. Các ghi chú kỹ thuật đã có được giữ lại để tra cứu, không
phải form báo cáo cuối. README và log phục vụ build, nạp và tinh chỉnh.

Dashboard trong `4_Web` dùng giao diện từ `4_Web (2).zip` và giao thức của firmware hiện tại. Xem `4_Web/README.md` để chạy và kết nối.

Dự án dùng ESP32-S3, MPU6050, A4988, NEMA17, nguồn12V hạ5V, **không encoder**.
Ảnh module xác nhận N16R8; GPIO đã được người dùng xác nhận. Firmware đã mở
khóa cấu hình phần cứng nhưng khởi động vẫn tắt motor, chỉ ARM khi IMU hợp lệ.
IMU từng trả mã 0x70, firmware hỗ trợ cả MPU6050 và MPU6500; cần kiểm tra số đo
thực tế và chiều phản hồi trước khi chạy. Tham số cơ khí còn mâu thuẫn.

Xem [chức năng vận hành và giới hạn hiện tại](CHUC_NANG_VAN_HANH.md).

## Cấu trúc và chạy lại

- `00_Nguon`: bản trích nguồn tham khảo và SHA256; không sửa nguồn gốc.
- `shared/robot.json`: thông số SI duy nhất; `design.json` và header firmware do script sinh.
- `01_LQR_CanBang`: MATLAB/Simulink cân bằng.
- `02_PID_ChuyenDong`: MATLAB/Simulink tích hợp chuyển động.
- `03_Firmware_ESP32S3`: lõi C++ độc lập và Arduino HAL.
- `4_Web`: nền server/dashboard USB Serial và TCP kế thừa nguồn tích hợp.
- `05_BaoCao`: khảo sát nguồn, phương trình, quyết định thiết kế và bằng chứng.
- `06_KiemThu/results`: CSV, đồ thị và chỉ tiêu mô phỏng Python.

Chạy tại thư mục dự án, Python có numpy/scipy/matplotlib/openpyxl:

```powershell
python -X utf8 tools/design.py
python -X utf8 -m unittest discover -s tests
python -X utf8 tools/simulate.py
python -X utf8 tools/sweep.py
./tools/build_host.ps1
matlab -batch "run('01_LQR_CanBang/run_all.m')"
matlab -batch "run('02_PID_ChuyenDong/run_all.m')"
```

MATLAB R2024b với Control System Toolbox và Simulink đã kiểm tra giấy phép.
Lệnh build host dùng Zig tại đường dẫn mặc định trong script, có thể truyền
`-Compiler` để đổi. Hướng dẫn từng phần nằm trong thư mục tương ứng.

## Diễn giải kết quả

LQR chỉ dùng pitch/tốc độ pitch; PID vận tốc và yaw nằm ngoài. Chế độ chỉ LQR
có thể giữ góc nhưng **không bảo đảm đứng yên**: vận tốc dư và độ trôi được báo
cáo riêng. PID vận tốc đặt0 giảm trôi trong giả định bám STEP. Xung đã phát
không chứng minh rotor quay đúng hoặc bánh không trượt.

Không sử dụng gain web/DC cũ: mô hình mới có đầu vào gia tốc bánh tương đối
với thân, có phản lực mômen motor. `/api/lqr` chỉ đọc cấu hình đã xuất;
không áp dụng `$K` cũ. Đổi cơ khí phải tính lại và kiểm thử toàn bộ cấu hình.

Trạng thái bàn giao được ghi trong `05_BaoCao/06_Trang_thai_kiem_chung.md`.
CSV/đồ thị là mô phỏng; test host không thay thế build target, đo jitter hay
thử xe thật. Xem `.arduino/next-todo.md` cho bước vật lý tiếp theo.

