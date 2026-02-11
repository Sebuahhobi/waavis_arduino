// Example: Capture ESP32 camera frame and send as media

#if !defined(ESP32)
#error "This example supports ESP32 only."
#endif

#include <WiFi.h>
#include <esp_camera.h>
#include <Waavis.h>

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

const char *deviceToken = "DEVICE_TOKEN";
const char *destination = "628xxxxxx";

WaavisClient waavis;

class BufferStream : public Stream {
public:
  BufferStream(const uint8_t *data, size_t length)
      : _data(data), _length(length), _pos(0) {}

  int available() override {
    return static_cast<int>(_length - _pos);
  }

  int read() override {
    if (_pos >= _length) {
      return -1;
    }
    return _data[_pos++];
  }

  int peek() override {
    if (_pos >= _length) {
      return -1;
    }
    return _data[_pos];
  }

  size_t write(uint8_t) override {
    return 0;
  }

private:
  const uint8_t *_data;
  size_t _length;
  size_t _pos;
};

static bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  return esp_camera_init(&config) == ESP_OK;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.println("WiFi connected");

  if (!initCamera()) {
    Serial.println("Camera init failed");
    return;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  BufferStream stream(fb->buf, fb->len);
  bool ok = waavis.sendChatMedia(
    deviceToken,
    destination,
    "Halo",
    false,
    "image",
    stream,
    fb->len,
    "esp32.jpg"
  );

  esp_camera_fb_return(fb);

  if (ok) {
    Serial.println("sendChatMedia OK");
  } else {
    Serial.print("sendChatMedia failed: ");
    Serial.println(waavis.lastError());
  }
}

void loop() {}
