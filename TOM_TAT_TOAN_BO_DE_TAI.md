# Tóm tắt toàn bộ đề tài và bàn giao để tiếp tục

## Bàn giao mới — 12/11/2026, khoảng 14:04–14:08 Asia/Bangkok

Phần này cập nhật trạng thái lịch sử bên dưới, không phải báo cáo tổng hợp.

- Máy hiện tại không có D:/TLHT/Two-wheeled-self-balancing-robot. Đã làm việc
  tại D:/blancing/Robot_Collab; Git sạch trước sửa. Không sửa 00_Nguon, gain,
  thông số cơ khí, GeneratedConfig.h hoặc cờ phần cứng.
- Đã đọc các tài liệu gốc, Excel Thong_so và 01_Danh_sach_can_do. Xung đột
  W01/C07 và đơn vị vẫn tồn tại; không chuyển các giá trị này vào cấu hình.
- Khi mới kiểm tra chưa có thiết bị USB robot. Sau khi người dùng cắm, cổng
  là COM17, VID 1A86 PID 55D3, serial 5B8E094288; không dùng lại COM13.
- Thu Serial thô 9 giây ở 115200: 100709 bytes, gửi GET sau 4 giây, nhận dữ
  liệu đúng cấu trúc. Không chủ động reset hoặc nạp. Telemetry có các trường
  cal_accel_std_mg/cal_gyro_std_mrad/imu_sample_fresh mới, WHO_AM_I=112 (0x70),
  st=0, left=right=u=0, hardware_confirmed=1. Không xác nhận danh tính binary
  trên flash chỉ từ telemetry. Giả thuyết sai USB CDC/UART0 không được chứng minh.
- Nguyên nhân chặn server đã tái hiện: shared/robot.json checkout CRLF cho hash
  35c3463892a84db68993753724b216eadc8495d32e4ca004d0ad0093a8be04d3; bản LF cho
  a2db1c218dd854f05cae4da1791f5e7c81cd8f6645aafd693e0014859e10f9b4, đúng với
  design/header và xe. Server từ chối khởi động bằng Design/config mismatch.
  Đã cố định eol=lf trong .gitattributes và chuẩn hóa file về LF; không đổi giá
  trị JSON, không sinh lại gain/hash. Thêm hồi quy trong test_configuration.py.
- web_server.js: theo dõi gửi CALGYRO, chặn gửi lặp khi chờ ACK, báo timeout
  2 giây, mất link và firmware từ chối. Không suy ra đạt từ ACK. Từ chối hiệu
  chuẩn khi telemetry mới cho biết Arming/Run; firmware vẫn kiểm tra lại.
- dashboard.js: không xếp hàng robotCmd khi socket mất kết nối; trạng thái
  phân biệt cổng mở/chưa có telemetry/khác cấu hình/telemetry mới. Giữ bố cục.
- Người dùng xác nhận giữ xe yên, motor tắt. Đã thử nút Hiệu chuẩn thật bằng
  trình duyệt trên bridge kiểm thử cổng 13010, COM17: nhận ACK:CALGYRO_PENDING;
  telemetry chuyển imu_diag 1 → 0 → 1, có 98 mẫu imu_calibrated=0 rồi trở lại
  1. Trong 273 mẫu có IMU diagnostics, toàn bộ st=0, left=right=0. Không gửi
  START, không bật motor. Sau đạt: cal_accel_mg=1055, accel_std=5 mg,
  gyro_std=5 mrad/s, theta khoảng 0,0212 rad. Không coi đây là xác minh góc
  chuẩn hoặc dấu pitch. Overruns tăng 7 → 13 trong khoảng thử, chưa xác minh
  nguyên nhân hay timing khi Run; không đổi deadline/ngưỡng để che số đếm.
- API/status sau sửa: connected=true, fresh=true, matching=true. malformed=1
  không tăng trong lần thử; raw ban đầu có đoạn đầu dòng bị cắt lúc mở cổng.
  Không suy ra parser hỏng từ một dòng ban đầu; cần theo dõi nếu tăng tiếp.
