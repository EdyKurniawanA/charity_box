#ifndef TELEGRAM_CONFIG_H
#define TELEGRAM_CONFIG_H

// WiFi Configuration
#define WIFI_SSID "esp32-iot"
#define WIFI_PASSWORD "esp32-iot"

// Telegram Bot Configuration
#define BOT_TOKEN "7950984672:AAGn7jHn4fqM12_8pwgR6wqFZzz_GNpvyYo"
#define CHAT_ID "767286229"

// Bot Commands
#define CMD_CAMERA "/camera"
#define CMD_GPS "/gps"
#define CMD_STATUS "/status"
#define CMD_DOOR "/door"
#define CMD_START_STREAM "/start_stream"
#define CMD_STOP_STREAM "/stop_stream"
#define CMD_CAPTURE "/capture"

// Telegram API URLs
#define TELEGRAM_API_BASE "https://api.telegram.org/bot"
#define TELEGRAM_SEND_MESSAGE "/sendMessage"
#define TELEGRAM_SEND_PHOTO "/sendPhoto"
#define TELEGRAM_GET_UPDATES "/getUpdates"

// System Messages
#define MSG_WELCOME "🕌 Kotak Amal Nurul Ilmi telah aktif!"
#define MSG_FINGERPRINT_READY "✅ Fingerprint siap digunakan"
#define MSG_FINGERPRINT_ERROR "❌ Fingerprint tidak siap"
#define MSG_ACCESS_GRANTED "✅ Akses diberikan"
#define MSG_ACCESS_DENIED "❌ Akses ditolak"
#define MSG_VIBRATION_DETECTED "⚠️ Terdeteksi getaran pada kotak!"
#define MSG_CAMERA_ACTIVE "📷 Kamera ESP32 aktif - Live streaming dimulai"
#define MSG_CAMERA_INACTIVE "📷 Kamera ESP32 dinonaktifkan"
#define MSG_GPS_LOCATION "📍 Lokasi GPS:"
#define MSG_GPS_NO_SIGNAL "❌ GPS belum mendapatkan sinyal satelit"
#define MSG_STREAMING_ACTIVE "📹 Live streaming masih aktif"
#define MSG_PHOTO_CAPTURED "📸 Foto berhasil diambil dan dikirim!"
#define MSG_DOOR_UNLOCKED "🔓 Pintu kotak dibuka"
#define MSG_DOOR_LOCKED "🔒 Pintu kotak dikunci"
#define MSG_DOOR_ACCESS_DENIED "🔒 Pintu kotak tetap terkunci"

#endif 