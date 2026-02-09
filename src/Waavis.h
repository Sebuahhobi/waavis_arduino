<<<<<<< HEAD
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
  String lastError() const;

private:
  String _baseUrl;
  bool _insecure;
  String _lastError;

  bool sendPost(const String &path, const String &token, const String &body);
  String urlEncode(const String &value) const;
};

#endif
=======
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
  String lastError() const;

private:
  String _baseUrl;
  bool _insecure;
  String _lastError;

  bool sendPost(const String &path, const String &token, const String &body);
  String urlEncode(const String &value) const;
};

#endif
>>>>>>> 6970986 (add sendChat with methode POST and sendChatLink)
