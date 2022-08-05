#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include <Arduino_JSON.h>

#include <AutoPID.h>

#include "web.h"
#include "statusled.h"

/***************************************************************
 * Configuracion Rapida
 ***************************************************************/
const char* ssid     = "PEpito";
const char* password = "123456789";

/***************************************************************
 * Setup del PID
 ***************************************************************/
#define OUTPUT_MIN  0
#define OUTPUT_MAX  128
#define KP  1.12
#define KI  0
#define KD  0

double pid_temp, setPoint, outputVal;

AutoPID myPID(&pid_temp, &setPoint, &outputVal, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);
unsigned long lastTempUpdate;

/***************************************************************
 * Setup del sensor de temperatura
 ***************************************************************/
#define ONE_WIRE_BUS 32
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//#define DS_DATA_PIN 34
#define DS_CONN_PIN 35

//  Variables de Estado del sensor de temperatura
String ds_temp = "";
int ds_state = HIGH;
int ds_state_last = HIGH;

String ds_read() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  if(tempC == -127.00) {
    //Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    //Serial.print("Temperature Celsius: ");
    //Serial.println(tempC);
    pid_temp = tempC;
  }
  return String(tempC);
}

/***************************************************************
 * Setup servidor web
 ***************************************************************/

JSONVar ds_info;
JSONVar web_info;

String slider_p = "0";
String slider_t = "25";
String checkbox = "false";

const char* PARAM_INPUT = "value";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String get_ds_values(){
  ds_info["ds_temp"] = ds_temp;
  ds_info["ds_bool"]= ds_state;

  return JSON.stringify(ds_info);
}

String get_web_values(){
  web_info["slider_p"] = slider_p;
  web_info["slider_t"] = slider_t;
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

//  No termino necesitando el Placeholder si nunca los uso en la web. Processor included just in case
// Replaces placeholder with DS18B20 values
String processor(const String& var){
  //Serial.println(var);
  if(var == "DS_TEMP"){
    return ds_temp;
  }
  return String();
}
//  En el parseador de JS voy a determinar la visibilidad. Eso no debe concernir al ESP.






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

  //  Startup Temperature Sensor
  sensors.begin();

  ds_temp = ds_read();
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

  //  Startup PID
  //myPID.setBangBang(4);
  myPID.setTimeStep(1000);

  //  Startup Webserver
  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  // GET input value on <ESP_IP>/slider_p?value=<inputMessage>
  server.on("/slider_p", HTTP_GET, [](AsyncWebServerRequest * request){
    String inputMessage;
    //Serial.print(inputMessage);
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      slider_p = inputMessage;
      //Serial.print("Slider_p value: ");
      //Serial.println(slider_p);
      //  Activate PWM
      /*
       * ledcWrite(PWM_PIN, slider_p.toInt()*255/200);
       */
      if(checkbox == "false")
      {
        //Serial.println("PWM on at: "+slider_p);
        ledcWrite(PWM_CH, slider_p.toInt()*128/100);
        if (slider_p.toInt() == 0)
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
  server.on("/slider_t", HTTP_GET, [](AsyncWebServerRequest * request){
    String inputMessage;
    //Serial.print(inputMessage);
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      slider_t = inputMessage;
      //Serial.print("Slider_t value: ");
      //Serial.println(slider_t);
      //  Activate Temperature PID
      if(checkbox == "true")
      {
        setPoint = slider_t.toDouble();
        myPID.run();
        upd_power_status( HEAT_ON );
      }
      notifyClients(get_web_values());
    }
    else {
      inputMessage = "No message sent";
    }
    request->send_P(200, "text/plain", "OK");
  });
  server.on("/checkbox", HTTP_GET, [](AsyncWebServerRequest * request){
    String inputMessage;
    //Serial.print(inputMessage);
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      checkbox = inputMessage;

      slider_p = "0";
      ledcWrite(PWM_CH, 0);
      upd_power_status( HEAT_OFF );
      
      slider_t = "25";
      //  Reset PID
      if(checkbox == "true")
      {
        setPoint = slider_t.toDouble();
        myPID.run();
        //ledcWrite(PWM_CH, outputVal/128);
      }
      //Serial.print("checkbox value: ");
      //Serial.println(checkbox);
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
      if(ds_state == LOW){
        ds_temp = ds_read();
      }
      else{
        ds_temp = "--";
      }
      notifyClients(get_ds_values());
    }

    if(ds_state == LOW)
    {
      ds_temp = ds_read();
      notifyClients(get_ds_values()); 
      
    }
    ds_state_last = reading;

    if(checkbox == "true")
    {
      myPID.run();
      ledcWrite(PWM_CH, outputVal);
      //Serial.println(outputVal);
    }
    
    lastTime = millis();
    //Serial.print(".");
  }  
}
