#include <RemoteSerialMonitor.h>

// WiFi credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Variables for demonstration
int counter = 0;
float temperature = 23.5;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, INPUT_PULLUP); // Button
  
  // Configure RemoteSerial
  RemoteSerial.setWiFiCredentials(ssid, password);
  RemoteSerial.setServerPort(8080); // Custom port
  RemoteSerial.setLogFilter(1); // Only show info and above (hide debug)
  
  if (RemoteSerial.begin()) {
    Serial.println("Remote Serial Monitor started!");
    Serial.print("Web interface: http://");
    Serial.print(RemoteSerial.getLocalIP());
    Serial.println(":8080");
    
    RemoteSerial.info("Advanced Remote Serial Monitor Demo");
    RemoteSerial.info("Connected clients: " + String(RemoteSerial.getClientCount()));
    
    // Log system information
    RemoteSerial.debug("Free RAM: " + String(freeMemory()) + " bytes");
    RemoteSerial.info("System ready for operation");
    
  } else {
    Serial.println("Failed to start Remote Serial Monitor");
    RemoteSerial.error("WiFi connection failed");
  }
}

void loop() {
  // Essential: Call this every loop iteration
  RemoteSerial.loop();
  
  // Simulate various system events
  handleSensorData();
  handleButtonPress();
  handlePeriodicTasks();
  
  delay(100); // Small delay for system stability
}

void handleSensorData() {
  static unsigned long lastReading = 0;
  
  if (millis() - lastReading > 3000) {
    // Simulate temperature sensor
    temperature += random(-10, 11) * 0.1;
    
    String tempMsg = "Temperature: " + String(temperature, 1) + "°C";
    
    if (temperature > 30.0) {
      RemoteSerial.error(tempMsg + " - OVERHEATING!");
    } else if (temperature < 10.0) {
      RemoteSerial.warning(tempMsg + " - Too cold");
    } else {
      RemoteSerial.info(tempMsg);
    }
    
    // Log additional sensor data
    int lightLevel = analogRead(A0);
    RemoteSerial.debug("Light level: " + String(lightLevel));
    
    lastReading = millis();
  }
}

void handleButtonPress() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(2);
  
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Button pressed
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    
    String ledMsg = "LED turned " + String(ledState ? "ON" : "OFF");
    RemoteSerial.info(ledMsg);
    
    // Demonstrate different log levels
    if (counter % 10 == 0) {
      RemoteSerial.warning("Button pressed 10 times! Counter: " + String(counter));
    }
    
    counter++;
  }
  
  lastButtonState = currentButtonState;
}

void handlePeriodicTasks() {
  static unsigned long lastStatusUpdate = 0;
  
  if (millis() - lastStatusUpdate > 30000) { // Every 30 seconds
    // System status report
    RemoteSerial.info("=== System Status Report ===");
    RemoteSerial.info("Uptime: " + String(millis() / 1000) + " seconds");
    RemoteSerial.info("Connected clients: " + String(RemoteSerial.getClientCount()));
    RemoteSerial.info("Buffer usage: " + String(RemoteSerial.getBufferCount()) + "/1024");
    RemoteSerial.info("WiFi signal: " + String(WiFi.RSSI()) + " dBm");
    RemoteSerial.info("Free memory: " + String(freeMemory()) + " bytes");
    
    // Clear buffer if it's getting full
    if (RemoteSerial.getBufferCount() > 900) {
      RemoteSerial.warning("Buffer nearly full, clearing old logs...");
      RemoteSerial.clearBuffer();
    }
    
    lastStatusUpdate = millis();
  }
}

// Simple memory check function
int freeMemory() {
  // This is a simplified version - actual implementation may vary
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}