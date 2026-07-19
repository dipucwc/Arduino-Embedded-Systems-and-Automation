#define buttonPin 2
#define ledPin 13
byte buttonState = 0;

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  if(digitalRead(buttonPin)){
buttonState = !buttonState;
digitalWrite(ledPin, buttonState);
     while(digitalRead(buttonPin));
  }
  delay(2000);
}
