#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
#define BUZZER_PIN 6
#define RELAY_PIN 4
#define PIR_PIN 5

#define I2C_SDA 3
#define I2C_SCL 1
#define SPI_SCK 12
#define SPI_MISO 13
#define SPI_MOSI 11

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

// Cấu trúc lưu trữ thông tin người dùng
struct UserInfo {
  String uid;        // UID của thẻ RFID
  String ten;        // Tên người dùng
  String sdt;        // Số điện thoại
  String biensoxe;   // Biển số xe
};

// Danh sách người dùng đã được setup sẵn (UID, tên, số điện thoại, biển số xe)
UserInfo users[] = {
  {"C3480A2A", "NguyenVanA", "0901234567", "51A-12345"},
  {"A02", "NguyenVanB", "0987654321", "51B-54321"}
};

// Thông tin WiFi
const char* ssid = "DaoThaiLan";         // Thay bằng tên WiFi của bạn
const char* password = "nhatnam05";        // Thay bằng mật khẩu WiFi của bạn

// URL cơ sở của Google Apps Script
const String baseURL = "https://script.google.com/macros/s/AKfycbx4VfFk93i5z1xq-rkd-2sQozUwktgOBkMxMXUrfF36raWRG62NrU7GqTP7k7hmRt0NGA/exec";

// Hàm tìm thông tin người dùng dựa vào UID
UserInfo* findUserByUID(String uid) {
  int len = sizeof(users) / sizeof(users[0]);
  for (int i = 0; i < len; i++) {
    if (users[i].uid.equalsIgnoreCase(uid)) {
      return &users[i];
    }
  }
  return NULL;
}

// Hàm mã hóa URL để đảm bảo dữ liệu được gửi đúng định dạng
String URLEncode(String str) {
  String encoded = "";
  char temp[3];
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      sprintf(temp, "%%%02X", c);
      encoded += temp;
    }
  }
  return encoded;
}

// Hàm xây dựng URL động dựa trên thông tin người dùng
String buildDynamicURL(const UserInfo& user) {
  String url = baseURL;
  url += "?sts=reg";
  url += "&uid=" + URLEncode(user.uid);
  url += "&ten=" + URLEncode(user.ten);
  url += "&sdt=" + URLEncode(user.sdt);
  url += "&biensoxe=" + URLEncode(user.biensoxe);
  return url;
}

void setup() {
  Serial.begin(9600);

  // Khởi tạo SPI với chân custom
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SS_PIN);
  mfrc522.PCD_Init();

  // Khởi tạo I2C LCD với chân custom
  Wire.begin(I2C_SDA, I2C_SCL, 100000); 

  // Kết nối Wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi: ");
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SMART TOLL STATION");
  delay(3000);
  lcd.clear();
  lcd.print("SCAN YOUR TOLL CARD");

  // Khởi tạo chân
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Báo hiệu khởi động xong
  tone(BUZZER_PIN, 1000);
  delay(300);
  noTone(BUZZER_PIN);
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // Xử lý chuỗi UID
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();

  Serial.print("RFID Card UID: ");
  Serial.println(uidString);

  // Tìm người dùng
  UserInfo* userInfo = findUserByUID(uidString);

  if (userInfo != NULL) {
    // Kiểm tra nếu là thẻ hợp lệ theo logic cũ
    if (checkValidCard()) {
      lockDoor(); // Gọi xử lý đóng cửa và đợi chuyển động
    }

    // Gửi request nếu có kết nối WiFi
    String requestURL = buildDynamicURL(*userInfo);
    Serial.print("Requesting URL: ");
    Serial.println(requestURL);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(requestURL);
      int httpCode = http.GET();

      if (httpCode > 0) {
        String response = http.getString();
        Serial.print("Response: ");
        Serial.println(response);
      } else {
        Serial.print("HTTP Request failed: ");
        Serial.println(http.errorToString(httpCode));
      }

      http.end();
    } else {
      Serial.println("WiFi not connected");
    }

    delay(2000);
  } else {
  Serial.println("Unknown UID. Sending to server...");

  // Gửi UID chưa biết lên Google Sheets
  String unknownURL = baseURL + "?sts=new&uid=" + URLEncode(uidString);
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(unknownURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("Response (unregistered): " + response);
    } else {
      Serial.println("Failed to send unregistered UID.");
    }

    http.end();
  }

  // Hiển thị cảnh báo
  lcd.clear();
  lcd.print("! UNKNOWN CARD !");
  Deniedbuzz(3);
  delay(1000);
  lcd.clear();
  lcd.print("SCAN YOUR TOLL CARD");
  delay(2000);
}


  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}


bool checkValidCard() {
  bool isName1 = true;
  bool isName2 = true; 
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != Name1[i]) isName1 = false;
    if (mfrc522.uid.uidByte[i] != Name2[i]) isName2 = false;
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
  while (millis() - startTime < 3000) { 
    if (detectMotion()) {
      Serial.println("Phát hiện chuyển động - Đóng cửa!");
      unlockDoor();
      return;
    }
    delay(2000); 
  }
}

bool detectMotion() {
  return (digitalRead(PIR_PIN) == HIGH);
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
