# Changelog

## v1.0.0 (2025-01-XX)

### Features
- ✅ Arduino UNO R4 WiFi library for remote serial monitoring
- ✅ WebSocket support for real-time log streaming (with fallback to REST polling)
- ✅ REST API endpoints for buffered log access
- ✅ Embedded web UI with modern, responsive design
- ✅ Non-blocking APIs for optimal Arduino performance
- ✅ Memory-efficient circular buffer (configurable size)
- ✅ Multiple log levels (Debug, Info, Warning, Error) with color coding
- ✅ Serial drop-in replacement (implements Print interface)
- ✅ Multi-client support (up to 4 concurrent connections)
- ✅ Log filtering and download functionality
- ✅ WiFi connection management
- ✅ Comprehensive examples and documentation

### API
- `RemoteSerial.setWiFiCredentials(ssid, password)` - Configure WiFi
- `RemoteSerial.setServerPort(port)` - Set custom server port
- `RemoteSerial.setLogFilter(level)` - Filter logs by minimum level
- `RemoteSerial.begin()` - Initialize WiFi and start server
- `RemoteSerial.loop()` - Non-blocking operation handler (call in main loop)
- `RemoteSerial.print/println()` - Serial-compatible logging
- `RemoteSerial.debug/info/warning/error()` - Level-specific logging
- `RemoteSerial.getBufferedLogs()` - Retrieve buffered logs
- `RemoteSerial.clearBuffer()` - Clear log buffer
- `RemoteSerial.isConnected()` - Check connection status
- `RemoteSerial.getClientCount()` - Get number of connected clients

### Web Interface
- Real-time log display with auto-scroll
- Command input box for sending data to Arduino
- Log level filtering (Debug, Info, Warning, Error)
- Download logs as text file
- Connection status indicator
- Responsive design for mobile and desktop

### Memory Optimization
- Circular buffer with configurable size (default: 1024 entries)
- Maximum message size: 256 characters
- Automatic buffer management with overflow protection
- Low memory footprint (~300KB RAM usage)

### Examples
- BasicUsage.ino - Simple setup and logging
- AdvancedFeatures.ino - System monitoring, multi-level logging, buffer management