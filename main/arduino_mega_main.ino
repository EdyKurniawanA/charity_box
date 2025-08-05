// LIBRARY
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "sms_backup.h"

// PIN DEFINISI
#define BUZZER_PIN 5
#define RELAY_PIN 4
#define VIBRA_PIN A0
#define ESP32_CAM_ENABLE_PIN 3  // Pin to enable/disable ESP32 Cam

// GPS → Serial2 (TX GPS ke RX 17, RX GPS ke TX 16)
#define GPS_RX 17
#define GPS_TX 16
TinyGPSPlus gps;
char chr;

// SIM800L di pin 9 dan 10 (for backup SMS if Telegram fails)
SoftwareSerial simSerial4(9, 10);  // RX, TX

// FINGERPRINT di Serial1 (RX1 = 19, TX1 = 18)
Adafruit_Fingerprint finger(&Serial1);

// LCD I2C 16x2 alamat 0x27
LiquidCrystal_I2C lcd(0x27, 20, 21);

// TELEGRAM BOT CONFIGURATION
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* botToken = "7950984672:AAGn7jHn4fqM12_8pwgR6wqFZzz_GNpvyYo";
const char* chatId = "5638142909";
String telegramApiUrl = "https://api.telegram.org/bot" + String(botToken);

// VARIABEL
int sensorVal = 0; // sensor getar
bool fingerprintReady = false;
bool cameraActive = false;
bool gpsActive = false;
unsigned long lastTelegramUpdate = 0;
const unsigned long telegramUpdateInterval = 5000; // 5 seconds

// SMS Backup
SMSBackup smsBackup(&simSerial4, SMS_NUMBER);

void setup() {
  // Serial untuk debug
  Serial.begin(9600);
  Serial2.begin(9600);       // GPS
  Serial1.begin(57600);      // Fingerprint
  simSerial4.begin(9600);    // SIM800L

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIBRA_PIN, INPUT);
  pinMode(ESP32_CAM_ENABLE_PIN, OUTPUT);

  // LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  
  // Display welcome message
  lcd.setCursor(0, 0);
  lcd.print("Assalamualaikum");
  lcd.setCursor(0, 1);
  lcd.print("User");
  delay(2000);
  
  // Check fingerprint status
  checkFingerprintStatus();
  
  // Initialize WiFi and Telegram
  initializeWiFi();
  sendTelegramMessage("🕌 Kotak Amal Nurul Ilmi telah aktif!");
  smsBackup.sendWelcomeMessage();
  
  delay(2000);
  lcd.clear();
}

void loop() {
  sensorVibra();
  fingerprintScan();
  gpsNeo6();
  handleTelegramCommands();
  updateTelegramStatus();
  delay(500);
}

void checkFingerprintStatus() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cek Fingerprint...");
  
  finger.begin(57600);
  if (finger.verifyPassword()) {
    fingerprintReady = true;
    Serial.println("Fingerprint siap");
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint Ready");
    sendTelegramMessage("✅ Fingerprint siap digunakan");
    smsBackup.sendFingerprintStatus(true);
  } else {
    fingerprintReady = false;
    Serial.println("Fingerprint error");
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint Not Ready");
    sendTelegramMessage("❌ Fingerprint tidak siap");
    smsBackup.sendFingerprintStatus(false);
  }
  delay(2000);
}

void initializeWiFi() {
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 1);
    lcd.print("WiFi Connected");
    Serial.println("WiFi Connected");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("WiFi Failed");
    Serial.println("WiFi Connection Failed");
  }
  delay(1000);
}

void sensorVibra() {
  sensorVal = analogRead(VIBRA_PIN);
  if (sensorVal > 500) {
    tone(BUZZER_PIN, 2000);
    delay(500);
    noTone(BUZZER_PIN);
    
    // Send vibration alert to Telegram
    sendTelegramMessage("⚠️ Terdeteksi getaran pada kotak!");
    smsBackup.sendVibrationAlert();
  }
}

void fingerprintScan() {
  lcd.setCursor(0, 0);
  lcd.print("Scan Sidik Jari");
  lcd.setCursor(0, 1);
  lcd.print("Menunggu...     ");

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    delay(10);
    return;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    delay(10);
    return;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Akses diberikan");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Finger Detected");
    lcd.setCursor(0, 1);
    lcd.print("Access Granted");
    
    sendTelegramMessage("✅ Akses diberikan - ID: " + String(finger.fingerID));
    smsBackup.sendAccessResult(true, finger.fingerID);

    // Unlock door (activate solenoid)
    digitalWrite(RELAY_PIN, LOW);    // relay aktif (buka solenoid)
    sendTelegramMessage("🔓 Pintu kotak dibuka - Akses diberikan");
    smsBackup.sendDoorUnlocked();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Unlocked");
    lcd.setCursor(0, 1);
    lcd.print("Access Granted");
    
    delay(3000);  // Keep door open for 3 seconds
    
    // Lock door (deactivate solenoid)
    digitalWrite(RELAY_PIN, HIGH);   // relay nonaktif (tutup solenoid)
    sendTelegramMessage("🔒 Pintu kotak dikunci kembali");
    smsBackup.sendDoorLocked();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Locked");
    lcd.setCursor(0, 1);
    lcd.print("System Ready");
  } else {
    Serial.print("Akses ditolak. Kode: ");
    Serial.println(p);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied!");
    lcd.setCursor(0, 1);
    lcd.print("Wrong Fingerprint");
    
    sendTelegramMessage("❌ Akses ditolak - Sidik jari tidak dikenal");
    smsBackup.sendAccessResult(false);
    
    // Door remains locked (solenoid stays inactive)
    sendTelegramMessage("🔒 Pintu kotak tetap terkunci - Akses ditolak");
    smsBackup.sendDoorAccessDenied();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied!");
    lcd.setCursor(0, 1);
    lcd.print("Door Locked");
    
    delay(2000);
    lcd.clear();
  }
}

