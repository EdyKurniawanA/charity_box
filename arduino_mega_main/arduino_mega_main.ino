// LIBRARY
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
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

// GPS Configuration (matching gpd_neo_8m.ino)
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

// FINGERPRINT di Serial1 (RX1 = 19, TX1 = 18)
Adafruit_Fingerprint finger(&Serial1);

// LCD I2C 16x2 alamat 0x27
LiquidCrystal_I2C lcd(0x27, 20, 21);

// VARIABEL
int sensorVal = 0; // sensor getar
bool fingerprintReady = false;

// Add these variables at the top with other variables
unsigned long lastVibrationPrint = 0;
const unsigned long vibrationPrintInterval = 2000; // 2 seconds between vibration prints

// GPS request handling
bool gpsRequested = false;
unsigned long gpsRequestTime = 0;

// Remove testing mode - we'll use direct Serial3 connection

void setup() {
  // Serial untuk debug
  Serial.begin(115200);
  Serial2.begin(GPSBaud);    // GPS (matching gpd_neo_8m.ino)
  Serial1.begin(57600);      // Fingerprint
  Serial3.begin(115200); // Serial3 for ESP32-CAM communication (TX3=14, RX3=15)

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIBRA_PIN, INPUT);
  pinMode(ESP32_CAM_ENABLE_PIN, OUTPUT);

  // LCD
  Wire.begin();
  lcd.begin();
  // lcd.init();
  lcd.backlight();
  
  // Display welcome message
  lcd.setCursor(0, 0);
  lcd.print("Assalamualaikum, ");
  lcd.setCursor(0, 1);
  lcd.print("User");
  delay(2000);
  
  // Check fingerprint status
  checkFingerprintStatus();
  
  // GPS Diagnostic information
  Serial.println(F("=== GPS Neo 8M Diagnostic Tool ==="));
  Serial.println(F("Hardware Serial2 connection (TX2-Pin16, RX2-Pin17)"));
  Serial.println();
  
  Serial.println(F("Expected wiring:"));
  Serial.println(F("GPS VCC (Red) -> Arduino 5V"));
  Serial.println(F("GPS GND (Black) -> Arduino GND"));
  Serial.println(F("GPS RX -> Arduino TX2 (Pin 16)"));
  Serial.println(F("GPS TX -> Arduino RX2 (Pin 17)"));
  Serial.println();
  
  Serial.println(F("Starting GPS diagnostics..."));
  Serial.println(F("Listening for raw GPS data..."));
  Serial.println();
  
  // Initialize system
  // ESP32-CAM will handle WiFi and Telegram communication
  
  delay(2000);
  lcd.clear();
}

