#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>           // <-- Add this line
#include <UniversalTelegramBot.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
// #define CAMERA_MODEL_ESP_EYE  // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_CAMS3_UNIT  // Has PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "esp32-iot";
const char *password = "esp32-iot";
// const char *ssid = "Stampan";
// const char *password = "1234567890";

// Telegram Bot credentials
const char* botToken = "7950984672:AAGn7jHn4fqM12_8pwgR6wqFZzz_GNpvyYo";
const char* chatId = "5638142909";
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

void startCameraServer();
void setupLedFlash(int pin);

// --- Helper callbacks for streaming photo ---
camera_fb_t *fb_g = nullptr;
int fb_pos = 0;

bool moreDataAvailable() {
  return fb_pos < fb_g->len;
}

uint8_t getNextByte() {
  return fb_g->buf[fb_pos++];
}

unsigned char* getNextBuffer() {
  return nullptr; // Not used, but required by API
}

int resetFunc() {
  fb_pos = 0;
  return 0;
}
// --- End helper callbacks ---

void sendGreetingToTelegram() {
  String greeting = "🟢 ESP32-CAM Online!\n";
  greeting += "Welcome to Kotak Amal Nurul Ilmi Bot.\n";
  greeting += "\nSupported commands (from Arduino Mega):\n";
  greeting += "- FINGERPRINT_READY\n";
  greeting += "- FINGERPRINT_NOT_READY\n";
  greeting += "- VIBRATION_ALERT\n";
  greeting += "- ACCESS_GRANTED:<id>\n";
  greeting += "- ACCESS_DENIED\n";
  greeting += "- DOOR_UNLOCKED\n";
  greeting += "- DOOR_LOCKED\n";
  greeting += "- DOOR_ACCESS_DENIED\n";
  greeting += "- CAPTURE_PHOTO\n";
  sendTelegram(greeting);
}

unsigned long lastBotCheck = 0;
const unsigned long botCheckInterval = 2000; // 2 seconds
String pendingCommand = "";

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    text.trim();
    if (text == "/capture") {
      Serial.println("[Bot] /capture command received");
      captureAndSendPhoto();
    } else if (text == "/gps") {
      Serial.println("[Bot] /gps command received");
      Serial.println("REQUEST_GPS"); // Send request to Mega
      pendingCommand = "gps";
    }
  }
}

void captureAndSendPhoto() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    sendTelegram("Failed to capture image");
    return;
  }
  fb_g = fb;
  fb_pos = 0;
  String result = bot.sendPhotoByBinary(
    String(chatId),
    "image/jpeg",
    fb->len,
    moreDataAvailable,
    getNextByte,
    getNextBuffer,
    resetFunc
  );
  esp_camera_fb_return(fb);
  if (result.indexOf("true") > 0) {
    Serial.println("[Bot] Photo sent to Telegram");
  } else {
    Serial.println("[Bot] Failed to send photo");
    sendTelegram("Failed to send photo to Telegram");
  }
}

void sendTelegram(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    bool sent = bot.sendMessage(chatId, message, "");
    if (sent) {
      Serial.print("[Telegram] Sent: ");
      Serial.println(message);
    } else {
      Serial.println("[Telegram] Failed to send message");
    }
  } else {
    Serial.println("[Telegram] WiFi not connected, cannot send message");
  }
}

void sendIPToTelegram() {
  String ip = WiFi.localIP().toString();
  String msg = "🌐 ESP32-CAM IP: <a href=\"http://" + ip + ":81/stream\">http://" + ip + ":81/stream</a>\n";
  msg += "Click the link to view the camera stream.\n\n";
  msg += "Use the buttons below to send commands.";
  String keyboard = "{\"inline_keyboard\":[[";
  keyboard += "{\"text\":\"Capture Photo\",\"callback_data\":\"/capture\"},";
  keyboard += "{\"text\":\"Get GPS\",\"callback_data\":\"/gps\"}]]}";
  String payload = "chat_id=" + String(chatId) + "&text=" + msg + "&parse_mode=HTML&reply_markup=" + keyboard;
  String url = "https://api.telegram.org/bot" + String(botToken) + "/sendMessage";
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http; // <-- Use HTTPClient, not HttpClient
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.println("[Telegram] IP address sent to chat");
  } else {
    Serial.println("[Telegram] Failed to send IP address");
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;  // Reduce from 20MHz to 10MHz
  config.frame_size = FRAMESIZE_VGA;  // Reduce from UXGA to VGA (640x480)
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 20;  // Increase quality number (lower quality = less power)
  config.fb_count = 1;  // Reduce frame buffer count

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  // Add power management for WiFi
  WiFi.setTxPower(WIFI_POWER_8_5dBm);  // Reduce WiFi power
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure(); // For Telegram HTTPS
  Serial.println("Telegram Bot ready.");

  sendGreetingToTelegram();
  sendIPToTelegram();

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // Listen for Serial commands from Arduino Mega
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) {
      if (command.startsWith("GPS:")) {
        // GPS:<lat>,<lng>,<alt>
        if (pendingCommand == "gps") {
          sendTelegram("📍 GPS Location: " + command.substring(4));
          pendingCommand = "";
        }
      } else {
        sendTelegram(command);
      }
    }
  }
  // Poll Telegram bot for new messages (reduced frequency)
  if (millis() - lastBotCheck > botCheckInterval) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastBotCheck = millis();
  }
  delay(50); // Increased delay to reduce CPU usage and heat
}
