# Remote Serial Monitor

A lightweight Arduino UNO R4 WiFi library that enables viewing and sending Serial data remotely through a web browser. Perfect for IoT projects, remote debugging, and wireless monitoring.

## 🚀 Features

- **Real-time WebSocket Communication**: Live serial data streaming
- **REST API Endpoints**: Access buffered logs via HTTP requests
- **Web-based UI**: Modern, responsive interface with:
  - Real-time log display with color-coded levels
  - Command input box for sending data to Arduino
  - Log filtering by level (Debug, Info, Warning, Error)
  - Download logs functionality
- **Non-blocking APIs**: Maintains Arduino performance
- **Memory Efficient**: Circular buffer with configurable size
- **Multiple Client Support**: Handle up to 4 concurrent connections
- **Serial Drop-in Replacement**: Use like Arduino's Serial class

## 📋 Requirements

- **Arduino UNO R4 WiFi** (uses WiFiS3 library)
- **Arduino IDE 2.0+** or PlatformIO
- **WiFi Network** with internet access

## 📦 Installation

### Arduino IDE
1. Download the library as ZIP
2. Go to **Sketch** → **Include Library** → **Add .ZIP Library**
3. Select the downloaded ZIP file

### Manual Installation
1. Clone or download this repository
2. Place the folder in your Arduino libraries directory:
   - Windows: `Documents/Arduino/libraries/`
   - macOS: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`

## 🔧 Quick Start

```cpp
#include <RemoteSerialMonitor.h>

void setup() {
  Serial.begin(115200);
  
  // Configure WiFi credentials
  RemoteSerial.setWiFiCredentials("YourWiFiSSID", "YourWiFiPassword");
  
  // Start the remote monitor
  if (RemoteSerial.begin()) {
    Serial.print("Open browser: http://");
    Serial.println(RemoteSerial.getLocalIP());
  }
}

void loop() {
  RemoteSerial.loop(); // Essential for non-blocking operation
  
  // Use like Serial
  RemoteSerial.println("Hello from Arduino!");
  
  delay(1000);
}
```

## 📚 API Reference

### Setup Methods

#### `setWiFiCredentials(ssid, password)`
Configure WiFi network credentials.
```cpp
RemoteSerial.setWiFiCredentials("MyNetwork", "MyPassword");
```

#### `setServerPort(port)`
Set custom server port (default: 80).
```cpp
RemoteSerial.setServerPort(8080);
```

#### `setLogFilter(level)`
Filter logs by minimum level (0=Debug, 1=Info, 2=Warning, 3=Error).
```cpp
RemoteSerial.setLogFilter(1); // Hide debug messages
```

### Lifecycle Methods

#### `begin()`
Initialize WiFi and start server. Returns `true` if successful.
```cpp
if (RemoteSerial.begin()) {
  Serial.println("Started successfully!");
}
```

#### `loop()`
**Must be called regularly** in your main loop for non-blocking operation.
```cpp
void loop() {
  RemoteSerial.loop(); // Essential!
  // Your other code here...
}
```

#### `end()`
Stop server and disconnect WiFi.
```cpp
RemoteSerial.end();
```

### Status Methods

#### `isConnected()`
Check if WiFi is connected and server is running.

#### `getClientCount()`
Get number of connected web clients.

#### `getLocalIP()`
Get Arduino's IP address.

### Logging Methods

#### Standard Print Interface
Use exactly like Arduino's Serial:
```cpp
RemoteSerial.print("Value: ");
RemoteSerial.println(sensorValue);
RemoteSerial.write(byteData, length);
```

#### Level-specific Logging
```cpp
RemoteSerial.debug("Debug information");   // Gray text
RemoteSerial.info("General information");  // Blue text
RemoteSerial.warning("Warning message");   // Orange text
RemoteSerial.error("Error occurred!");     // Red text
```

#### Custom Level Logging
```cpp
RemoteSerial.print("Custom message", 2);   // 0=debug, 1=info, 2=warning, 3=error
```

### Buffer Management

#### `getBufferedLogs(maxEntries)`
Get JSON string of buffered logs.

#### `clearBuffer()`
Clear the circular buffer.

#### `getBufferCount()`
Get current number of entries in buffer.

## 🌐 Web Interface

The embedded web interface provides:

- **Real-time Log Display**: Live updates via WebSocket
- **Command Input**: Send commands to Arduino
- **Log Filtering**: Show/hide different log levels
- **Download Logs**: Save logs as text file
- **Connection Status**: Visual WiFi connection indicator

### REST API Endpoints

- `GET /` - Main web interface
- `GET /api/logs` - Get buffered logs as JSON
- `GET /api/clear` - Clear log buffer
- `POST /api/command` - Send command to Arduino

## 💡 Examples

### Basic Usage
```cpp
#include <RemoteSerialMonitor.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

