# Kotak Amal Nurul Ilmi - Sistem Keamanan IoT

## Gambaran Umum
Ini adalah proyek sistem keamanan IoT menggunakan Arduino Mega, ESP32 Cam, Telegram Bot, dan berbagai sensor. Sistem ini menyediakan autentikasi sidik jari, pelacakan GPS, streaming kamera langsung, dan kemampuan pemantauan jarak jauh.

## Komponen yang Digunakan
- **Arduino Mega 2560** - Kontroler utama
- **ESP32 Cam** - Modul kamera untuk streaming langsung
- **Sensor Sidik Jari (R307)** - Autentikasi biometrik
- **GPS NEO-6M** - Pelacakan lokasi
- **LCD I2C 16x2** - Tampilan status
- **Sensor Getaran** - Deteksi gerakan
- **Buzzer** - Peringatan suara
- **Modul Relay** - Kontrol kunci pintu
- **SIM800L** - Komunikasi SMS cadangan

## Fitur
1. **Salam Islami** - LCD menampilkan "Assalamualaikum, User" saat startup
2. **Autentikasi Sidik Jari** - Kontrol akses aman dengan notifikasi Telegram
3. **Pelacakan GPS** - Pemantauan lokasi jarak jauh melalui Telegram
4. **Sistem Kamera Hybrid** - ESP32 Cam dengan antarmuka Web + kontrol Telegram
5. **Deteksi Getaran** - Peringatan gerakan dikirim ke Telegram
6. **Pemantauan Status** - Pembaruan status sistem real-time

## Struktur File
```
├── arduino_mega_main/
│   ├── arduino_mega_main.ino       # Program utama Arduino Mega
│   └── sms_backup.h                # Konfigurasi SMS backup
├── CameraWebServer/
│   ├── CameraWebServer.ino         # Program ESP32 Cam hybrid (Web + Telegram)
│   ├── app_httpd.cpp               # Implementasi web server
│   ├── camera_pins.h               # Definisi pin kamera
│   ├── camera_index.h              # HTML antarmuka web
│   ├── partitions.csv              # Konfigurasi partisi ESP32
│   └── ci.json                     # Konfigurasi CI/CD
├── telegram_config/
│   └── telegram_config.h           # Konfigurasi Telegram Bot
├── camera_pins.h                   # Definisi pin kamera global
├── camera_test_simple.ino          # Program testing kamera sederhana
├── simple_bridge.ps1               # Script PowerShell untuk bridge debugging
└── README.md                       # File ini
```

## Penjelasan File Khusus

### simple_bridge.ps1
File `simple_bridge.ps1` adalah **script PowerShell untuk debugging dan testing komunikasi** antara Arduino Mega dan ESP32-CAM dalam proyek Kotak Amal Nurul Ilmi.

#### Fungsi Utama:
1. **Bridge Manual untuk Testing**: Script ini membantu developer melakukan testing manual komunikasi antara dua perangkat tanpa perlu mengimplementasikan komunikasi serial otomatis.
2. **Debugging Komunikasi**: Memungkinkan developer untuk melihat dan memverifikasi pesan yang dikirim dari Arduino Mega ke ESP32-CAM.
3. **Testing Sensor**: Memudahkan testing berbagai sensor (sidik jari, getaran, GPS) dengan melihat output pesan secara real-time.

#### Cara Penggunaan:
```powershell
.\simple_bridge.ps1 COM11 COM7
```

#### Cara Kerja:
1. **Parameter Input**: Script menerima dua parameter COM port:
   - `$ArduinoPort` (default: COM11) - Port Arduino Mega
   - `$ESP32Port` (default: COM7) - Port ESP32-CAM

2. **Validasi Port**: Memeriksa apakah kedua COM port tersedia dan dapat diakses.

