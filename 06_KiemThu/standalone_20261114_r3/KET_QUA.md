# Firmware mới phải hiệu chuẩn lần đầu — r3, 14/11/2026

Yêu cầu đã xác nhận: mỗi image firmware mới phải chờ người dùng hiệu chuẩn gyro thực tế đạt/lưu thành công mới được chạy. Các lần bật nguồn cùng image dùng lại bộ đã lưu. Nạp lại đúng cùng binary không yêu cầu đo lại.

## Thay đổi và bằng chứng

- ImuHal.cpp lấy app_elf_sha256 từ esp_app_get_description, đối chiếu với record NVS. API/field đã đọc trong SDK ESP32-S3 core 3.3.12 cài trên máy.
- SavedImuCalibration.h đổi record từ version1/72 byte sang version2/104 byte; thêm SHA256 firmware, giữ CRC/context/range checks. Record cũ hoặc khác firmware bị từ chối, không tự ước lượng bias. Chỉ CALGYRO thực tế mới tạo record; readback đạt trước ready=true.
- Khi chưa có record cho image hiện tại: diag7, snapshot.valid=false, START trả ERR:IMU, auto không chạy. Web chỉ sửa nhãn chờ hiệu chuẩn; README sửa quy trình mỗi image mới.
- Không đổi LQR/PID, GPIO, ngưỡng40°/10°/5s, giới hạn tốc độ/gia tốc/STEP, cờ phần cứng hay generated config. Đối chiếu r2 trong previous-source.zip: firmware chỉ đổi ImuHal.cpp, SavedImuCalibration.h và README.md.

## Kiểm chứng vừa chạy

- tools/build_standalone_host.ps1: 3 executable PASS; gồm controller, IMU acceptance và ImuHal.cpp thật với Wire/NVS/clock/app-descriptor giả. FirmwareA→B giữ nguyên NVS vẫn chặn manual/auto; explicit calibrationB mới cho controller chạy; power-cycleB không ghi/đo lại; rollbackA bị chặn; CRC/context/invalid identity/write/I2C failures giữ motor không sẵn sàng. Dữ liệu mock không được dùng làm bias thật.
- node --check 4_Web/public/dashboard.js: PASS. git diff --check phần tracked sửa: PASS (cảnh báo CRLF). Không chạy lại suite web toàn phần vì chỉ đổi nhãn diag7.
- Arduino CLI compile ESP32-S3 core3.3.12, USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi, jobs1: PASS. Flash951079/1310720 byte, RAM49076/327680 byte. Log firmware-build.log.
- Binary SHA256 f005f01a6e3fa6f025d8911d276f737ed7f954d405eb50672fafb66df9a583c0. Source ZIP ac9b7d22e9789fe6cde544238c29941493bc4c084a472800f9d37e18b9ff8a7e. ZIP readback trùng source; C++/header Arduino build trùng source sau bỏ #line do Arduino thêm. verification-inputs.zip chứa test/mocks/script/config/web đã kiểm tra readback. Manifest SOURCE_SHA256.json.
- Lần tạo manifest đầu lỗi path relative/absolute; sửa riêng manifest, không ảnh hưởng source/build/test.

## Thao tác tiếp theo và giới hạn

Tự nạp sketch balancebot trong source ZIP hoặc thư mục chính; không xóa toàn flash. Kê chắc, motor tắt, giữ yên và bấm Hiệu chuẩn gyro. Chờ imu_calibrated=1 và imu_cal_saved=1. Sau đó mỗi lần bật nguồn cùng image dùng lại bias; dựng dưới10° ổn định5s để tự cân bằng nếu không có STOP chủ động. Nhấn STOP khi cần ngăn tự khởi động. Lần sau nạp image khác phải hiệu chuẩn lại.

Chưa nạp, chưa đo gyro thật, chưa mở COM hoặc bật motor. Host/mocks/build không chứng minh cân bằng thực tế, NVS khi mất nguồn hay timing trên board. Source đầu phiên thiếu Wi-Fi settings/LQR2 dù web tham chiếu; không khôi phục ngoài phạm vi, không lấy suite legacy làm PASS.

Sau khi r3 đóng gói, người dùng cung cấp log r2 mới về lùi lúc khởi động; phân tích/sửa tiếp được tách thành r4, không coi r3 đã chữa hiện tượng lùi.
