#include "Waavis.h"

#ifndef WAAVIS_DEBUG
#define WAAVIS_DEBUG 1
#endif

#if WAAVIS_DEBUG
#define WAAVIS_LOG(message) Serial.println(message)
#else
#define WAAVIS_LOG(message) do {} while (0)
#endif

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HardwareSerial.h>
#include <SPIFFS.h>
#include <FS.h>
#else
#error "Waavis library supports ESP8266 and ESP32 only."
#endif

WaavisClient::WaavisClient(const String &baseUrl)
    : _baseUrl(baseUrl), _insecure(true), _sslCert(nullptr), _lastError("") {}

void WaavisClient::setInsecure(bool insecure) {
  _insecure = insecure;
}

void WaavisClient::setCertificate(const char* cert) {
  if (cert == nullptr || cert[0] == '\0') {
    _sslCert = nullptr;
    _insecure = true;
    return;
  }
  _sslCert = cert;
  _insecure = false;
}

String WaavisClient::lastError() const {
  return _lastError;
}

bool WaavisClient::sendChat(const String &token, const String &to, const String &message) {
  if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected";
    return false;
  }

  String url = _baseUrl + "/v1/send_chat?token=" + urlEncode(token) +
               "&to=" + urlEncode(to) +
               "&message=" + urlEncode(message);

#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  if (_insecure) {
    client.setInsecure();
  }
  HTTPClient http;
  if (!http.begin(client, url)) {
    _lastError = "HTTP begin failed";
    return false;
  }
#elif defined(ESP32)
  WiFiClientSecure client;
  if (_insecure) {
    client.setInsecure();
  }
  HTTPClient http;
  if (!http.begin(client, url)) {
    _lastError = "HTTP begin failed";
    return false;
  }
#endif

  int httpCode = http.GET();
  if (httpCode <= 0) {
    _lastError = http.errorToString(httpCode);
    http.end();
    return false;
  }

  if (httpCode < 200 || httpCode >= 300) {
    String response = http.getString();
    WAAVIS_LOG("[waavis] response: " + response);
    
    // Extract error from JSON
    int errorIdx = response.indexOf("\"error\":");
    if (errorIdx >= 0) {
      int startQuote = response.indexOf('"', errorIdx + 8);
      int endQuote = response.indexOf('"', startQuote + 1);
      if (startQuote >= 0 && endQuote > startQuote) {
        _lastError = response.substring(startQuote + 1, endQuote);
      } else {
        _lastError = "HTTP " + String(httpCode);
      }
    } else {
      _lastError = "HTTP " + String(httpCode);
    }
    http.end();
    return false;
  }

  _lastError = "";
  http.end();
  return true;
}

bool WaavisClient::sendChatPost(const String &token, const String &to,
                                const String &message, bool typing) {
  String body = "to=" + urlEncode(to) +
                "&message=" + urlEncode(message) +
                "&typing=" + String(typing ? "true" : "false");
  return sendPost("/v1/send_chat", token, body);
}

bool WaavisClient::sendChatLink(const String &token, const String &to,
                                const String &message, bool typing,
                                const String &link, const String &linkTitle,
                                const String &linkDescription) {
  String body = "to=" + urlEncode(to) +
                "&message=" + urlEncode(message) +
                "&typing=" + String(typing ? "true" : "false") +
                "&link=" + urlEncode(link) +
                "&link_title=" + urlEncode(linkTitle) +
                "&link_description=" + urlEncode(linkDescription);
  return sendPost("/v1/send_chat_link", token, body);
}

bool WaavisClient::sendChatMedia(const String &token, const String &to,
                                 const String &message, bool typing,
                                 const String &type, Stream &file,
                                 size_t fileSize, const String &fileName) {
  return sendChatMediaStream(token, to, message, typing, type, file, fileSize, fileName);
}

bool WaavisClient::sendChatMediaFromUrl(const String &token, const String &to,
                                        const String &caption, bool typing,
                                        const String &imageUrl) {
  if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected";
    return false;
  }

  String body = "to=" + urlEncode(to) +
                "&caption=" + urlEncode(caption) +
                "&typing=" + String(typing ? "true" : "false") +
                "&type=image_url" +
                "&image_url=" + urlEncode(imageUrl);
  return sendPost("/v1/send_chat_media", token, body);
}

bool WaavisClient::sendChatMediaBuffer(const String &token, const String &to,
                                       const String &message, bool typing,
                                       const String &type, const uint8_t *data,
                                       size_t dataSize, const String &fileName) {
  if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected";
    return false;
  }

  if (dataSize == 0) {
    _lastError = "File is empty";
    return false;
  }

  String boundary = "----WaavisBoundary" + String(millis());
  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"to\"\r\n\r\n" + to + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"message\"\r\n\r\n" + message + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"typing\"\r\n\r\n" +
          String(typing ? "true" : "false") + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"type\"\r\n\r\n" + type + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; filename=\"" +
          fileName + "\"\r\n";
  head += "Content-Type: application/octet-stream\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";
  size_t contentLength = head.length() + dataSize + tail.length();

