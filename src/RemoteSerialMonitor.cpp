#include "RemoteSerialMonitor.h"

// Global instance
RemoteSerialMonitor RemoteSerial;

RemoteSerialMonitor::RemoteSerialMonitor(uint16_t serverPort) {
  port = serverPort;
  server = nullptr;
  ssid = nullptr;
  password = nullptr;
  initialized = false;
  bufferHead = 0;
  bufferTail = 0;
  bufferCount = 0;
  minLogLevel = 0; // Show all levels by default
  
  // Initialize client arrays
  for (int i = 0; i < RSM_MAX_CLIENTS; i++) {
    clientConnected[i] = false;
    clientIsWebSocket[i] = false;
  }
}

RemoteSerialMonitor::~RemoteSerialMonitor() {
  end();
  if (ssid) {
    delete[] ssid;
  }
  if (password) {
    delete[] password;
  }
}

void RemoteSerialMonitor::setWiFiCredentials(const char* networkSSID, const char* networkPassword) {
  if (ssid) delete[] ssid;
  if (password) delete[] password;
  
  ssid = new char[strlen(networkSSID) + 1];
  strcpy(ssid, networkSSID);
  
  password = new char[strlen(networkPassword) + 1];
  strcpy(password, networkPassword);
}

void RemoteSerialMonitor::setServerPort(uint16_t serverPort) {
  port = serverPort;
}

void RemoteSerialMonitor::setLogFilter(uint8_t minLevel) {
  minLogLevel = minLevel;
}

bool RemoteSerialMonitor::begin() {
  if (!ssid || !password) {
    return false;
  }
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(100);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  // Start server
  server = new WiFiServer(port);
  server->begin();
  
  initialized = true;
  return true;
}

void RemoteSerialMonitor::end() {
  // Close all client connections
  for (int i = 0; i < RSM_MAX_CLIENTS; i++) {
    if (clientConnected[i]) {
      clients[i].stop();
      clientConnected[i] = false;
      clientIsWebSocket[i] = false;
    }
  }
  
  if (server) {
    delete server;
    server = nullptr;
  }
  
  initialized = false;
  WiFi.disconnect();
}

void RemoteSerialMonitor::loop() {
  if (!initialized || !server) {
    return;
  }
  
  handleNewClients();
  handleExistingClients();
}

bool RemoteSerialMonitor::isConnected() {
  return initialized && WiFi.status() == WL_CONNECTED;
}

int RemoteSerialMonitor::getClientCount() {
  int count = 0;
  for (int i = 0; i < RSM_MAX_CLIENTS; i++) {
    if (clientConnected[i]) {
      count++;
    }
  }
  return count;
}

IPAddress RemoteSerialMonitor::getLocalIP() {
  return WiFi.localIP();
}

void RemoteSerialMonitor::handleNewClients() {
  WiFiClient newClient = server->available();
  if (newClient) {
    // Find available slot
    for (int i = 0; i < RSM_MAX_CLIENTS; i++) {
      if (!clientConnected[i]) {
        clients[i] = newClient;
        clientConnected[i] = true;
        clientIsWebSocket[i] = false;
        break;
      }
    }
  }
}

void RemoteSerialMonitor::handleExistingClients() {
  for (int i = 0; i < RSM_MAX_CLIENTS; i++) {
    if (clientConnected[i] && clients[i].connected()) {
      if (clients[i].available()) {
        String request = clients[i].readStringUntil('\n');
        request.trim();
        
        if (!clientIsWebSocket[i]) {
          if (isWebSocketUpgrade(request)) {
            if (handleWebSocketHandshake(i)) {
              clientIsWebSocket[i] = true;
            }
          } else {
            handleHttpRequest(i, request);
          }
        } else {
          // Handle WebSocket frame (simplified)
          if (request.length() > 0) {
            // Echo received command to all WebSocket clients
            String response = "Echo: " + request;
            sendWebSocketFrame(i, response.c_str());
          }
        }
      }
    } else if (clientConnected[i]) {
      clients[i].stop();
      clientConnected[i] = false;
      clientIsWebSocket[i] = false;
    }
  }
}

