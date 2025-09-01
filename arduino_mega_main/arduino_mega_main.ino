
// LIBRARY
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Adafruit_Fingerprint.h>
// #include <WiFi.h>
// #include <HttpClient.h>
// #include <b64.h>
// #include <HTTPClient.h>
#include <ArduinoJson.h>

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

// R308 FINGERPRINT MODULE di Serial1 (RX1 = 19, TX1 = 18)
// R308 uses same VCC, GND, and Data pins but optimized for optical sensor
Adafruit_Fingerprint finger(&Serial1);

// LCD I2C 16x2 alamat 0x27
LiquidCrystal_I2C lcd(0x27, 20, 21);

// VARIABEL
int sensorVal = 0; // sensor getar
bool fingerprintReady = false;

// R308 specific variables
unsigned long lastFingerprintCheck = 0;
const unsigned long fingerprintCheckInterval = 100; // R308 needs more frequent checks
bool fingerDetected = false;

// Add these variables at the top with other variables
bool lastGpsValid = false;
unsigned long lastVibrationPrint = 0;
const unsigned long vibrationPrintInterval = 2000; // 2 seconds between vibration prints

// Remove testing mode - we'll use direct Serial3 connection

void setup() {
  // Serial untuk debug
  Serial.begin(9600);
  Serial2.begin(9600);       // GPS
  Serial1.begin(57600);      // R308 Fingerprint Module
  Serial3.begin(115200); // Serial3 for ESP32-CAM communication (TX3=14, RX3=15)

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIBRA_PIN, INPUT);
  pinMode(ESP32_CAM_ENABLE_PIN, OUTPUT);

  // LCD
  Wire.begin();
  lcd.begin();
  lcd.backlight();
  
  // Display welcome message
  lcd.setCursor(0, 0);
  lcd.print("Assalamualaikum, ");
  lcd.setCursor(0, 1);
  lcd.print("User");
  delay(2000);
  
  // Check fingerprint status
  checkFingerprintStatus();
  
  // Configure R308 module if ready
  if (fingerprintReady) {
    configureR308();
  }
  
  // Initialize system
  // ESP32-CAM will handle WiFi and Telegram communication
  
  delay(2000);
  lcd.clear();
}

void loop() {
  sensorVibra();
  fingerprintScan();
  gpsNeo6();
  // handleTelegramCommands();
  // updateTelegramStatus();
  delay(500);
  // Add Serial command to trigger enrollment
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("enroll")) {
      int id = cmd.substring(6).toInt();
      if (id > 0 && id <= 127) {
        enrollFingerprint(id);
      } else {
        Serial.println("Invalid ID for enrollment. Use ID 1-127.");
        sendToESP32("R308_ENROLL_ERROR: Invalid ID, must be 1-127");
      }
    }
    // Add manual command testing
    else if (cmd == "test_fingerprint") {
      sendToESP32("R308_FINGERPRINT_READY");
      Serial.println("Sent: R308_FINGERPRINT_READY");
    }
    else if (cmd == "test_vibration") {
      sendToESP32("VIBRATION_ALERT");
      Serial.println("Sent: VIBRATION_ALERT");
    }
    else if (cmd == "test_access") {
      sendToESP32("R308_ACCESS_GRANTED:1");
      Serial.println("Sent: R308_ACCESS_GRANTED:1");
    }
    else if (cmd == "test_door") {
      sendToESP32("DOOR_UNLOCKED");
      Serial.println("Sent: DOOR_UNLOCKED");
    }
    else if (cmd == "r308_info") {
      getR308Info();
    }
    else if (cmd == "template_count") {
      getTemplateCount();
    }
    else if (cmd == "clear_all") {
      clearAllTemplates();
    }
    else if (cmd.startsWith("delete")) {
      int id = cmd.substring(6).toInt();
      if (id > 0 && id <= 127) {
        deleteFingerprint(id);
      } else {
        Serial.println("Invalid ID for deletion. Use ID 1-127.");
        sendToESP32("R308_DELETE_ERROR: Invalid ID, must be 1-127");
      }
    }
    else if (cmd == "help") {
      Serial.println("Available test commands:");
      Serial.println("  test_fingerprint - Send R308_FINGERPRINT_READY");
      Serial.println("  test_vibration - Send VIBRATION_ALERT");
      Serial.println("  test_access - Send R308_ACCESS_GRANTED:1");
      Serial.println("  test_door - Send DOOR_UNLOCKED");
      Serial.println("  enroll <id> - Enroll R308 fingerprint");
      Serial.println("  delete <id> - Delete R308 fingerprint");
      Serial.println("  r308_info - Get R308 module information");
      Serial.println("  template_count - Get number of stored templates");
      Serial.println("  clear_all - Clear all stored templates");
    }
  }
}

