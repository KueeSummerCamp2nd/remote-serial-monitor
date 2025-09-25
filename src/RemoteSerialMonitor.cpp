#include "RemoteSerialMonitor.h"

// ---------------- HTML（最小ビュー） ----------------
static const char RSM_PAGE_INDEX[] PROGMEM =
R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name=viewport content="width=device-width,initial-scale=1">
<title>RemoteSerialMonitor</title>
<style>
body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;margin:16px}
#log{white-space:pre-wrap;border:1px solid #ddd;border-radius:8px;padding:12px;min-height:60vh;overflow:auto}
h1{font-size:1.2rem;margin:.2rem 0 .6rem}
.small{color:#666;font-size:.9rem}
</style></head><body>
<h1>RemoteSerialMonitor</h1>
<div class=small>Auto-refreshing… Open from multiple devices if you want.</div>
<div id=log></div>
<script>
let since = 0;
const box = document.getElementById('log');
async function tick(){
  try{
    const r = await fetch('/api/log?since=' + since);
    if(r.ok){
      const j = await r.json();
      // 追記
      if (j && j.lines && j.lines.length){
        const atBottom = Math.abs(box.scrollHeight - box.scrollTop - box.clientHeight) < 4;
        for(const line of j.lines){
          const div = document.createElement('div');
          div.textContent = line;
          box.appendChild(div);
        }
        if (atBottom) box.scrollTop = box.scrollHeight;
        since = j.last_id || since;
      }
    }
  }catch(e){}
  setTimeout(tick, 500);
}
tick();
</script></body></html>)HTML";

// ---------------- 実装 ----------------
RemoteSerialMonitor RemoteSerial; // デフォルト

RemoteSerialMonitor::RemoteSerialMonitor(uint16_t port)
: server_(port), head_(0), count_(0), next_id_(1), pending_len_(0) {
    for (uint16_t i=0;i<RSM_MAX_LINES;++i){
        buf_[i][0] = '\0';
        id_[i] = 0;
    }
    pending_[0] = '\0';
}

bool RemoteSerialMonitor::beginAP(const char* ssid, const char* pass){
    uint8_t st = WiFi.beginAP(ssid, pass);
    return (st == WL_AP_LISTENING || st == WL_CONNECTED);
}

bool RemoteSerialMonitor::beginSTA(const char* ssid, const char* pass, uint32_t timeout_ms){
    WiFi.disconnect();
    WiFi.end();
    WiFi.setTimeout(timeout_ms);
    return (WiFi.begin(ssid, pass) == WL_CONNECTED);
}

IPAddress RemoteSerialMonitor::localIP() const {
    return WiFi.localIP();
}

void RemoteSerialMonitor::beginServer(){
    server_.begin();
}

void RemoteSerialMonitor::handle(){
    WiFiClient client = server_.available();
    if (!client) return;

    // 1行目（GET …）だけ読む
    char line[160];
    if (!readLine(client, line, sizeof(line))) { client.stop(); return; }

    route(client, line);

    delay(1);
    client.stop();
}

void RemoteSerialMonitor::print(const char* s){
    appendToPending(s);
    commitPendingIfNeeded(); // 文字列に \n が含まれていれば行として確定
}

void RemoteSerialMonitor::println(const char* s){
    appendToPending(s);
    appendToPending("\n");
    commitPendingIfNeeded();
}

void RemoteSerialMonitor::printf(const char* fmt, ...){
    char tmp[RSM_MAX_COLS];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    print(tmp);
}

// ---- 内部：HTTP ----
bool RemoteSerialMonitor::readLine(WiFiClient &c, char *buf, size_t n){
    size_t i=0; unsigned long t=millis();
    while (millis() - t < 1000){
        if (c.available()){
            char ch = c.read();
            if (ch=='\r') continue;
            if (ch=='\n'){ buf[i]=0; return true; }
            if (i+1 < n) buf[i++]=ch;
        }
    }
    buf[i]=0; return (i>0);
}

