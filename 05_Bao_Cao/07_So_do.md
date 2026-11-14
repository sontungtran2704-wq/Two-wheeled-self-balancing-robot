# Sơ đồ điều khiển và luồng firmware

```mermaid
flowchart LR
 U[Dashboard: v và yaw đặt] --> L[Lease và ramp]
 L --> P[PID vận tốc 50 Hz]
 L --> Y[PID yaw 50 Hz]
 P --> R[Gia tốc và pitch tham chiếu]
 R --> K[LQR pitch 200 Hz]
 I[MPU6050 và ước lượng] --> K
 I --> Y
 K --> M[Bộ trộn ưu tiên cân bằng]
 Y --> M
 M --> S[STEP DIR và watchdog]
 S --> B[Hai motor và cơ học xe]
 B --> I
 S --> O[Đếm xung: vận tốc ước lượng]
 O --> P
```

Đường đếm xung là phản hồi theo giả định bám bước, không phải encoder.

```mermaid
flowchart TD
 BOOT[Khởi động: driver vô hiệu] --> IDLE[IDLE]
 IDLE -->|START có chủ đích| ARM[ARMING]
 ARM -->|IMU hợp lệ và thẳng đứng liên tục| RUN[RUN LQR]
 RUN -->|Cân bằng liên tục và yêu cầu motion| PID[RUN LQR cộng PID]
 PID -->|Mất lệnh| ZERO[Ramp đặt về 0, vẫn cân bằng]
 PID -->|Tắt motion| DEC[Dừng êm rồi LQR riêng]
 DEC --> RUN
 RUN -->|Ngã, IMU cũ, trễ hạn| FAULT[FAULT chốt, tắt driver]
 PID -->|Ngã, IMU cũ, trễ hạn| FAULT
 FAULT -->|RESET rồi START mới| IDLE
 RUN -->|STOP hoặc ESTOP| IDLE
```

STOP là dừng driver; không đồng nghĩa lệnh vận tốc0. ESTOP/fault cụ thể phải
đọc state machine firmware; kết nối lại web không tự kích hoạt xe.
