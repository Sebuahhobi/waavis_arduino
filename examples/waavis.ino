// Example: Send a chat message using Waavis API

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "This example supports ESP8266 and ESP32 only."
#endif

#include <Waavis.h>

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

const char *deviceToken = "DEVICE_TOKEN";
const char *destination = "628xxxxxx";
const char *destinationJid = "62812xxxx@s.whatsapp.net"; // or plain number

WaavisClient waavis;

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

	bool ok = waavis.sendChat(deviceToken, destination, "Halo");
	if (ok) {
		Serial.println("sendChat OK");
	} else {
		Serial.print("sendChat failed: ");
		Serial.println(waavis.lastError());
	}

	bool okPost = waavis.sendChatPost(deviceToken, destinationJid, "Halo", true);
	if (okPost) {
		Serial.println("sendChatPost OK");
	} else {
		Serial.print("sendChatPost failed: ");
		Serial.println(waavis.lastError());
	}

	bool okLink = waavis.sendChatLink(deviceToken, destination, "Halo", true,
							"https://waavis.com", "Waavis API",
							"WhatsApp API Service");
	if (okLink) {
		Serial.println("sendChatLink OK");
	} else {
		Serial.print("sendChatLink failed: ");
		Serial.println(waavis.lastError());
	}
}

void loop() {}
