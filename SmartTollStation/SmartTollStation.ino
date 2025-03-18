#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// Định nghĩa chân kết nối
#define SS_PIN  10
#define RST_PIN  9
#define BUZZER_PIN 6
#define RELAY_PIN  7
#define PIR_PIN  8  

// Khai báo đối tượng
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// UID của thẻ hợp lệ
const String validUID = "C348A2A";

// Trạng thái cửa
bool isLocked = false; 

// Biến chống nhiễu cảm biến PIR
unsigned long lastMotionTime = 0;
const unsigned long debounceDelay = 5000; // Giảm nhiễu 5 giây

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(RELAY_PIN, LOW); // Ban đầu mở khóa

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SMART TOLL STATION");
  delay(3000);
  lcd.clear();
  lcd.print("SCAN YOUR TOLL CARD");

  // Âm báo khởi động
  tone(BUZZER_PIN, 1000);
  delay(300);
  noTone(BUZZER_PIN);
}

void loop() {
  // Kiểm tra quét thẻ NFC
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getUID();
    Serial.println("Card UID: " + uid);

    if (uid == validUID) {
      lockDoor();
    } else {
      lcd.clear();
      Deniedbuzz(3);
      lcd.print("! WRONG TOLL CARD !");
      delay(1000);
      lcd.clear();
      lcd.print("SCAN YOUR TOLL CARD");
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void lockDoor() {
  isLocked = true;
  digitalWrite(RELAY_PIN, HIGH);

  lcd.clear();
  lcd.print("! PLEASE CONTINUE !");
  Accessbuzz(1);

  Serial.println("Chờ phát hiện chuyển động...");
  unsigned long startTime = millis();

  while (millis() - startTime < 1000) { // Chờ tối đa 1 giây
    if (detectMotion()) { // Nếu phát hiện chuyển động (chống nhiễu)
      Serial.println("Phát hiện chuyển động - Mở cửa!");
      unlockDoor();
      return;
    }
    delay(2000); // Giảm tải CPU, tránh đọc quá nhanh
  }
}

// Hàm kiểm tra chuyển động với chống nhiễu
bool detectMotion() {
  int motionCount = 0;
  while(true) {
    if (digitalRead(PIR_PIN) == HIGH) {
      motionCount++;
      break;
    }
  }
  return (motionCount > 0); 
}

// Mở cửa
void unlockDoor() {
  isLocked = false;
  digitalWrite(RELAY_PIN, LOW);
  
  lcd.clear();
  lcd.print("SCAN YOUR TOLL CARD");
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