void checkFingerprintStatus() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cek R308 Module...");
  
  finger.begin(57600);
  
  // R308 specific: Add delay for module initialization
  delay(1000);
  
  // Then check if it's ready
  if (finger.verifyPassword()) {
    fingerprintReady = true;

    // First check if fingerprint module is detected/connected
    Serial.println("\n==============================");
    Serial.println("[R308 FINGERPRINT] Module detected");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("R308 Module");
    lcd.setCursor(0, 1);
    lcd.print("detected");
    delay(3000);

    Serial.println("[R308 FINGERPRINT] Status: READY");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("R308 siap");
    lcd.setCursor(0, 1);
    lcd.print("                ");  // Clear second line
    
    // Consolidated message for ESP32-CAM
    String statusMessage = "R308_STATUS: Module detected, Status: READY";
    sendToESP32(statusMessage);
    
    // smsBackup.sendFingerprintStatus(true);
  } else {
    fingerprintReady = false;

    // First check if fingerprint module is detected/connected
    Serial.println("\n==============================");
    Serial.println("[R308 FINGERPRINT] Module not detected");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("R308 Module");
    lcd.setCursor(0, 1);
    lcd.print("not detected");
    delay(3000);

    Serial.println("[R308 FINGERPRINT] Status: NOT READY");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("R308");
    lcd.setCursor(0, 1);
    lcd.print("belum siap");
    
    // Consolidated message for ESP32-CAM
    String statusMessage = "R308_STATUS: Module not detected, Status: NOT READY";
    sendToESP32(statusMessage);
    
    // smsBackup.sendFingerprintStatus(false);
  }
  Serial.println("==============================\n");
  delay(3000);
}

// void initializeWiFi() {
//   WiFi.begin(ssid, password);
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("Connecting WiFi...");
  
//   int attempts = 0;
//   while (WiFi.status() != WL_CONNECTED && attempts < 20) {
//     delay(500);
//     Serial.print(".");
//     attempts++;
//   }
  
//   if (WiFi.status() == WL_CONNECTED) {
//     lcd.setCursor(0, 1);
//     lcd.print("WiFi Connected");
//     Serial.println("WiFi Connected");
//   } else {
//     lcd.setCursor(0, 1);
//     lcd.print("WiFi Failed");
//     Serial.println("WiFi Connection Failed");
//   }
//   delay(1000);
// }

void sensorVibra() {
  sensorVal = analogRead(VIBRA_PIN);
  
  if (sensorVal < 500) {
    // Only print if enough time has passed since last vibration print
    if (millis() - lastVibrationPrint > vibrationPrintInterval) {
      Serial.println("\n---- VIBRATION SENSOR ----");
      Serial.print("Vibration sensor value: ");
      Serial.println(sensorVal);
      Serial.println("[VIBRATION] Detected!");
      Serial.println("-------------------------\n");
      lastVibrationPrint = millis();
    }
    
    tone(BUZZER_PIN, 2000);
    delay(1000);
    noTone(BUZZER_PIN);
    
    // Send vibration alert to ESP32-CAM
    sendToESP32("VIBRATION_ALERT");
  }
}