void loop() {
  // Process GPS data continuously (highest priority) - like in working diagnostic file
  gpsNeo6();
  
  // Process other functions only occasionally to avoid GPS interruption
  static unsigned long lastSensorCheck = 0;
  static unsigned long lastFingerprintCheck = 0;
  
  // Check vibration sensor every 100ms
  if (millis() - lastSensorCheck > 100) {
    sensorVibra();
    lastSensorCheck = millis();
  }
  
  // Check fingerprint every 200ms
  if (millis() - lastFingerprintCheck > 200) {
    fingerprintScan();
    lastFingerprintCheck = millis();
  }
  
  // handleTelegramCommands();
  // updateTelegramStatus();
  
  // Check for GPS requests from ESP32-CAM
  if (Serial3.available()) {
    String command = Serial3.readStringUntil('\n');
    command.trim();
    if (command == "REQUEST_GPS") {
      gpsRequested = true;
      gpsRequestTime = millis();
      Serial.println("[GPS] Request received from ESP32-CAM");
    }
  }
  
  // Check for GPS request timeout (30 seconds)
  if (gpsRequested && millis() - gpsRequestTime > 30000) {
    String gpsMessage = "GPS_STATUS: Request timeout - no GPS data available";
    sendToESP32(gpsMessage);
    Serial.println(F("[GPS] Request timeout - no GPS data available"));
    gpsRequested = false;
  }
  
  // No delay - let GPS process continuously
  // Add Serial command to trigger enrollment
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("enroll")) {
      int id = cmd.substring(6).toInt();
      if (id > 0) {
        enrollFingerprint(id);
      } else {
        Serial.println("Invalid ID for enrollment.");
      }
    }
    // Add manual command testing
    else if (cmd == "test_fingerprint") {
      sendToESP32("FINGERPRINT_READY");
      Serial.println("Sent: FINGERPRINT_READY");
    }
    else if (cmd == "test_vibration") {
      sendToESP32("VIBRATION_ALERT");
      Serial.println("Sent: VIBRATION_ALERT");
    }
    else if (cmd == "test_access") {
      sendToESP32("ACCESS_GRANTED:1");
      Serial.println("Sent: ACCESS_GRANTED:1");
    }
    else if (cmd == "test_door") {
      sendToESP32("DOOR_UNLOCKED");
      Serial.println("Sent: DOOR_UNLOCKED");
    }
    else if (cmd == "gps_test") {
      Serial.println("=== GPS ISOLATION TEST ===");
      Serial.println("Disabling other functions for 30 seconds...");
      Serial.println("GPS should work like in diagnostic file now");
      unsigned long testStart = millis();
      while (millis() - testStart < 30000) {
        gpsNeo6();
      }
      Serial.println("=== GPS TEST COMPLETE ===");
    }
    else if (cmd == "request_gps") {
      Serial.println("=== MANUAL GPS REQUEST ===");
      gpsRequested = true;
      gpsRequestTime = millis();
      Serial.println("GPS request sent - waiting for valid data...");
    }
    else if (cmd == "help") {
      Serial.println("Available test commands:");
      Serial.println("  test_fingerprint - Send FINGERPRINT_READY");
      Serial.println("  test_vibration - Send VIBRATION_ALERT");
      Serial.println("  test_access - Send ACCESS_GRANTED:1");
      Serial.println("  test_door - Send DOOR_UNLOCKED");
      Serial.println("  gps_test - Test GPS in isolation for 30 seconds");
      Serial.println("  request_gps - Manually request GPS data");
      Serial.println("  enroll <id> - Enroll fingerprint");
    }
  }
}

void checkFingerprintStatus() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cek Fingerprint...");
  
  finger.begin(57600);
  
  // Then check if it's ready
  if (finger.verifyPassword()) {
    fingerprintReady = true;

    // First check if fingerprint module is detected/connected
    Serial.println("\n==============================");
    Serial.println("[FINGERPRINT] Module detected");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("module detected");
    delay(3000);

    Serial.println("[FINGERPRINT] Status: READY");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint siap");
    lcd.setCursor(0, 1);
    lcd.print("                ");  // Clear second line
    
    // Consolidated message for ESP32-CAM
    String statusMessage = "FINGERPRINT_STATUS: Module detected, Status: READY";
    sendToESP32(statusMessage);
    
    // smsBackup.sendFingerprintStatus(true);
  } else {
    fingerprintReady = false;

    // First check if fingerprint module is detected/connected
    Serial.println("\n==============================");
    Serial.println("[FINGERPRINT] Module not detected");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("module not detected");
    delay(3000);

    Serial.println("[FINGERPRINT] Status: NOT READY");
    
    lcd.clear();  // Clear before new message
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("belum siap");
    
    // Consolidated message for ESP32-CAM
    String statusMessage = "FINGERPRINT_STATUS: Module not detected, Status: NOT READY";
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
    
    // tone(BUZZER_PIN, 2000);
    digitalWrite(BUZZER_PIN, 4000);
    // Reduced delay to prevent GPS data loss
    delay(100);
    noTone(BUZZER_PIN);
    
    // Send vibration alert to ESP32-CAM
    sendToESP32("VIBRATION_ALERT");
  }
}

