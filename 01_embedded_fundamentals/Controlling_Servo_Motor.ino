#include <Servo.h>
#define servoPin 3
Servo mServo;

void setup() {
mServo.attach(servoPin);
}

void loop() {
mServo.write(180);
delay(500);
mServo.write(40);
delay(600);
}
