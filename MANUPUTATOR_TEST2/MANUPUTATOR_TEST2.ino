#include <Servo.h>

// Define servo objects
Servo baseRotationServo;   // New: Base rotation
Servo baseServo;           // Shoulder
Servo armServo;            // Elbow
Servo forearmServo;        // Wrist
Servo gripperServo;        // Gripper

// Define potentiometer pins
const int potBaseRotation = A4;
const int potBase = A0;
const int potArm = A1;
const int potForearm = A2;
const int potGripper = A3;

void setup() {
  // Attach servos
  baseRotationServo.attach(10);
  baseServo.attach(3);
  armServo.attach(6);
  forearmServo.attach(9);
  gripperServo.attach(5);

  // Start Serial Monitor
  Serial.begin(9600);
}

void loop() {
  // Read smoothed analog values
  int valBaseRotation = smoothAnalog(potBaseRotation);
  int valBase = smoothAnalog(potBase);
  int valArm = smoothAnalog(potArm);
  int valForearm = smoothAnalog(potForearm);
  int valGripper = smoothAnalog(potGripper);

  // Map values to servo angles
  baseRotationServo.write(map(valBaseRotation, 0, 1023, 0, 180));
  baseServo.write(map(valBase, 0, 1023, 0, 180));
  armServo.write(map(valArm, 0, 1023, 0, 180));
  forearmServo.write(map(valForearm, 0, 1023, 0, 180));
  gripperServo.write(map(valGripper, 0, 1023, 0, 180));

  // Print to Serial Monitor
  Serial.print("BaseRot: "); Serial.print(valBaseRotation);
  Serial.print(" | Base: "); Serial.print(valBase);
  Serial.print(" | Arm: "); Serial.print(valArm);
  Serial.print(" | Forearm: "); Serial.print(valForearm);
  Serial.print(" | Gripper: "); Serial.println(valGripper);

  delay(100); // Optional: reduce if more responsiveness is needed
}

// Smoothing function to reduce signal noise
int smoothAnalog(int pin) {
  int total = 0;
  const int samples = 10;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return total / samples;
}
