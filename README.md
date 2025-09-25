# RemoteSerialMonitor

Arduino UNO R4 WiFi（WiFiS3）向けの**超軽量リモート・シリアルモニタ**です。  
`Serial.print()` 互換のAPI（`print / println / printf`）で出力した文字列を、**Wi‑Fiアクセスポイント経由でブラウザにリアルタイム表示**します。

- CPU/メモリに優しい**シンプルHTTP + ポーリング**方式（SSE/WSなし）
- **リングバッファ**（既定：行×列=24×96 など）に保存
- **差分取得 API**：`GET /api/log?since=<last_id>`
- インデックスページ `/` を同梱（即見える）
- UNO R4 WiFi + **WiFiS3** で動作（他ボードは未検証）
---

## 目次
- [RemoteSerialMonitor](#remoteserialmonitor)
  - [目次](#目次)
  - [クイックスタート](#クイックスタート)
  - [インストール](#インストール)
  - [API](#api)
    - [使い方の流れ](#使い方の流れ)
  - [HTTP エンドポイント](#http-エンドポイント)
  - [メモリとサイズ調整](#メモリとサイズ調整)
  - [例: BasicUsage](#例-basicusage)

---

## クイックスタート

1. ライブラリを `Documents/Arduino/libraries/RemoteSerialMonitor/` に配置します（構成例は後述）。  
2. 例スケッチを書き込み、**UNO R4 WiFi** を起動。
3. PC/スマホで SSID に接続 → ブラウザで `http://192.168.4.1/` を開く。
4. ログが自動で流れます。

---

## インストール

```
RemoteSerialMonitor/
 ├─ library.properties           （任意）
 ├─ src/
 │   ├─ RemoteSerialMonitor.h
 │   └─ RemoteSerialMonitor.cpp
 └─ examples/
     └─ BasicUsage/
         └─ BasicUsage.ino
```

- Arduino IDE の **スケッチブックフォルダ** 内 `libraries` 直下に `RemoteSerialMonitor` フォルダごと置いてください。
- 既存の `WiFiS3` ライブラリ（UNO R4 WiFi 標準）に依存します。

---

## API

```cpp
class RemoteSerialMonitor {
public:
  explicit RemoteSerialMonitor(uint16_t port = 80);

  // Wi-Fi
  bool beginAP (const char* ssid, const char* pass);
  bool beginSTA(const char* ssid, const char* pass, uint32_t timeout_ms = 15000);
  IPAddress localIP() const;

  // HTTPサーバ
  void beginServer();
  void handle();           // loop() から頻繁に呼ぶ

  // 出力（Serial互換）
  void print  (const char* s);
  void println(const char* s);
  void printf (const char* fmt, ...);

  // リングバッファ容量（コンパイル定数）
  uint16_t capacityLines() const; // 例：24 or 32 など
  uint16_t capacityCols () const; // 例：96
};

// 使いやすいグローバルインスタンス
extern RemoteSerialMonitor RemoteSerial;
```

### 使い方の流れ
1. `RemoteSerial.beginAP(ssid, pass);`
2. `RemoteSerial.beginServer();`
3. `loop()` 内で `RemoteSerial.handle();` を**毎回呼ぶ**
4. 出したい箇所で `RemoteSerial.println("message");` など

---

## HTTP エンドポイント

- `GET /`  
  組み込みのインデックスページ（ログビューア）を返します。

- `GET /api/log?since=<id>`  
  `id` より新しい行を JSON で返します。クライアントは、サーバから受け取った `last_id` を保持してポーリングします。  
  例レスポンス：
  ```json
  {"last_id": 42, "lines": ["boot ok","A0=512","..."]}
  ```

---

## メモリとサイズ調整

UNO R4 WiFi の SRAM は約 **32 KB** です。ライブラリはメモリ節約を重視していますが、**用途に合わせて以下のマクロで調整**できます。

```cpp
// RemoteSerialMonitor.h をインクルードする前に定義すると上書き可能
#define RSM_MAX_LINES 24   // 保存行数（リングバッファ）
#define RSM_MAX_COLS  96   // 1行最大長（UTF-8, 超過は切り捨て）
#include "RemoteSerialMonitor.h"
```

目安：  
- 24×96 ≈ **2.3 KB**（文字） + ID配列等 ≈ **少量**  
- 32×96 ≈ **3.2 KB**  
- 64×128 は約 **8 KB 以上**になり、他の配列と衝突しやすいです。
---

## 例: BasicUsage

```cpp
#include <Arduino.h>
#include <WiFiS3.h>

// 必要ならここで上書き可能（例: さらに省メモリ化）
// #define RSM_MAX_LINES 24
// #define RSM_MAX_COLS  96
#include "RemoteSerialMonitor.h"

const char* AP_SSID = "ArduinoR4_AP";
const char* AP_PASS = "arduino123"; // 8文字以上

void setup(){
  Serial.begin(115200);
  delay(300);

  if (!RemoteSerial.beginAP(AP_SSID, AP_PASS)) {
    Serial.println(F("AP start failed"));
    while (1) { delay(500); }
  }
  RemoteSerial.beginServer();

  Serial.print(F("AP SSID: ")); Serial.println(AP_SSID);
  Serial.print(F("AP PASS: ")); Serial.println(AP_PASS);
  Serial.print(F("AP IP  : ")); Serial.println(RemoteSerial.localIP());

  RemoteSerial.println("RemoteSerialMonitor started");
  RemoteSerial.printf("Open: http://%s/\n", RemoteSerial.localIP().toString().c_str());
}

void loop(){
  RemoteSerial.handle();  // 必須

  // デモ：1秒ごとにA0の値を配信
  static uint32_t t0 = millis();
  if (millis() - t0 > 1000){
    t0 += 1000;
    int v = analogRead(A0);
    RemoteSerial.printf("A0=%d\n", v);
  }
}
```

<!-- ## 設計メモ

- **通信方式**：HTTP/1.1 + 簡易ポーリング（SSE/WSよりRAM使用量が小さく、実装が堅牢）。
- **Content-Length**：
  - HTMLは正確な `Content-Length` を送信（PROGMEM からチャンクコピー）。
  - JSONは **チャンク状にストリーミング**し、`Connection: close` で終端。
- **文字コード**：UTF‑8（1行最大長 `RSM_MAX_COLS`、超過は切り捨て）。
- **CORS**：`Access-Control-Allow-Origin: *` を付与。外部ページからフェッチする用途も可能。
- **セキュリティ**：SoftAP のパスワードは **8文字以上**。公開環境では使わないでください。

--- -->
