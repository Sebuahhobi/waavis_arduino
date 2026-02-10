# Waavis Arduino Library

Library Arduino untuk mengirim pesan WhatsApp melalui Waavis API menggunakan metode GET dan POST.

## Fitur

- Mengirim chat sederhana dengan endpoint `send_chat`
- Mengirim chat dengan metode POST (typing)
- Mengirim chat dengan link
- Mengirim chat media (gambar, video, dokumen)
- Mendukung ESP8266 dan ESP32
- URL encoding otomatis untuk parameter query

## Struktur

- `src/Waavis.h` dan `src/Waavis.cpp`
- `examples/waavis.ino`
- `library.properties`

## Instalasi

- Pindahkan folder `waavis` ke folder library Arduino Anda.
- Buka Arduino IDE, lalu `Sketch` -> `Include Library` -> `waavis`.

## Contoh Pemakaian

Buka contoh di `examples/waavis.ino`, lalu isi:

- SSID dan password WiFi
- `DEVICE_TOKEN`
- Nomor tujuan (format internasional, contoh `628xxxxxx`)

Endpoint yang dipakai:

```
https://api.waavis.com/v1/send_chat?token=DEVICE_TOKEN&to=628xxxxxx&message=Halo
```

Untuk metode POST:

```
POST https://api.waavis.com/v1/send_chat
Authorization: DEVICE_TOKEN
Content-Type: application/x-www-form-urlencoded

to=628xxxxxx&message=Halo&typing=false
```

Untuk send chat dengan link:

```
POST https://api.waavis.com/v1/send_chat_link
Authorization: DEVICE_TOKEN
Content-Type: application/x-www-form-urlencoded

to=628xxxxxx&message=Halo&typing=false&link=https%3A%2F%2Fexample.com&link_title=Judul&link_description=Deskripsi
```

Untuk send chat media:

```
POST https://api.waavis.com/v1/send_chat_media
Authorization: DEVICE_TOKEN
Content-Type: multipart/form-data; boundary=----boundary

to=628xxxxxx
message=Halo
typing=false
type=image
file=@image.jpg
```

## Cara Pakai di Kode

```cpp
#include <Waavis.h>

WaavisClient waavis;

bool ok = waavis.sendChat("DEVICE_TOKEN", "628xxxxxx", "Halo");
if (!ok) {
  Serial.println(waavis.lastError());
}
```

Contoh `sendChatPost`:

```cpp
#include <Waavis.h>

WaavisClient waavis;

bool ok = waavis.sendChatPost("DEVICE_TOKEN", "628xxxxxx", "Halo", false);
if (!ok) {
  Serial.println(waavis.lastError());
}
```

Contoh `sendChatLink`:

```cpp
#include <Waavis.h>

WaavisClient waavis;

bool ok = waavis.sendChatLink(
  "DEVICE_TOKEN",
  "628xxxxxx",
  "Halo",
  false,
  "https://example.com",
  "Judul",
  "Deskripsi"
);
if (!ok) {
  Serial.println(waavis.lastError());
}
```

Contoh `sendChatMedia`:

```cpp
#include <Waavis.h>
#include <SPIFFS.h>

WaavisClient waavis;

File file = SPIFFS.open("/image.jpg", "r");
if (!file) {
  Serial.println("File open failed");
  return;
}

bool ok = waavis.sendChatMedia(
  "DEVICE_TOKEN",
  "628xxxxxx",
  "Halo",
  false,
  "image",
  file,
  file.size(),
  "image.jpg"
);
if (!ok) {
  Serial.println(waavis.lastError());
}
```

## Catatan Keamanan

Library menggunakan koneksi HTTPS dengan mode `setInsecure()` secara default agar mudah dipakai.

## Dukungan Board

- ESP8266
- ESP32

## Lisensi

MIT License. Lihat file LICENSE.
