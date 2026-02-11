#ifndef WAAVIS_H
#define WAAVIS_H

#include <Arduino.h>

class WaavisClient {
public:
  explicit WaavisClient(const String &baseUrl = "https://api.waavis.com");
  void setInsecure(bool insecure);
  void setCertificate(const char* cert);
  bool sendChat(const String &token, const String &to, const String &message);
  bool sendChatPost(const String &token, const String &to, const String &message,
                    bool typing = false);
  bool sendChatLink(const String &token, const String &to, const String &message,
                    bool typing, const String &link, const String &linkTitle,
                    const String &linkDescription);
  bool sendChatMedia(const String &token, const String &to, const String &message,
                     bool typing, const String &type, Stream &file,
                     size_t fileSize, const String &fileName);
  bool sendChatMediaFromUrl(const String &token, const String &to,
                            const String &caption, bool typing,
                            const String &imageUrl);
#if defined(ESP32)
  void listSPIFFSFiles();
#endif
  String lastError() const;

private:
  String _baseUrl;
  bool _insecure;
  const char* _sslCert;
  String _lastError;

  bool sendPost(const String &path, const String &token, const String &body);
  bool sendChatMediaStream(const String &token, const String &to,
                           const String &message, bool typing,
                           const String &type, Stream &file,
                           size_t fileSize, const String &fileName);
  bool sendChatMediaBuffer(const String &token, const String &to,
                           const String &message, bool typing,
                           const String &type, const uint8_t *data,
                           size_t dataSize, const String &fileName);
  bool sendChatMediaStreamChunked(const String &token, const String &to,
                                  const String &message, bool typing,
                                  const String &type, Stream &file,
                                  const String &fileName);
  String urlEncode(const String &value) const;
};

#endif