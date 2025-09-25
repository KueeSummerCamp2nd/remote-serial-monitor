#include <Arduino.h>
#include <WiFiS3.h>
#include "RemoteSerialMonitor.h"

// AP情報
const char* AP_SSID = "ArduinoR4_AP";
const char* AP_PASS = "arduino123";

void setup(){
  pinMode(LED_BUILTIN, OUTPUT);
  // 必要なら通常のSerialも
  Serial.begin(115200);
  delay(300);

  // APで起動
  if (!RemoteSerial.beginAP(AP_SSID, AP_PASS)) {
    Serial.println("AP start failed");
    while (1) { digitalWrite(LED_BUILTIN, HIGH); delay(200); digitalWrite(LED_BUILTIN, LOW); delay(200); }
  }
  RemoteSerial.beginServer();

  Serial.print("AP SSID: "); Serial.println(AP_SSID);
  Serial.print("AP PASS: "); Serial.println(AP_PASS);
  Serial.print("AP IP  : "); Serial.println(RemoteSerial.localIP());

  // 初期メッセージ
  RemoteSerial.println("RemoteSerial started.");
  RemoteSerial.printf("Connect to SSID '%s' and open http://%s/ \n",
                   AP_SSID, RemoteSerial.localIP().toString().c_str());
}

void loop(){
  // 必須：HTTP処理
  RemoteSerial.handle();

  // デモ：1秒ごとに擬似センサ値を送る
  static uint32_t t0 = millis();
  if (millis() - t0 > 1000){
    t0 += 1000;
    int val = analogRead(A0);
    RemoteSerial.printf("A0=%d\n", val);
  }
}
