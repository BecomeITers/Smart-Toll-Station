#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// Định nghĩa chân kết nối
#define SS_PIN  10
#define RST_PIN  9
#define BUZZER_PIN 6
#define RELAY_PIN  7

// Khai báo đối tượng
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// UID của thẻ hợp lệ
const String validUID = "C348A2A";

// Trạng thái cửa (true = đóng, false = mở)
bool isLocked = false; 

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH); // Lúc đầu mở khóa (Relay bật)

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SMART PARKING LOT");
  delay(5000);
  lcd.setCursor(0, 1);

  // Âm báo khởi động
  tone(BUZZER_PIN, 1000);
  delay(300);
  noTone(BUZZER_PIN);
  delay(200);
  lcd.clear();
  lcd.print("SCAN PARKING CARD");
}

void loop() {
  // Kiểm tra quét thẻ NFC
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getUID();
    Serial.println("Card UID: " + uid);
    if (uid == validUID) {
      lockDoor(); // Đóng cửa khi quét thẻ hợp lệ
    } else {
      lcd.clear();
      Deniedbuzz(3);
      lcd.print("WRONG PARKING CARD");
      delay(5000);
      lcd.clear();
      lcd.print("SCAN PARKING CARD");
      Deniedbuzz(1);
      delay(2000);
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

// Đóng cửa và tự động mở lại sau 10 giây
void lockDoor() {
  isLocked = true;
  digitalWrite(RELAY_PIN, LOW); // Đóng cửa (Relay tắt)
  
  lcd.clear();
  lcd.print("WELLCOME TO PARKING");
  Accessbuzz(1);

  // Chờ 10 giây rồi mở lại
  delay(10000);
  unlockDoor();
}

// Mở cửa
void unlockDoor() {
  isLocked = false;
  digitalWrite(RELAY_PIN, HIGH); // Mở cửa (Relay bật)
  
  lcd.clear();
  lcd.print("SCAN PARKING CARD");
  Deniedbuzz(3);
}

// Âm báo khi mở khóa thành công
void Accessbuzz(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 1000);
    delay(500);
    noTone(BUZZER_PIN);
    delay(100);
  }
}

// Âm báo khi nhập sai hoặc truy cập bị từ chối
void Deniedbuzz(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 1000);
    delay(100);
    noTone(BUZZER_PIN);
    delay(100);
  }
}

// Lấy UID của thẻ NFC
String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}
