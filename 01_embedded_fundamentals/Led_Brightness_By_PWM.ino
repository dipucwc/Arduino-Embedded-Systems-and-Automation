#define ledPin 6
void setup() {
pinMode(ledPin, OUTPUT);
}

void loop() {
     for (int i = 0; i < 256; ++i) {
analogWrite(ledPin, i);
delay(10);
}
for (int i = 255; i >= 0; --i) {
analogWrite(ledPin, i);
delay(50);
}
} 
