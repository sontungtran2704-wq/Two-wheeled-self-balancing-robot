// Điểm vào của sketch Arduino. setup chuyển việc khởi tạo sang Runtime.cpp.
// loop chỉ nhường CPU qua loopRobot vì IMU, cân bằng và giao tiếp chạy trong các tác vụ riêng.
void setupRobot();
void loopRobot();
void setup() { setupRobot(); }
void loop() { loopRobot(); }