- Kiểm thử mới: node --test 4_Web/tests/api.test.js PASS 2/2; kiểm tra gửi,
  ACK, từ chối, timeout, mất link, không tạo telemetry từ ACK. Browser test
  PASS 1/1 với PLAYWRIGHT_MODULE trên máy DELL. Python test_configuration
  PASS 3/3. Toàn bộ unittest discover INCOMPLETE: thiếu scipy khiến 2 module
  không import được. Lệnh npm không có trong PATH; đã chạy trực tiếp node.
- Không build/nạp firmware trong phiên này vì không sửa firmware. Không gọi
  kết quả build/host/mô phỏng lịch sử là kết quả mới. Chưa xác minh chiều bánh,
  dấu pitch bằng thao tác có đối chiếu, độ chính xác góc, NVS, hoặc cân bằng.
- Evidence: .build/diagnostics/com17-115200.{bin,txt,json} và
  web-com17-calibration.{json,png}; browser regression ở 4_Web/results.
- Server dự án hiện chạy http://127.0.0.1:3000 và nối COM17 115200; không có
  Serial Monitor hoặc công cụ nạp cùng giữ COM. Bridge kiểm thử 13010 đã đóng.
  Chỉ ngắt trên web trước mở monitor/nạp; không kill tất cả node.exe.

Bước tiếp theo: Ctrl+F5 trang web, giữ motor tắt, kiểm tra nhãn telemetry mới.
Giữ xe thẳng rồi nghiêng chậm về trước/sau, đối chiếu theta và omega: trước
theo quy ước THU_XE phải theta dương, omega cùng dấu tốc độ thay đổi theta.
Ghi góc nghiêng đối chiếu và số đo; chưa đặt zero để che sai hướng/góc. Chưa
bấm START. Sau khi dấu IMU đã được xác nhận mới chuẩn bị phép thử riêng từng
bánh có timeout trên giá đỡ; firmware hiện chưa có JOG tùy ý qua web.

---

Cập nhật: 12/11/2026 (Asia/Saigon). Đây là ghi chú kỹ thuật và trạng thái làm việc, không phải báo cáo học thuật theo form của trường.

## 1. Mục tiêu và yêu cầu đã thống nhất

Xe hai bánh tự cân bằng dùng ESP32-S3, hai động cơ bước NEMA17 và A4988. Vòng trong LQR giữ pitch, vòng ngoài PID bám vận tốc dọc và tốc độ yaw. Không có encoder. MATLAB, Simulink, mô phỏng Python và lõi C++ dùng cấu hình chung để có thể tái lập.

Ưu tiên chạy xe trước; người dùng tự làm báo cáo tổng hợp. Giữ nguyên bố cục giao diện web đã gửi cho thầy, chỉ sửa adapter, luồng dữ liệu và phản hồi cần thiết. Không đưa thuật toán điều khiển của bản cũ vào bản mới. Không sửa nguồn gốc. Không tuyên bố thử xe thành công dựa trên build hoặc giả lập.

Thư mục chính: `D:/TLHT/Two-wheeled-self-balancing-robot`.

## 2. Nguồn tham khảo

- Excel: `C:/Users/USER/Downloads/Bang_do_thong_so_thiet_ke_lai_xe_NEMA17.xlsx`, đặc biệt `Thong_so`, `01_Danh_sach_can_do`. Có xung đột r/W với W01/C07; xem bảng nguồn trong `05_BaoCao/01_Khao_sat_nguon.md` trước khi thay cơ khí.
- ZIP LQR: `C:/Users/USER/Downloads/Two--wheeled-self-balancing-robot-main.zip`; các đường dẫn LQR/Code/LQR/LQR.ino, LQR/matlab/CacFunctionTrongSimulink_LQR.m và Mophong.slx.
- Đề tài cũ: `D:/TLHT/HTDKTD_Xe_Can_Banh`; firmware cũ tại `3_Firmware_ESP32S3/balancebot_stepper_esp32s3`.
- Giao diện người dùng yêu cầu giữ: `D:/TLHT/HTDKTD_Xe_Can_Banh/4_Web (2).zip`.
- Video thử cũ: `C:/Users/USER/Downloads/video_test.mp4`. Không suy ra hiệu năng hiện tại từ video này.
- YouTube: https://www.youtube.com/watch?v=eeaqPCHMAXg&t=1918s . Ledger nguồn ghi không truy xuất được; không gán kết luận cho mốc chưa kiểm chứng.

