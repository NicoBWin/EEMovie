#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Arduino.h>
#include <Arduino_JSON.h>

#include "web.h"
#include "statusled.h"

/***************************************************************
 * Configuracion Rapida
 ***************************************************************/

const char* ssid     = "FINAL";
const char* password = "123456789"; //Capaz usar solo numeros

/***************************************************************
 * Setup del PID
 ***************************************************************/
double outputVal;

/***************************************************************
 * Setup del sensor de temperatura
 ***************************************************************/

//#define DS_DATA_PIN 34
#define DS_CONN_PIN 35

//  Variables de Estado del sensor de temperatura
int ds_state = HIGH;
int ds_state_last = HIGH;

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
      upd_client_status( CLIENT_SI );
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      upd_client_status( CLIENT_NO );
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
#define  PWM_PIN 4
#define  PWM_CH  0
#define  PWM_F   1
#define  PWM_RES 8

//  Timer
unsigned long lastTime = 0;
unsigned long timerDelay = 100;

/***************************************************************
 * Funcionamiento del ESP
 ***************************************************************/

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();

  // Status LED initialization
  init_status_led();
  
  pinMode(DS_CONN_PIN, INPUT);

  //  Startup PWM
  pinMode(PWM_PIN, OUTPUT);
  ledcSetup(PWM_CH, PWM_F, PWM_RES);
  ledcAttachPin(PWM_PIN, PWM_CH);

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

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  // GET input value on <ESP_IP>/slider_p?value=<inputMessage>
  server.on("/slider_f", HTTP_GET, [](AsyncWebServerRequest * request){
    String inputMessage;
    Serial.print(inputMessage);
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      slider_f = inputMessage;
      Serial.print("Slider_f value: ");
      Serial.println(slider_f);
      //  Activate PWM
      
      ledcWrite(PWM_PIN, slider_f.toInt()*255/200);
      
      if(checkbox == "false")
      {
        Serial.println("PWM on at: "+slider_f);
        ledcWrite(PWM_CH, slider_f.toInt()*128/100);
        if (slider_f.toInt() == 0)
        {
          upd_power_status( HEAT_OFF );
        }
        else
        {
          upd_power_status( HEAT_ON );
        }
        
      }
      else if(checkbox == "true")
      {
        ledcWrite(PWM_CH, 0);
      }
      
      notifyClients(get_web_values());
    }
    else {
      inputMessage = "No message sent";
    }
    request->send_P(200, "text/plain", "OK");
  });
//  server.on("/slider_t", HTTP_GET, [](AsyncWebServerRequest * request){
//    String inputMessage;
//    //Serial.print(inputMessage);
//    if(request->hasParam(PARAM_INPUT)) {
//      inputMessage = request->getParam(PARAM_INPUT)->value();
//      slider_t = inputMessage;
//      //Serial.print("Slider_t value: ");
//      //Serial.println(slider_t);
//      //  Activate Temperature PID
//      if(checkbox == "true")
//      {
//        setPoint = slider_t.toDouble();
//        myPID.run();
//        upd_power_status( HEAT_ON );
//      }
//      notifyClients(get_web_values());
//    }
//    else {
//      inputMessage = "No message sent";
//    }
//    request->send_P(200, "text/plain", "OK");
//  });
  server.on("/checkbox", HTTP_GET, [](AsyncWebServerRequest * request){
    String inputMessage;
    Serial.print(inputMessage);
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      checkbox = inputMessage;

      slider_f = "0";
      ledcWrite(PWM_CH, 0);
      upd_power_status( HEAT_OFF );
      
      if(checkbox == "true")
      {
        ledcWrite(PWM_CH, outputVal/128);
      }
      Serial.print("checkbox value: ");
      Serial.println(checkbox);
      //  Enable PWM or PID output
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

void loop() {
  if ((millis() - lastTime) > timerDelay)
  {
    int reading = digitalRead(DS_CONN_PIN);
    cycle_status_led();
    if (reading != ds_state_last)
    {
      ds_state = reading;
      notifyClients(get_ds_values());
    }

    if(ds_state == LOW)
    {
      notifyClients(get_ds_values()); 
      
    }
    ds_state_last = reading;

    if(checkbox == "true")
    {
      ledcWrite(PWM_CH, outputVal);
      //Serial.println(outputVal);
    }
    
    lastTime = millis();
    //Serial.print(".");
  }  
}
