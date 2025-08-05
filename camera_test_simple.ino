/*
 * Simple ESP32 Camera Test Sketch
 * This is a minimal test to verify camera model compilation
 */

// ===================
// Camera Model Selection - MUST BE FIRST!
// ===================
#define CAMERA_MODEL_AI_THINKER // AI Thinker ESP32-CAM

#include "esp_camera.h"

// Include the camera pins header (camera model must be defined above)
#include "camera_pins.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP32-CAM Camera Model Test");
  
  // Test if camera model is properly defined
  #ifdef CAMERA_MODEL_AI_THINKER
    Serial.println("Camera Model: AI_THINKER - DEFINED ✓");
  #else
    Serial.println("Camera Model: AI_THINKER - NOT DEFINED ✗");
  #endif
  
  // Print pin definitions to verify they're loaded
  Serial.println("Pin Definitions:");
  Serial.printf("PWDN_GPIO_NUM: %d\n", PWDN_GPIO_NUM);
  Serial.printf("RESET_GPIO_NUM: %d\n", RESET_GPIO_NUM);
  Serial.printf("XCLK_GPIO_NUM: %d\n", XCLK_GPIO_NUM);
  Serial.printf("SIOD_GPIO_NUM: %d\n", SIOD_GPIO_NUM);
  Serial.printf("SIOC_GPIO_NUM: %d\n", SIOC_GPIO_NUM);
  
  // Basic camera configuration test
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
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;  // Smaller frame size for testing
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  
  Serial.println("Camera initialized successfully! ✓");
  Serial.println("Compilation test PASSED - Camera model is properly selected");
}

void loop() {
  // Test camera capture
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    return;
  }
  
  Serial.printf("Picture taken! Size: %zu bytes\n", fb->len);
  esp_camera_fb_return(fb);
  
  delay(5000); // Take a picture every 5 seconds
}
