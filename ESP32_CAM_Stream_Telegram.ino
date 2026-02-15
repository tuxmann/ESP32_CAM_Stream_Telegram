/*
 * ESP32-CAM: Local Streaming + Telegram Bot (FIXED VERSION)
 * 
 * Fixes for photo sending issues:
 * - Lower resolution photos for Telegram (more reliable)
 * - Better memory management
 * - Improved error handling
 * - Alternative photo sending method
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ============ CONFIGURATION ============
// WiFi credentials
const char* ssid = "Your-Wifi";
const char* password = "Wifi-Password1234";

// Telegram bot credentials
#define BOTtoken "123456789:ABCdefGHIjklMNOpqrsTUVwxyz"
#define CHAT_ID "123456789"

// ============ CAMERA PIN DEFINITION (AI-THINKER) ============
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN 4  // Built-in flash LED

// ============ GLOBAL VARIABLES ============
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

unsigned long lastTimeBotRan;
const int botRequestDelay = 1000;
bool flashState = LOW;

// ============ CAMERA INITIALIZATION ============
bool setupCamera() {
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // REDUCED quality settings for more reliable Telegram sending
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;  // 800x600 - more reliable than UXGA
    config.jpeg_quality = 12;  // Slightly lower quality for smaller file size
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  // Adjust sensor settings
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_special_effect(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 0);
  s->set_ae_level(s, 0);
  s->set_aec_value(s, 300);
  s->set_gain_ctrl(s, 1);
  s->set_agc_gain(s, 0);
  s->set_gainceiling(s, (gainceiling_t)0);
  s->set_bpc(s, 0);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  s->set_dcw(s, 1);
  s->set_colorbar(s, 0);
  
  Serial.println("Camera configured successfully");
  return true;
}

// ============ IMPROVED TELEGRAM PHOTO FUNCTION ============
String sendPhotoTelegram() {
  Serial.println("Preparing to send photo to Telegram...");
  
  // Turn on flash
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(300);  // Give more time for proper exposure
  
  // Capture photo
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  
  // Turn off flash immediately after capture
  digitalWrite(FLASH_LED_PIN, LOW);
  
  if(!fb) {
    Serial.println("ERROR: Camera capture failed");
    return "Camera capture failed";
  }
  
  Serial.printf("Photo captured: %d bytes, %dx%d\n", fb->len, fb->width, fb->height);
  
  // Check if photo is too large (Telegram has limits)
  if(fb->len > 3000000) {  // ~3MB limit
    Serial.println("WARNING: Photo may be too large for Telegram");
  }
  
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  Serial.println("Connecting to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful!");
    
    String head = "--RandomBoundary12345\r\n";
    head += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    head += String(CHAT_ID) + "\r\n";
    head += "--RandomBoundary12345\r\n";
    head += "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32cam.jpg\"\r\n";
    head += "Content-Type: image/jpeg\r\n\r\n";

    String tail = "\r\n--RandomBoundary12345--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot" + String(BOTtoken) + "/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomBoundary12345");
    clientTCP.println("Connection: keep-alive");
    clientTCP.println();
    clientTCP.print(head);
    
    Serial.println("Sending image data...");
    
    // Send image in chunks
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    size_t chunkSize = 1024;
    
    for (size_t n = 0; n < fbLen; n += chunkSize) {
      if (n + chunkSize < fbLen) {
        clientTCP.write(fbBuf, chunkSize);
        fbBuf += chunkSize;
      } else {
        size_t remainder = fbLen % chunkSize;
        clientTCP.write(fbBuf, remainder);
      }
      
      // Print progress
      if(n % (chunkSize * 10) == 0) {
        Serial.printf("Sent: %d/%d bytes (%.1f%%)\n", n, fbLen, (float)n/fbLen*100);
      }
    }
    
    clientTCP.print(tail);
    Serial.println("Image data sent, waiting for response...");
    
    // Return frame buffer immediately to free memory
    esp_camera_fb_return(fb);
    fb = NULL;
    
    // Wait for response
    unsigned long timeout = millis();
    while (clientTCP.connected() && millis() - timeout < 15000) {
      while (clientTCP.available()) {
        char c = clientTCP.read();
        getBody += c;
        timeout = millis(); // Reset timeout on data received
      }
      delay(1);
    }
    
    clientTCP.stop();
    
    Serial.println("Response received:");
    Serial.println(getBody);
    
    // Check for success
    if(getBody.indexOf("\"ok\":true") > 0) {
      Serial.println("SUCCESS: Photo sent to Telegram!");
      return "Success";
    } else if(getBody.indexOf("\"ok\":false") > 0) {
      Serial.println("ERROR: Telegram API returned error");
      return "API Error: " + getBody;
    } else {
      Serial.println("WARNING: Unexpected response");
      return "Unexpected response";
    }
  }
  else {
    if(fb) esp_camera_fb_return(fb);
    Serial.println("ERROR: Connection to api.telegram.org failed");
    return "Connection failed";
  }
}

// ============ ALTERNATIVE METHOD - Uses UniversalTelegramBot library ============
// If the above method still doesn't work, uncomment this function and use it instead
/*
bool sendPhotoTelegramAlt() {
  Serial.println("Taking photo with alternative method...");
  
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(300);
  
  camera_fb_t * fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);
  
  if(!fb) {
    Serial.println("Camera capture failed");
    return false;
  }
  
  Serial.printf("Photo size: %d bytes\n", fb->len);
  
  // Use the library's built-in method
  bool result = bot.sendPhotoByBinary(CHAT_ID, "image/jpeg", fb->len, 
                                      fb->buf, NULL, NULL);
  
  esp_camera_fb_return(fb);
  
  if(result) {
    Serial.println("Photo sent successfully!");
  } else {
    Serial.println("Failed to send photo");
  }
  
  return result;
}
*/

