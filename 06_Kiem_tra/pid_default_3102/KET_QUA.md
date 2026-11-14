# PID mặc định và khung nút — 12/11/2026

Phạm vi: mặc định PID vận tốc 3/1/0.2 theo người dùng; gom bốn nút tiến/lùi/xoay trái/phải trong một fieldset. Nhấn giữ/thả/mất focus giữ hành vi đã có. Tốc độ yêu cầu vẫn 5 cm/s và 20 độ/s. Không tăng giới hạn hoặc chỉnh thuật toán/gain LQR/yaw/cơ khí/phần cứng.

Nguồn shared/robot.json chỉ đổi pid.velocity; chạy tools/design.py sinh shared/design.json và balancebot/GeneratedConfig.h. Đối chiếu trước/sau PASS: header chỉ đổi 3 hệ số và SHA256; K LQR giống nguyên giá trị. Không sửa tay header. Các sketch LQR-only/60-test giữ nguyên, dùng hash cũ.

Web index.html gom khung và đồng bộ giá trị PID ban đầu/nhãn mặc định; style.css bổ sung kiểu khung/nút; browser.test.js cập nhật tên nút và kiểm tra có đúng 4 nút trong khung. Không sửa backend hoặc dashboard.js trong bước này.

Kiểm chứng vừa chạy: test_configuration.py 3/3 PASS; control_tests.cpp PASS, có xác nhận tuning mặc định 3/1/0.2; API/browser 3/3 PASS (fake TCP), nhấn giữ/thả và mất focus về 0. Đã xem ảnh results/drive-pad.png. Build ESP32-S3 core3.3.12 profile USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi PASS exit0: flash947459 bytes (72%), static RAM49076 bytes(14%). App SHA256 1B8960378543F478A9AFA7083DD3697C0791B98912CB820F05CFC46C774E94E5. Chưa nạp hoặc chạy motor.

Source ZIP đã đọc lại đối chiếu hash; SOURCE_SHA256.json định danh các file firmware.

Sau nạp: khởi động lại đúng server web dự án, Ctrl+F5 để nạp metadata/nút mới. Hash mới b3a774dc8d5a5891cf70cace06f40e6272ab717040ef0031ed9c320f20c68872. NVS tuning có hash cũ không được restore (cả zero và PID/yaw đã lưu); giữ cơ chế NVS hiện có, không xóa flash. Do đó kiểm tra lại gyro và điểm zero khi motor tắt trước thử. Không coi gain người dùng chọn là đã được kiểm chứng ổn định trên phần cứng.