## 3. Phần cứng

| Thành phần | Thông tin |
|---|---|
| Board | Người dùng xác nhận ESP32-S3-DevKitC-1, 44 chân; ảnh module ghi N16R8 |
| IMU | Ban đầu gọi MPU6050; WHO_AM_I thực tế 112 = 0x70, firmware có nhánh MPU6500 |
| Motor/driver | Hai NEMA17, hai A4988; full step tạm dùng 200/vòng |
| Nguồn | VMOT 12 V; hạ áp 5 V cho board; logic A4988 3,3 V |
| Microstep | MS1/MS2/MS3 nối VDD 3,3 V → 1/16 |
| Sleep/reset | SLP nối RST, nối 3V3 từ ESP32 |
| STEP/DIR trái | GPIO4 / GPIO5 |
| STEP/DIR phải | GPIO6 / GPIO7 |
| Enable chung | GPIO15, A4988 kích hoạt mức thấp |
| I2C | SDA GPIO8, SCL GPIO9, địa chỉ 0x68 |
| USB | COM13 từng xác nhận, thiết bị báo nhà sản xuất wch.cn |
| Encoder | Không có; vận tốc từ xung không đo được mất bước/trượt |

HardwareConfig.h hiện mở khóa cấu hình phần cứng mặc định (`ROBOT_HARDWARE_CONFIRMED=1`). Khởi động vẫn tắt motor. `invertL=false`, `invertR=true`; hướng cơ khí/dấu pitch chưa có bằng chứng thử hoàn chỉnh.

**Bất nhất cần biết:** `shared/robot.json` vẫn ghi `hardware_confirmed=false`, còn HardwareConfig.h là true. Không coi flag trong JSON là trạng thái khóa thực tế. Không đổi JSON tùy tiện vì hash và header cần sinh lại đồng bộ.

## 4. Cấu trúc project

| Đường dẫn | Vai trò |
|---|---|
| 00_Nguon | Nguồn trích và manifest |
| shared/robot.json | Thông số SI và giới hạn thiết kế |
| shared/design.json | Ma trận, gain, cực kín, hash cấu hình |
| 01_LQR_CanBang | MATLAB/Simulink cân bằng |
| 02_PID_ChuyenDong | MATLAB/Simulink chuyển động |
| 03_Firmware_ESP32S3/balancebot | Sketch Arduino và các HAL/lõi điều khiển |
| 4_Web | Express/Socket.IO, Serial/TCP và giao diện kế thừa |
| tools | Sinh gain, mô phỏng, quét tham số, build |
| tests | Kiểm thử Python và C++ host |
| 4_Web/tests | API và trình duyệt với TCP giả lập |
| 06_KiemThu/results | Dữ liệu mô phỏng |
| 05_BaoCao | Ghi chú kỹ thuật có từ trước, không phải báo cáo tổng hợp cuối |
| .build | Binary/log build; không sửa source trong đây |
| DANH_MUC_FILE.md | Danh mục file chi tiết đã có, có thể chưa bao gồm file mới nhất |
| CHUC_NANG_VAN_HANH.md | Chức năng vận hành, giới hạn và cập nhật hiệu chuẩn |

Firmware: ControlCore.h/.cpp chứa LQR/PID, bộ trộn, parser và state machine; GeneratedConfig.h được tools/design.py sinh; HardwareConfig.h chứa board/GPIO; ImuHal.cpp đọc và lọc IMU; ImuCalibration.h thống kê hiệu chuẩn; StepperHal.cpp/StepNco.h phát xung; Runtime.cpp chạy task, giao tiếp và lưu NVS; balancebot.ino gọi setupRobot/loopRobot.

## 5. Mô hình và điều khiển

Bộ cơ khí tạm dùng: mb=1,141 kg; mw=0,033 kg mỗi bánh theo mô hình; r=0,033 m; W=0,228 m; l=0,035 m; h=0,14 m; Ir=3,6e-6 kg.m². Phải xác nhận lại khối lượng bánh/thân, trọng tâm và mốc quán tính. `physical_status=UNVERIFIED`.

