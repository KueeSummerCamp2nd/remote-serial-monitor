#ifndef REMOTE_SERIAL_MONITOR_H
#define REMOTE_SERIAL_MONITOR_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <ArduinoHttpClient.h>

// Configuration constants
#define RSM_DEFAULT_PORT 80
#define RSM_MAX_CLIENTS 1
#define RSM_BUFFER_SIZE 64
#define RSM_MAX_MESSAGE_SIZE 96
#define RSM_WEBSOCKET_TIMEOUT 100

// WebSocket frame types
#define WS_FRAME_TEXT 0x81
#define WS_FRAME_BINARY 0x82
#define WS_FRAME_CLOSE 0x88
#define WS_FRAME_PING 0x89
#define WS_FRAME_PONG 0x8A

struct LogEntry {
  unsigned long timestamp;
  char message[RSM_MAX_MESSAGE_SIZE];
  uint8_t level; // 0=debug, 1=info, 2=warning, 3=error
};

class RemoteSerialMonitor : public Print {
private:
  // WiFi and server components
  WiFiServer* server;
  WiFiClient clients[RSM_MAX_CLIENTS];
  bool clientConnected[RSM_MAX_CLIENTS];
  bool clientIsWebSocket[RSM_MAX_CLIENTS];
  
  // Configuration
  char* ssid;
  char* password;
  uint16_t port;
  bool initialized;
  
  // Circular buffer for log storage
  LogEntry logBuffer[RSM_BUFFER_SIZE];
  uint16_t bufferHead;
  uint16_t bufferTail;
  uint16_t bufferCount;
  
  // Filtering
  uint8_t minLogLevel;
  
  // Internal methods
  void handleNewClients();
  void handleExistingClients();
  bool handleWebSocketHandshake(int clientIndex);
  void sendWebSocketFrame(int clientIndex, const char* data, uint8_t opcode = WS_FRAME_TEXT);
  void sendHttpResponse(int clientIndex, const String& content, const String& contentType = "text/html");
  void handleHttpRequest(int clientIndex, const String& request);
  void addToBuffer(const char* message, uint8_t level = 1);
  String generateWebUI();
  String generateLogJson();
  String base64Encode(const char* data, int length);
  String sha1Hash(const String& input);
  bool isWebSocketUpgrade(const String& request);
  
public:
  // Constructor
  RemoteSerialMonitor(uint16_t serverPort = RSM_DEFAULT_PORT);
  
  // Destructor  
  ~RemoteSerialMonitor();
  
  // Setup and configuration
  void setWiFiCredentials(const char* ssid, const char* password);
  void setServerPort(uint16_t port);
  void setLogFilter(uint8_t minLevel);
  
  // Core lifecycle methods
  bool begin();
  void end();
  void loop(); // Non-blocking - call in main loop
  
  // Status methods
  bool isConnected();
  int getClientCount();
  IPAddress getLocalIP();
  
  // Print interface implementation (for Serial-like usage)
  virtual size_t write(uint8_t byte) override;
  virtual size_t write(const uint8_t *buffer, size_t size) override;
  
  // Additional print methods
  void print(const String& message, uint8_t level = 1);
  void println(const String& message, uint8_t level = 1);
  void debug(const String& message);
  void info(const String& message);
  void warning(const String& message);
  void error(const String& message);
  
  // Buffer management
  String getBufferedLogs(int maxEntries = -1);
  void clearBuffer();
  int getBufferCount();
  
  // Command interface
  bool sendCommand(const String& command);
  bool hasIncomingData();
  String readIncomingData();
};

extern RemoteSerialMonitor RemoteSerial;

#endif // REMOTE_SERIAL_MONITOR_H