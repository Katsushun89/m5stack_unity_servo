#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <M5Stack.h>
#include "config.h"

WebSocketsClient webSocket;
DynamicJsonDocument doc(1024);


std::string parseReceivedJson(uint8_t *payload)
{
  char *json = (char *)payload;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return "none";
  }

  JsonObject obj = doc.as<JsonObject>();

  // You can use a String to get an element of a JsonObject
  // No duplication is done.
  return obj[String("angle")];
}

void ctrlServo(uint8_t *payload)
{
  std::string angle = parseReceivedJson(payload);

  Serial.printf("angle: %s\n", angle.c_str());
  M5.Lcd.fillRect(0, 0, 240, 40, 0);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%s\n", angle.c_str());
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:
			Serial.printf("[WSc] Connected to url: %s\n", payload);
			//webSocket.sendTXT("Connected");
			break;
		case WStype_TEXT:
			Serial.printf("[WSc] get text: %s\n", payload);
      ctrlServo(payload);
			break;
		case WStype_BIN:
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
	}
}

void setupWiFi()
{
  WiFi.begin(ssid, passwd);

  // Wait some time to connect to wifi
  for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
      Serial.print(".");
      delay(1000);
  }

  // Check if connected to wifi
  if(WiFi.status() != WL_CONNECTED) {
      Serial.println("No Wifi!");
      return;
  }

  Serial.println("Connected to Wifi, Connecting to server.");
	// server address, port and URL
	webSocket.begin("192.168.10.13", 8080, "/");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	//webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
}

void setup()
{
  Serial.begin(115200);
  // Power ON Stabilizing...
  delay(500);
  M5.begin();

  setupWiFi();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(4);
}

void loop() {
  static uint32_t pre_send_time = 0;
  uint32_t time = millis();
  if(time - pre_send_time > 10){
    pre_send_time = time;
    String str = "hoge";
    webSocket.sendTXT(str);
  }
  webSocket.loop();

  M5.update();
}
