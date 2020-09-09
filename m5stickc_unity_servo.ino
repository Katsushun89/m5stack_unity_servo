#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <M5StickC.h>
#include <ESP32Servo.h>
#include "config.h"

WebSocketsClient webSocket;
DynamicJsonDocument doc(1024);

TaskHandle_t th[4];

//Servo
Servo servo1; // create four servo objects 
int32_t servo1_pin = 26;
const int32_t MIN_US = 500;
const int32_t MAX_US = 2400;

int16_t cur_servo_pos = 0;
int16_t goal_servo_pos = 0;

void setupServo(void)
{
  servo1.setPeriodHertz(50); // Standard 50hz servo
  servo1.attach(servo1_pin, MIN_US, MAX_US);

  cur_servo_pos = servo1.read();
  Serial.print("cur_servo_pos:");
  Serial.println(cur_servo_pos);
}

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
  goal_servo_pos = atoi(angle.c_str());
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
  setupServo();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.setTextSize(3);

  xTaskCreatePinnedToCore(servoControl, "servoControl", 4096, NULL, 1, &th[0], 0);

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


void moveServo()
{
  if(goal_servo_pos == cur_servo_pos) return;

  servo1.write(goal_servo_pos);
  cur_servo_pos = goal_servo_pos;
  delay(100);
}

void servoControl(void *pvParameters)
{
  while(1){
    moveServo();
    delay(1);
  }
}