void RemoteSerialMonitor::sendResp(WiFiClient &c, const char* content, const char* type){
    char head[180];
    size_t len = strlen_P(content); // PROGMEM対応
    int m = snprintf(head, sizeof(head),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n", type, (unsigned)len);
    c.write((const uint8_t*)head, m);

    // PROGMEM の場合に対応
    char chunk[128];
    size_t sent = 0;
    while (sent < len){
        size_t n = min(sizeof(chunk)-1, len - sent);
        memcpy_P(chunk, content + sent, n);
        c.write((const uint8_t*)chunk, n);
        sent += n;
    }
}

void RemoteSerialMonitor::route(WiFiClient &c, const char* reqline){
    // ヘッダ残りを捨てる
    while (c.connected() && c.available()) c.read();

    if (startsWith(reqline, "GET /api/log")){
        uint32_t since = 0;
        findParamSince(reqline, since);
        serveLog(c, since);
    } else { // index
        serveIndex(c);
    }
}

void RemoteSerialMonitor::serveIndex(WiFiClient &c){
    sendResp(c, RSM_PAGE_INDEX, "text/html");
}

void RemoteSerialMonitor::serveLog(WiFiClient &c, uint32_t since){
    // Content-Length を付けずに close で終端（HTTP/1.1でも大抵OK）
    const char* head =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n";
    c.write((const uint8_t*)head, strlen(head));

    char tmp[48];
    int m = snprintf(tmp, sizeof(tmp),
                     "{\"last_id\":%lu,\"lines\":[",
                     (unsigned long)(next_id_ ? next_id_-1 : 0));
    c.write((const uint8_t*)tmp, m);

    bool first = true;
    uint16_t n = count_;
    uint16_t start = (count_ < RSM_MAX_LINES) ? 0 : head_;
    for (uint16_t k=0; k<n; ++k){
        uint16_t idx = (start + k) % RSM_MAX_LINES;
        if (id_[idx] <= since) continue;

        if (!first) c.write((const uint8_t*)",", 1);
        first = false;

        c.write((const uint8_t*)"\"", 1);
        const char* s = buf_[idx];
        for (; *s; ++s){
            char ch = *s;
            if (ch == '\\' || ch == '\"'){ c.write((const uint8_t*)"\\",1); c.write((const uint8_t*)&ch,1); }
            else if (ch == '\n'){ c.write((const uint8_t*)"\\n",2); }
            else { c.write((const uint8_t*)&ch,1); }
        }
        c.write((const uint8_t*)"\"", 1);
    }
    c.write((const uint8_t*)"]}", 2);
}

// ---- 内部：ログ ----
void RemoteSerialMonitor::pushLine(const char* line){
    // 空行でも受ける（UI上は空行として表示）
    uint16_t slot = head_;
    // 切り詰めコピー
    size_t L = strnlen(line, RSM_MAX_COLS - 1);
    memcpy(buf_[slot], line, L);
    buf_[slot][L] = '\0';

    id_[slot] = next_id_++;
    head_ = (head_ + 1) % RSM_MAX_LINES;
    if (count_ < RSM_MAX_LINES) ++count_;
}

void RemoteSerialMonitor::appendToPending(const char* s){
    while (*s){
        char ch = *s++;
        if (ch == '\n'){
            // これまでを1行として確定
            pending_[pending_len_] = '\0';
            pushLine(pending_);
            pending_len_ = 0;
            pending_[0] = '\0';
        } else {
            if (pending_len_ + 1 < RSM_MAX_COLS){
                pending_[pending_len_++] = ch;
            } else {
                // 行が長すぎる場合は強制確定して次へ
                pending_[pending_len_] = '\0';
                pushLine(pending_);
                pending_len_ = 0;
                pending_[pending_len_++] = ch;
            }
        }
    }
}

void RemoteSerialMonitor::commitPendingIfNeeded(){
    // printだけでは確定しない。必要ならここを有効化して「都度確定」もできる。
    // （現在はprintlnまたは'\n'で確定）
}

bool RemoteSerialMonitor::startsWith(const char* s, const char* prefix){
    while (*prefix){
        if (*s++ != *prefix++) return false;
    }
    return true;
}

int RemoteSerialMonitor::findParamSince(const char* reqline, uint32_t &since_out){
    const char* q = strstr(reqline, "?");
    if (!q) return 0;
    const char* p = strstr(q, "since=");
    if (!p) return 0;
    p += 6;
    since_out = (uint32_t)strtoul(p, nullptr, 10);
    return 1;
}