Trạng thái LQR là [theta rad, omega rad/s], không phải vector 3/4 trạng thái của web cũ. Đầu vào u là gia tốc tiếp tuyến bánh tương đối thân (m/s²), không phải lực N hay PWM. Luật cân bằng u_b=-K*[theta,omega]. Tích phân u ra tốc độ bánh tương đối, đổi ra STEP theo 3200/(2*pi*r) bước/m; có giới hạn tốc độ/gia tốc và phân bổ ưu tiên cân bằng.

Mô hình phi tuyến trong tools/design.py với trạng thái [theta, omega, q, x]:

- M = mb + 2mw + 2*(0,5*mw*r² + Ir)/r².
- Jc = mb*h²/12 - 2Ir (xấp xỉ thân hộp, cần đo lại); J=Jc+mb*l².
- H(theta)=J+2*r*mb*l*cos(theta)+r²*M; b(theta)=mb*l*cos(theta)+r*M.
- theta_dot=omega.
- omega_dot=[mb*g*l*sin(theta)-b(theta)*u+r*mb*l*sin(theta)*omega²+push]/H(theta).
- q_dot=u; x_dot=q+r*omega.

PID vận tốc tạo gia tốc tham chiếu và góc nghiêng tham chiếu; yaw tạo tác động vi sai. LQR không phản hồi vận tốc nên tránh hai bộ điều khiển cùng sửa vận tốc. PID chuyển động chỉ hoạt động sau khoảng ổn định đủ lâu. LQR đơn thuần không bảo đảm xe đứng yên hoặc giữ vị trí.

Chu kỳ LQR/IMU 5 ms; PID 20 ms; telemetry 40 ms. Lệnh chuyển động thuê hạn 400 ms. Giới hạn đặt v=±0,2 m/s, yaw=±0,8 rad/s; tốc độ bánh tương đối ±0,6 m/s, gia tốc ±3 m/s², STEP tối đa 15 kHz. Có ramp, giới hạn tích phân, ưu tiên phần cân bằng và xử lý đổi chế độ.

### Ma trận và gain đang xuất

```json
{
  "config_id": "nema17-relative-v1",
  "config_sha256": "a2db1c218dd854f05cae4da1791f5e7c81cd8f6645aafd693e0014859e10f9b4",
  "Ts": 0.005,
  "Q": [
    400,
    4
  ],
  "R": 1,
  "A": [
    [
      0.0,
      1.0
    ],
    [
      54.05536032666299,
      0.0
    ]
  ],
  "B": [
    [
      0.0
    ],
    [
      -11.186475821402645
    ]
  ],
  "Ad": [
    [
      1.0006757681007918,
      0.005001126229435869
    ],
    [
      0.2703376803712813,
      1.0006757681007918
    ]
  ],
  "Bd": [
    [
      -0.00013984669558578488
    ],
    [
      -0.05594497764536692
    ]
  ],
  "K": [
    -23.793396834761598,
    -2.7542335398052704
  ],
  "poles": [
    [
      0.9499955693349229,
      0.0
    ],
    [
      0.8939430051280324,
      0.0
    ]
  ],
  "baseline_K": [
    -10.564075158889256,
    -1.5201449660572182
  ],
  "pid": {
    "velocity": {
      "kp": 1.2,
      "ki": 0.25,
      "kd": 0.04
    },
    "yaw": {
      "kp": 3.0,
      "ki": 0.8,
      "kd": 0.02
    },
    "derivative_tau_s": 0.08,
    "accel_filter_s": 0.12,
    "accel_max": 0.35,
    "yaw_accel_max": 2.0,
    "velocity_max": 0.2,
    "yaw_rate_max": 0.8,
    "velocity_slew": 0.12,
    "yaw_slew": 0.6
  }
}
```

## 6. Firmware và trạng thái an toàn

Idle → START hợp lệ → Arming giữ thẳng 1 giây → Run. Fault cần RESET có chủ đích; STOP không tự xóa Fault. Kiểm tra pose khoảng 4 độ để ARM, ngã khoảng 22 độ, IMU không quá 15 ms, lỗi deadline khi đang ARM/Run vượt 15 ms; bão hòa tốc độ kéo dài 300 ms gây fault.

Task balance/IMU tách network. Giao tiếp không thực hiện thuật toán cân bằng. Flash NVS chỉ ghi khi motor tắt, trong task sở hữu state để không chồng lệnh START. SAVE lưu PID V/Y và điểm zero với hash cấu hình; không lưu trạng thái Run, không bỏ qua hiệu chuẩn sau bật nguồn.