void handleNewMessages(int numNewMessages) {
  Serial.println("Handling " + String(numNewMessages) + " new message(s)");

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    
    // Security check
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      Serial.println("Unauthorized access attempt from: " + chat_id);
      continue;
    }
    
    String text = bot.messages[i].text;
    Serial.println("Command received: " + text);
    
    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";
    
    // Handle commands
    if (text == "/photo") {
      bot.sendMessage(chat_id, "📸 Taking photo, please wait...", "");
      String result = sendPhotoTelegram();
      
      // Alternative method if main method fails
      // Uncomment these lines and comment out the lines above if needed:
      /*
      bool result = sendPhotoTelegramAlt();
      if(result) {
        bot.sendMessage(chat_id, "✅ Photo sent!", "");
      } else {
        bot.sendMessage(chat_id, "❌ Failed to send photo. Check serial monitor.", "");
      }
      */
      
      if(result.indexOf("Success") >= 0 || result.indexOf("\"ok\":true") >= 0) {
        // Photo was sent successfully, no need for confirmation
        Serial.println("Photo delivered successfully");
      } else {
        bot.sendMessage(chat_id, "❌ Error: " + result, "");
      }
    }
    
    else if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
      bot.sendMessage(chat_id, flashState ? "💡 Flash ON" : "⚫ Flash OFF", "");
    }
    
    else if (text == "/start") {
      String welcome = "🎥 ESP32-CAM Bot\n\n";
      welcome += "Commands:\n";
      welcome += "/photo - Take and send photo\n";
      welcome += "/flash - Toggle flash LED\n";
      welcome += "/status - Camera status\n";
      welcome += "/stream - Get stream URL\n";
      welcome += "/test - Test connection";
      bot.sendMessage(chat_id, welcome, "");
    }
    
    else if (text == "/status") {
      String status = "Camera Status:\n";
      status += "Local IP: " + WiFi.localIP().toString() + "\n";
      status += "Stream URL: http://" + WiFi.localIP().toString() + "/\n";
      status += "WiFi RSSI: " + String(WiFi.RSSI()) + " dBm\n";
      status += "\nNERD Details:";
      status += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
      status += "PSRAM: " + String(psramFound() ? "Yes" : "No") + "\n";
      if(psramFound()) {
        status += "Free PSRAM: " + String(ESP.getFreePsram()) + " bytes\n";
      }
      bot.sendMessage(chat_id, status, "");
    }
    
    else if (text == "/stream") {
      String msg = "🌐 Stream URL:\n";
      msg += "http://" + WiFi.localIP().toString() + "/\n\n";
      msg += "Only accessible on local network";
      bot.sendMessage(chat_id, msg, "");
    }
    
    else if (text == "/test") {
      bot.sendMessage(chat_id, "✅ Bot is working! Connection OK.", "");
    }
    
    else {
      bot.sendMessage(chat_id, "❓ Unknown command. Send /start for help.", "");
    }
  }
}

