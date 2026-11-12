# Hồ sơ phần cứng

Đã được người dùng xác nhận: ESP32-S3 loại44 chân; MPU6050; A4988; nguồn12V,
hạ5V; NEMA17; không encoder. Chưa có mã module, flash/PSRAM, revision board,
mã motor, dòng/điện cảm, Rsense driver, Vref, jumper vi bước, hướng lắp IMU,
sơ đồ dây, cổng nạp, kết quả đo nguồn và tín hiệu.

resolution_status: needs-disambiguation
physical_status: unverified
Toolchain tham chiếu: Arduino CLI1.5.1, ESP32 core3.3.12.
Mức logic ESP32 phải được bảo vệ theo board/schematic thực tế; nguồn5V cấp
board không có nghĩa GPIO chịu5V. Không chốt GPIO dựa riêng số chân.
`nCập nhật người dùng: ESP32-S3-DevKitC-1; STEP trái GPIO4, DIR trái GPIO5, STEP phải GPIO6, DIR phải GPIO7; hai EN chung GPIO15. MS1/MS2/MS3 nối VDD (1/16); SLP nối RST nối nguồn ghi là3V. Chờ xác nhận SDA/SCL, mã module và VDD driver/nguồn3V3.
SDA GPIO8, SCL GPIO9; VDD A4988 lấy3V3 từ ESP32 đã được người dùng xác nhận. Trang pindexio được cung cấp nhưng công cụ không truy cập được; không suy mã flash/PSRAM từ trang đó.