Lệnh chính: GET, START, STOP, ESTOP, RESET, CALGYRO, ZPHERE, ZP+, ZP-, SAVE, $P,V,kp,ki,kd, $P,Y,kp,ki,kd, MOTION,0/1, CMD,session,sequence,v,w. SPEED dùng cm/s, TURN dùng độ/s qua đường tương thích. CMD dùng SI. $K bị từ chối, không dùng bộ giải cũ.

## 7. Web và kết nối

Mở http://localhost:3000. Server mặc định chỉ bind 127.0.0.1. USB 115200 baud. Web/Serial Monitor/esptool không được giữ cùng COM13 một lúc.

Khi firmware chưa cấu hình SSID nhà, phát Wi-Fi `BalanceBot-Bench`, mật khẩu cố định `BalanceBot13`, TCP `192.168.4.1:8080`. Wi-Fi này không có Internet là bình thường. Web vẫn chạy trên máy tính; phải có bridge Node đang chạy.

Giữ giao diện bản cũ, sửa nhập/xuất. Các chức năng đã nối: kết nối, GET, START/STOP, tiến/lùi/quay, hiệu chuẩn, lấy góc, ZP, PID vận tốc, SAVE, ba đồ thị và CSV. PID yaw có giao thức nhưng chưa có bảng chỉnh riêng trong giao diện cũ. Đồ thị thứ ba là gia tốc điều khiển m/s², không phải lực N; vận tốc suy từ xung, không phải encoder.

Giới hạn còn: /api/lqr chỉ đọc cấu hình đã xuất; cập nhật gain trực tuyến từ bộ giải cũ chưa hỗ trợ. Lưu Wi-Fi nhà từ web chưa hỗ trợ. Một số nhãn/hướng dẫn tĩnh của giao diện gốc còn mô tả PID vị trí hoặc giới hạn cũ; cần rà soát để tránh hiểu nhầm, không thay bố cục.

## 8. Sự cố IMU và sửa mới nhất

Bản cũ Imu.cpp hiệu chuẩn 400 mẫu gyro, kiểm tra độ lệch chuẩn 0,02 rad/s, không bắt từng mẫu gia tốc phải trong 0,95–1,05 g. Bản mới trước sửa bắt 500 mẫu liên tiếp trong dải đó, đồng thời khóa cập nhật góc nếu hiệu chuẩn thất bại. Vì vậy cảm biến có độ lớn quanh 1,05 g có thể đứng yên vẫn không đạt.

Telemetry từng quan sát: imu_who=112 (0x70), imu_diag=5, imu_calibrated=0, cal_accel_mg khoảng 1047–1050. Giá trị đó trước sửa là mẫu cuối đã làm tròn, không phải thống kê cửa sổ. Không kết luận cảm biến hỏng chỉ từ con số này.

Sửa ngày 12/11:

- ImuCalibration.h dùng thống kê Welford cho từng trục, 500 mẫu/cửa sổ, tối đa 3 cửa sổ.
- Chấp nhận trung bình độ lớn 0,90–1,10 g, độ lệch chuẩn mỗi trục gia tốc ≤0,02 g, gyro ≤0,02 rad/s; gyro thô không vượt 0,15 rad/s.
- Đây là giới hạn kỹ thuật tạm dùng, không phải hiệu chuẩn offset/scale gia tốc kế. Không tự chuẩn hóa độ lớn để giấu lỗi. Cần thử nhiều tư thế và kiểm tra góc thực trước chạy tự do.
- Luôn xuất góc accelerometer/gyro thô khi chưa hiệu chuẩn để chẩn đoán, nhưng Sensors.valid=false nên START không được phép.
- Khi hiệu chuẩn đạt, cập nhật bias gyro rồi dùng lọc bổ sung như trước.
- Telemetry mới: cal_accel_mg là trung bình; cal_accel_std_mg, cal_gyro_std_mrad là độ lệch chuẩn lớn nhất theo trục; imu_sample_fresh là độ mới của số đo chẩn đoán. Web bỏ góc khi mẫu quá hạn 20 ms.

