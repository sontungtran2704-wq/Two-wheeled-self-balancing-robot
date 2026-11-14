# Thiết kế và kế hoạch triển khai — 10/11/2026

Phạm vi được giao: đề tài mới, nguồn gốc chỉ đọc, mô phỏng và kiểm thử tái lập;
không nạp hoặc chạy động cơ khi chưa xác nhận phần cứng. Thư mục đích rỗng trước
khi khảo sát. Quyết định kỹ thuật dưới đây là lựa chọn của người triển khai,
không phải thông số phần cứng người dùng đã xác nhận.

## Kiến trúc

- Dùng Arduino ESP32 core đã cài, lõi C++ độc lập Arduino cho host và firmware.
- Đầu vào mô hình là gia tốc tiếp tuyến tương đối rotor/thân `u` (m/s²), tích
  phân thành tốc độ tương đối rồi thành tần số STEP. Không dùng gain DC/PWM.
- LQR rời rạc cho `[theta, omega]`, theta dương nghiêng trước, rad và rad/s.
  PID vận tốc dọc và tốc độ yaw là hai vòng ngoài độc lập; PID tạo gia tốc đặt.
  Chế độ LQR đơn thuần ổn định góc, không hứa giữ vị trí/vận tốc bằng encoder.
- Cấu hình chung JSON sinh header C++ và được MATLAB/Simulink/web cùng đọc.
- USB Serial và Wi-Fi TCP sử dụng adapter dòng văn bản từ 4_Web cũ. Web chỉ
  đọc gain đã xuất; từ chối bộ `$K` cũ vì sai mô hình, trạng thái và đơn vị.
- Một task sở hữu điều khiển/driver; IMU và mạng cung cấp snapshot có timestamp.
  Khởi động khóa driver, lỗi chốt, STOP khẩn, tái kích hoạt có chủ đích.

So sánh: mô hình lực + ánh xạ STEP đòi hỏi bộ bám lực không có trên driver
STEP/DIR; mô hình gia tốc tương đối phù hợp tầng chấp hành nhưng chỉ có giá trị
khi không mất bước/trượt và còn dự trữ mô-men. Chọn phương án thứ hai, đánh giá
thêm trễ, sai số bám và yêu cầu mô-men. Không dùng LQR 3 trạng thái có vận tốc
đồng thời với PID vận tốc, tránh hai vòng cạnh tranh cùng sai số.

## Tiêu chí đặt trước tinh chỉnh

- LQR 200 Hz (5 ms), PID 50 Hz (20 ms), telemetry 25 Hz.
- Tuyến tính: controllable/observable phần pitch, mọi cực kín |z|<1.
- Phi tuyến danh định 5°: về dải ±0.5° và |omega|<0.05 rad/s trong 2 s;
  không vượt góc ngã 22°, gia tốc tương đối <=3 m/s², tốc độ bánh <=0.6 m/s.
- Theo lệnh dọc ±0.15 m/s, yaw ±0.6 rad/s: cuối đoạn sai số dọc <0.03 m/s,
  yaw <0.08 rad/s, pitch <8°; lệnh 0 phải có kịch bản dừng riêng.
- PID chỉ hoạt động sau 1 s cân bằng liên tục trong ±3°, |omega|<0.15 rad/s.
  Command lease 400 ms; hết hạn giảm lệnh đặt về 0 bằng ramp, không tắt LQR.
- IMU age <=15 ms; trễ điều khiển >15 ms gây fault. Jitter mục tiêu <0.5 ms,
  thời gian tick <1 ms là yêu cầu đo trên board, chưa phải kết quả thực nghiệm.
- Đối chiếu cùng dữ liệu: MATLAB–SciPy gain <1e-8; lõi C++–MATLAB/Simulink
  đầu ra <1e-5 (lõi double), có đơn vị và phiên bản trong CSV/JSON.

## Công việc và cổng bằng chứng

- [x] Kiểm tra thư mục đích, đọc nguồn, giữ hash SHA256, trích mã chọn lọc.
- [ ] Chốt phương trình phi tuyến, tuyến tính, ánh xạ STEP, giao diện PID.
- [ ] Tính gain, quét Q/R, mô phỏng và xuất đồ thị/CSV có điều kiện thử.
- [ ] Tạo và chạy hai mô hình SLX độc lập thư mục; kiểm tra nguồn SLX cũ.
- [ ] C++ core, firmware adapter, giả lập thiết bị, tích hợp dashboard 4_Web.
- [ ] Test biên/lỗi/reconnect/lease; đối chiếu số học; build chỉ cấu hình tham chiếu.
- [ ] Báo cáo tiếng Việt, README, ledger bằng chứng và next-todo vật lý.

Không có board cụ thể: tiếp tục toàn bộ phần độc lập, không chốt GPIO thật.
Các file skill tham chiếu đến `~/.agents/docs` và inventory board không hiện diện
trong bản cài; dùng tài liệu hãng và giữ trạng thái `needs-disambiguation`.
