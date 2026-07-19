void showOne(){
  for (byte i = 0; i < 7; ++i) {
 digitalWrite(ledPins[i], one[i]);
}
}
void showTwo(){
  for (byte i = 0; i < 7; ++i) {
 digitalWrite(ledPins[i], ((two >> (6-i)) & 0b1));
}
}
void showDigit(byte value){
  for (byte i = 0; i < 7; ++i) {
 digitalWrite(ledPins[i], 
((value >> (6-i)) & 0b1));
}
}
boolean buttonState = 0;
byte ledPins[] = {2, 3, 4, 5, 6, 7, 8};
byte one[] = {0, 1, 1, 0, 0, 0, 0};
byte two = 109;

void setup() {
  for (byte i = 0; i < 7; ++i) {
pinMode(ledPins[i], OUTPUT);
  }
  Serial.begin(9600);
}

void loop() {
  showOne();
  delay(1000);
  showTwo();
  delay(1000);
}