#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  if (_sslCert != nullptr) {
    BearSSL::X509List cert(_sslCert);
    client.setTrustAnchors(&cert);
  } else if (_insecure) {
    client.setInsecure();
  }
  HTTPClient http;
  if (!http.begin(client, _baseUrl + "/v1/send_chat_media")) {
    _lastError = "HTTP begin failed";
    return false;
  }
#elif defined(ESP32)
  WiFiClientSecure client;
  if (_sslCert != nullptr) {
    client.setCACert(_sslCert);
  } else if (_insecure) {
    client.setInsecure();
  }
  HTTPClient http;
  if (!http.begin(client, _baseUrl + "/v1/send_chat_media")) {
    _lastError = "HTTP begin failed";
    return false;
  }
#endif

  http.addHeader("Authorization", token);
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  class MultipartStream : public Stream {
  public:
    MultipartStream(const String &head, const uint8_t *data, size_t dataSize,
                    const String &tail)
        : _head(head), _data(data), _dataSize(dataSize), _tail(tail),
          _headPos(0), _dataPos(0), _tailPos(0) {}

    int available() override {
      return static_cast<int>((_head.length() - _headPos) +
                              (_dataSize - _dataPos) +
                              (_tail.length() - _tailPos));
    }

    int read() override {
      if (_headPos < _head.length()) {
        return _head[_headPos++];
      }
      if (_dataPos < _dataSize) {
        return _data[_dataPos++];
      }
      if (_tailPos < _tail.length()) {
        return _tail[_tailPos++];
      }
      return -1;
    }

    int peek() override {
      return -1;
    }

    size_t write(uint8_t) override {
      return 0;
    }

  private:
    const String &_head;
    const uint8_t *_data;
    size_t _dataSize;
    const String &_tail;
    size_t _headPos;
    size_t _dataPos;
    size_t _tailPos;
  };

  MultipartStream body(head, data, dataSize, tail);
  int httpCode = http.sendRequest("POST", &body, contentLength);
  if (httpCode <= 0) {
    _lastError = http.errorToString(httpCode);
    http.end();
    return false;
  }

  if (httpCode < 200 || httpCode >= 300) {
    String response = http.getString();
    WAAVIS_LOG("[waavis] response: " + response);
    
    // Extract error from JSON
    int errorIdx = response.indexOf("\"error\":");
    if (errorIdx >= 0) {
      int startQuote = response.indexOf('"', errorIdx + 8);
      int endQuote = response.indexOf('"', startQuote + 1);
      if (startQuote >= 0 && endQuote > startQuote) {
        _lastError = response.substring(startQuote + 1, endQuote);
      } else {
        _lastError = "HTTP " + String(httpCode);
      }
    } else {
      _lastError = "HTTP " + String(httpCode);
    }
    http.end();
    return false;
  }

  _lastError = "";
  http.end();
  return true;
}

bool WaavisClient::sendChatMediaStream(const String &token, const String &to,
                                       const String &message, bool typing,
                                       const String &type, Stream &file,
                                       size_t fileSize, const String &fileName) {
  if (fileSize == 0) {
    _lastError = "File is empty";
    return false;
  }

#if defined(ESP32)
  // Use chunked upload on ESP32 to avoid large RAM allocations.
  return sendChatMediaStreamChunked(token, to, message, typing, type, file, fileName);
#else
  // For external stream, we need to read into buffer first
  if (fileSize > 102400) {
    _lastError = "File too large (max 100KB for stream)";
    return false;
  }
  
  uint8_t *buffer = (uint8_t *)malloc(fileSize);
  if (buffer == nullptr) {
    _lastError = "Out of memory";
    return false;
  }
  
  size_t totalRead = 0;
  while (totalRead < fileSize && file.available()) {
    size_t remaining = fileSize - totalRead;
    size_t toRead = remaining > 1024 ? 1024 : remaining;
    size_t readBytes = file.readBytes((char *)(buffer + totalRead), toRead);
    if (readBytes == 0) {
      break;
    }
    totalRead += readBytes;
  }
  
  bool ok = false;
  if (totalRead == fileSize) {
    ok = sendChatMediaBuffer(token, to, message, typing, type, buffer, totalRead, fileName);
  } else {
    _lastError = "Incomplete read";
  }
  
  free(buffer);
  return ok;
#endif
}