3. **Instruksi Manual**: Memberikan panduan step-by-step untuk:
   - Membuka Serial Monitor Arduino Mega (9600 baud)
   - Membuka Serial Monitor ESP32-CAM (115200 baud)
   - Menyalin pesan dari Arduino ke ESP32 secara manual

4. **Format Pesan**: Script mendeteksi pesan dengan format `[To ESP32-CAM]: [PESAN]` dari output Arduino.

#### Contoh Pesan yang Dapat Ditest:
- `[To ESP32-CAM]: FINGERPRINT_READY` - Sensor sidik jari siap
- `[To ESP32-CAM]: VIBRATION_ALERT` - Peringatan getaran terdeteksi
- `[To ESP32-CAM]: ACCESS_GRANTED:1` - Akses diberikan untuk user ID 1
- `[To ESP32-CAM]: DOOR_UNLOCKED` - Pintu telah dibuka

#### Kapan Digunakan:
- **Development Phase**: Saat mengembangkan dan testing fitur baru
- **Debugging**: Ketika ada masalah komunikasi antara Arduino dan ESP32
- **Sensor Testing**: Untuk memverifikasi sensor berfungsi dengan benar
- **Integration Testing**: Sebelum implementasi komunikasi otomatis

Script ini sangat berguna untuk fase development karena memungkinkan developer untuk memahami alur komunikasi dan memastikan semua komponen berfungsi dengan baik sebelum implementasi final.

## Instruksi Setup

### 1. Koneksi Hardware

#### Koneksi Arduino Mega:
- **Modul GPS**: TX → Pin 17, RX → Pin 16
- **Sensor Sidik Jari**: TX → Pin 19, RX → Pin 18
- **LCD I2C**: SDA → Pin 20, SCL → Pin 21
- **Buzzer**: Pin 5
- **Relay**: Pin 4
- **Sensor Getaran**: A0
- **ESP32 Cam Enable**: Pin 3

#### Koneksi ESP32 Cam:
- **Kamera**: Sensor OV2640 built-in
- **WiFi**: Modul WiFi built-in
- **Kontrol**: Menerima perintah dari Arduino Mega melalui GPIO

### 2. Setup Software

#### Library yang Diperlukan:
```cpp
// Arduino Mega
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ESP32 Cam
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
```

#### Instalasi:
1. Buka Arduino IDE
2. Buka Tools → Manage Libraries
3. Install library berikut:
   - LiquidCrystal_I2C
   - TinyGPS++
   - Adafruit Fingerprint Sensor
   - ArduinoJson

### 3. Setup Telegram Bot

1. **Buat Telegram Bot**:
   - Kirim pesan ke @BotFather di Telegram
   - Kirim `/newbot`
   - Ikuti instruksi untuk membuat bot Anda
   - Simpan token bot

2. **Dapatkan Chat ID**:
   - Kirim pesan ke bot Anda
   - Kunjungi: `https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates`
   - Temukan chat ID Anda dalam respons

3. **Update Konfigurasi**:
   - Edit `config/telegram_config.h`
   - Ganti nilai placeholder dengan kredensial Anda yang sebenarnya

### 4. Konfigurasi WiFi

Update kredensial WiFi di kedua program:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 5. Upload Program

1. **Arduino Mega**:
   - Hubungkan Arduino Mega ke komputer
   - Buka `sketch_jun30c/sketch_jun30c.ino`
   - Pilih board: Arduino Mega 2560
   - Upload program

2. **ESP32 Cam**:
   - Hubungkan ESP32 Cam ke komputer
   - Buka `CameraWebServer/CameraWebServer.ino`
   - Pilih board: ESP32 Dev Module
   - Upload program

## Perintah Telegram Bot

### Perintah Sistem Utama:
- `/camera` - Toggle streaming ESP32 Cam
- `/gps` - Dapatkan lokasi GPS saat ini
- `/status` - Dapatkan status sistem
- `/door` - Toggle kunci pintu secara manual (akses darurat)

