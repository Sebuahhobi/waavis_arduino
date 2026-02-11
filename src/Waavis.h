#ifndef WAAVIS_H
#define WAAVIS_H

#include <Arduino.h>

class WaavisClient {
public:
  explicit WaavisClient(const String &baseUrl = "https://api.waavis.com");
  void setInsecure(bool insecure);
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
                            const String &message, bool typing,
                            const String &type, const String &fileUrl,
                            const String &fileName = "");
  String lastError() const;

private:
  String _baseUrl;
  bool _insecure;
  String _lastError;

  bool sendPost(const String &path, const String &token, const String &body);
  bool sendChatMediaStream(const String &token, const String &to,
                           const String &message, bool typing,
                           const String &type, Stream &file,
                           size_t fileSize, const String &fileName);
  String urlEncode(const String &value) const;
};

#endif
