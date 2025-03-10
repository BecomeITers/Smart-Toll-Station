#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  10
#define RST_PIN  9
#define BUZZER_PIN 6
#define SERVO_PIN  8
#define MAX_LENGTH  4

Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

const char* password = "0123";
char entered_pass[MAX_LENGTH + 1];
bool isLocked = true;
bool Reenter = true;
byte digits = 0;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte row_Pins[ROWS] = {7, 6, 5, 4};
byte col_Pins[COLS] = {3, 2, A5, A4};
Keypad keypad(makeKeymap(keys), row_Pins, col_Pins, ROWS, COLS);

// UID hợp lệ
const String validUID = "C348A2A"; // UID duy nhất được chấp nhận

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  servo.attach(SERVO_PIN);
  servo.write(50); // Khóa cửa ban đầu

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("DOOR LOCK SYSTEM");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Kiểm tra còi báo động khi khởi động
  tone(BUZZER_PIN, 1000); // Bật còi 1KHz
  delay(300);
  noTone(BUZZER_PIN); // Tắt còi
  delay(200);

  lcd.clear();
}


void loop() {
  if (Reenter) {
    lcd.setCursor(0, 0);
    lcd.print("ENTER PASSWORD");
    Reenter = false;
  }

  char key = keypad.getKey();
  if (key) {
    process_key(key);
  }

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getUID();
    Serial.println("Card UID: " + uid);
    if (uid == validUID) {
      unlockDoor();
    } else {
      lcd.clear();
      lcd.print("ACCESS DENIED!");
      Deniedbuzz(3);
      delay(2000);
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void process_key(char key) {
  if (key == 'C') {
    reset_Password();
  } else if (digits < MAX_LENGTH) {
    entered_pass[digits++] = key;
    lcd.setCursor(digits + 4, 1);
    lcd.print("*");

    if (digits == MAX_LENGTH) {
      check_Password();
    }
  }
}

void check_Password() {
  entered_pass[digits] = '\0';
  lcd.clear();
  if (strcmp(entered_pass, password) == 0) {
    unlockDoor();
  } else {
    Deniedbuzz(2);
    lcd.print("ERROR PASSWORD");
    delay(1000);
  }
  reset_Password();
}

void unlockDoor() {
  isLocked = false;
  servo.write(110);
  Accessbuzz(1);
  lcd.clear();
  lcd.print(" UNLOCKED! ");
  delay(3000);
  servo.write(50);
  isLocked = true;
  lcd.clear();
}

void reset_Password() {
  digits = 0;
  lcd.clear();
  memset(entered_pass, 0, sizeof(entered_pass));
  Reenter = true;
}

void Accessbuzz(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 1000); // Bật còi với tần số 1000Hz
    delay(500);
    noTone(BUZZER_PIN); // Tắt còi
    delay(100);
  }
}

void Deniedbuzz(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 1000); // Bật còi với tần số 1000Hz
    delay(100);
    noTone(BUZZER_PIN); // Tắt còi
    delay(100);
  }
}

String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}