### Perintah ESP32 Cam (Telegram):
- `/capture` - Ambil foto dan kirim ke Telegram
- `/status` - Dapatkan status kamera melalui Telegram
- `/stream_start` - Mulai pemantauan streaming
- `/stream_stop` - Hentikan pemantauan streaming
- `/help` - Tampilkan perintah yang tersedia

### Antarmuka Web ESP32 Cam:
- **http://[ESP32-IP]** - Antarmuka kamera utama
- **http://[ESP32-IP]/stream** - Stream video langsung
- **http://[ESP32-IP]/capture** - Ambil foto
- **http://[ESP32-IP]/status** - Status kamera
- **http://[ESP32-IP]/control** - Kontrol kamera (kecerahan, kontras, dll.)

## Alur Sistem

1. **Startup**:
   - LCD menampilkan "Assalamualaikum, User"
   - Memeriksa status sensor sidik jari
   - Menghubungkan ke WiFi
   - Mengirim pesan startup ke Telegram

2. **Autentikasi Sidik Jari**:
   - Pengguna menempatkan jari pada sensor
   - Sistem memvalidasi sidik jari
   - Mengirim hasil ke Telegram
   - Mengontrol relay (kunci pintu)
   - **Pengguna Terdaftar**: Pintu terbuka selama 3 detik, kemudian terkunci otomatis
   - **Pengguna Tidak Terdaftar**: Pintu tetap terkunci, akses ditolak

3. **Kontrol Jarak Jauh**:
   - Pengguna mengirim perintah melalui Telegram
   - Sistem menjalankan perintah
   - Mengirim pesan konfirmasi

4. **Pemantauan**:
   - Deteksi getaran
   - Pelacakan GPS
   - Pembaruan status
   - Streaming kamera langsung

## Troubleshooting

### Masalah Umum:

1. **Koneksi WiFi Gagal**:
   - Periksa kredensial WiFi
   - Pastikan sinyal kuat
   - Restart sistem

2. **Sidik Jari Tidak Berfungsi**:
   - Bersihkan permukaan sensor
   - Daftar ulang sidik jari
   - Periksa koneksi kabel

3. **GPS Tidak Ada Sinyal**:
   - Pindah ke area terbuka
   - Tunggu kunci satelit
   - Periksa koneksi antena

4. **Pesan Telegram Tidak Terkirim**:
   - Verifikasi token bot dan chat ID
   - Periksa koneksi internet
   - Pastikan bot tidak diblokir

### Informasi Debug:
- Serial Monitor menampilkan informasi debug detail
- Periksa Serial Monitor untuk pesan error
- Gunakan perintah `/status` untuk memeriksa kesehatan sistem

## Fitur Keamanan

1. **Autentikasi Biometrik** - Akses sidik jari yang aman
2. **Pemantauan Jarak Jauh** - Pembaruan status real-time
3. **Deteksi Gerakan** - Peringatan sensor getaran
4. **Pelacakan Lokasi** - Koordinat GPS
5. **Video Langsung** - Streaming ESP32 Cam
6. **Komunikasi Cadangan** - Fallback SMS SIM800L

## Kustomisasi

### Menambah Perintah Baru:
1. Tambah definisi perintah di `telegram_config.h`
2. Tambah handler perintah di program utama
3. Update pesan bantuan

### Memodifikasi Pesan:
- Edit konstanta pesan di `telegram_config.h`
- Update pesan tampilan LCD di program utama

### Menambah Sensor Baru:
1. Definisikan koneksi pin
2. Tambah library sensor
3. Buat fungsi sensor
4. Integrasikan dengan notifikasi Telegram

## Dukungan

Untuk dukungan teknis atau pertanyaan, silakan merujuk ke dokumentasi Arduino dan ESP32, atau hubungi tim pengembangan.

## Lisensi

Proyek ini dikembangkan untuk tujuan pendidikan dan amal. Silakan hormati nilai-nilai dan prinsip Islam dalam penggunaannya. 