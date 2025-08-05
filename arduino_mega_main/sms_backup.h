#ifndef SMS_BACKUP_H
#define SMS_BACKUP_H

#include <SoftwareSerial.h>

// SMS Backup Configuration
#define SMS_NUMBER "082296389933"  // Backup phone number for SMS
#define SMS_ENABLED true           // Enable/disable SMS backup

// SMS Messages
#define SMS_WELCOME "Kotak Amal Nurul Ilmi aktif"
#define SMS_FINGERPRINT_READY "Fingerprint siap"
#define SMS_FINGERPRINT_ERROR "Fingerprint error"
#define SMS_ACCESS_GRANTED "Akses diberikan"
#define SMS_ACCESS_DENIED "Akses ditolak"
#define SMS_VIBRATION "Terdeteksi getaran"
#define SMS_CAMERA_ACTIVE "Kamera aktif"
#define SMS_CAMERA_INACTIVE "Kamera nonaktif"
#define SMS_GPS_LOCATION "Lokasi GPS: "
#define SMS_DOOR_UNLOCKED "Pintu dibuka"
#define SMS_DOOR_LOCKED "Pintu dikunci"
#define SMS_DOOR_ACCESS_DENIED "Pintu tetap terkunci"

class SMSBackup {
private:
  SoftwareSerial* simSerial;
  String number;
  
public:
  SMSBackup(SoftwareSerial* serial, String phoneNumber) {
    simSerial = serial;
    number = phoneNumber;
  }
  
  void sendSMS(String message) {
    if (!SMS_ENABLED) return;
    
    simSerial->println("AT+CMGF=1");    // Set SMS to text mode
    delay(200);
    simSerial->println("AT+CMGS=\"" + number + "\"\r"); // Set phone number
    delay(200);
    simSerial->println(message);
    delay(100);
    simSerial->println((char)26);  // ASCII code of CTRL+Z
    delay(200);
  }
  
  void sendWelcomeMessage() {
    sendSMS(SMS_WELCOME);
  }
  
  void sendFingerprintStatus(bool ready) {
    if (ready) {
      sendSMS(SMS_FINGERPRINT_READY);
    } else {
      sendSMS(SMS_FINGERPRINT_ERROR);
    }
  }
  
  void sendAccessResult(bool granted, int fingerID = 0) {
    if (granted) {
      sendSMS(String(SMS_ACCESS_GRANTED) + " - ID: " + String(fingerID));
    } else {
      sendSMS(SMS_ACCESS_DENIED);
    }
  }
  
  void sendVibrationAlert() {
    sendSMS(SMS_VIBRATION);
  }
  
  void sendCameraStatus(bool active) {
    if (active) {
      sendSMS(SMS_CAMERA_ACTIVE);
    } else {
      sendSMS(SMS_CAMERA_INACTIVE);
    }
  }
  
  void sendGPSLocation(double lat, double lng, double alt) {
    String locationMsg = SMS_GPS_LOCATION;
    locationMsg += "Lat:" + String(lat, 6);
    locationMsg += " Lng:" + String(lng, 6);
    locationMsg += " Alt:" + String(alt) + "m";
    sendSMS(locationMsg);
  }
  
  void sendDoorUnlocked() {
    sendSMS(SMS_DOOR_UNLOCKED);
  }
  
  void sendDoorLocked() {
    sendSMS(SMS_DOOR_LOCKED);
  }
  
  void sendDoorAccessDenied() {
    sendSMS(SMS_DOOR_ACCESS_DENIED);
  }
};

#endif 