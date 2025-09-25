#pragma once
#include <Arduino.h>
#include <WiFiS3.h>

#ifndef RSM_MAX_LINES
#define RSM_MAX_LINES 64      // 保存行数（リングバッファ）
#endif
#ifndef RSM_MAX_COLS
#define RSM_MAX_COLS 128      // 1行あたり最大長（UTF-8前提、超過は切り捨て）
#endif

class RemoteSerialMonitor {
public:
    RemoteSerialMonitor(uint16_t port = 80);

    // --- WiFi ---
    // APとして起動
    bool beginAP(const char* ssid, const char* pass);
    // STAとして既存APに接続（必要なら）
    bool beginSTA(const char* ssid, const char* pass, uint32_t timeout_ms = 15000);
    IPAddress localIP() const;

    // --- サーバ開始 ---
    void beginServer();

    // --- メインループで必ず呼ぶ ---
    void handle();

    // --- 出力API（Serial.print互換の簡易版） ---
    void print(const char* s);
    void print(const String& s) { print(s.c_str()); }
    void println(const char* s);
    void println(const String& s) { println(s.c_str()); }
    void printf(const char* fmt, ...);

    // バッファサイズ調整（必要ならコンストラクタ定数を書き換えて再ビルドを推奨）
    uint16_t capacityLines() const { return RSM_MAX_LINES; }
    uint16_t capacityCols()  const { return RSM_MAX_COLS;  }

private:
    // HTTP
    bool readLine(WiFiClient &c, char *buf, size_t n);
    void sendResp(WiFiClient &c, const char* content, const char* type="text/html");
    void route(WiFiClient &c, const char* reqline);
    void serveIndex(WiFiClient &c);
    void serveLog(WiFiClient &c, uint32_t since);

    // ログ
    void pushLine(const char* line);
    void appendToPending(const char* s);
    void commitPendingIfNeeded();

    // 文字列ユーティリティ
    static bool startsWith(const char* s, const char* prefix);
    static int  findParamSince(const char* reqline, uint32_t &since_out);

private:
    WiFiServer server_;
    // リングバッファ
    char  buf_[RSM_MAX_LINES][RSM_MAX_COLS];
    uint32_t id_[RSM_MAX_LINES];        // 各行に付与する連番ID
    uint16_t head_;                     // 次に書く位置
    uint16_t count_;                    // 現在の行数（最大RSM_MAX_LINES）
    uint32_t next_id_;                  // 次に割り当てるID

    // println相当のための未確定行
    char pending_[RSM_MAX_COLS];
    uint16_t pending_len_;
};

extern RemoteSerialMonitor RemoteSerial; // デフォルトインスタンス