## 9. Bằng chứng kiểm thử và build

Đã quan sát trong lần sửa gần nhất:

- tools/build_host.ps1 exit 0: cửa sổ ổn định quanh 1,05 g đạt; rung, quay, gia tốc sai lớn, thiếu mẫu, NaN bị từ chối; các test lõi điều khiển cũng qua.
- npm test exit 0: API từ chối thay gain trực tuyến và origin không hợp lệ.
- npm run test:browser exit 0: TCP giả lập, dữ liệu ba đồ thị, SAVE, CALGYRO, lấy góc, START/STOP, PID, vận tốc, CSV, dữ liệu sai/hết hạn; thêm ca vẫn hiện góc chẩn đoán nhưng không gửi START khi imu_calibrated=0.
- Build ESP32-S3 N16R8 exit 0: flash 947459 bytes (72%), globals 49076 bytes (14%).
- Binary: `.build/esp32s3-n16r8/balancebot.ino.bin`.
- SHA256: `2FC98ED0405E1E51F9D2311E1A75108F84D6C8ACE31CBFDD153A1CC6A200A233`.
- FQBN: `esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi`; core trong build options là 3.3.12.

Build/test máy tính không chứng minh đã nạp đúng firmware hoặc xe cân bằng được. Chưa có xác nhận chạy cân bằng thành công cho bản mới. Các kết quả MATLAB/Simulink/mô phỏng từ các lần trước cần đọc log/artifact tương ứng; không tự gán PASS mới. Ledger 05_BaoCao/06_Trang_thai_kiem_chung.md có nội dung cũ hơn, chưa phản ánh build/test mới.

## 10. Vấn đề đang mở ngay tại thời điểm ghi file

Người dùng báo: “không nhấn được hiệu chỉnh trên web hiện tại”. Chưa giải quyết xong, không khẳng định nút bị disabled.

Đã kiểm tra source: nút gọi robotCmd('calgyro'); robotCmd phát robotControl qua socket. Không thấy thuộc tính disabled ở nút. Trước đó browser test với thiết bị giả lập bấm được nút và nhận CALGYRO.

Trạng thái server thực tế vừa đọc: connected=true nhưng fresh=false, matching=false, telemetry=null; malformed=1, owner=null. COM13 vẫn được liệt kê. Đã gọi /api/disconnect rồi /api/connect COM13 115200; cả hai trả success=true, nhưng telemetry vẫn null sau đó. Như vậy mở được COM chưa chứng minh có dữ liệu từ ESP32.

Chưa xác nhận bản firmware mới đã nạp trên xe. Không tự gửi START, không ép imu_calibrated=1. Không có kết quả hiệu chuẩn thật sau bản sửa mới nhất.

**Bước tiếp theo ưu tiên:**

1. Kiểm tra trang thực người dùng và phản hồi socket khi nhấn Hiệu chuẩn; phân biệt không click được, không gửi lệnh, không có ACK, và calibration bị từ chối.
2. Xác định tại sao COM13 mở nhưng không có telemetry: đúng firmware/cổng USB, board còn chạy hay reset, output Serial đi native USB CDC hay UART0 qua WCH. Cấu hình build bật CDCOnBoot trong khi COM13 là wch.cn là điểm cần kiểm tra, chưa phải nguyên nhân đã chứng minh.
3. Ngắt bridge trước khi mở Serial Monitor hoặc nạp; ghi log boot và dữ liệu raw, không để hai chương trình tranh cổng.
4. Sau khi có telemetry mới với các trường mới, thử CALGYRO trên xe được kê chắc; kiểm tra ACK, chẩn đoán, thống kê, góc khi nghiêng và dấu pitch.
5. Chỉ khi các bước trên đạt mới thử chiều bánh/ARM trên giá thử. Sau đó mới tinh chỉnh LQR/PID từ log thực.
6. Hoàn thiện các chức năng chưa hỗ trợ (LQR tuning đã xác minh, Wi-Fi nhà, PID yaw UI) theo phạm vi người dùng, không quảng cáo là full option khi chưa làm.

## 11. Lệnh chạy lại

Chạy từ thư mục gốc project:

