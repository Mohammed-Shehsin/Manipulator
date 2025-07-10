#include <Servo.h>

// Define servo objects
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

// Movement threshold and delay time
const int threshold = 4;            // Minimum analog difference to consider as a change
const int settleTime = 100;         // ms: how long a change must persist
const int stepDelay = 5;           // ms: delay between each servo degree step

// Previous smoothed analog values
int prevBaseRot = 0;
int prevBase = 0;
int prevArm = 0;
int prevForearm = 0;
int prevGripper = 0;

// Target angles to be reached
int targetBaseRot = 0;
int targetBase = 0;
int targetArm = 0;
int targetForearm = 0;
int targetGripper = 0;

void setup() {
  baseRotationServo.attach(10);
  baseServo.attach(3);
  armServo.attach(6);
  forearmServo.attach(9);
  gripperServo.attach(5);
  Serial.begin(9600);

  // Initialize with current positions
  prevBaseRot = smoothAnalog(potBaseRotation);
  prevBase = smoothAnalog(potBase);
  prevArm = smoothAnalog(potArm);
  prevForearm = smoothAnalog(potForearm);
  prevGripper = smoothAnalog(potGripper);

  targetBaseRot = map(prevBaseRot, 0, 1023, 0, 180);
  targetBase = map(prevBase, 0, 1023, 0, 180);
  targetArm = map(prevArm, 0, 1023, 0, 180);
  targetForearm = map(prevForearm, 0, 1023, 0, 180);
  targetGripper = map(prevGripper, 0, 1023, 0, 180);

  baseRotationServo.write(targetBaseRot);
  baseServo.write(targetBase);
  armServo.write(targetArm);
  forearmServo.write(targetForearm);
  gripperServo.write(targetGripper);

  delay(1000);
}

void loop() {
  updateServo(potBaseRotation, &prevBaseRot, &targetBaseRot, baseRotationServo, "BaseRot");
  updateServo(potBase, &prevBase, &targetBase, baseServo, "Base");
  updateServo(potArm, &prevArm, &targetArm, armServo, "Arm");
  updateServo(potForearm, &prevForearm, &targetForearm, forearmServo, "Forearm");
  updateServo(potGripper, &prevGripper, &targetGripper, gripperServo, "Gripper");

  delay(50); // main loop delay
}

int smoothAnalog(int pin) {
  long total = 0;
  const int samples = 10;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return total / samples;
}

void updateServo(int pin, int* prevVal, int* targetAngle, Servo &servo, const char* label) {
  int currentVal = smoothAnalog(pin);

  if (abs(currentVal - *prevVal) > threshold) {
    delay(settleTime);  // Let the change settle
    int stableCheck = smoothAnalog(pin);
    if (abs(stableCheck - currentVal) < threshold) {
      *prevVal = currentVal;
      int newTarget = map(currentVal, 0, 1023, 0, 180);

      // Smoothly move to new target
      for (int pos = *targetAngle; pos != newTarget; pos += (newTarget > *targetAngle ? 1 : -1)) {
        servo.write(pos);
        delay(stepDelay);
      }
      *targetAngle = newTarget;

      // Debug output
      Serial.print(label);
      Serial.print(": ");
      Serial.println(currentVal);
    }
  }
}
