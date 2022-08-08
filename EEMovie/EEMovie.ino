#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Arduino.h>
#include <Arduino_JSON.h>

#include "web.h"

/***************************************************************
 * Configuracion Rapida
 ***************************************************************/

const char* ssid     = "FINAL3";
const char* password = "123456789"; //Capaz usar solo numeros

/***************************************************************
 * Setup del PID
 ***************************************************************/
String outputVal;

/***************************************************************
 * Setup servidor web
 ***************************************************************/
JSONVar ds_info;
JSONVar web_info;

String slider_f = "0";
String checkbox = "false";

const char* PARAM_INPUT = "value";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String get_ds_values(){
  ds_info["ds_bool"]= ds_state;

  return JSON.stringify(ds_info);
}

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
 * Setup la placa ESP
 ***************************************************************/
// PWM
// the number of the LED pin
const int PWMPin = 4;  // 16 corresponds to GPIO16

// setting PWM properties
const int freq = 500;
const int ledChannel = 0;
const int resolution = 8;

/***************************************************************
 * Funcionamiento del ESP
 ***************************************************************/
void setup() {
  // PWM SETUP
  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(PWMPin, ledChannel);
  //*******************************

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

  notifyClients(get_ds_values());
  notifyClients(get_web_values());
}


/***************************************************************
 * Funcionamiento del programa
 ***************************************************************/
void loop() {
  // increase the LED brightness
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){   
    // changing the LED brightness with PWM
    ledcWrite(ledChannel, dutyCycle);
    delay(15);
  }

  // decrease the LED brightness
  for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
    // changing the LED brightness with PWM
    ledcWrite(ledChannel, dutyCycle);   
    delay(15);
  }
}
