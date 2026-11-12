# Skill áp dụng và tài liệu hãng

| Skill đã đọc | Phần áp dụng |
|---|---|
| using-superpowers, brainstorming, writing-plans | Khảo sát, so sánh mô hình, ghi kế hoạch theo phạm vi đã giao |
| embedded-project-loop | Hồ sơ phần cứng, bước đo tiếp theo, tách bằng chứng vật lý |
| arduino-workflow-router, arduino-cli-skill | Chọn Arduino core có sẵn; build riêng, không upload |
| embedded-architect, realtime-control-design | Giao diện hai vòng, một chủ sở hữu chấp hành, ngân sách thời gian |
| board-support, pin-assignment | Giữ trạng thái chưa chốt module/GPIO, không suy pin từ44 chân |
| embedded-developer, ponytail | Lõi C++ độc lập, HAL và parser hữu hạn |
| wireless-telemetry | Lease, session/sequence, telemetry và reconnect |
| spreadsheets | Đọc ô nguồn Excel, giữ nguyên workbook, truy vết nguồn và đơn vị |
| sensor-calibration-workbench | Quy trình bias gyro và ranh giới chưa đo, không tuyên bố hiệu chuẩn vật lý |
| test-driven-development, systematic-debugging | Test dấu, cực kín và phát hiện lỗi bộ trộn tại biên |
| embedded-qa, verification-before-completion | Tách mô phỏng/host/build/xe thật, chỉ ghi PASS theo log |
| dispatching-parallel-agents | Tách công việc MATLAB, firmware và web theo quyền sở hữu file |

Các tham chiếu phụ `~/.agents/docs` và inventory board mà một số skill trỏ
tới không có trong bản cài; đã ghi giới hạn thay vì coi profile là xác nhận.

Tài liệu hãng đã tra:

- [A4988 datasheet](https://www.allegromicro.com/-/media/files/datasheets/a4988-datasheet.pdf): chế độ vi bước và giao diện STEP/DIR.
- [MPU6050 register map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf): sample divider, DLPF và thang gyro/accel.
- [ESP32-S3 DevKitC-1](https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html): tham chiếu họ board, không thay thế nhận dạng board44 chân thực tế.

Không có phép đo dòng motor, điện áp sụt, jitter hoặc ảnh dây trong phiên này.