void fingerprintScan() {
  lcd.setCursor(0, 0);
  lcd.print("Scan Sidik Jari");
  lcd.setCursor(0, 1);
  lcd.print("     ...     ");

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    // Reduced delay to prevent GPS data loss
    delay(1);
    return;
  }
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    // Reduced delay to prevent GPS data loss
    delay(1);
    return;
  }
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("\n------------------------------");
    Serial.println("[FINGERPRINT] Scan started");
    Serial.println("[FINGERPRINT] Access: GRANTED");
    Serial.print("[FINGERPRINT] ID: ");
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
    String accessMessage = "ACCESS_GRANTED: ID=" + String(finger.fingerID) + ", Door: Unlocked->Locked, Status: Success";
    sendToESP32(accessMessage);
    
    Serial.println("------------------------------\n");
  } else if (p == FINGERPRINT_NOTFOUND) {
    // Only print when finger is not found (not for every scan attempt)
    Serial.println("\n------------------------------");
    Serial.println("[FINGERPRINT] Scan started");
    Serial.print("[FINGERPRINT] Access: DENIED. Code: ");
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
    String accessMessage = "ACCESS_DENIED: Code=" + String(p) + ", Door: Locked, Status: Failed";
    sendToESP32(accessMessage);
    
    Serial.println("------------------------------\n");
  }
}

void gpsNeo6() {
  static unsigned long lastCheck = 0;
  static unsigned long rawDataCount = 0;
  static unsigned long validSentences = 0;
  
  // Read and process GPS data continuously (exactly like working diagnostic file)
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    rawDataCount++;
    
    // Display raw characters (first 100 chars only to avoid spam)
    if (rawDataCount <= 100) {
      Serial.print(c);
    }
    
    // Try to encode the character - this is the critical part
    if (gps.encode(c)) {
      validSentences++;
      displayGpsInfo();
    }
  }
  
  // Display diagnostics every 10 seconds (less frequent to avoid GPS interruption)
  if (millis() - lastCheck > 10000) {
    lastCheck = millis();
    
    Serial.println(F("\n=== GPS DIAGNOSTIC INFO ==="));
    Serial.print(F("Raw characters received: "));
    Serial.println(rawDataCount);
    Serial.print(F("Characters processed by GPS library: "));
    Serial.println(gps.charsProcessed());
    Serial.print(F("Valid sentences decoded: "));
    Serial.println(validSentences);
    Serial.print(F("Failed checksum count: "));
    Serial.println(gps.failedChecksum());
    Serial.print(F("Passed checksum count: "));
    Serial.println(gps.passedChecksum());
    
    if (rawDataCount == 0) {
      Serial.println(F("\n*** NO DATA RECEIVED ***"));
      Serial.println(F("Possible issues:"));
      Serial.println(F("1. Check power connections (VCC to 5V, GND to GND)"));
      Serial.println(F("2. Verify TX/RX connections are not swapped"));
      Serial.println(F("3. GPS module may be faulty"));
      Serial.println(F("4. Try different baud rate (38400 or 115200)"));
      
      // Send diagnostic status to ESP32-CAM
      String diagnosticMessage = "GPS_DIAGNOSTIC: No data received, check connections";
      sendToESP32(diagnosticMessage);
      
    } else if (gps.charsProcessed() == 0) {
      Serial.println(F("\n*** RECEIVING DATA BUT NOT GPS FORMAT ***"));
      Serial.println(F("Data might be at wrong baud rate or corrupted"));
      Serial.println(F("Try changing baud rate to 38400 or 115200"));
      
      // Send diagnostic status to ESP32-CAM
      String diagnosticMessage = "GPS_DIAGNOSTIC: Data received but not GPS format, check baud rate";
      sendToESP32(diagnosticMessage);
      
    } else if (validSentences == 0) {
      Serial.println(F("\n*** RECEIVING GPS DATA BUT NO VALID SENTENCES ***"));
      Serial.println(F("GPS is working but may need time to get satellite fix"));
      Serial.println(F("Move to an open area with clear sky view"));
      
      // Send diagnostic status to ESP32-CAM
      String diagnosticMessage = "GPS_DIAGNOSTIC: GPS data received but no valid sentences, waiting for satellite fix";
      sendToESP32(diagnosticMessage);
      
    } else {
      Serial.println(F("\n*** GPS IS WORKING CORRECTLY ***"));
      
      // Send diagnostic status to ESP32-CAM
      String diagnosticMessage = "GPS_DIAGNOSTIC: GPS working correctly, " + String(validSentences) + " valid sentences";
      sendToESP32(diagnosticMessage);
    }
    
    Serial.println(F("=======================\n"));
  }
  
  // Extended timeout for initial detection
  if (millis() > 30000 && rawDataCount == 0) {
    Serial.println(F("\n*** 30 SECONDS - NO DATA RECEIVED ***"));
    Serial.println(F("Please check:"));
    Serial.println(F("1. Power connections"));
    Serial.println(F("2. TX/RX wiring"));
    Serial.println(F("3. Try different baud rates"));
    
    // Send critical diagnostic status to ESP32-CAM
    String criticalMessage = "GPS_CRITICAL: 30 seconds no data, check hardware connections";
    sendToESP32(criticalMessage);
    
    // Try different baud rates
    Serial.println(F("\nTrying different baud rates..."));
    
    uint32_t baudRates[] = {4800, 9600, 38400, 115200};
    for (int i = 0; i < 4; i++) {
      Serial.print(F("Testing baud rate: "));
      Serial.println(baudRates[i]);
      Serial2.end();
      delay(100);
      Serial2.begin(baudRates[i]);
      delay(2000);
      
      int testCount = 0;
      while (Serial2.available() > 0 && testCount < 50) {
        Serial.print((char)Serial2.read());
        testCount++;
      }
      
      if (testCount > 0) {
        Serial.print(F("\n*** FOUND DATA AT BAUD RATE: "));
        Serial.print(baudRates[i]);
        Serial.println(F(" ***"));
        Serial.println(F("Update your code with this baud rate!"));
        
        // Send baud rate discovery to ESP32-CAM
        String baudMessage = "GPS_BAUD_DISCOVERY: Found data at baud rate " + String(baudRates[i]);
        sendToESP32(baudMessage);
        
        // Reset to original baud rate for now
        Serial2.end();
        delay(100);
        Serial2.begin(GPSBaud);
        break;
      }
    }
    
    if (rawDataCount == 0) {
      Serial.println(F("\nNo data found at any baud rate."));
      Serial.println(F("Check hardware connections."));
      
      // Send final diagnostic status to ESP32-CAM
      String finalMessage = "GPS_FINAL_DIAGNOSTIC: No data at any baud rate, check hardware";
      sendToESP32(finalMessage);
    }
  }
}

