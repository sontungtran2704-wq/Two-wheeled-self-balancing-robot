//==============================================================================
//  XE HAI BÁNH TỰ CÂN BẰNG (ESP32-S3) - ĐIỀU KHIỂN KẾT HỢP: PID VÒNG NGOÀI + LQR VÒNG TRONG
//
//  CẤU TRÚC FILE (mỗi module là 1 struct + 1 instance toàn cục):
//    Config.h          Chân, cơ khí, thông số điều khiển, an toàn, WiFi, mặc định K
//    StepperDrive      struct StepChannel / StepperDrive : phát xung STEP bằng timer (DDS 40 kHz), odometry
//    Imu               struct Imu          : MPU6050 (I2C), hiệu chuẩn gyro
//    AngleFilter       struct AngleFilter  : lọc bù -> θ, θ̇
//    Params            struct Params       : K, A41, B4, zeropoint + NVS
//    Controller        struct Controller   : máy trạng thái + PID vị trí ngoài + LQR cân bằng trong + failsafe
//    Net               struct Net          : WiFi (STA/AP) + TCP 8080
//    Commands          bộ lệnh dùng chung Serial/TCP
//    Status            telemetry + LED trạng thái
//    Log / LineReader / Util   tiện ích
//
//    Board: ESP32-S3 (Arduino: "ESP32S3 Dev Module"). Bật "USB CDC On Boot" để Serial qua cổng USB.
//    Module N8R8/N16R8 (OPI PSRAM) chiếm GPIO26-37; firmware không dùng các chân này.
//    Nối dây S3:  STEP_L=GPIO4  DIR_L=GPIO5  STEP_R=GPIO6  DIR_R=GPIO7  EN=GPIO15  SDA=GPIO8  SCL=GPIO9  (MPU6050 cấp 3V3)
//    Driver A4988 / DRV8825 / TMC2209 (chế độ STEP/DIR), 2 cái, chung chân EN (nên kéo EN lên 3V3 bằng 10k).
//    Đặt vi bước bằng jumper MS1..MS3 cho khớp MICROSTEP trong Config.h.
//    Cấp nguồn động cơ 12-24 V (momen tốc độ cao tốt hơn khi điện áp cao).
//
//  QUY ƯỚC DẤU:  θ > 0 = thân nghiêng ra TRƯỚC;  x > 0 = xe đi tới.
//  Kiểm tra dấu lần đầu (KHÔNG ARM): đọc telemetry, nghiêng xe ra trước bằng tay
//  -> theta phải TĂNG. Nếu giảm: đổi SIGN_THETA = -1.0f trong Config.h.
//
//  LỆNH SERIAL (115200, kết thúc bằng \n):
//    START            : bắt đầu arm (giữ xe thẳng đứng ~1 s thì tự chạy)
//    STOP             : dừng, tắt driver
//    SPEED,<cm/s>     : vận tốc mục tiêu (dương = tiến)
//    TURN,<deg/s>     : tốc độ quay (dương = quay trái)
//                       (SPEED/TURN khác 0 phải được gửi lại < CMD_TIMEOUT_MS, nếu không xe tự về 0)
//    ZP,<deg>         : đặt zeropoint            ZPHERE : lấy góc hiện tại (xe giữ tay ở điểm cân bằng)
//    ZP+ / ZP-        : tăng / giảm zeropoint 0.1°
//    CALGYRO          : hiệu chuẩn gyro (xe đặt yên)
//    $K,k1,k2,k3,A41,B4,FrefGain : nạp 3 gain LQR vòng trong + mô hình gia tốc (đổi dấu bị từ chối)
//    $P,Kp,Ki,Kd      : chỉnh PID vị trí vòng ngoài khi đang chạy (không cần nạp lại); SAVE để lưu
//    JOG,<L cm/s>,<R cm/s>,<ms> : quay thử 2 bánh (chỉ khi IDLE, tối đa 15 cm/s, 2 s) để kiểm tra chiều quay
//    SAVE             : lưu NVS (không khi đang RUN)
//    DEFAULTS         : về giá trị mặc định + xoá NVS (giữ WiFi; không khi đang RUN)
//    GET              : in tham số + địa chỉ WiFi + số lần vòng điều khiển bị trễ
//    WIFI,<ssid>,<pass> : lưu WiFi nhà (NVS), cần REBOOT để áp dụng    REBOOT : khởi động lại (không khi đang RUN)
//
//  WIFI (cùng bộ lệnh như serial, thêm cổng TCP 8080, 1 client; telemetry cũng gửi qua đó):
//    - Có WiFi đã lưu và kết nối được -> chế độ STA: tìm xe bằng  balancebot.local  hoặc IP in ra lúc khởi động.
//    - Không có / không kết nối được trong WIFI_CONNECT_MS -> tự phát WiFi riêng "BalanceBot" (AP), IP 192.168.4.1.
//    - Rút USB: xe chạy bằng pin, vẫn nhận lệnh/tinh chỉnh qua WiFi. Mất kết nối WiFi: xe đứng yên giữ thăng bằng
//      (SPEED/TURN về 0 sau CMD_TIMEOUT_MS), không tắt motor.
//==============================================================================
#include <Arduino.h>
#include "Config.h"
#include "AngleFilter.h"
#include "Commands.h"
#include "Controller.h"
#include "Imu.h"
#include "Log.h"
#include "Net.h"
#include "Params.h"
#include "Status.h"
#include "StepperDrive.h"

void setup() {
  stepper.begin();                            // tắt driver ngay từ đầu, cấu hình chân STEP/DIR/EN
  statusLed.begin();

  Serial.begin(115200);
  delay(300);
  outln("\n=== BALANCEBOT STEPPER ESP32-S3 - PID OUTER + LQR INNER ===");
  outf("RESET_REASON=%d (1=bat nguon, 3=SW, 4=PANIC, 5-7=WDT, 15=SUT NGUON/BROWNOUT)\n", bbResetReason());

  params.load();
  net.loadCreds();
  stepper.startTimer();

  if (!imu.begin()) {
    outln("EVT:IMU_INIT_FAIL");
    bot.setState(ST_FAULT);
  } else {
    bool ok = false;
    for (int i = 0; i < 5 && !ok; i++) {
      outln("Calib gyro - giu xe yen...");
      ok = imu.calibrateGyro();
    }
    outln(ok ? "GYRO OK" : "GYRO FAIL (dung bias = 0)");
    filt.initFromAccel(imu);
    params.print();
    outln("San sang. Go START de arm (giu xe thang dung ~1 s).");
  }
  net.begin();                                // sau khi hiệu chuẩn xong (xe không cần yên nữa)
}

void loop() {
  static uint32_t lastTele = 0;
  pollSerialCommands();
  net.poll();
  bot.poll();

  uint32_t ms = millis();
  if (ms - lastTele >= 40) {
    lastTele = ms;
    sendTelemetry();
    statusLed.update(bot.state);
  }
}