void fingerprintScan() {
  // R308 optimized: Check timing for better responsiveness
  if (millis() - lastFingerprintCheck < fingerprintCheckInterval) {
    return;
  }
  lastFingerprintCheck = millis();
  
  lcd.setCursor(0, 0);
  lcd.print("Scan Sidik Jari");
  lcd.setCursor(0, 1);
  lcd.print("     ...     ");

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    // R308 specific: Different delay for optical sensor
    if (p == FINGERPRINT_NOFINGER) {
      fingerDetected = false;
    }
    delay(50); // R308 needs slightly longer delay
    return;
  }
  
  // R308 specific: Set finger detected flag
  fingerDetected = true;
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    delay(50); // R308 needs slightly longer delay
    return;
  }
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("\n------------------------------");
    Serial.println("[R308 FINGERPRINT] Scan started");
    Serial.println("[R308 FINGERPRINT] Access: GRANTED");
    Serial.print("[R308 FINGERPRINT] ID: ");
    Serial.println(finger.fingerID);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Finger Detected");
    lcd.setCursor(0, 1);
    lcd.print("Access Granted");
    
    digitalWrite(RELAY_PIN, LOW);    // relay aktif (buka solenoid)
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Unlocked");
    lcd.setCursor(0, 1);
    lcd.print("Access Granted");
    delay(3000);  // Keep door open for 3 seconds
    digitalWrite(RELAY_PIN, HIGH);   // relay nonaktif (tutup solenoid)
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Locked");
    lcd.setCursor(0, 1);
    lcd.print("System Ready");
    
    // Consolidated message for ESP32-CAM
    String accessMessage = "R308_ACCESS_GRANTED: ID=" + String(finger.fingerID) + ", Door: Unlocked->Locked, Status: Success";
    sendToESP32(accessMessage);
    
    Serial.println("------------------------------\n");
  } else if (p == FINGERPRINT_NOTFOUND) {
    // Only print when finger is not found (not for every scan attempt)
    Serial.println("\n------------------------------");
    Serial.println("[R308 FINGERPRINT] Scan started");
    Serial.print("[R308 FINGERPRINT] Access: DENIED. Code: ");
    Serial.println(p);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied!");
    lcd.setCursor(0, 1);
    lcd.print("Wrong Fingerprint");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied!");
    lcd.setCursor(0, 1);
    lcd.print("Door Locked");
    delay(3000);
    lcd.clear();
    
    // Consolidated message for ESP32-CAM
    String accessMessage = "R308_ACCESS_DENIED: Code=" + String(p) + ", Door: Locked, Status: Failed";
    sendToESP32(accessMessage);
    
    Serial.println("------------------------------\n");
  }
}