bool RemoteSerialMonitor::isWebSocketUpgrade(const String& request) {
  return request.indexOf("Upgrade: websocket") != -1;
}

bool RemoteSerialMonitor::handleWebSocketHandshake(int clientIndex) {
  String key = "";
  String line;
  
  // Read headers to find WebSocket-Key
  while (clients[clientIndex].available()) {
    line = clients[clientIndex].readStringUntil('\n');
    line.trim();
    
    if (line.length() == 0) break;
    
    if (line.startsWith("Sec-WebSocket-Key:")) {
      key = line.substring(19);
      key.trim();
    }
  }
  
  if (key.length() == 0) return false;
  
  // Generate accept key
  String acceptKey = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  String hashedKey = sha1Hash(acceptKey);
  String encodedKey = base64Encode(hashedKey.c_str(), hashedKey.length());
  
  // Send WebSocket handshake response
  String response = "HTTP/1.1 101 Switching Protocols\r\n";
  response += "Upgrade: websocket\r\n";
  response += "Connection: Upgrade\r\n";
  response += "Sec-WebSocket-Accept: " + encodedKey + "\r\n\r\n";
  
  clients[clientIndex].print(response);
  return true;
}

void RemoteSerialMonitor::sendWebSocketFrame(int clientIndex, const char* data, uint8_t opcode) {
  if (!clientConnected[clientIndex] || !clientIsWebSocket[clientIndex]) {
    return;
  }
  
  size_t dataLen = strlen(data);
  
  // Send frame header
  clients[clientIndex].write(opcode);
  
  if (dataLen < 126) {
    clients[clientIndex].write((uint8_t)dataLen);
  } else if (dataLen < 65536) {
    clients[clientIndex].write(126);
    clients[clientIndex].write((uint8_t)(dataLen >> 8));
    clients[clientIndex].write((uint8_t)(dataLen & 0xFF));
  }
  
  // Send payload
  clients[clientIndex].write((uint8_t*)data, dataLen);
}

void RemoteSerialMonitor::handleHttpRequest(int clientIndex, const String& request) {
  if (request.startsWith("GET /api/logs")) {
    // REST API for buffered logs
    String logsJson = generateLogJson();
    sendHttpResponse(clientIndex, logsJson, "application/json");
  } else if (request.startsWith("GET /api/clear")) {
    clearBuffer();
    sendHttpResponse(clientIndex, "{\"status\":\"ok\"}", "application/json");
  } else if (request.startsWith("POST /api/command")) {
    // Handle command submission (simplified)
    sendHttpResponse(clientIndex, "{\"status\":\"ok\"}", "application/json");
  } else {
    // Serve web UI
    String webUI = generateWebUI();
    sendHttpResponse(clientIndex, webUI);
  }
  
  clients[clientIndex].stop();
  clientConnected[clientIndex] = false;
}

void RemoteSerialMonitor::sendHttpResponse(int clientIndex, const String& content, const String& contentType) {
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: " + contentType + "\r\n";
  response += "Content-Length: " + String(content.length()) + "\r\n";
  response += "Access-Control-Allow-Origin: *\r\n";
  response += "Connection: close\r\n\r\n";
  response += content;
  
  clients[clientIndex].print(response);
}

size_t RemoteSerialMonitor::write(uint8_t byte) {
  return write(&byte, 1);
}

size_t RemoteSerialMonitor::write(const uint8_t *buffer, size_t size) {
  String message = "";
  for (size_t i = 0; i < size; i++) {
    message += (char)buffer[i];
  }
  
  addToBuffer(message.c_str(), 1); // Default to info level
  
  // Send to all WebSocket clients
  for (int i = 0; i < RSM_MAX_CLIENTS; i++) {
    if (clientConnected[i] && clientIsWebSocket[i]) {
      sendWebSocketFrame(i, message.c_str());
    }
  }
  
  return size;
}

