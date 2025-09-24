#ifndef REMOTE_SERIAL_UTILS_H
#define REMOTE_SERIAL_UTILS_H

#include "RemoteSerialMonitor.h"

// Helper macros for easier logging
#define RSM_DEBUG(msg) RemoteSerial.debug(msg)
#define RSM_INFO(msg) RemoteSerial.info(msg)  
#define RSM_WARNING(msg) RemoteSerial.warning(msg)
#define RSM_ERROR(msg) RemoteSerial.error(msg)

// Formatted logging helpers
#define RSM_LOG_VALUE(name, value) RemoteSerial.info(String(name) + ": " + String(value))
#define RSM_LOG_SENSOR(name, value, unit) RemoteSerial.info(String(name) + ": " + String(value) + String(unit))

// System monitoring helpers
class RemoteSerialUtils {
public:
  static void logSystemStatus() {
    RemoteSerial.info("=== System Status ===");
    RemoteSerial.info("Uptime: " + String(millis() / 1000) + "s");
    RemoteSerial.info("Free Memory: " + String(getFreeMemory()) + " bytes");
    RemoteSerial.info("Connected Clients: " + String(RemoteSerial.getClientCount()));
    RemoteSerial.info("Buffer Usage: " + String(RemoteSerial.getBufferCount()) + "/1024");
    
    if (WiFi.status() == WL_CONNECTED) {
      RemoteSerial.info("WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");
      RemoteSerial.info("IP Address: " + WiFi.localIP().toString());
    }
  }
  
  static void logSensorReading(const String& sensorName, float value, const String& unit = "") {
    String msg = sensorName + ": " + String(value, 2);
    if (unit.length() > 0) {
      msg += unit;
    }
    RemoteSerial.info(msg);
  }
  
  static void logError(const String& component, const String& error) {
    RemoteSerial.error(component + " ERROR: " + error);
  }
  
  static void logWarning(const String& component, const String& warning) {
    RemoteSerial.warning(component + " WARNING: " + warning);
  }
  
  static int getFreeMemory() {
    // Simplified memory check - implementation may vary by platform
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  }
  
  static void benchmark(const String& taskName, void (*task)()) {
    unsigned long startTime = millis();
    task();
    unsigned long duration = millis() - startTime;
    RemoteSerial.debug("BENCHMARK " + taskName + ": " + String(duration) + "ms");
  }
};

#endif // REMOTE_SERIAL_UTILS_H