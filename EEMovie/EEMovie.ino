/***************************************************************
 * @authors: Alan Mechoulam | Nicolás Bustelo
 * @brief: Simulador de electroestimulador para una película
 ***************************************************************/

/***************************************************************
 * Bibliotecas
 ***************************************************************/
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Arduino.h>
#include <Arduino_JSON.h>

// Página Web (HTML en const char)
#include "web.h"

/***************************************************************
 * Setup Server credentials
 ***************************************************************/
const char* ssid     = "FINAL4";
const char* password = "123456789"; //Capaz usar solo numeros

/***************************************************************
 * Setup PWM
 ***************************************************************/
// The ouputs
const int PWMPin = 4;  // 4 corresponds to GPIO4
const int PWMPin2 = 2;  // 2 corresponds to GPIO2

// Setting PWM properties
const int freq = 500;
const int freq2 = 1000;
const int ledChannel = 0;
const int ledChannel2 = 0;
const int resolution = 8;


/***************************************************************
 * Setup Pote
 ***************************************************************/
float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


/***************************************************************
 * Setup servidor web
 ***************************************************************/
JSONVar web_info;

String slider_f = "0";
String checkbox = "false";

const char* PARAM_INPUT = "value";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String get_web_values(){
  web_info["slider_f"] = slider_f;
  web_info["sw_ctrl"] = checkbox;

  return JSON.stringify(web_info);
}

void notifyClients(String notif){
  ws.textAll(notif);
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      notifyClients(get_web_values());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      //handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


/***************************************************************
 * Setup del ESP
 ***************************************************************/
void setup() {
  // Configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  ledcSetup(ledChannel2, freq2, resolution);
  // Attach the channel to the GPIO to be controlled
  ledcAttachPin(PWMPin, ledChannel);
  ledcAttachPin(PWMPin2, ledChannel2);
  //***************************

  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();

  //  Startup Access Point: generate its own access point to connect to the ESP.
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.println("Web Server IP address: ");
  Serial.print(IP);

  if(!MDNS.begin("esp32")) {
    Serial.println("Error starting mDNS");
    return;
  }

  //  Startup Webserver
  initWebSocket();

  // Rutina de acceso
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  // Rutina de cambio de slider
  server.on("/slider_f", HTTP_GET, [](AsyncWebServerRequest * request){
    String inputMessage;
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      slider_f = inputMessage;
      Serial.print("Slider_f value: ");
      Serial.println(slider_f);

      outputVal = slider_f;
      
      notifyClients(get_web_values());
    }
    else {
      inputMessage = "No message sent";
    }
    request->send_P(200, "text/plain", "OK");
  });

  // Rutina de cambio de checkbox
  server.on("/checkbox", HTTP_GET, [](AsyncWebServerRequest * request){
    String inputMessage;
    Serial.print("checkbox inputmessage: ");
    Serial.println(inputMessage);
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      checkbox = inputMessage;
      Serial.print("checkbox value: ");
      Serial.println(checkbox);

      notifyClients(get_web_values());
    }
    else {
      inputMessage = "No message sent";
    }
    request->send_P(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
  notifyClients(get_web_values());
}


/***************************************************************
 * Funcionamiento del programa
 ***************************************************************/
void loop() {   
  // Pote
  // read the input on analog pin GIOP36:
  int analogValue = analogRead(36);
  // Rescale to potentiometer's voltage (from 0V to 3.3V):
  float voltage = floatMap(analogValue, 0, 4095, 0, 3.3);
  // print out the value you read:
  Serial.print("Analog: ");
  Serial.print(analogValue);
  Serial.print(", Voltage: ");
  Serial.println(voltage);
  delay(1000);

  // changing the LED brightness with PWM
  int dutyCycle = 100;  //dutyCycle must me between 0-255
  ledcWrite(ledChannel, dutyCycle);
}