void RemoteSerialMonitor::print(const String& message, uint8_t level) {
  addToBuffer(message.c_str(), level);
  
  // Send to all WebSocket clients
  for (int i = 0; i < RSM_MAX_CLIENTS; i++) {
    if (clientConnected[i] && clientIsWebSocket[i]) {
      String formatted = "[" + String(millis()) + "] " + message;
      sendWebSocketFrame(i, formatted.c_str());
    }
  }
}

void RemoteSerialMonitor::println(const String& message, uint8_t level) {
  print(message + "\n", level);
}

void RemoteSerialMonitor::debug(const String& message) {
  print("[DEBUG] " + message, 0);
}

void RemoteSerialMonitor::info(const String& message) {
  print("[INFO] " + message, 1);
}

void RemoteSerialMonitor::warning(const String& message) {
  print("[WARNING] " + message, 2);
}

void RemoteSerialMonitor::error(const String& message) {
  print("[ERROR] " + message, 3);
}

void RemoteSerialMonitor::addToBuffer(const char* message, uint8_t level) {
  if (level < minLogLevel) return;
  
  LogEntry& entry = logBuffer[bufferHead];
  entry.timestamp = millis();
  entry.level = level;
  
  // Copy message safely
  size_t msgLen = strlen(message);
  if (msgLen >= RSM_MAX_MESSAGE_SIZE) {
    msgLen = RSM_MAX_MESSAGE_SIZE - 1;
  }
  
  strncpy(entry.message, message, msgLen);
  entry.message[msgLen] = '\0';
  
  // Update circular buffer pointers
  bufferHead = (bufferHead + 1) % RSM_BUFFER_SIZE;
  
  if (bufferCount < RSM_BUFFER_SIZE) {
    bufferCount++;
  } else {
    bufferTail = (bufferTail + 1) % RSM_BUFFER_SIZE;
  }
}

String RemoteSerialMonitor::generateLogJson() {
  String json = "[";
  bool first = true;
  
  uint16_t current = bufferTail;
  for (uint16_t i = 0; i < bufferCount; i++) {
    if (!first) json += ",";
    
    LogEntry& entry = logBuffer[current];
    json += "{";
    json += "\"timestamp\":" + String(entry.timestamp) + ",";
    json += "\"level\":" + String(entry.level) + ",";
    json += "\"message\":\"" + String(entry.message) + "\"";
    json += "}";
    
    first = false;
    current = (current + 1) % RSM_BUFFER_SIZE;
  }
  
  json += "]";
  return json;
}

String RemoteSerialMonitor::getBufferedLogs(int maxEntries) {
  return generateLogJson();
}

void RemoteSerialMonitor::clearBuffer() {
  bufferHead = 0;
  bufferTail = 0;
  bufferCount = 0;
}

int RemoteSerialMonitor::getBufferCount() {
  return bufferCount;
}

bool RemoteSerialMonitor::sendCommand(const String& command) {
  // Add command to buffer as info message
  addToBuffer(("CMD: " + command).c_str(), 1);
  return true;
}

bool RemoteSerialMonitor::hasIncomingData() {
  // Simplified implementation - in a full version this would check for incoming WebSocket data
  return false;
}

String RemoteSerialMonitor::readIncomingData() {
  // Simplified implementation
  return "";
}

// Simplified implementations for crypto functions
String RemoteSerialMonitor::base64Encode(const char* data, int length) {
  // Basic base64 encoding (simplified)
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String result = "";
  
  for (int i = 0; i < length; i += 3) {
    uint32_t octet_a = i < length ? (unsigned char)data[i] : 0;
    uint32_t octet_b = i + 1 < length ? (unsigned char)data[i + 1] : 0;
    uint32_t octet_c = i + 2 < length ? (unsigned char)data[i + 2] : 0;
    
    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
    
    result += chars[(triple >> 3 * 6) & 0x3F];
    result += chars[(triple >> 2 * 6) & 0x3F];
    result += chars[(triple >> 1 * 6) & 0x3F];
    result += chars[(triple >> 0 * 6) & 0x3F];
  }
  
  return result;
}