void gpsNeo6() {
  while (Serial2.available()) {
    chr = Serial2.read();
    gps.encode(chr);
  }
  
  bool currentGpsValid = gps.location.isValid();
  
  // Only print GPS data when status changes or when valid data is available
  if (currentGpsValid != lastGpsValid || (currentGpsValid && millis() % 10000 < 100)) {
    if (currentGpsValid) {
      Serial.println("\n******** GPS DATA ********");
      Serial.print("Latitude : ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("Longitude: ");
      Serial.println(gps.location.lng(), 6);
      Serial.print("Altitude : ");
      Serial.print(gps.altitude.meters());
      Serial.println(" m");
      
      // Consolidated GPS message for ESP32-CAM
      String gpsMessage = "GPS_DATA: Lat=" + String(gps.location.lat(), 6) + 
                         ", Lng=" + String(gps.location.lng(), 6) + 
                         ", Alt=" + String(gps.altitude.meters()) + "m";
      sendToESP32(gpsMessage);
      
      Serial.println("**************************\n");
    } else {
      Serial.println("\n******** GPS DATA ********");
      Serial.println("GPS belum lock satelit..");
      
      // Consolidated GPS status message for ESP32-CAM
      String gpsMessage = "GPS_STATUS: No satellite signal, GPS not locked";
      sendToESP32(gpsMessage);
      
      Serial.println("**************************\n");
    }
    lastGpsValid = currentGpsValid;
  }
}

// void handleTelegramCommands() {
//   if (WiFi.status() != WL_CONNECTED) return;
  
//   HTTPClient http;
//   String url = telegramApiUrl + "/getUpdates?offset=-1&limit=1";
//   http.begin(url);
  
//   int httpCode = http.GET();
//   if (httpCode == HTTP_CODE_OK) {
//     String payload = http.getString();
//     DynamicJsonDocument doc(1024);
//     deserializeJson(doc, payload);
    
//     if (doc["ok"] == true && doc["result"].size() > 0) {
//       String message = doc["result"][0]["message"]["text"].as<String>();
//       String chatId = doc["result"][0]["message"]["chat"]["id"].as<String>();
      
//       if (message == "/camera" || message == "/camera@kotak_amal_nurul_ilmi_bot") {
//         toggleCamera();
//       } else if (message == "/gps" || message == "/gps@kotak_amal_nurul_ilmi_bot") {
//         getGPSLocation();
//       } else if (message == "/status" || message == "/status@kotak_amal_nurul_ilmi_bot") {
//         sendSystemStatus();
//       } else if (message == "/door" || message == "/door@kotak_amal_nurul_ilmi_bot") {
//         toggleDoorLock();
//       }
//     }
//   }
//   http.end();
// }

// void toggleCamera() {
//   if (!cameraActive) {
//     digitalWrite(ESP32_CAM_ENABLE_PIN, HIGH);
//     cameraActive = true;
//     sendTelegramMessage("📷 Kamera ESP32 aktif - Live streaming dimulai");
//     smsBackup.sendCameraStatus(true);
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Camera Active");
//     lcd.setCursor(0, 1);
//     lcd.print("Live Streaming");
//   } else {
//     digitalWrite(ESP32_CAM_ENABLE_PIN, LOW);
//     cameraActive = false;
//     sendTelegramMessage("📷 Kamera ESP32 dinonaktifkan");
//     smsBackup.sendCameraStatus(false);
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Camera Stopped");
//   }
// }

// void getGPSLocation() {
//   if (gps.location.isValid()) {
//     String locationMsg = "📍 Lokasi GPS:\n";
//     locationMsg += "Latitude: " + String(gps.location.lat(), 6) + "\n";
//     locationMsg += "Longitude: " + String(gps.location.lng(), 6) + "\n";
//     locationMsg += "Altitude: " + String(gps.altitude.meters()) + " m\n";
//     locationMsg += "Satellites: " + String(gps.satellites.value());
    
//     sendTelegramMessage(locationMsg);
//     smsBackup.sendGPSLocation(gps.location.lat(), gps.location.lng(), gps.altitude.meters());
    
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("GPS Location");
//     lcd.setCursor(0, 1);
//     lcd.print("Sent to Telegram");
//     delay(2000);
//     lcd.clear();
//   } else {
//     sendTelegramMessage("❌ GPS belum mendapatkan sinyal satelit");
//   }
// }

// void sendSystemStatus() {
//   String statusMsg = "📊 Status Sistem:\n";
//   statusMsg += "Fingerprint: " + String(fingerprintReady ? "Ready" : "Not Ready") + "\n";
//   statusMsg += "Camera: " + String(cameraActive ? "Active" : "Inactive") + "\n";
//   statusMsg += "GPS: " + String(gps.location.isValid() ? "Valid" : "No Signal") + "\n";
//   statusMsg += "WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\n";
//   statusMsg += "Door Lock: " + String(digitalRead(RELAY_PIN) == HIGH ? "Locked" : "Unlocked");
  
//   sendTelegramMessage(statusMsg);
// }

// void toggleDoorLock() {
//   if (digitalRead(RELAY_PIN) == HIGH) {
//     // Door is locked, unlock it
//     digitalWrite(RELAY_PIN, LOW);
//     sendTelegramMessage("🔓 Pintu kotak dibuka manual via Telegram");
//     smsBackup.sendDoorUnlocked();
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Door Unlocked");
//     lcd.setCursor(0, 1);
//     lcd.print("Manual Control");
    
//     delay(5000);  // Keep door open for 5 seconds
    
//     // Lock door again
//     digitalWrite(RELAY_PIN, HIGH);
//     sendTelegramMessage("🔒 Pintu kotak dikunci kembali");
//     smsBackup.sendDoorLocked();
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Door Locked");
//     lcd.setCursor(0, 1);
//     lcd.print("System Ready");
//   } else {
//     // Door is unlocked, lock it
//     digitalWrite(RELAY_PIN, HIGH);
//     sendTelegramMessage("🔒 Pintu kotak dikunci manual via Telegram");
//     smsBackup.sendDoorLocked();
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Door Locked");
//     lcd.setCursor(0, 1);
//     lcd.print("Manual Control");
//   }
// }

// void sendTelegramMessage(String message) {
//   if (WiFi.status() != WL_CONNECTED) return;
  
//   HTTPClient http;
//   String url = telegramApiUrl + "/sendMessage";
//   http.begin(url);
//   http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
//   String postData = "chat_id=" + String(chatId) + "&text=" + message;
//   int httpCode = http.POST(postData);
  
//   if (httpCode == HTTP_CODE_OK) {
//     Serial.println("Telegram message sent successfully");
//   } else {
//     Serial.println("Failed to send Telegram message");
//   }
  
//   http.end();
// }

// void updateTelegramStatus() {
//   unsigned long currentTime = millis();
//   if (currentTime - lastTelegramUpdate >= telegramUpdateInterval) {
//     lastTelegramUpdate = currentTime;
    
//     // Send periodic status updates if needed
//     if (cameraActive) {
//       sendTelegramMessage("📹 Live streaming masih aktif");
//     }
//   }
// }

// Add this function to enroll a new fingerprint for R308
void enrollFingerprint(uint8_t id) {
  Serial.println("\n==============================");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enroll R308 ID:");
  lcd.setCursor(0, 1);
  lcd.print(id);
  Serial.print("[R308 ENROLL] Finger ID: ");
  Serial.println(id);
  
  // Consolidated enrollment start message
  String enrollMessage = "R308_ENROLL_START: ID=" + String(id) + ", Status: Starting enrollment";
  sendToESP32(enrollMessage);
  
  delay(3000);
  int p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger...");
  Serial.println("[R308 ENROLL] Place finger...");
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) break;
    if (p == FINGERPRINT_NOFINGER) {
      lcd.setCursor(0, 1);
      lcd.print("Waiting...     ");
      Serial.println("[R308 ENROLL] Waiting for finger...");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      lcd.setCursor(0, 1);
      lcd.print("Comm error     ");
      Serial.println("[R308 ENROLL] Communication error");
    } else if (p == FINGERPRINT_IMAGEFAIL) {
      lcd.setCursor(0, 1);
      lcd.print("Imaging error  ");
      Serial.println("[R308 ENROLL] Imaging error");
    }
    delay(150); // R308 needs longer delay for optical sensor
  }
  
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Image->Tz1 fail");
    Serial.println("[R308 ENROLL] Image to template 1 failed");
    
    // Consolidated error message
    String errorMessage = "R308_ENROLL_ERROR: Step=Image2Tz1, Code=" + String(p) + ", Status: Failed";
    sendToESP32(errorMessage);
    
    delay(2000);
    Serial.println("==============================\n");
    return;
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Remove finger  ");
  Serial.println("[R308 ENROLL] Remove finger");
  delay(2000);
  
  while (finger.getImage() != FINGERPRINT_NOFINGER);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place same fing");
  lcd.setCursor(0, 1);
  lcd.print("again...       ");
  Serial.println("[R308 ENROLL] Place same finger again...");
  
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) break;
    delay(150); // R308 needs longer delay for optical sensor
  }
  
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Image->Tz2 fail");
    Serial.println("[R308 ENROLL] Image to template 2 failed");
    
    // Consolidated error message
    String errorMessage = "R308_ENROLL_ERROR: Step=Image2Tz2, Code=" + String(p) + ", Status: Failed";
    sendToESP32(errorMessage);
    
    delay(2000);
    Serial.println("==============================\n");
    return;
  }
  
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Model fail     ");
    Serial.println("[R308 ENROLL] Model creation failed");
    
    // Consolidated error message
    String errorMessage = "R308_ENROLL_ERROR: Step=CreateModel, Code=" + String(p) + ", Status: Failed";
    sendToESP32(errorMessage);
    
    delay(2000);
    Serial.println("==============================\n");
    return;
  }
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("R308 Enroll Success!");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(id);
    Serial.print("[R308 ENROLL] Success! ID: ");
    Serial.println(id);
    
    // Consolidated success message
    String successMessage = "R308_ENROLL_SUCCESS: ID=" + String(id) + ", Status: Fingerprint enrolled successfully";
    sendToESP32(successMessage);
    
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("R308 Enroll Failed! ");
    Serial.println("[R308 ENROLL] Failed!");
    
    // Consolidated failure message
    String failureMessage = "R308_ENROLL_FAILED: ID=" + String(id) + ", Code=" + String(p) + ", Status: Storage failed";
    sendToESP32(failureMessage);
  }
  delay(2000);
  Serial.println("==============================\n");
}