void setup() {
  Serial.begin(115200);
  
  RemoteSerial.setWiFiCredentials(ssid, password);
  
  if (RemoteSerial.begin()) {
    RemoteSerial.info("System started");
    Serial.print("Web UI: http://");
    Serial.println(RemoteSerial.getLocalIP());
  }
}

void loop() {
  RemoteSerial.loop();
  
  int sensorValue = analogRead(A0);
  RemoteSerial.info("Sensor: " + String(sensorValue));
  
  if (sensorValue > 800) {
    RemoteSerial.warning("High sensor value!");
  }
  
  delay(2000);
}
```

### Advanced Features
```cpp
#include <RemoteSerialMonitor.h>

void setup() {
  Serial.begin(115200);
  
  // Custom configuration
  RemoteSerial.setWiFiCredentials("MyNetwork", "MyPassword");
  RemoteSerial.setServerPort(8080);
  RemoteSerial.setLogFilter(1); // Hide debug messages
  
  if (RemoteSerial.begin()) {
    RemoteSerial.info("Advanced example started");
  }
}

void loop() {
  RemoteSerial.loop();
  
  // System monitoring
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 10000) {
    RemoteSerial.info("Uptime: " + String(millis() / 1000) + "s");
    RemoteSerial.info("Clients: " + String(RemoteSerial.getClientCount()));
    RemoteSerial.debug("Buffer: " + String(RemoteSerial.getBufferCount()));
    
    lastUpdate = millis();
  }
  
  delay(100);
}
```

## ⚙️ Configuration

### Memory Usage
- **Default buffer**: 1024 log entries
- **Max message size**: 256 characters per entry
- **Max clients**: 4 concurrent connections
- **RAM usage**: ~300KB (adjustable via constants in header)

### Customization
Edit these constants in `RemoteSerialMonitor.h`:
```cpp
#define RSM_BUFFER_SIZE 1024        // Number of log entries
#define RSM_MAX_MESSAGE_SIZE 256    // Max characters per message
#define RSM_MAX_CLIENTS 4           // Max concurrent connections
```

## 🔧 Troubleshooting

### Common Issues

**WiFi Connection Failed**
- Verify SSID and password
- Check WiFi signal strength
- Ensure 2.4GHz network (UNO R4 doesn't support 5GHz)

**Web Interface Not Loading**
- Check IP address in Serial Monitor
- Verify port (default 80, or custom port)
- Ensure firewall allows connections

**Memory Issues**
- Reduce `RSM_BUFFER_SIZE` if running low on memory
- Call `clearBuffer()` periodically
- Monitor with `getBufferCount()`

**WebSocket Disconnections**
- Web interface auto-reconnects every 3 seconds
- Check WiFi stability
- Monitor client count with `getClientCount()`

### Debug Tips
```cpp
void setup() {
  Serial.begin(115200);
  
  if (!RemoteSerial.begin()) {
    Serial.println("Failed to start RemoteSerial");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
  } else {
    Serial.print("IP: ");
    Serial.println(RemoteSerial.getLocalIP());
  }
}
```

## 📄 License

MIT License - see [LICENSE](LICENSE) file.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with Arduino UNO R4 WiFi
5. Submit a pull request

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/KueeSummerCamp2nd/remote-serial-monitor/issues)
- **Examples**: See `examples/` directory
- **Documentation**: This README and inline code comments