// Example: Send media from URL using Waavis API

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "This example supports ESP8266 and ESP32 only."
#endif

#include <Waavis.h>

const char *ssid = "SSID";
const char *password = "WIFI_PASSWORD";

const char *deviceToken = "TOKEN";
const char *destination = "628xxxxx";
WaavisClient waavis;

void setup() {
  Serial.begin(115200);
  Serial.println("Start ESP32 Send Foto");
  delay(200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.println("WiFi connected");
  waavis.setInsecure(true);
  waavis.listSPIFFSFiles();
  delay(5000);
}

void loop() {
  Serial.println("kirim foto");
  bool ok = waavis.sendChatMediaFromUrl(
    deviceToken,
    destination,
    "Saya berhasil kirim foto menggunakan ESP32",
    true, "https://placehold.co/600x400/png"
  );

  if (ok) {
    Serial.println("sendChatMediaFromUrl OK");
  } else {
    Serial.print("sendChatMediaFromUrl failed: ");
    Serial.println(waavis.lastError());
  }

  delay(60000);
}
