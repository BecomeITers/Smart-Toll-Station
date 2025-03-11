#define RELAY_PIN 7  // Chân điều khiển relay

void setup() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);  // Ban đầu tắt relay (khóa điện đóng)
}

void loop() {
    digitalWrite(RELAY_PIN, HIGH);  // Kích hoạt relay (mở khóa)
    delay(100); 
    digitalWrite(RELAY_PIN, LOW);   // Tắt relay (đóng khóa)
    delay(100); 
}
