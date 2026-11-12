//==============================================================================
//CẤU HÌNH XE ESP32-S3 (chân, cơ khí, điều khiển, an toàn, WiFi)
//==============================================================================
#pragma once
#include <Arduino.h>
#include <math.h>

// Firmware nay CHI danh cho ESP32-S3 (vd ESP32-S3-DevKitC-1). Chon board "ESP32S3 Dev Module".
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Chon board ESP32-S3: so do chan trong Config.h khong dung cho ESP32 thuong (GPIO6-9 noi flash)"
#endif

//------------------------------------------------------------------------------
// 1. CHÂN
//------------------------------------------------------------------------------
  #define PIN_STEP_L     4
  #define PIN_DIR_L      5
  #define PIN_STEP_R     6
  #define PIN_DIR_R      7
  #define PIN_EN        15      // chung cho 2 driver (không dùng trong ISR)
  #define PIN_SDA        8      // chân I2C mặc định của board
  #define PIN_SCL        9
  #define PIN_RGB       38      // LED RGB onboard (board v1.1; v1.0 là GPIO48). Đặt -1 để tắt
  #define PIN_LED       -1

// Chan ESP32-S3 khong duoc dung: 26-32 (flash/PSRAM), 33-37 (OPI PSRAM neu module N8R8/N16R8),
// 19-20 (USB), 0/3/45/46 (strapping). Chan dang dung 4-9, 15 deu hop le.
#define BB_PIN_OK(p) ((p) >= 0 && (p) != 0 && (p) != 3 && (p) != 19 && (p) != 20 && ((p) < 26 || (p) > 37) && (p) != 45 && (p) != 46)
static_assert(BB_PIN_OK(PIN_STEP_L) && BB_PIN_OK(PIN_DIR_L) && BB_PIN_OK(PIN_STEP_R) && BB_PIN_OK(PIN_DIR_R) &&
              BB_PIN_OK(PIN_EN) && BB_PIN_OK(PIN_SDA) && BB_PIN_OK(PIN_SCL),
              "Chan STEP/DIR/EN/I2C trung chan hanh/PSRAM/USB/strapping cua ESP32-S3");
static_assert(PIN_STEP_L < 32 && PIN_DIR_L < 32 && PIN_STEP_R < 32 && PIN_DIR_R < 32,
              "STEP/DIR phai < GPIO32 vi ISR ghi thang thanh ghi GPIO_OUT_W1TS/W1TC");
#define EN_ACTIVE_LOW  1      // A4988/DRV8825/TMC2209: EN = LOW là bật

//------------------------------------------------------------------------------
// 2. CƠ KHÍ
//------------------------------------------------------------------------------
// Co khi do nguoi dung cung cap 2026-11-08; Ibody xap xi mb*h^2/12.
#define BODY_MASS_KG     1.141f
#define WHEEL_MASS_KG    0.033f   // moi banh
#define COM_HEIGHT_M    0.035f   // trong tam so voi truc banh
#define BODY_HEIGHT_M   0.14f
#define ROTOR_INERTIA   3.6e-6f  // moi dong co, kg.m^2
#define MODEL_REVISION  20261108.0f
#define WHEEL_RADIUS_M   0.033f   // bán kính bánh (m)
#define TRACK_M          0.228f   // khoảng cách 2 bánh (m)
#define MOTOR_STEPS_REV  200      // động cơ 1.8°
#define MICROSTEP        16       // phải khớp jumper MS1..MS3 của driver
#define MOTOR_L_INVERT   0        // đảo chiều nếu bánh quay ngược
#define MOTOR_R_INVERT   1        // 2 động cơ gắn đối xứng -> thường phải đảo 1 bên

//------------------------------------------------------------------------------
// 3. ĐIỀU KHIỂN
//------------------------------------------------------------------------------
#define CTRL_HZ          200      // chu kỳ điều khiển (Ts = 5 ms)
#define V_MAX            0.80f    // giới hạn vận tốc xe (m/s)
#define A_MAX            6.0f     // giới hạn gia tốc xe (m/s^2) - tránh mất bước
#define V_TARGET_MAX     0.40f    // vận tốc mục tiêu tối đa từ lệnh (m/s)
#define W_MAX            0.20f    // vi sai bánh tối đa (m/s)
#define X_REF_MAX        0.30f    // sai lệch vị trí tối đa khi đang chạy (m)
#define SIGN_THETA       1.0f     // đổi thành -1.0f nếu θ ngược dấu
#define FILTER_TAU_S     1.0f     // hằng số thời gian lọc bù (s)

// Mapping trục IMU. Mặc định: trục X cảm biến hướng TRƯỚC, trục Z hướng LÊN.
// Nếu lắp khác, chỉ cần đổi các macro này (và SIGN_THETA).
#define AX_FWD(ax,ay,az)   (ax)
#define AZ_UP(ax,ay,az)    (az)
#define GY_PITCH(gx,gy,gz) (gy)

// LQR VÒNG TRONG: trạng thái [theta - theta_ref, theta_dot, v_x]
// MATLAB lqr va SciPy CARE doi chieu: Q=diag([100,1,10]), R=1.
// Da kiem chung mo phong; chua xac nhan tren phan cung xe that.
#define DEF_K0   (-32.36122708f)
#define DEF_K1   (-2.51018007f)
#define DEF_K2   (-3.16227766f)
#define DEF_A41  (-6.33185948f)
#define DEF_B4   (1.31993760f)
#define DEF_FREF_GAIN (12.22925950f)  // -a/b, F_feedforward = DEF_FREF_GAIN*theta_ref (N)

