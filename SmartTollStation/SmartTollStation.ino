#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// Định nghĩa chân kết nối
#define SS_PIN 10
#define RST_PIN 9
#define BUZZER_PIN 6
#define RELAY_PIN 7
#define PIR_PIN 8  

// Khai báo đối tượng
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Danh sách thẻ hợp lệ
byte Name1[4] = {0xC3, 0x48, 0x0A, 0x2A};
byte Name2[4] = {0x01, 0x06, 0x78, 0x7B};

// Thông tin người dùng
String Name;
long Number;
bool isLocked = false;

// Biến chống nhiễu cảm biến PIR
unsigned long lastMotionTime = 0;
const unsigned long debounceDelay = 5000;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SMART TOLL STATION");
  delay(3000);
  lcd.clear();
  lcd.print("SCAN YOUR TOLL CARD");

  tone(BUZZER_PIN, 1000);
  delay(300);
  noTone(BUZZER_PIN);
  Serial.println("CLEARSHEET");
  Serial.println("LABEL,Date,Time,Name,Number");
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (checkValidCard()) {
      Serial.print("DATA,DATE,TIME," + Name);
      Serial.print(",");
      Serial.println(Number);
      Serial.println("SAVEWORKBOOKAS,Names/WorkNames");
      lockDoor();
    } else {
      lcd.clear();
      lcd.print("! WRONG TOLL CARD !");
      Deniedbuzz(3);
      delay(1000);
      lcd.clear();
      lcd.print("SCAN YOUR TOLL CARD");
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

bool checkValidCard() {
  bool isName1 = true;
  bool isName2 = false;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != Name1[i]) isName1 = false;
    if (mfrc522.uid.uidByte[i] != Name2[i]) isName2 = true;
  }
  if (isName1) {
    Name = "Vu Duc Thang";
    Number = 23162095;
    return true;
  } else if (isName2) {
    Name = "Nguyen Nhat Nam";
    Number = 23162058;
    return false;
  }
  return false;
}

void lockDoor() {
  isLocked = true;
  digitalWrite(RELAY_PIN, HIGH);
  lcd.clear();
  lcd.print("! PLEASE CONTINUE !");
  Accessbuzz(1);

  unsigned long startTime = millis();
  while (millis() - startTime < 5000) { // Chờ tối đa 5 giây
      if (detectMotion()) { // Nếu phát hiện chuyển động (chống nhiễu)
        Serial.println("Phát hiện chuyển động - Mở cửa!");
        unlockDoor();
        return;
      }
      delay(2000); // Giảm tải CPU, tránh đọc quá nhanh
    }
}

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

void unlockDoor() {
  isLocked = false;
  digitalWrite(RELAY_PIN, LOW);
  lcd.clear();
  lcd.print("SCAN YOUR TOLL CARD");
  Deniedbuzz(3);
}

void Accessbuzz(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 1000);
    delay(500);
    noTone(BUZZER_PIN);
    delay(100);
  }
}

void Deniedbuzz(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 1000);
    delay(100);
    noTone(BUZZER_PIN);
    delay(100);
  }
}