void gpsNeo6() {
  while (Serial2.available()) {
    chr = Serial2.read();
    gps.encode(chr);
  }

  if (gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Lng: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Alt: ");
    Serial.print(gps.altitude.meters());
    Serial.println(" m");
  } else {
    Serial.println("GPS belum lock satelit..");
  }
}

void handleTelegramCommands() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = telegramApiUrl + "/getUpdates?offset=-1&limit=1";
  http.begin(url);
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    if (doc["ok"] == true && doc["result"].size() > 0) {
      String message = doc["result"][0]["message"]["text"].as<String>();
      String chatId = doc["result"][0]["message"]["chat"]["id"].as<String>();
      
      if (message == "/camera" || message == "/camera@kotak_amal_nurul_ilmi_bot") {
        toggleCamera();
      } else if (message == "/gps" || message == "/gps@kotak_amal_nurul_ilmi_bot") {
        getGPSLocation();
      } else if (message == "/status" || message == "/status@kotak_amal_nurul_ilmi_bot") {
        sendSystemStatus();
      } else if (message == "/door" || message == "/door@kotak_amal_nurul_ilmi_bot") {
        toggleDoorLock();
      }
    }
  }
  http.end();
}

void toggleCamera() {
  if (!cameraActive) {
    digitalWrite(ESP32_CAM_ENABLE_PIN, HIGH);
    cameraActive = true;
    sendTelegramMessage("📷 Kamera ESP32 aktif - Live streaming dimulai");
    smsBackup.sendCameraStatus(true);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Camera Active");
    lcd.setCursor(0, 1);
    lcd.print("Live Streaming");
  } else {
    digitalWrite(ESP32_CAM_ENABLE_PIN, LOW);
    cameraActive = false;
    sendTelegramMessage("📷 Kamera ESP32 dinonaktifkan");
    smsBackup.sendCameraStatus(false);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Camera Stopped");
  }
}

void getGPSLocation() {
  if (gps.location.isValid()) {
    String locationMsg = "📍 Lokasi GPS:\n";
    locationMsg += "Latitude: " + String(gps.location.lat(), 6) + "\n";
    locationMsg += "Longitude: " + String(gps.location.lng(), 6) + "\n";
    locationMsg += "Altitude: " + String(gps.altitude.meters()) + " m\n";
    locationMsg += "Satellites: " + String(gps.satellites.value());
    
    sendTelegramMessage(locationMsg);
    smsBackup.sendGPSLocation(gps.location.lat(), gps.location.lng(), gps.altitude.meters());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("GPS Location");
    lcd.setCursor(0, 1);
    lcd.print("Sent to Telegram");
    delay(2000);
    lcd.clear();
  } else {
    sendTelegramMessage("❌ GPS belum mendapatkan sinyal satelit");
  }
}

void sendSystemStatus() {
  String statusMsg = "📊 Status Sistem:\n";
  statusMsg += "Fingerprint: " + String(fingerprintReady ? "Ready" : "Not Ready") + "\n";
  statusMsg += "Camera: " + String(cameraActive ? "Active" : "Inactive") + "\n";
  statusMsg += "GPS: " + String(gps.location.isValid() ? "Valid" : "No Signal") + "\n";
  statusMsg += "WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\n";
  statusMsg += "Door Lock: " + String(digitalRead(RELAY_PIN) == HIGH ? "Locked" : "Unlocked");
  
  sendTelegramMessage(statusMsg);
}

void toggleDoorLock() {
  if (digitalRead(RELAY_PIN) == HIGH) {
    // Door is locked, unlock it
    digitalWrite(RELAY_PIN, LOW);
    sendTelegramMessage("🔓 Pintu kotak dibuka manual via Telegram");
    smsBackup.sendDoorUnlocked();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Unlocked");
    lcd.setCursor(0, 1);
    lcd.print("Manual Control");
    
    delay(5000);  // Keep door open for 5 seconds
    
    // Lock door again
    digitalWrite(RELAY_PIN, HIGH);
    sendTelegramMessage("🔒 Pintu kotak dikunci kembali");
    smsBackup.sendDoorLocked();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Locked");
    lcd.setCursor(0, 1);
    lcd.print("System Ready");
  } else {
    // Door is unlocked, lock it
    digitalWrite(RELAY_PIN, HIGH);
    sendTelegramMessage("🔒 Pintu kotak dikunci manual via Telegram");
    smsBackup.sendDoorLocked();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Locked");
    lcd.setCursor(0, 1);
    lcd.print("Manual Control");
  }
}

void sendTelegramMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = telegramApiUrl + "/sendMessage";
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "chat_id=" + String(chatId) + "&text=" + message;
  int httpCode = http.POST(postData);
  
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Telegram message sent successfully");
  } else {
    Serial.println("Failed to send Telegram message");
  }
  
  http.end();
}

void updateTelegramStatus() {
  unsigned long currentTime = millis();
  if (currentTime - lastTelegramUpdate >= telegramUpdateInterval) {
    lastTelegramUpdate = currentTime;
    
    // Send periodic status updates if needed
    if (cameraActive) {
      sendTelegramMessage("📹 Live streaming masih aktif");
    }
  }
}