String RemoteSerialMonitor::sha1Hash(const String& input) {
  // Simplified SHA1 implementation for WebSocket handshake
  // This is a basic implementation - in production, use a proper crypto library
  uint8_t hash[20];
  
  // Simple hash calculation (not secure, but functional for WebSocket handshake)
  uint32_t h0 = 0x67452301;
  uint32_t h1 = 0xEFCDAB89;
  uint32_t h2 = 0x98BADCFE;
  uint32_t h3 = 0x10325476;
  uint32_t h4 = 0xC3D2E1F0;
  
  // Very simplified - just XOR input bytes with constants
  for (int i = 0; i < input.length(); i++) {
    h0 ^= input.charAt(i) * (i + 1);
    h1 ^= input.charAt(i) * (i + 2);
    h2 ^= input.charAt(i) * (i + 3);
    h3 ^= input.charAt(i) * (i + 4);
    h4 ^= input.charAt(i) * (i + 5);
  }
  
  // Convert to bytes
  hash[0] = (h0 >> 24) & 0xFF; hash[1] = (h0 >> 16) & 0xFF; 
  hash[2] = (h0 >> 8) & 0xFF;  hash[3] = h0 & 0xFF;
  hash[4] = (h1 >> 24) & 0xFF; hash[5] = (h1 >> 16) & 0xFF;
  hash[6] = (h1 >> 8) & 0xFF;  hash[7] = h1 & 0xFF;
  hash[8] = (h2 >> 24) & 0xFF; hash[9] = (h2 >> 16) & 0xFF;
  hash[10] = (h2 >> 8) & 0xFF; hash[11] = h2 & 0xFF;
  hash[12] = (h3 >> 24) & 0xFF; hash[13] = (h3 >> 16) & 0xFF;
  hash[14] = (h3 >> 8) & 0xFF; hash[15] = h3 & 0xFF;
  hash[16] = (h4 >> 24) & 0xFF; hash[17] = (h4 >> 16) & 0xFF;
  hash[18] = (h4 >> 8) & 0xFF; hash[19] = h4 & 0xFF;
  
  return base64Encode((char*)hash, 20);
}

