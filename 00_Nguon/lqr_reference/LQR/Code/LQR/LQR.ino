#include <MPU6050.h>
#include <Wire.h>
#include "Kalman.h"  

// --- ĐỊNH NGHĨA NÚT BẤM ---
#define FORWARD   'F'
#define BACKWARD  'B'
#define LEFT      'L'
#define RIGHT     'R'
#define CIRCLE    'C'
#define CROSS     'X' 
#define TRIANGLE  'T' 
#define SQUARE    'S' 
#define START     'A' 
#define PAUSE     'P' 

Kalman kalmanX;

// --- Cấu hình chân ---
int leftpwm = 46;
int leftmotor1 = 50;
int leftmotor2 = 48;
int righpwm = 44;
int righmotor1 = 40;
int righmotor2 = 42;

// --- Biến Encoder ---
volatile long leftencoder;
volatile long righencoder;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100; 

int leftencoder_a = 3;
int leftencoder_b = 5;
int righencoder_a = 2;
int righencoder_b = 4;

// --- Biến IMU & LQR ---
float theta, psi, phi, en=1;
float thetadot, psidot, phidot;
float thetaold, psiold, phiold;
float AcX, AcY, AcZ;
float Gxro, Gxrate;
float pitch, offset, GHPWM;
uint32_t timer;
uint8_t i2cData[14];
unsigned long last_time;

// --- CẤU HÌNH ĐIỀU KHIỂN ---
// 1. TIẾN LÙI:
long speed_val = 200;

// 2. TRÁI PHẢI:
float turn_force = 50.0;
float manual_turn = 0; 

// 3. Bù ma sát
float min_PWM = 7;          
unsigned long lastLogTime = 0;
const unsigned long logInterval = 20;   // ms (50Hz)
int pwmL_cmd = 0;
int pwmR_cmd = 0;
// --- MA TRẬN TRỌNG SỐ LQR ---
float K[6] = {26, 1.3, 0.2, 0.25, 2, 0.5};
//float K[6] = {25, 1.3, 0.1, 0.11, 2, 0.5}; 
//float K[6] = {30, 1.5, 0.6, 0.8, 2, 0.5};
//float K[6] = {26, 1.3, 0.2, 0.2, 2, 0.5}; 
unsigned long lastCmdTime = 0;

// Khai báo nguyên mẫu hàm
void left_isr();
void righ_isr();
int i2cWrite(int reg, uint8_t data, bool stop);
int i2cRead(int reg, uint8_t* data, int len);

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); 
  
  // Chỉnh Offset: Chống trôi xe
  offset = 0.5; 
  //Xe trôi về sau $\rightarrow$ Cần tăng offset lên (Ví dụ: từ 0 lên 1.5 hoặc 2).
  //Xe trôi về trước $\rightarrow$ Cần giảm offset xuống (Ví dụ: từ 0 xuống -1.5 hoặc -2).
  GHPWM = 255; 

  pinMode(leftpwm, OUTPUT);
  pinMode(righpwm, OUTPUT);
  pinMode(leftmotor1, OUTPUT);
  pinMode(leftmotor2, OUTPUT);
  pinMode(righmotor1, OUTPUT);
  pinMode(righmotor2, OUTPUT);
  
  pinMode(leftencoder_a, INPUT_PULLUP);
  pinMode(leftencoder_b, INPUT_PULLUP);
  pinMode(righencoder_a, INPUT_PULLUP);
  pinMode(righencoder_b, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(leftencoder_a), left_isr, RISING);
  attachInterrupt(digitalPinToInterrupt(righencoder_a), righ_isr, RISING);
  
  Wire.begin();
  while (i2cWrite(0x6B, 0x01, true)); 
  while (i2cRead(0x75, i2cData, 1));
  if (i2cData[0] != 0x68) { while (1); }
  delay(100);
  while (i2cRead(0x3B, i2cData, 6));
  AcX = ((i2cData[0] << 8) | i2cData[1]);
  AcZ = ((i2cData[4] << 8) | i2cData[5]);
  pitch = (atan2(AcX, AcZ)) * RAD_TO_DEG;
  kalmanX.setAngle(pitch);
  timer = micros();
}

