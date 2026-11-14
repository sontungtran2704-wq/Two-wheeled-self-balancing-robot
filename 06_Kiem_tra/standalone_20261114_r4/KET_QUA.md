# Giữ vận tốc 0 ngay khi vào Run — r4, 14/11/2026

## Bằng chứng của lỗi khởi động r2

Người dùng xác nhận chạy r2: vừa cân bằng xe lùi1–2m rồi đứng im. File robot_1794634350468.json gồm3000 mẫu, nhiều session và một lần thời gian quay về đầu. Chỉ dùng segment Run cuối session3847868548 để phân tích khởi động; không nối tích phân qua reboot/session khác.

- Trước Run: Idle tại40.308s; mẫu Run đầu40.348s. motion=0 tới trước43.340s, cửa sổ chờ2.992s. fresh=0 suốt segment này nên chưa thấy lệnh chuyển động còn hiệu lực từ web.
- Vận tốc suy từ STEP đạt cực tiểu−0.484516m/s; tích phân đến mẫu motion=1 khoảng−0.520557m. Tổng tới45.752s khoảng−0.988696m, mẫu cuối v≈−0.01049m/s, fault=NONE.
- Source r2: clearMotion reset stable_=0 khi vào Run. NumericController chỉ active_=true sau pitch dưới3° và |omega|<0.15rad/s liên tục1s. velocity.update cũng bị chặn bởi active_. Vì vậy PID giữ vận tốc0 bị tắt trong lúc xe đã cân bằng góc/chạy bánh. Sự rung làm chờ gần3s trong log; sau đó PID mới triệt vận tốc.
- Khoảng cách trên là tích phân vận tốc suy từ xung phát, không đo encoder hoặc chứng minh đúng1–2m thực địa. Log không chứa toàn bộ trạng thái tích phân PID/zero thực, chưa loại trừ lệchzero, trượt/mất bước hay đặc tính cơ khí.

## Sửa tối thiểu

ControlCore.cpp: gọi PID vận tốc khi motion||active_ thay vì chỉ active_. Khi đã vào Run và chưa qua gate chuyển động, vref vẫn0 nên vòng vận tốc bắt đầu giữ0 ngay từ nhịp điều khiển đầu; không chờ thêm cửa sổ1s. Yaw và mở lệnh tiến/lùi/xoay vẫn giữ gate ổn định cũ. Khi đã active_, nhả motion vẫn dùng nhánh giảm tốc cũ. Khi motion=false và chưa active_ thì PID vẫn tắt.

Không đổi gain LQR, PID3/1/0.2, zero, mô hình cơ khí, GPIO, gia tốc/tốc độ/STEP, FALL40°, vùng khởi động10°, dwell5s, STOP/RESET và các lỗi chốt. Đối chiếu source r3: chỉ ControlCore.cpp và README.md đổi; SavedImuCalibration/ImuHal r3 giữ nguyên. Mỗi firmware mới vẫn bắt buộc hiệu chuẩn gyro thực tế trước khi hoạt động; tắt/bật cùng firmware dùng lại bias đã lưu. Web chỉ đổi nhãn diag7 từ r3, không thay bố cục.

motion=0 trước gate giờ nghĩa là lệnh chuyển động chưa mở, không có nghĩa PID giữ vận tốc0 đang tắt. Không thêm vòng giữ vị trí: không encoder thì chưa thể cam kết vị trí tuyệt đối không dịch chuyển.

## Kiểm chứng mới

- tools/build_standalone_host.ps1:3 executable PASS (host.log). Regression mới: v=±0.45m/s, pitch5° chưa đạt gate; PID tác động ngược dấu sai lệch vận tốc ngay nhịp đầu, bị chặn trong accel_max; giữ3s vẫn không mở sớm lệnh tiến/lùi/yaw; không link vẫn giữ0; motion=false trước active không bật PID. Các bài khởi động/auto5s/FALL/SPEED_RAIL/STOP/RESET/stale/motion/gap/wrap/phiên lệnh vẫn PASS.
- IMU acceptance và mock actual ImuHal.cpp PASS: firmwareA→B chặn START/auto dù giữ NVS, explicit calibration mới cho Run; power-cycle cùng image không đo/ghi lại, lỗiCRC/context/identity/storage/I2C giữ chưa sẵn sàng. Các giá trịmock không được đưa vào firmware làm bias thật.
- git diff --check phần tracked sửa: PASS, chỉ cảnh báoCRLF.
- Arduino compile ESP32-S3 core3.3.12, USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,jobs1: PASS. Flash951099/1310720byte, RAM49076/327680byte. firmware-build.log là buildr4, không lấy sốr3.
- Binary SHA256 b398397584f41ec657b774c1a4ee8e368ab524ad4346d362edc91f08c6144ead. ZIP source SHA256 ca6dd9174fff4be768ca08107cb4abc67c3cd1d7f07687bb21573c8ad5d10a4a. Đã đối chiếu C++/header trong build và ZIPreadback với source. verification-inputs.zip chứatest/mocks/scripts/config/web; SOURCE_SHA256.json chứa checksum. previous-source.zip giữr3.

## Tự nạp và thử lại

1. Mở sketch chính 03_Firmware_ESP32S3/balancebot/balancebot.ino hoặc giải nén balancebot-standalone-r4-source.zip, mở balancebot/balancebot.ino; giữ mọi file cùng thư mục. Không dùng lqr_only/lqr_60_test.
2. Kê xe chắc, motor tắt, ngắt bridge/Serial Monitor khỏiCOM trước khi tự nạp. Giữ cấu hình N16R8 đã xác minh, Erase All Flash Disabled.
3. Đây là image mới: firmware phải chờ hiệu chuẩn, không tự dùng biasr2. Giữ xe yên, bấm Hiệu chuẩn gyro, chờ imu_calibrated=1 và imu_cal_saved=1. BấmSTOP để ngăn tự khởi động trong lúc chuẩn bị; STOP không bị hiệu chuẩn gỡ.
4. Dùng khung chống ngã và khoảng trống, dựng gần đúng điểm cân bằng. START nếu đãSTOP, hoặc bật nguồn lại rồi dựng dưới10° ổn định5s để tự chạy. Không nhấn nút hướng trong lần đo này. Không lấy ngưỡng10° làm mục tiêu đặt xe nghiêng.
5. XuấtJSON từ trước Run đến sau khi đứng yên; so thời gian/vận tốc/quãng dịch chuyển và quan sát bánh thật. Nếu vẫn lùi đáng kể, cần kiểm tra zero thực tế và phản hồi xung/bánh theo log mới, không tự tănggain hay nới giới hạn.

Chưa nạp hoặc thửr4 trên xe thật, chưa chứng minh hết lùi hoặc giữ vị trí tại chỗ. Không truy cậpCOM, chạyCALGYRO/START hoặc bậtmotor trong phiên sửa. NVS mấtnguồn, chiều bánh, bước thật và timing trênboard chưa được kiểm chứng. Source đầu phiên thiếu WifiSettings/LQR2 dù webthamchiếu; không khôi phục ngoài phạmvi này.