// R308 specific configuration function
void configureR308() {
  Serial.println("\n==============================");
  Serial.println("[R308] Configuring module...");
  
  // Get system parameters for R308 optical sensor
  uint8_t p = finger.getParameters();
  if (p == FINGERPRINT_OK) {
    Serial.println("[R308] Parameters retrieved successfully");
    Serial.print("[R308] Status register: 0x");
    Serial.println(finger.status_reg, HEX);
    Serial.print("[R308] System ID: 0x");
    Serial.println(finger.system_id, HEX);
    Serial.print("[R308] Capacity: ");
    Serial.println(finger.capacity);
    Serial.print("[R308] Security level: ");
    Serial.println(finger.security_level);
    Serial.print("[R308] Device address: 0x");
    Serial.println(finger.device_addr, HEX);
    Serial.print("[R308] Packet size: ");
    Serial.println(finger.packet_len);
    Serial.print("[R308] Baud rate: ");
    Serial.println(finger.baud_rate);
  } else {
    Serial.println("[R308] Failed to get parameters");
  }
  
  Serial.println("[R308] Configuration complete");
  Serial.println("==============================\n");
}

// Delete fingerprint function for R308
void deleteFingerprint(uint8_t id) {
  Serial.println("\n==============================");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Delete R308 ID:");
  lcd.setCursor(0, 1);
  lcd.print(id);
  Serial.print("[R308 DELETE] Finger ID: ");
  Serial.println(id);
  
  // Send deletion start message
  String deleteMessage = "R308_DELETE_START: ID=" + String(id) + ", Status: Starting deletion";
  sendToESP32(deleteMessage);
  
  delay(2000);
  
  uint8_t p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Delete Success!");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(id);
    Serial.print("[R308 DELETE] Success! ID: ");
    Serial.println(id);
    
    // Send success message
    String successMessage = "R308_DELETE_SUCCESS: ID=" + String(id) + ", Status: Fingerprint deleted successfully";
    sendToESP32(successMessage);
    
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Delete Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Error Code: ");
    lcd.print(p);
    Serial.print("[R308 DELETE] Failed! Error: ");
    Serial.println(p);
    
    // Send failure message
    String failureMessage = "R308_DELETE_FAILED: ID=" + String(id) + ", Code=" + String(p) + ", Status: Deletion failed";
    sendToESP32(failureMessage);
  }
  
  delay(2000);
  Serial.println("==============================\n");
}