static String parseHost(const String &url, bool &isHttps, uint16_t &port) {
  String lower = url;
  lower.toLowerCase();
  isHttps = lower.startsWith("https://");
  bool isHttp = lower.startsWith("http://");
  if (!isHttps && !isHttp) {
    return "";
  }

  int start = isHttps ? 8 : 7;
  int slash = url.indexOf('/', start);
  String hostPort = (slash >= 0) ? url.substring(start, slash) : url.substring(start);
  int colon = hostPort.indexOf(':');
  if (colon >= 0) {
    port = static_cast<uint16_t>(hostPort.substring(colon + 1).toInt());
    return hostPort.substring(0, colon);
  }

  port = isHttps ? 443 : 80;
  return hostPort;
}

static void writeChunk(Stream &client, const uint8_t *data, size_t len) {
  if (len == 0) {
    return;
  }
  String hex = String(len, HEX);
  hex.toUpperCase();
  client.print(hex);
  client.print("\r\n");
  client.write(data, len);
  client.print("\r\n");
}

bool WaavisClient::sendChatMediaStreamChunked(const String &token, const String &to,
                                              const String &message, bool typing,
                                              const String &type, Stream &file,
                                              const String &fileName) {
  WAAVIS_LOG("[waavis] chunked upload start");
  if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected";
    WAAVIS_LOG("[waavis] WiFi not connected");
    return false;
  }

  bool isHttps = false;
  uint16_t port = 0;
  String host = parseHost(_baseUrl, isHttps, port);
  if (host.length() == 0) {
    _lastError = "Invalid base URL";
    WAAVIS_LOG("[waavis] Invalid base URL");
    return false;
  }

  String boundary = "----WaavisBoundary" + String(millis());
  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"to\"\r\n\r\n" + to + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"message\"\r\n\r\n" + message + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"typing\"\r\n\r\n" +
          String(typing ? "true" : "false") + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"type\"\r\n\r\n" + type + "\r\n";
  head += "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; filename=\"" +
          fileName + "\"\r\n";
  head += "Content-Type: application/octet-stream\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

#if defined(ESP8266)
  BearSSL::WiFiClientSecure secureClient;
  WiFiClient plainClient;
  Stream *client = nullptr;
  if (isHttps) {
    if (_sslCert != nullptr) {
      BearSSL::X509List cert(_sslCert);
      secureClient.setTrustAnchors(&cert);
    } else if (_insecure) {
      secureClient.setInsecure();
    }
    if (!secureClient.connect(host.c_str(), port)) {
      _lastError = "HTTP connect failed";
      WAAVIS_LOG("[waavis] HTTPS connect failed");
      return false;
    }
    client = &secureClient;
  } else {
    if (!plainClient.connect(host.c_str(), port)) {
      _lastError = "HTTP connect failed";
      WAAVIS_LOG("[waavis] HTTP connect failed");
      return false;
    }
    client = &plainClient;
  }
#elif defined(ESP32)
  WiFiClientSecure secureClient;
  WiFiClient plainClient;
  Stream *client = nullptr;
  if (isHttps) {
    if (_sslCert != nullptr) {
      secureClient.setCACert(_sslCert);
    } else if (_insecure) {
      secureClient.setInsecure();
    }
    if (!secureClient.connect(host.c_str(), port)) {
      _lastError = "HTTP connect failed";
      WAAVIS_LOG("[waavis] HTTPS connect failed");
      return false;
    }
    client = &secureClient;
  } else {
    if (!plainClient.connect(host.c_str(), port)) {
      _lastError = "HTTP connect failed";
      WAAVIS_LOG("[waavis] HTTP connect failed");
      return false;
    }
    client = &plainClient;
  }
