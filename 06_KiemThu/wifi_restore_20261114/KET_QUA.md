# Khôi phục riêng lưu Wi-Fi nhà/AP — 14/11/2026

## Nguyên nhân có bằng chứng và phạm vi

Ảnh web báo firmware chưa hỗ trợ lưu Wi-Fi. Bản r4 thực tế không chứa WifiSettings.h, không nhận WIFI_SSID/WIFI_PASS/WIFI_SAVE và không xuất wifi_cfg=1. Bridge đã có luồng gửi cấu hình và chủ động chặn khi thiếu capability; đây không phải bằng chứng nhập sai mật khẩu/router.

Đã xác minh tất cả source chính trùng ZIP r4 trước khi sửa; bản r4 được giữ trong previous-r4-source.zip. Chỉ sửa firmware Runtime.cpp, khôi phục WifiSettings.h từ source trong bản Arduino build 13/11 và cập nhật README. Không lấy Runtime/ControlCore cũ ghi đè r4. Source LQR/PID, ngưỡng 40°/10°/5s, gia tốc/tốc độ/STEP, cơ khí/GPIO, generated config, ImuHal/SavedImuCalibration và web được kiểm tra giữ nguyên checksum r4. Không làm app điện thoại, không khôi phục LQR2 ngoài yêu cầu này.

## Hành vi đã triển khai

- Runtime xuất wifi_cfg=1, nhận WIFI_SSID/WIFI_PASS dạng hex và WIFI_SAVE đúng giao thức bridge hiện có. Staging giới hạn SSID32 byte/password trống hoặc8–63 byte, chặn NUL/control/hex lỗi, khác nguồn USB/TCP, sai thứ tự và hết hạn10s.
- Chỉ task điều khiển nhận ghi khi Idle, không đang hiệu chuẩn. Dữ liệu lưu NVS balancebot/wifi-v1, tương thích cấu trúc version1 cũ; đọc lại và đối chiếu trước ACK:WIFI_SAVED. Không trả mật khẩu trong phản hồi. Lỗi ghi trả ERR:STORAGE, không hứa reboot thành công. Đã đọc SDK Preferences: putBytes gọi nvs_set_blob rồi nvs_commit.
- Sau lưu thành công, START và các yêu cầu thay đổi khác bị chặn bởi WIFI_RESTART_PENDING; tự arming được giữ chờ và output motor tắt. Network task phát ACK qua luồng cũ rồi reboot sau khoảng400ms. STOP/ESTOP vẫn nhận qua luồng dừng ưu tiên. Sau reboot áp dụng lại quy tắc khởi động r4; cần giữ nguồn motor tắt khi cấu hình.
- AP_STA: AP BalanceBot-Bench/BalanceBot13 vẫn được khởi tạo dù có cấu hình STA. Khi AP thành công, TCP8080 phục vụ ngay cả khi STA chưa nối/sai mật khẩu. WiFi.begin chỉ gọi một lần mỗi boot; SDK setAutoReconnect(true) quản lý nối lại theo sự kiện. Đã đọc STA.cpp/core3.3.12; không gọi reconnect định kỳ chen vào lúc đang nối.
- GET trả EVT:WIFI2 với IP AP/IP STA/trạng thái/configured. IP0.0.0.0 thể hiện chưa có đường kết nối; CONFIGURED chỉ có nghĩa thông tin đã lưu. Không khẳng định kết nối router chỉ dựa vào ACK lưu.
- Giữ các bản lưu gyro/PID/zero riêng biệt. Firmware mới này vẫn yêu cầu hiệu chuẩn một lần theo lựa chọn người dùng; reboot do lưu Wi-Fi cùng image không bắt hiệu chuẩn lại.

## Kiểm chứng vừa chạy

