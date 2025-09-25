#include <RemoteSerialMonitor.h>

// WiFi credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

void setup() {
  // Initialize standard Serial for debugging
  Serial.begin(115200);
  
  // Configure RemoteSerial with WiFi credentials
  RemoteSerial.setWiFiCredentials(ssid, password);
  RemoteSerial.setServerPort(80); // Optional: default is 80
  
  // Start the remote serial monitor
  if (RemoteSerial.begin()) {
    Serial.println("Remote Serial Monitor started!");
    Serial.print("Open your browser to: http://");
    Serial.println(RemoteSerial.getLocalIP());
    
    // Send initial messages
    RemoteSerial.info("System initialized");
    RemoteSerial.debug("Debug messages enabled");
  } else {
    Serial.println("Failed to start Remote Serial Monitor");
  }
}

void loop() {
  // IMPORTANT: Call loop() regularly for non-blocking operation
  RemoteSerial.loop();
  
  // Example: Log sensor data every 5 seconds
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 5000) {
    int sensorValue = analogRead(A0);
    
    RemoteSerial.info("Sensor reading: " + String(sensorValue));
    
    // Log with different levels based on value
    if (sensorValue > 800) {
      RemoteSerial.warning("High sensor value detected!");
    } else if (sensorValue < 200) {
      RemoteSerial.error("Low sensor value - check connection!");
    }
    
    lastLog = millis();
  }
  
  // Example: Use RemoteSerial like Serial
  RemoteSerial.print("Current millis: ");
  RemoteSerial.println(String(millis()));
  
  // Small delay to prevent flooding
  delay(1000);
}