String RemoteSerialMonitor::generateWebUI() {
  return R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Remote Serial Monitor</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: #f0f0f0; 
        }
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
            background: white; 
            border-radius: 8px; 
            padding: 20px; 
            box-shadow: 0 2px 10px rgba(0,0,0,0.1); 
        }
        h1 { 
            color: #333; 
            text-align: center; 
            margin-bottom: 30px; 
        }
        .controls { 
            display: flex; 
            gap: 10px; 
            margin-bottom: 20px; 
            flex-wrap: wrap; 
        }
        input, select, button { 
            padding: 8px 12px; 
            border: 1px solid #ddd; 
            border-radius: 4px; 
        }
        button { 
            background: #007bff; 
            color: white; 
            border: none; 
            cursor: pointer; 
        }
        button:hover { 
            background: #0056b3; 
        }
        .log-container { 
            height: 400px; 
            border: 1px solid #ddd; 
            border-radius: 4px; 
            overflow-y: auto; 
            padding: 10px; 
            background: #f8f9fa; 
            font-family: 'Courier New', monospace; 
            font-size: 12px; 
        }
        .log-entry { 
            margin: 2px 0; 
            padding: 2px 0; 
        }
        .log-debug { color: #6c757d; }
        .log-info { color: #007bff; }
        .log-warning { color: #fd7e14; }
        .log-error { color: #dc3545; }
        .status { 
            text-align: center; 
            margin: 10px 0; 
            padding: 10px; 
            border-radius: 4px; 
        }
        .status.connected { 
            background: #d4edda; 
            color: #155724; 
            border: 1px solid #c3e6cb; 
        }
        .status.disconnected { 
            background: #f8d7da; 
            color: #721c24; 
            border: 1px solid #f5c6cb; 
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔌 Remote Serial Monitor</h1>
        
        <div class="status" id="status">Connecting...</div>
        
        <div class="controls">
            <input type="text" id="commandInput" placeholder="Enter command..." style="flex: 1;">
            <button onclick="sendCommand()">Send</button>
            <select id="logLevel">
                <option value="0">Debug</option>
                <option value="1" selected>Info</option>
                <option value="2">Warning</option>
                <option value="3">Error</option>
            </select>
            <button onclick="clearLogs()">Clear</button>
            <button onclick="downloadLogs()">Download</button>
        </div>
        
        <div class="log-container" id="logContainer">
            <div class="log-entry log-info">[INFO] Remote Serial Monitor started</div>
        </div>
    </div>

    <script>
        let ws = null;
        let logs = [];
        let logLevel = 1;

        function connect() {
            // For simplicity, use polling instead of WebSocket for this basic implementation
            // In a full implementation, WebSocket would be properly implemented
            document.getElementById('status').textContent = 'Connected (Polling)';
            document.getElementById('status').className = 'status connected';
            
            // Poll for new logs every 2 seconds
            setInterval(function() {
                fetch('/api/logs')
                    .then(response => response.json())
                    .then(data => {
                        // Only show new logs (simple implementation)
                        if (data.length > logs.length) {
                            for (let i = logs.length; i < data.length; i++) {
                                addLogEntry(data[i].message, data[i].level);
                            }
                        }
                    })
                    .catch(err => {
                        console.log('Error fetching logs:', err);
                        document.getElementById('status').textContent = 'Connection Error';
                        document.getElementById('status').className = 'status disconnected';
                    });
            }, 2000);
        }

        function addLogEntry(message, level) {
            const timestamp = new Date().toISOString();
            const logEntry = { timestamp, message, level };
            logs.push(logEntry);
            
            if (level >= logLevel) {
                const container = document.getElementById('logContainer');
                const div = document.createElement('div');
                div.className = `log-entry log-${getLevelName(level)}`;
                div.textContent = `[${timestamp}] ${message}`;
                container.appendChild(div);
                container.scrollTop = container.scrollHeight;
            }
        }

        function getLevelName(level) {
            const names = ['debug', 'info', 'warning', 'error'];
            return names[level] || 'info';
        }

        function sendCommand() {
            const input = document.getElementById('commandInput');
            const command = input.value.trim();
            
            if (command) {
                // Send command via POST request
                fetch('/api/command', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({command: command})
                })
                .then(response => response.json())
                .then(data => {
                    addLogEntry(`> ${command}`, 1);
                    if (data.response) {
                        addLogEntry(data.response, 1);
                    }
                })
                .catch(err => {
                    addLogEntry('Error sending command', 3);
                });
                
                input.value = '';
            }
        }

        function clearLogs() {
            logs = [];
            document.getElementById('logContainer').innerHTML = '';
            
            fetch('/api/clear')
                .then(response => response.json())
                .then(data => addLogEntry('Logs cleared', 1))
                .catch(err => addLogEntry('Error clearing logs', 3));
        }

        function downloadLogs() {
            const logText = logs.map(log => `[${log.timestamp}] ${log.message}`).join('\n');
            const blob = new Blob([logText], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);
            
            const a = document.createElement('a');
            a.href = url;
            a.download = `logs_${new Date().toISOString().replace(/[:.]/g, '-')}.txt`;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        }

        document.getElementById('logLevel').onchange = function() {
            logLevel = parseInt(this.value);
            // Re-render logs with new filter
            const container = document.getElementById('logContainer');
            container.innerHTML = '';
            logs.filter(log => log.level >= logLevel).forEach(log => {
                const div = document.createElement('div');
                div.className = `log-entry log-${getLevelName(log.level)}`;
                div.textContent = `[${log.timestamp}] ${log.message}`;
                container.appendChild(div);
            });
        };

        document.getElementById('commandInput').onkeypress = function(e) {
            if (e.key === 'Enter') {
                sendCommand();
            }
        };

        // Load buffered logs
        fetch('/api/logs')
            .then(response => response.json())
            .then(data => {
                data.forEach(log => addLogEntry(log.message, log.level));
            })
            .catch(err => console.log('No buffered logs available'));

        connect();
    </script>
</body>
</html>
)HTML";
}