- tools/build_wifi_host.ps1: PASS. Biên dịch Runtime.cpp thật và ControlCore.cpp cùng mock FreeRTOS queues/clock, Serial, NVS, Wi-Fi/TCP. Kiểm tra staging/ACK/persistence sau boot, byte limits, open network, TTL, ownership, calibration/Run/Arming/Fault rejection, flash failure, không echo password, ACK trước reboot, pending chặn START/auto/motor, giữ nguyên blob gyro/PID. AP+STA/IP và AP khi STA thất bại được kiểm tra bằng adapter giả; đây không phải RF thật.
- Lần chạy test đầu đọc nhầm GET reply đang đợi thành reply WIFI_SAVE; sửa fixture để xử lý/drain đúng GET, sau đó toàn bộ test PASS. Không đổi logic firmware/ngưỡng để ép PASS.
- tools/build_standalone_host.ps1:3 executable PASS; giữ hồi quy r4 và calibration theo image, STOP/RESET và các fault gates. Log standalone-host.log.
- Edge headless chạy 4_Web/tests/browser.test.js:1/1 PASS. Kiểm tra form Wi-Fi gửi từng bước, ACK, hiện IP nhà/AP và các chức năng web cũ trên TCP giả. Không sửa source web hoặc tài khoản thật; server production không restart và không kết nối robot trong test.
- git diff --check Runtime + whitespace các file mới: PASS (cảnh báo LF/CRLF). Đối chiếu ZIP r4: chỉ Runtime.cpp, WifiSettings.h, README.md khác;16 file bảo toàn được kiểm tra checksum. Source ZIP/test inputs readback và C++/header trong build đã đối chiếu.
- Arduino CLI compile ESP32-S3 core3.3.12, USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,jobs1: PASS. Flash953563/1310720 byte, RAM49300/327680 byte. Log firmware-build.log.
- Binary balancebot.ino.bin SHA256:8eea2d59935f4d063a034ff5f8d03685a03859416e41fdb68be96ea239cfa7d2. ZIP balancebot-wifi-source.zip SHA256:24b11e3018bcb416af6e6edb7df10aaba4633cade88c6fa4161eb195c0c8a488. Có verification-inputs.zip, SOURCE_SHA256.json và artifact-check.log.

## Thao tác trên xe tiếp theo

1. Tự nạp sketch chính 03_Firmware_ESP32S3/balancebot/balancebot.ino; giữ16 file cùng thư mục. Có thể giải nén source ZIP rồi mở balancebot/balancebot.ino. Motor tắt, bridge/Serial Monitor nhả COM; Erase All Flash Disabled.
2. Vì đây là firmware mới, giữ xe yên và hiệu chuẩn gyro thực tế đạt/lưu trước khi thử motor. Không điền sẵn hoặc giả giá trị hiệu chuẩn. Việc cấu hình Wi-Fi có thể thực hiện khi motor tắt và xe Chờ, không cần làm nó chạy.
3. Nối web qua USB hoặc BalanceBot-Bench, IP192.168.4.1:8080. DỪNG/RESET về Chờ, nhập đúng SSID mạng nhà2,4GHz và mật khẩu, bấm Lưu WiFi và khởi động lại xe. Không đổi PID/LQR để thử mạng.
4. Kết nối lại USB/AP sau reboot, bấm Kiểm tra WiFi xe (xem IP). Chỉ khi có IP STA khác0.0.0.0 mới chuyển máy tính sang mạng nhà, ngắt đường truyền cũ trên web rồi kết nối IP_STA:8080. Máy tính ở mạng nhà mới vừa dùng Internet vừa điều khiển được. Nếu nhập sai, quay lại AP riêng để sửa/lưu lại; AP riêng không chia sẻ Internet.
5. Muốn dùng AP riêng, nối BalanceBot-Bench rồi dùng192.168.4.1:8080. Chỉ một TCP client tại một thời điểm. Không cần thay web hoặc restart server cho cờ hỗ trợ firmware mới.

## Chưa kiểm chứng trên phần cứng

Chưa nạp, không mở COM, không gửi lệnh đến xe, không lưu mật khẩu thật, không bật motor. Cần kiểm tra RF AP+STA, DHCP của router, reconnect khi mất mạng, mất nguồn trong lúc NVS commit, thời gian phản hồi ACK/reboot và jitter khi radio hoạt động. Mock/build không chứng minh xe cân bằng hoặc router thật kết nối thành công. ESP32-S3 dùng mạng nhà2,4GHz; AP và STA chia sẻ radio nên AP có thể đổi kênh khi STA nối router, thiết bị điều khiển có thể cần nối lại.