#endif

  client->print("POST /v1/send_chat_media HTTP/1.1\r\n");
  client->print("Host: ");
  client->print(host);
  client->print("\r\n");
  client->print("Authorization: ");
  client->print(token);
  client->print("\r\n");
  client->print("Content-Type: multipart/form-data; boundary=");
  client->print(boundary);
  client->print("\r\n");
  client->print("Transfer-Encoding: chunked\r\n");
  client->print("Connection: close\r\n\r\n");
  WAAVIS_LOG("[waavis] headers sent");

  writeChunk(*client, reinterpret_cast<const uint8_t *>(head.c_str()), head.length());

  uint8_t buffer[1024];
  unsigned long lastRead = millis();
  while (true) {
    int available = file.available();
    if (available > 0) {
      size_t toRead = static_cast<size_t>(available);
      if (toRead > sizeof(buffer)) {
        toRead = sizeof(buffer);
      }
      size_t readBytes = file.readBytes(reinterpret_cast<char *>(buffer), toRead);
      if (readBytes > 0) {
        writeChunk(*client, buffer, readBytes);
        lastRead = millis();
      }
      continue;
    }

    if (millis() - lastRead > 5000) {
      break;
    }
    delay(10);
  }

  writeChunk(*client, reinterpret_cast<const uint8_t *>(tail.c_str()), tail.length());
  client->print("0\r\n\r\n");
  WAAVIS_LOG("[waavis] body sent");

  String statusLine = client->readStringUntil('\n');
  int status = -1;
  if (statusLine.startsWith("HTTP/")) {
    int firstSpace = statusLine.indexOf(' ');
    if (firstSpace >= 0 && firstSpace + 3 < static_cast<int>(statusLine.length())) {
      status = statusLine.substring(firstSpace + 1, firstSpace + 4).toInt();
    }
  }
  
  // Skip headers to get to body
  while (client->available()) {
    String headerLine = client->readStringUntil('\n');
    if (headerLine == "\r" || headerLine.length() == 0) {
      break; // End of headers
    }
  }
  
  // Read response body
  String responseBody = "";
  unsigned long bodyStart = millis();
  while (client->available() && millis() - bodyStart < 3000) {
    if (client->available()) {
      responseBody += (char)client->read();
    } else {
      delay(10);
    }
  }
  
  if (responseBody.length() > 0) {
    WAAVIS_LOG("[waavis] response: " + responseBody);
    
    // Extract error message from JSON if present
    int errorIdx = responseBody.indexOf("\"error\":");
    if (errorIdx >= 0) {
      int startQuote = responseBody.indexOf('"', errorIdx + 8);
      int endQuote = responseBody.indexOf('"', startQuote + 1);
      if (startQuote >= 0 && endQuote > startQuote) {
        String errorMsg = responseBody.substring(startQuote + 1, endQuote);
        _lastError = errorMsg;
        WAAVIS_LOG("[waavis] error: " + errorMsg);
      }
    }
  }

  if (status < 200 || status >= 300) {
    if (_lastError.length() == 0) {
      _lastError = "HTTP " + String(status);
    }
    return false;
  }

  _lastError = "";
  WAAVIS_LOG("[waavis] chunked upload ok");
  return true;
}

bool WaavisClient::sendPost(const String &path, const String &token, const String &body) {
  if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected";
    return false;
  }

  String url = _baseUrl + path;

#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  if (_sslCert != nullptr) {
    BearSSL::X509List cert(_sslCert);
    client.setTrustAnchors(&cert);
  } else if (_insecure) {
    client.setInsecure();
  }
  HTTPClient http;
  if (!http.begin(client, url)) {
    _lastError = "HTTP begin failed";
    return false;
  }
#elif defined(ESP32)
  WiFiClientSecure client;
  if (_sslCert != nullptr) {
    client.setCACert(_sslCert);
  } else if (_insecure) {
    client.setInsecure();
  }
  HTTPClient http;
  if (!http.begin(client, url)) {
    _lastError = "HTTP begin failed";
    return false;
  }
#endif

  http.addHeader("Authorization", token);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(body);
  if (httpCode <= 0) {
    _lastError = http.errorToString(httpCode);
    http.end();
    return false;
  }

  if (httpCode < 200 || httpCode >= 300) {
    String response = http.getString();
    WAAVIS_LOG("[waavis] response: " + response);
    
    // Extract error from JSON
    int errorIdx = response.indexOf("\"error\":");
    if (errorIdx >= 0) {
      int startQuote = response.indexOf('"', errorIdx + 8);
      int endQuote = response.indexOf('"', startQuote + 1);
      if (startQuote >= 0 && endQuote > startQuote) {
        _lastError = response.substring(startQuote + 1, endQuote);
      } else {
        _lastError = "HTTP " + String(httpCode);
      }
    } else {
      _lastError = "HTTP " + String(httpCode);
    }
    http.end();
    return false;
  }

  _lastError = "";
  http.end();
  return true;
}

String WaavisClient::urlEncode(const String &value) const {
  String encoded = "";
  const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else {
      encoded += '%';
      encoded += hex[(c >> 4) & 0x0F];
      encoded += hex[c & 0x0F];
    }
  }
  return encoded;
}

void WaavisClient::listSPIFFSFiles() {
#if defined(ESP32)
  if (!SPIFFS.begin(true)) {
    WAAVIS_LOG("[waavis] SPIFFS Mount Failed");
    return;
  }
  WAAVIS_LOG("[waavis] Listing SPIFFS files:");
  File root = SPIFFS.open("/");
  if(!root){
      WAAVIS_LOG("[waavis] Failed to open root directory");
      return;
  }
  File file = root.openNextFile();
  while(file){
      String fileName = file.name();
      size_t fileSize = file.size();
      WAAVIS_LOG("  FILE: " + fileName + "  SIZE: " + String(fileSize));
      file = root.openNextFile();
  }
  WAAVIS_LOG("[waavis] End of list");
#else
  WAAVIS_LOG("[waavis] listSPIFFSFiles only supported on ESP32 in this version");
#endif
}
