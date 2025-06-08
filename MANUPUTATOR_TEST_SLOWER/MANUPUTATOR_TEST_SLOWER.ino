#include <Servo.h>

// Servo objects
Servo baseRotationServo;
Servo baseServo;
Servo armServo;
Servo forearmServo;
Servo gripperServo;

// Potentiometer pins
const int potBaseRotation = A4;
const int potBase = A0;
const int potArm = A1;
const int potForearm = A2;
const int potGripper = A3;

// Raw analog home values
const int homeBaseRot = 536;
const int homeBase = 238;
const int homeArm = 348;
const int homeForearm = 56;
const int homeGripper = 1020;

// Current servo positions
int posBaseRot = 0;
int posBase = 0;
int posArm = 0;
int posForearm = 0;
int posGripper = 0;

void setup() {
  Serial.begin(9600);
  baseRotationServo.attach(10);
  baseServo.attach(3);
  armServo.attach(6);
  forearmServo.attach(9);
  gripperServo.attach(5);

  // Move to home position smoothly
  posBaseRot = map(homeBaseRot, 0, 1023, 0, 180);
  posBase = map(homeBase, 0, 1023, 0, 180);
  posArm = map(homeArm, 0, 1023, 0, 180);
  posForearm = map(homeForearm, 0, 1023, 0, 180);
  posGripper = map(homeGripper, 0, 1023, 0, 180);

  baseRotationServo.write(posBaseRot);
  baseServo.write(posBase);
  armServo.write(posArm);
  forearmServo.write(posForearm);
  gripperServo.write(posGripper);
}

void loop() {
  // Read analog target values
  int targetBaseRot = map(smoothAnalog(potBaseRotation), 0, 1023, 0, 180);
  int targetBase = map(smoothAnalog(potBase), 0, 1023, 0, 180);
  int targetArm = map(smoothAnalog(potArm), 0, 1023, 0, 180);
  int targetForearm = map(smoothAnalog(potForearm), 0, 1023, 0, 180);
  int targetGripper = map(smoothAnalog(potGripper), 0, 1023, 0, 180);

  // Smooth movement
  posBaseRot = smoothMove(baseRotationServo, posBaseRot, targetBaseRot);
  posBase = smoothMove(baseServo, posBase, targetBase);
  posArm = smoothMove(armServo, posArm, targetArm);
  posForearm = smoothMove(forearmServo, posForearm, targetForearm);
  posGripper = smoothMove(gripperServo, posGripper, targetGripper);

  // Serial debug
  Serial.print("BaseRot: "); Serial.print(targetBaseRot);
  Serial.print(" | Base: "); Serial.print(targetBase);
  Serial.print(" | Arm: "); Serial.print(targetArm);
  Serial.print(" | Forearm: "); Serial.print(targetForearm);
  Serial.print(" | Gripper: "); Serial.println(targetGripper);

  delay(20); // Adjust for smoothness/speed
}

// Smooth analog read
int smoothAnalog(int pin) {
  int total = 0;
  const int samples = 10;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return total / samples;
}

// Gradually move to target
int smoothMove(Servo& servo, int current, int target) {
  if (current < target) current++;
  else if (current > target) current--;
  servo.write(current);
  return current;
}
