# Kiểm chứng bản LQR riêng — 12/11/2026

- Sketch: 03_Firmware_ESP32S3/balancebot_lqr_only. Không sửa sketch balancebot.
- Target build PASS, exit 0: Arduino CLI với core ESP32 3.3.12;
  esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi.
  Lệnh tái lập: tools/build_lqr_only.ps1. Flash 939743/1310720 bytes,
  globals 49076/327680 bytes. Log mới: build.log; binary và SHA256: firmware/.
- Source regression PASS 3/3: bundled Python -B -X utf8 -m unittest discover
  -s tests -p test_lqr_only.py. Kiểm tra nguồn cấu hình/IMU/driver không đổi,
  thay đổi ControlCore giới hạn đúng hai khối khóa motion và từ chối lệnh,
  Runtime chỉ bổ sung nhận diện GET. Đây là kiểm tra nguồn, không phải chạy
  thuật toán C++ trên host hoặc phần cứng.
- ZIP source đã đọc lại, so khớp 14 file nguyên byte. SHA256 ZIP:
  c319418e1585d6f0d272005668ad5d4abf36eeebf240f26d823e527926c4ce07.
- Lần build đầu lỗi ar.exe tạo core.a (Invalid argument). Đổi sang build path
  sạch .build/lqrbench và --jobs 1 đã giải quyết trong lần build kế tiếp.
- NOT RUN: nạp, hiệu chuẩn trên binary mới, chiều motor, dấu pitch thực tế,
  timing trên xe, NVS qua mất nguồn, thử cân bằng. Không tự gửi START.
- Không nới ngưỡng, không đổi gain/cơ khí, không sửa tay GeneratedConfig.h.
  LQR riêng không giữ vận tốc/vị trí; chạm speed rail/ngã vẫn phải ngắt motor.
- Hướng dẫn thao tác: README.md trong sketch. NVS dùng chung với bản chính;
  điểm zero đã lưu vẫn được đọc, SAVE có thể ảnh hưởng khi quay lại bản chính.