void loop() {
  READ_MPU();
  
  if (Serial1.available()) {
    char command = Serial1.read();
    lastCmdTime = millis();
    executeCommand(command);
  }

  // Auto Brake
  if (millis() - lastCmdTime > 150 && en == 1) {
    leftencoder = 0;
    righencoder = 0;
    manual_turn = 0;
  }
  // Bảo vệ ngã
  if ((abs(pitch) > 40) || en == 0) {
    stopMotors();
    manual_turn = 0;
    righencoder = 0; leftencoder = 0;
    righencoder_1 = 0; leftencoder_1 = 0;
    thetaold = 0; phiold = 0;
    en = 0;
  }
  if (en == 1) {
    if ((micros() - last_time) > 5000) {
      float dt = (float)(micros() - last_time) / 1000000.0;
      last_time = micros();

      theta = 0.55 * (leftencoder + righencoder);
      phi = 0.25 * (leftencoder - righencoder);    
      thetadot = (theta - thetaold) / dt;
      phidot = (phi - phiold) / dt;
      thetaold = theta;
      phiold = phi;

      // Tính toán LQR
      float x1[6] = {pitch, Gxrate, theta, thetadot, phi, phidot};
      float x2[6] = {pitch, Gxrate, theta, thetadot, phi, phidot};

      float u1 = 0;
      float u2 = 0;

      for (int i = 0; i < 6; i++) {
        u1 += K[i] * x1[i];
        u2 += K[i] * x2[i];
      }
      
      // TRỘN LÁI: LQR + Manual Turn
      float motorL = -u1 + manual_turn;
      float motorR = -u2 - manual_turn;

      motorcontrol(motorL, motorR);
    }
  }
     if(millis() - lastLogTime >= logInterval) {
        lastLogTime = millis();

        // format: 8 số, cách nhau bởi dấu phẩy, kết thúc newline
        Serial.print(theta, 6);      Serial.print(',');
        Serial.print(thetadot, 6);   Serial.print(',');
        Serial.print(pitch, 6);      Serial.print(',');  // psi
        Serial.print(Gxrate, 6);     Serial.print(',');  // psi_dot
        Serial.print(phi, 6);        Serial.print(',');
        Serial.print(phidot, 6);     Serial.print(',');
        Serial.print(pwmL_cmd);      Serial.print(',');
        Serial.println(pwmR_cmd);    // kết thúc 1 dòng
      }
}
// --- XỬ LÝ LỆNH ---
void executeCommand(char command) {
  switch (command) {
    // 1. TIẾN LÙI: Gán Encoder NGƯỢC DẤU
    case FORWARD: 
      leftencoder = -speed_val; 
      righencoder =  speed_val; 
      manual_turn = 0;
      break;
      
    case BACKWARD: 
      // Ngược lại của Forward
      leftencoder =  speed_val; 
      righencoder = -speed_val;  
      manual_turn = 0;
      break;
      
    // 2. TRÁI PHẢI
    case LEFT: 
      manual_turn = -turn_force;
      leftencoder = 0;          
      righencoder = 0;
      break;
      
    case RIGHT: 
      manual_turn = turn_force; 
      leftencoder = 0;
      righencoder = 0;
      break;
      
    case CIRCLE:
    case CROSS: 
      leftencoder = 0;
      righencoder = 0;
      manual_turn = 0;
      break;
    case START: en = 1; break;
    case PAUSE: en = 0; stopMotors(); break;
  }
}

void motorcontrol(float output1, float output2) {
  int pwm1 = abs(output1);
  int pwm2 = abs(output2);

  if (pwm1 > 0) pwm1 += min_PWM;
  if (pwm2 > 0) pwm2 += min_PWM;

  pwm1 = constrain(pwm1, 0, GHPWM);
  pwm2 = constrain(pwm2, 0, GHPWM);
   // LƯU PWM THỰC (để gửi Serial)
  pwmL_cmd = (output1 >= 0) ? pwm1 : -pwm1;   // có dấu
  pwmR_cmd = (output2 >= 0) ? pwm2 : -pwm2;

  if (output1 > 0) leftmotor(pwm1, 1); else leftmotor(pwm1, 0);
  if (output2 > 0) righmotor(pwm2, 1); else righmotor(pwm2, 0);
}

void stopMotors() {
  analogWrite(leftpwm, 0); analogWrite(righpwm, 0);
  digitalWrite(leftmotor1, LOW); digitalWrite(leftmotor2, LOW);
  digitalWrite(righmotor1, LOW); digitalWrite(righmotor2, LOW);
}

void leftmotor(uint8_t lpwm, int direct) {
  if (direct == 1) { digitalWrite(leftmotor1, HIGH); digitalWrite(leftmotor2, LOW); }
  else { digitalWrite(leftmotor1, LOW); digitalWrite(leftmotor2, HIGH); }
  analogWrite(leftpwm, lpwm);
}

void righmotor(uint8_t rpwm, int direct) {
  if (direct == 1) { digitalWrite(righmotor1, HIGH); digitalWrite(righmotor2, LOW); }
  else { digitalWrite(righmotor1, LOW); digitalWrite(righmotor2, HIGH); }
  analogWrite(righpwm, rpwm);
}

void left_isr() {
  if (digitalRead(leftencoder_b)){ leftencoder--; leftencoder_1--;}
  else{ leftencoder++; leftencoder_1++;}
}

void righ_isr() {
  if (digitalRead(righencoder_b)){ righencoder++; righencoder_1++;}
  else{ righencoder--; righencoder_1--;}
}

// --- CÁC HÀM I2C ---
int i2cRead(int reg, uint8_t* data, int len) {
    Wire.beginTransmission(0x68);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, len);
    for (int i = 0; i < len; i++) {
        if (Wire.available()) data[i] = Wire.read();
        else return 1; 
    }
    return 0; 
}

int i2cWrite(int reg, uint8_t data, bool stop) {
    Wire.beginTransmission(0x68);
    Wire.write(reg);
    Wire.write(data);
    return Wire.endTransmission(stop);
}

void READ_MPU() {
  while (i2cRead(0x3B, i2cData, 14));
  AcX = ((i2cData[0] << 8) | i2cData[1]);
  AcZ = ((i2cData[4] << 8) | i2cData[5]);
  Gxro = -((i2cData[10] << 8) | i2cData[11]);
  Gxrate = (double)Gxro / 131.0;
  float dt = (float)(micros() - timer) / 1000000.0;
  timer = micros();
  pitch = kalmanX.getAngle((atan2(AcX, AcZ)) * RAD_TO_DEG, Gxrate, dt);
  pitch -= (1 + offset);
}