// ============ WEB SERVER FOR STREAMING ============
WiFiServer server(80);

String index_html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>"
  "body{font-family:Arial;text-align:center;margin:0;background:#1a1a1a;color:#fff;}"
  ".container{max-width:800px;margin:50px auto;padding:20px;}"
  "h1{color:#4CAF50;}"
  "img{max-width:100%;height:auto;border:3px solid #4CAF50;border-radius:10px;}"
  ".info{background:#2a2a2a;padding:15px;margin:20px 0;border-radius:5px;}"
  "</style></head><body>"
  "<div class='container'>"
  "<h1>ESP32-CAM Live Stream</h1>"
  "<div class='info'>Stream updates automatically</div>"
  "<img id='stream' src='/stream'>"
  "</div></body></html>";

void handleWebServer() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  String currentLine = "";
  String header = "";
  
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      header += c;
      if (c == '\n') {
        if (currentLine.length() == 0) {
          if (header.indexOf("GET /stream") >= 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
            client.println();
            
            while(client.connected()) {
              camera_fb_t * fb = esp_camera_fb_get();
              if (!fb) {
                Serial.println("Stream: Camera capture failed");
                delay(100);
                continue;
              }
              
              client.print("--frame\r\n");
              client.print("Content-Type: image/jpeg\r\n\r\n");
              client.write(fb->buf, fb->len);
              client.print("\r\n");
              
              esp_camera_fb_return(fb);
              
              if (!client.connected()) break;
            }
          } else {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
            client.println(index_html);
          }
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  client.stop();
}

// ============ SETUP ============
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.println("\n\nESP32-CAM Starting...");
  
  // Setup flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  
  // Initialize camera
  Serial.println("Initializing camera...");
  if (!setupCamera()) {
    Serial.println("FATAL: Camera setup failed!");
    while(1) {
      delay(1000);
      Serial.println("Camera init failed - check connections");
    }
  }
  
  // Check PSRAM
  if(psramFound()) {
    Serial.println("✓ PSRAM found");
  } else {
    Serial.println("⚠ PSRAM not found - using internal RAM");
  }
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi: " + String(ssid));
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("Stream: http://" + WiFi.localIP().toString());
    Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("\n✗ WiFi connection failed!");
    Serial.println("Check SSID and password");
  }
  
  // Start web server
  server.begin();
  Serial.println("✓ Web server started");
  
  // Configure Telegram
  clientTCP.setInsecure();  // Skip certificate validation
  Serial.println("✓ Telegram client configured");
  
  // Send startup message
  String welcome = "ESP32-CAM Telegram Bot Started!\n\n";
  welcome += "Stream URL: http://" + WiFi.localIP().toString() + "/\n";
  welcome += "\n";
  welcome += "/photo : Take and send a photo\n";
  welcome += "/flash : Toggle flash LED\n";
  welcome += "/status : Get camera status\n";
  bot.sendMessage(CHAT_ID, welcome, "");
  
  Serial.println("\n=== System Ready ===");
  Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
  if(psramFound()) {
    Serial.println("Free PSRAM: " + String(ESP.getFreePsram()) + " bytes");
  }
  Serial.println("====================\n");
}

// ============ MAIN LOOP ============
void loop() {
  // Handle web streaming
  handleWebServer();
  
  // Check for Telegram messages
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