// Get R308 module information
void getR308Info() {
  Serial.println("\n==============================");
  Serial.println("[R308] Getting module information...");
  
  uint8_t p = finger.getParameters();
  if (p == FINGERPRINT_OK) {
    Serial.println("[R308] Module information:");
    Serial.print("  Status register: 0x");
    Serial.println(finger.status_reg, HEX);
    Serial.print("  System ID: 0x");
    Serial.println(finger.system_id, HEX);
    Serial.print("  Capacity: ");
    Serial.println(finger.capacity);
    Serial.print("  Security level: ");
    Serial.println(finger.security_level);
    Serial.print("  Device address: 0x");
    Serial.println(finger.device_addr, HEX);
    Serial.print("  Packet size: ");
    Serial.println(finger.packet_len);
    Serial.print("  Baud rate: ");
    Serial.println(finger.baud_rate);
    
    // Get template count
    p = finger.getTemplateCount();
    if (p == FINGERPRINT_OK) {
      Serial.print("  Templates stored: ");
      Serial.println(finger.templateCount);
    }
    
    // Send info to ESP32-CAM
    String infoMessage = "R308_INFO: Capacity=" + String(finger.capacity) + 
                        ", Security=" + String(finger.security_level) + 
                        ", PacketSize=" + String(finger.packet_len) + 
                        ", BaudRate=" + String(finger.baud_rate) +
                        ", Templates=" + String(finger.templateCount);
    sendToESP32(infoMessage);
  } else {
    Serial.println("[R308] Failed to get module information");
    sendToESP32("R308_INFO_ERROR: Failed to retrieve module parameters");
  }
  
  Serial.println("==============================\n");
}

