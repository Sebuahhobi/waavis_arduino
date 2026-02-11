#include "Waavis.h"

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#error "Waavis library supports ESP8266 and ESP32 only."
#endif

WaavisClient::WaavisClient(const String &baseUrl)
    : _baseUrl(baseUrl), _insecure(true), _lastError("") {}

void WaavisClient::setInsecure(bool insecure) {
  _insecure = insecure;
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
    _lastError = "HTTP " + String(httpCode);
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
                                        const String &message, bool typing,
                                        const String &type, const String &fileUrl,
                                        const String &fileName) {
  if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected";
    return false;
  }

  bool isHttps = fileUrl.startsWith("https://");

#if defined(ESP8266)
  HTTPClient httpFile;
  if (isHttps) {
    BearSSL::WiFiClientSecure client;
    if (_insecure) {
      client.setInsecure();
    }
    if (!httpFile.begin(client, fileUrl)) {
      _lastError = "HTTP begin failed";
      return false;
    }
  } else {
    WiFiClient client;
    if (!httpFile.begin(client, fileUrl)) {
      _lastError = "HTTP begin failed";
      return false;
    }
  }
#elif defined(ESP32)
  HTTPClient httpFile;
  if (isHttps) {
    WiFiClientSecure client;
    if (_insecure) {
      client.setInsecure();
    }
    if (!httpFile.begin(client, fileUrl)) {
      _lastError = "HTTP begin failed";
      return false;
    }
  } else {
    WiFiClient client;
    if (!httpFile.begin(client, fileUrl)) {
      _lastError = "HTTP begin failed";
      return false;
    }
  }
#endif

  int httpCode = httpFile.GET();
  if (httpCode <= 0) {
    _lastError = httpFile.errorToString(httpCode);
    httpFile.end();
    return false;
  }

  if (httpCode < 200 || httpCode >= 300) {
    _lastError = "HTTP " + String(httpCode);
    httpFile.end();
    return false;
  }

  int size = httpFile.getSize();
  if (size <= 0) {
    _lastError = "File size unknown";
    httpFile.end();
    return false;
  }

  String finalName = fileName;
  if (finalName.length() == 0) {
    int qIndex = fileUrl.indexOf('?');
    String cleanUrl = (qIndex >= 0) ? fileUrl.substring(0, qIndex) : fileUrl;
    int slashIndex = cleanUrl.lastIndexOf('/');
    if (slashIndex >= 0 && slashIndex + 1 < static_cast<int>(cleanUrl.length())) {
      finalName = cleanUrl.substring(slashIndex + 1);
    } else {
      finalName = "file";
    }
  }

  Stream &stream = *httpFile.getStreamPtr();
  bool ok = sendChatMediaStream(token, to, message, typing, type, stream,
                                static_cast<size_t>(size), finalName);
  httpFile.end();
  return ok;
}

bool WaavisClient::sendChatMediaStream(const String &token, const String &to,
                                       const String &message, bool typing,
                                       const String &type, Stream &file,
                                       size_t fileSize, const String &fileName) {
  if (WiFi.status() != WL_CONNECTED) {
    _lastError = "WiFi not connected";
    return false;
  }

  if (fileSize == 0) {
    _lastError = "File is empty";
    return false;
  }

  String boundary = "----WaavisBoundary" + String(millis());
  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"to\"\r\n\r\n" + to + "\r\n";
  head += "Content-Disposition: form-data; name=\"message\"\r\n\r\n" + message + "\r\n";
  head += "Content-Disposition: form-data; name=\"typing\"\r\n\r\n" +
          String(typing ? "true" : "false") + "\r\n";
  head += "Content-Disposition: form-data; name=\"type\"\r\n\r\n" + type + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; filename=\"" +
          fileName + "\"\r\n";
  head += "Content-Type: application/octet-stream\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";
  size_t contentLength = head.length() + fileSize + tail.length();

#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  if (_insecure) {
    client.setInsecure();
  }
  HTTPClient http;
  if (!http.begin(client, _baseUrl + "/v1/send_chat_media")) {
    _lastError = "HTTP begin failed";
    return false;
  }
#elif defined(ESP32)
  WiFiClientSecure client;
  if (_insecure) {
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
    MultipartStream(const String &head, Stream &file, size_t fileSize,
                    const String &tail)
        : _head(head), _file(file), _fileSize(fileSize), _tail(tail),
          _headPos(0), _filePos(0), _tailPos(0) {}

    int available() override {
      return static_cast<int>((_head.length() - _headPos) +
                              (_fileSize - _filePos) +
                              (_tail.length() - _tailPos));
    }

    int read() override {
      if (_headPos < _head.length()) {
        return _head[_headPos++];
      }
      if (_filePos < _fileSize) {
        int c = _file.read();
        if (c < 0) {
          return -1;
        }
        _filePos++;
        return c;
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
    Stream &_file;
    size_t _fileSize;
    const String &_tail;
    size_t _headPos;
    size_t _filePos;
    size_t _tailPos;
  };

  MultipartStream body(head, file, fileSize, tail);
  int httpCode = http.sendRequest("POST", &body, contentLength);
  if (httpCode <= 0) {
    _lastError = http.errorToString(httpCode);
    http.end();
    return false;
  }

  if (httpCode < 200 || httpCode >= 300) {
    _lastError = "HTTP " + String(httpCode);
    http.end();
    return false;
  }

  _lastError = "";
  http.end();
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

  http.addHeader("Authorization", token);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(body);
  if (httpCode <= 0) {
    _lastError = http.errorToString(httpCode);
    http.end();
    return false;
  }

  if (httpCode < 200 || httpCode >= 300) {
    _lastError = "HTTP " + String(httpCode);
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
