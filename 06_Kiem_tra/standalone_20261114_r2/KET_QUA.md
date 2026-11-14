# Tự cân bằng không phụ thuộc web — bản sửa r2, 14/11/2026

Yêu cầu bổ sung: bật nguồn với hiệu chuẩn đã lưu hợp lệ thì tự cân bằng khi trong ±10° và ổn định liên tục 5 giây; không cần START/web/PC. Sau FALL hoặc SPEED_RAIL, dựng lại theo cùng điều kiện thì tự cân bằng. Web chỉ bổ trợ giám sát, điều khiển và hiệu chỉnh.

## Nguyên nhân trong source và thay đổi

- Bản trước đã bật autonomous ở setupRobot nhưng chỉ cho Fault FALL tự phục hồi. SPEED_RAIL còn chốt lỗi. CALGYRO/SAVE/chỉnh PID/zero gọi suspendAutonomous, gây khóa tự chạy cho tới START hoặc reboot.
- ControlCore.h cho phép cả FALL và SPEED_RAIL vào luồng phục hồi. Dùng lại nguyên điều kiện IMU mới/hợp lệ, pitch thô và zero-corrected dưới ±10°, omega/yaw nhỏ hơn0,15rad/s và đếm liên tục5giây. Ra vùng, dữ liệu stale/NaN, chuyển động hoặc gián đoạn vòng điều khiển làm đếm lại.
- Thêm restartAutonomousCountdown cho các thao tác cấu hình: chỉ xóa bộ đếm, không khóa vĩnh viễn và không được phép xóa STOP chủ động. Runtime dùng nó cho CALGYRO/SAVE; bộ điều khiển dùng cho chỉnh PID/zero. Trong lúc đo IMU valid=false; trong lúc ghi flash motor tắt. Sau đó bắt đầu đếm mới khi đủ điều kiện.
- STOP/ESTOP/RESET do người dùng vẫn chặn tự chạy; cấu hình/hiệu chuẩn không được gỡ chặn đó. Muốn chạy lại sau DỪNG chủ động thì START hoặc bật nguồn lại. Lỗi IMU, DEADLINE, STEP_WATCHDOG, ESTOP và INITIALIZATION vẫn chốt.
- Không đổi gain, PID, cơ khí, GPIO, bảo vệ tốc độ/gia tốc/STEP, thông số40°/10°/5s hay nguồn cấu hình/generated header. Bộ gyro đã lưu vẫn được dùng lại, không tự đo lại khi boot. SPEED_RAIL là lệnh bánh chạm0,6m/s liên tục300ms; không phải số đo encoder.
- Web/server, Serial, firmware trên board và phần cứng không bị thao tác trong phiên này. Chỉ sửa source và build; không gửi START/CALGYRO tới xe thật, không nạp.

## Kiểm chứng mới

- tools/build_standalone_host.ps1: 3 executable PASS (host.log). Bổ sung fault SPEED_RAIL phát sinh từ controller thật ở20° (dưới40°), giữ10° không chạy lại, chuyển động làm đếm lại5giây, phục hồi xóa lỗi/đổi session/mục tiêu0; STOP chặn phục hồi. Mô phỏng CALGYRO pending valid=false và hoàn tất không START, SAVE/chỉnh PID/zero đếm lại nhưng không khóa autonomous; STOP không bị callback gỡ. Có bài boot và giữ Run10giây không lệnh web, stale/NaN/motion/gap/wrap và lỗi không được phục hồi.
- Hai executable IMU acceptance và ImuHal.cpp với Wire/NVS/clock giả cũng chạy lại PASS trong cùng lệnh. Không dùng số mock làm hiệu chuẩn cho firmware.
- Lần compile test đầu trùng tên biến saved giữa Device và SavedGyro; sửa tên fixture rồi chạy lại đạt. Không đổi logic/ngưỡng để ép test PASS.
- git diff --check trên phần code sửa: PASS (chỉ cảnh báo quy ước CRLF).
- Build ESP32-S3 core3.3.12, N16R8, USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi, jobs1: PASS. Flash950687/1310720byte, RAM49076/327680byte. firmware-build.log là kết quả mới của r2.
- Binary balancebot.ino.bin SHA25661314342d1ebe2b09501f799e5843ac7aabc8be593445ff6d98ca5fe6ced1147. ZIP balancebot-standalone-r2-source.zip chứa sketch chính và hướng dẫn. Đã đối chiếu C++/header trong build với source (bỏ directive #line do Arduino thêm) và đọc lại ZIP so từng byte: PASS. SOURCE_SHA256.json ghi hash nguồn/check/artifact.

## Cách kiểm chứng trên xe sau khi tự nạp

1. Tắt nguồn motor, ngắt bridge/Serial Monitor khỏi COM rồi nạp sketch chính balancebot/balancebot.ino; Erase All Flash Disabled để giữ NVS.
2. Nếu chưa có gyro lưu từ IMU thật, hiệu chuẩn qua web một lần và đợi xác nhận đã lưu. Nếu có bản lưu hợp lệ thì không hiệu chuẩn lại khi bật nguồn.
3. Kê khung chống ngã, đóng web/tắt PC; bật nguồn xe. Giữ trong±10° ổn định5giây: phải tự cân bằng, không nhấn START.
4. Kiểm chứng phục hồi FALL và SPEED_RAIL có kiểm soát trên khung thử: sau lỗi motor phải tắt; dựng trong±10° và giữ ổn định5giây phải chạy lại. Ra vùng hoặc chuyển động phải đếm lại; lệnh tiến/lùi/xoay cũ không được chạy lại.
5. Thử DỪNG khi dùng web: phải giữ motor tắt kể cả dựng đúng góc; sau DỪNG cần START hoặc bật nguồn lại. Không chủ động gây lỗi watchdog/I2C trên xe để thử trong bước này.

Chưa kiểm chứng trên phần cứng: binary đang nạp, bias thực tế, lưu NVS qua mất nguồn, timing, chiều bánh và khả năng bắt cân bằng ở10°. Source gốc thiếu Wi-Fi nhà/LQR2 như đã ghi ở README; r2 không khôi phục phần ngoài yêu cầu đó. previous-source.zip là bản r1 để đối chiếu/khôi phục source.