// Get template count function
void getTemplateCount() {
  Serial.println("\n==============================");
  Serial.println("[R308] Getting template count...");
  
  uint8_t p = finger.getTemplateCount();
  if (p == FINGERPRINT_OK) {
    Serial.print("[R308] Templates stored: ");
    Serial.println(finger.templateCount);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Templates: ");
    lcd.print(finger.templateCount);
    lcd.setCursor(0, 1);
    lcd.print("Capacity: ");
    lcd.print(finger.capacity);
    
    // Send to ESP32-CAM
    String countMessage = "R308_TEMPLATE_COUNT: " + String(finger.templateCount) + "/" + String(finger.capacity);
    sendToESP32(countMessage);
  } else {
    Serial.println("[R308] Failed to get template count");
    sendToESP32("R308_TEMPLATE_COUNT_ERROR: Failed to retrieve count");
  }
  
  delay(3000);
  Serial.println("==============================\n");
}

// Clear all templates function
void clearAllTemplates() {
  Serial.println("\n==============================");
  Serial.println("[R308] Clearing all templates...");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clear All");
  lcd.setCursor(0, 1);
  lcd.print("Templates...");
  
  // Send clear start message
  sendToESP32("R308_CLEAR_START: Clearing all templates");
  
  uint8_t p = finger.emptyDatabase();
  if (p == FINGERPRINT_OK) {
    Serial.println("[R308] All templates cleared successfully");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All Templates");
    lcd.setCursor(0, 1);
    lcd.print("Cleared!");
    
    // Send success message
    sendToESP32("R308_CLEAR_SUCCESS: All templates cleared successfully");
  } else {
    Serial.println("[R308] Failed to clear templates");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Clear Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Error Code: ");
    lcd.print(p);
    
    // Send failure message
    sendToESP32("R308_CLEAR_FAILED: Error code " + String(p));
  }
  
  delay(3000);
  Serial.println("==============================\n");
}

// Helper function to send commands to ESP32-CAM
void sendToESP32(String command) {
  // Send to USB Serial (for ESP32-CAM via bridge)
  Serial.print("[To ESP32-CAM]: ");
  Serial.println(command);
  
  // Also send to Serial3 (in case of direct connection)
  Serial3.println(command);
}
