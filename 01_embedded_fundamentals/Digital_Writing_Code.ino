void setup() {
  //Sets digital pin 8 as output
  DDRB = 0b0000001;
}

void loop() {
  //Sets pin 8 on and of
  PORTB = 0b00000001;
  delay(2000);
  PORTB = 0b00000000;
  delay(2000);