// PID VÒNG NGOÀI: vị trí -> góc nghiêng tham chiếu (rad). Gain ban đầu bảo thủ,
// chỉ là điểm khởi đầu để mô phỏng/tinh chỉnh, KHÔNG phải thông số đã xác nhận trên xe.
#define POS_KP          0.50f   // rad/m  (0.8/0.6 cu gay dao dong khi co loc v_est 40 ms)
#define POS_KI          0.00f   // rad/(m.s)
#define POS_KD          0.20f   // rad/(m/s)
#define POS_I_LIMIT     0.20f  // giới hạn tích phân (m.s)
#define THETA_REF_MAX   0.10f  // rad, khoảng 5.7 độ
#define V_EST_TAU_S     0.04f  // lọc vận tốc odometry từ xung STEP
// Giới hạn khi chỉnh PID bằng lệnh $P (chặn gõ nhầm); gain mặc định ở trên
#define POS_KP_MAX      3.0f
#define POS_KI_MAX      1.0f
#define POS_KD_MAX      2.0f

//------------------------------------------------------------------------------
// 4. AN TOÀN
//------------------------------------------------------------------------------
#define FALL_DEG         22.0f    // đổ quá góc này -> tắt motor
#define ARM_DEG           4.0f    // chỉ arm khi |θ-zp| nhỏ hơn
#define ARM_HOLD_MS     1000      // phải giữ thẳng liên tục bấy nhiêu ms
#define AUTO_REARM         0      // 1 = tự arm lại khi dựng xe lên (sau khi đổ)
#define SAT_FAULT_MS     800      // chạm V_MAX liên tục quá lâu -> lỗi
#define K_LIMIT        500.0f     // giới hạn hợp lệ của |K|
#define CMD_TIMEOUT_MS  400       // SPEED/TURN phải được gửi lại định kỳ; quá lâu không có lệnh -> về 0
#define IMU_FAIL_LIMIT    5       // số lần đọc IMU lỗi liên tiếp -> FAULT
#define STEP_WD_MS       30       // ISR tự ngắt xung nếu vòng điều khiển không cập nhật vận tốc quá lâu
#define STALL_FAULT_MS   60       // vòng điều khiển bị treo quá lâu khi RUN -> FAULT
#define JOG_V_MAX        0.15f    // lệnh JOG (thử chiều quay bánh khi nhấc xe): tối đa m/s
#define JOG_MS_MAX       2000     // và tối đa ms

//------------------------------------------------------------------------------
// 5. WIFI
//------------------------------------------------------------------------------
#define WIFI_ENABLE       1
#define WIFI_SSID_DEF     ""            // để trống = mặc định phát WiFi riêng; hoặc đặt bằng lệnh WIFI,ssid,pass
#define WIFI_PASS_DEF     ""
#define WIFI_CONNECT_MS   15000         // chờ kết nối WiFi nhà tối đa bấy nhiêu ms, quá thì chuyển sang AP
#define AP_SSID           "BalanceBot"
#define AP_PASS           "balance123"  // >= 8 ký tự (WPA2)
#define HOSTNAME          "balancebot"  // mDNS: balancebot.local
#define WIFI_TX_POWER     WIFI_POWER_15dBm  // 19.5 dBm làm dòng đỉnh lớn -> dễ sụt áp/brownout khi cấp qua LM2596 yếu; tăng lại nếu xa sóng
#define TCP_PORT          8080
#define TCP_DROP_AFTER_FAILS 250        // ~10 s (250 dòng telemetry liên tiếp không gửi được) -> bỏ client kẹt để không treo vòng điều khiển
#define STA_RECONNECT_MS  10000         // chế độ STA mất WiFi nhà quá bấy nhiêu ms thì tự gọi reconnect

//------------------------------------------------------------------------------
// 6. IMU MPU6050
//------------------------------------------------------------------------------
#define MPU_ADDR         0x68               // AD0 nối GND
#define GYRO_LSB_PER_DPS 65.5f              // ±500 dps
#define ACC_LSB_PER_G    8192.0f            // ±4 g

//------------------------------------------------------------------------------
// 7. HẰNG SỐ DẪN XUẤT (không cần sửa)
//------------------------------------------------------------------------------
#define STEP_ISR_HZ      40000UL
#define STEP_WD_TICKS    ((uint32_t)(STEP_WD_MS * STEP_ISR_HZ / 1000UL))
#define MAX_STEP_HZ      15000.0f           // phải < STEP_ISR_HZ/2
static_assert(MAX_STEP_HZ < (float)STEP_ISR_HZ / 2.0f, "MAX_STEP_HZ phai < STEP_ISR_HZ/2");
#define STEPS_PER_REV    (MOTOR_STEPS_REV * MICROSTEP)
#define STEPS_PER_M      ((float)STEPS_PER_REV / (2.0f * (float)M_PI * WHEEL_RADIUS_M))
#define CTRL_US          (1000000UL / CTRL_HZ)
#define DEG2RAD          0.01745329252f

static_assert((V_MAX * STEPS_PER_M) <= MAX_STEP_HZ, "V_MAX vuot tan so STEP toi da");
