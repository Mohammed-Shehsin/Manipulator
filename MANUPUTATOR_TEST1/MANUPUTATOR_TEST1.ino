#include <Servo.h>

Servo baseServo;
Servo armServo;
Servo forearmServo;
Servo gripperServo;

const int potBase = A0;
const int potArm = A1;
const int potForearm = A2;
const int potGripper = A3;  // NEW: Gripper controlled by potentiometer

void setup() {
  baseServo.attach(3);      // Base joint
  armServo.attach(6);       // Arm joint
  forearmServo.attach(9);   // Forearm joint
  gripperServo.attach(5);   // Gripper

  // No need for switch pin anymore
}

void loop() {
  int baseVal = analogRead(potBase);
  int armVal = analogRead(potArm);
  int forearmVal = analogRead(potForearm);
  int gripperVal = analogRead(potGripper);  // Read 4th pot

  // Map analog readings to servo angles
  baseServo.write(map(baseVal, 0, 1023, 0, 180));
  armServo.write(map(armVal, 0, 1023, 0, 180));
  forearmServo.write(map(forearmVal, 0, 1023, 0, 180));
  gripperServo.write(map(gripperVal, 0, 1023, 0, 90));  // Limit gripper to 0-90°
}
