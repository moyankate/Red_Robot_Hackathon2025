// Replace 12345 with the correct team number and then uncomment the line below.
#define TEAM_NUMBER 15

#ifndef TEAM_NUMBER
#error "Define your team number with `#define TEAM_NUMBER 12345` at the top of the file."
#elif TEAM_NUMBER < 1 || 40 < TEAM_NUMBER
#error "Team number must be within 1 and 40"
#endif

// ======================= EDIT: include for fabsf =======================
#include <math.h>  // EDIT: needed for fabs/fabsf
#include <Servo.h>

void setup()
{
  Serial.begin(115200);
}

int temp = 0;

// ======================= ADDED: automation helpers & state =======================
enum Mode { MODE_MANUAL = 0, MODE_LINE = 1, MODE_AVOID = 2 };
Mode mode = MODE_MANUAL;
bool prevLB = false; // for edge detection on LB

inline float sat(float v){ return v < -1.f ? -1.f : (v > 1.f ? 1.f : v); }
inline float deadband(float v, float t=0.08f){ return (fabsf(v) < t) ? 0.f : v; }

void driveArcade(float throttle, float steer) {
  RR_setMotor1(sat(throttle + steer));
  RR_setMotor2(sat(throttle - steer));
}

void setMotors(float leftMotorSpeed, float rightMotorSpeed) {
  
  if (leftMotorSpeed > 1.0f) leftMotorSpeed = 1.0f;
  if (leftMotorSpeed < -1.0f) leftMotorSpeed = -1.0f;
  if (rightMotorSpeed > 1.0f) rightMotorSpeed = 1.0f;
  if (rightMotorSpeed < -1.0f) rightMotorSpeed = -1.0f;


  RR_setMotor1(leftMotorSpeed);
  RR_setMotor2(rightMotorSpeed);
}

void waveServoNonBlocking() {
    // 静态变量保存状态
    static int angle = 90;          // 从 90 度开始
    static int step = 5;
    static bool increasing = true;
    static unsigned long lastUpdate = 0;
    const unsigned long interval = 20; // ms, 控制挥手速度

    // 检查是否到达更新时间
    unsigned long now = millis();
    if (now - lastUpdate < interval) return; // 未到更新时间就返回
    lastUpdate = now;

    // 设置舵机角度
    RR_setServo1(angle);

    // 改变角度
    if (increasing) {
        angle += step;
        if (angle >= 140) {       // 上限改为 180
            angle = 140;
            increasing = false;
        }
    } else {
        angle -= step;
        if (angle <= 90) {        // 下限改为 90
            angle = 90;
            increasing = true;
        }
    }
}

// ======================= EDIT: line detection threshold =======================
void autonLineFollowPID() {
    int sensors[6];
    RR_getLineSensors(sensors);  // Read 6 QTR line sensor values

    int threshold = 2000; // Black/white threshold

    float weightedSum = 0.0f;
    float sum = 0.0f;

    for (int i = 0; i < 6; i++) {
        // Adjust this depending on whether black gives large or small value
        float value = (sensors[i] > threshold) ? sensors[i] : 0.0f; // only black contributes
        weightedSum += value * i;
        sum += value;
    }

    // Position of line center (0~5)
    float position = (sum > 0.0f) ? (weightedSum / sum) : 2.55f;
    float error = position - 2.55f;  // deviation from center

    float Kp = 0.06f;      // proportional gain
    float baseSpeed = 0.2f; // base speed

    float leftSpeed  = baseSpeed + Kp * error;
    float rightSpeed = baseSpeed - Kp * error;

    // Clamp
    leftSpeed  = constrain(leftSpeed, -1.0f, 1.0f);
    rightSpeed = constrain(rightSpeed, -1.0f, 1.0f);

    setMotors(leftSpeed, rightSpeed);

    Serial.print("pos="); Serial.print(position);
    Serial.print(" err="); Serial.print(error);
    Serial.print(" L="); Serial.print(leftSpeed);
    Serial.print(" R="); Serial.println(rightSpeed);
}

 

// ===================== end ADDED: automation helpers & state =====================

void loop()
{
  // Serial.println("helloworld");
  // Read the four joystick axes
  // These will be in the range [-1.0, 1.0]
  float rightX = RR_axisRX();
  float rightY = RR_axisRY();
  float leftX = RR_axisLX();
  float leftY = RR_axisLY();

  // ======================= EDIT: remove unconditional motor writes =======================
  // (These two lines used to command the motors every loop, fighting automation.)
  // RR_setMotor1(leftY + rightX);   // EDIT: removed; handled inside MODE_MANUAL
  // RR_setMotor2(leftY - rightX);   // EDIT: removed; handled inside MODE_MANUAL

  // Get the button states
  bool btnA = RR_buttonA();
  bool btnB = RR_buttonB();
  bool btnX = RR_buttonX();
  bool btnY = RR_buttonY();
  bool btnRB = RR_buttonRB();
  bool btnLB = RR_buttonLB();

  // ======================= ADDED: mode switching =======================
  // Tap LB to cycle modes: Manual → Line → Avoid → Manual
  bool lbEdge = (btnLB && !prevLB);
  prevLB = btnLB;
  if (lbEdge) {
    mode = (Mode)((int(mode) + 1) % 3);
  }
  // Hold RB for immediate manual override
  if (btnRB) mode = MODE_MANUAL;
  // ===================== end ADDED: mode switching =====================

  // ======================= ADDED: behavior selection =======================
  if (mode == MODE_LINE) {
      // 循线控制
      autonLineFollowPID();

      // 舵机挥手
      waveServoNonBlocking();
  }
  else if (mode == MODE_MANUAL) {
      // 手动驾驶
      driveArcade(leftY, rightX);

      // 手动模式下舵机1用左右控制
      if (RR_dpad() == 6) {        // left
          if (temp1 > 0) temp1 -= 10;
      } else if (RR_dpad() == 2) { // right
          if (temp1 < 180) temp1 += 10;
      }
      RR_setServo1(temp1);

      // 手动模式下舵机2用上下控制
      if (RR_dpad() == 0) {        // up
          if (temp2 > 0) temp2 -= 10;
      } else if (RR_dpad() == 4) { // down
          if (temp2 < 180) temp2 += 10;
      }
      RR_setServo2(temp2);
  }
  else { // MODE_AVOID
      //autonAvoid();
}

  // ===================== end ADDED: behavior selection =====================

  

  // read the ultrasonic sensors
  Serial.print("Ultrasonic=");
  Serial.print(RR_getUltrasonic());
  Serial.print(" ;; ");
  int sensors[6];

  Serial.print("Line sensors=");
  RR_getLineSensors(sensors);
  for (int i = 0; i < 6; ++i)
  {
    Serial.print(sensors[i]);
    Serial.print(" ");
  }
  Serial.print(btnA ? 1 : 0);
  Serial.print(btnB ? 1 : 0);
  Serial.print(btnX ? 1 : 0);
  Serial.print(btnY ? 1 : 0);

  // ======================= ADDED: mode telemetry =======================
  Serial.print("  Mode=");
  if (mode == MODE_MANUAL) Serial.print("MANUAL");
  else if (mode == MODE_LINE) Serial.print("LINE");
  else Serial.print("AVOID");
  // ===================== end ADDED: mode telemetry =====================

  Serial.println();

  // This is important - it sleeps for 0.02 seconds (= 50 times / second)
  delay(20);
}

// vim: tabstop=2 shiftwidth=2 expandtab