```powershell
./tools/build_host.ps1
./tools/build_firmware.ps1
python -X utf8 -m unittest discover -s tests
# Chỉ sinh lại khi chủ động thay cấu hình, vì sẽ đổi header/gain/hash:
python -X utf8 tools/design.py
python -X utf8 tools/simulate.py
python -X utf8 tools/sweep.py
matlab -batch "run('01_LQR_CanBang/run_all.m')"
matlab -batch "run('02_PID_ChuyenDong/run_all.m')"
cd 4_Web
npm test
npm run test:browser
npm start
```

Sau sửa JS: Ctrl+F5; sửa server thì khởi động lại đúng tiến trình dự án. Không dừng tất cả node.exe. Upload thủ công: mở sketch 03_Firmware_ESP32S3/balancebot/balancebot.ino, kiểm tra N16R8 và COM thực tế, ngắt kết nối web/Serial Monitor trước upload. Cần kiểm chứng lựa chọn USB CDC theo cổng đang dùng trước khẳng định firmware Serial hoạt động.

## 12. Nguyên tắc tiếp tục

- Ưu tiên dữ liệu thực và sửa nguyên nhân; không giả trạng thái Run/ready để vượt lỗi.
- Giữ giao diện gốc, tách lỗi web khỏi lỗi cảm biến và lỗi điều khiển.
- Dùng chung cấu hình mô hình, không sửa tay GeneratedConfig.h.
- Không đổi ngưỡng/gain chỉ để hiện PASS; ghi giả định và phép thử.
- Skills đã áp dụng gần đây: systematic-debugging, verification-before-completion; có test host và browser cho thay đổi. Các skill/công đoạn lịch sử xem 05_BaoCao/09_Skill_va_nguon_hang.md.
- File này là điểm bắt đầu cho phiên sau; kiểm tra trạng thái mới trước khi lặp lại thao tác hoặc kết luận.

## Bổ sung nút RESET trên web — 12/11/2026
Theo yêu cầu người dùng: thêm btn-reset cạnh phần điều khiển trong public/index.html; dashboard.js gửi RESET có chủ đích, đưa slider về 0, báo ACK và mã fault, theo telemetry trở về Chờ. Chặn thao tác khi giao diện đang báo ARM/Run; firmware vẫn giữ kiểm tra riêng. Không tự xóa lỗi hoặc tự START. Browser regression TCP giả lập kiểm tra fault → bấm RESET → ACK/telemetry Idle, không phát sinh START. Không thử RESET trên xe thật trong lần sửa này. Không sửa firmware hoặc gain. Tải lại Ctrl+F5; không cần khởi động lại server vì chỉ đổi tài nguyên tĩnh và test.

## Bản thử LQR riêng — 12/11/2026
Theo yêu cầu người dùng, chuẩn bị sketch độc lập 03_Firmware_ESP32S3/balancebot_lqr_only; giữ nguyên balancebot hiện tại. Lõi khóa motion=false mỗi tick, từ chối MOTION/SPEED/TURN/CMD/$P bằng ERR:LQR_ONLY. GET có EVT:CONTROL,LQR_ONLY,PID_DISABLED. Giữ nguyên gain, cấu hình, IMU, driver, an toàn, Wi-Fi và NVS; đọc kỹ lưu ý điểm zero dùng chung. Build target mới PASS (939743 bytes flash, 49076 bytes globals), source regression 3/3 PASS; chưa chạy C++ host hoặc thử binary trên xe. Artifact source ZIP, firmware, checksum và log tại 06_KiemThu/lqr_only; hướng dẫn nạp trong README của sketch. Không tự nạp, không gửi START hoặc bật motor. Xem 06_KiemThu/lqr_only/KET_QUA.md cho phạm vi chứng cứ và lỗi build đầu đã khắc phục.

Phân tích thử mới: log robot_1794479194395.json xác nhận SPEED_RAIL tại t=2030.302 s, góc khoảng 5.28 độ, motion=0. Hai bánh giữ lệnh +0.6 m/s khoảng 300 ms rồi tắt; không phải FALL trong log này. Video đã xem 16 frame, nhiều đoạn thử bánh trên không/giữ thân; chưa đồng bộ với log hay xác nhận chiều bánh. Không sửa gain/giới hạn. Bằng chứng và chi tiết: 06_KiemThu/lqr_only/20261112_speed_rail/PHAN_TICH.md.