void displayGpsInfo() {
  Serial.println(F("\n=== GPS LOCATION DATA ==="));
  
  // Latitude
  Serial.print(F("Latitude: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.println(F(" degrees"));
  } else {
    Serial.println(F("INVALID"));
  }
  
  // Longitude
  Serial.print(F("Longitude: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lng(), 6);
    Serial.println(F(" degrees"));
  } else {
    Serial.println(F("INVALID"));
  }
  
  // Altitude
  Serial.print(F("Altitude: "));
  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters(), 2);
    Serial.println(F(" meters"));
  } else {
    Serial.println(F("INVALID"));
  }
  
  // Additional useful information
  Serial.print(F("Satellites: "));
  if (gps.satellites.isValid()) {
    Serial.println(gps.satellites.value());
  } else {
    Serial.println(F("INVALID"));
  }
  
  // Date and Time
  Serial.print(F("Date/Time: "));
  if (gps.date.isValid() && gps.time.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
    Serial.print(F(" "));
    
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.println(F(" UTC"));
  } else {
    Serial.println(F("INVALID"));
  }
  
  // CSV format output for easy data logging
  Serial.print(F("CSV: "));
  if (gps.location.isValid() && gps.altitude.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    Serial.print(F(","));
    Serial.print(gps.altitude.meters(), 2);
  } else {
    Serial.print(F("INVALID,INVALID,INVALID"));
  }
  Serial.println();
  
  // Send GPS data to ESP32-CAM only if requested
  if (gpsRequested) {
    if (gps.location.isValid()) {
      String gpsMessage = "GPS_DATA: Latitude=" + String(gps.location.lat(), 6) + 
                         ", Longitude=" + String(gps.location.lng(), 6) + 
                         ", Altitude=" + String(gps.altitude.meters()) + "m" +
                         ", Satellite=" + String(gps.satellites.value());
      sendToESP32(gpsMessage);
      Serial.println(F("[GPS] Data sent to ESP32-CAM"));
    } else {
      String gpsMessage = "GPS_STATUS: No satellite signal, Satellites=" + String(gps.satellites.value());
      sendToESP32(gpsMessage);
      Serial.println(F("[GPS] Status sent to ESP32-CAM"));
    }
    gpsRequested = false; // Reset request flag
  }
  
  Serial.println(F("========================"));
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

// Add this function to enroll a new fingerprint
void enrollFingerprint(uint8_t id) {
  Serial.println("\n==============================");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enroll Finger ID:");
  lcd.setCursor(0, 1);
  lcd.print(id);
  Serial.print("[ENROLL] Finger ID: ");
  Serial.println(id);
  
  // Consolidated enrollment start message
  String enrollMessage = "ENROLL_START: ID=" + String(id) + ", Status: Starting enrollment";
  sendToESP32(enrollMessage);
  
  delay(3000);
  int p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger...");
  Serial.println("[ENROLL] Place finger...");
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) break;
    if (p == FINGERPRINT_NOFINGER) {
      lcd.setCursor(0, 1);
      lcd.print("Waiting...     ");
      Serial.println("[ENROLL] Waiting for finger...");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      lcd.setCursor(0, 1);
      lcd.print("Comm error     ");
      Serial.println("[ENROLL] Communication error");
    } else if (p == FINGERPRINT_IMAGEFAIL) {
      lcd.setCursor(0, 1);
      lcd.print("Imaging error  ");
      Serial.println("[ENROLL] Imaging error");
    }
    delay(100);
  }
  
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Image->Tz1 fail");
    Serial.println("[ENROLL] Image to template 1 failed");
    
    // Consolidated error message
    String errorMessage = "ENROLL_ERROR: Step=Image2Tz1, Code=" + String(p) + ", Status: Failed";
    sendToESP32(errorMessage);
    
    delay(2000);
    Serial.println("==============================\n");
    return;
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Remove finger  ");
  Serial.println("[ENROLL] Remove finger");
  delay(2000);
  
  while (finger.getImage() != FINGERPRINT_NOFINGER);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place same fing");
  lcd.setCursor(0, 1);
  lcd.print("again...       ");
  Serial.println("[ENROLL] Place same finger again...");
  
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) break;
    delay(100);
  }
  
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Image->Tz2 fail");
    Serial.println("[ENROLL] Image to template 2 failed");
    
    // Consolidated error message
    String errorMessage = "ENROLL_ERROR: Step=Image2Tz2, Code=" + String(p) + ", Status: Failed";
    sendToESP32(errorMessage);
    
    delay(2000);
    Serial.println("==============================\n");
    return;
  }
  
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Model fail     ");
    Serial.println("[ENROLL] Model creation failed");
    
    // Consolidated error message
    String errorMessage = "ENROLL_ERROR: Step=CreateModel, Code=" + String(p) + ", Status: Failed";
    sendToESP32(errorMessage);
    
    delay(2000);
    Serial.println("==============================\n");
    return;
  }
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enroll Success!");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(id);
    Serial.print("[ENROLL] Success! ID: ");
    Serial.println(id);
    
    // Consolidated success message
    String successMessage = "ENROLL_SUCCESS: ID=" + String(id) + ", Status: Fingerprint enrolled successfully";
    sendToESP32(successMessage);
    
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enroll Failed! ");
    Serial.println("[ENROLL] Failed!");
    
    // Consolidated failure message
    String failureMessage = "ENROLL_FAILED: ID=" + String(id) + ", Code=" + String(p) + ", Status: Storage failed";
    sendToESP32(failureMessage);
  }
  delay(2000);
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