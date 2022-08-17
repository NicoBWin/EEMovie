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
const char* ssid     = "ESP_IMPACTADOS_ALAN";
const char* password = "123456789"; //Capaz usar solo numeros

/***************************************************************
 * Input
 ***************************************************************/
#define BUTTON_PIN 21  // GIOP21 pin connected to button
#define BUTTON_ONOFF_PIN 3  // GIOP21 pin connected to button
#define LEDGN_PIN 32  // 
#define LEDRD_PIN 33  //  
#define LEDSS_PIN 25  //  

// Variables will change:
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
int OnOffState = LOW; // the current reading from the input pin

//VARIABLE MALA
int count = 0;
/***************************************************************
 * Setup PWM
 ***************************************************************/
// The ouputs
const int PWMPin1 = 2;  // 2 corresponds to GPIO2

// Setting PWM properties
const int freq1 = 100;
const int ledChannel = 1;
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
JSONVar ds_info;

String slider_f = "100";
String slider_d = "10";
String checkbox = "false";

const char* PARAM_INPUT = "value";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String get_web_values(){
  web_info["slider_f"] = slider_f;
  //web_info["sw_ctrl"] = checkbox;

  return JSON.stringify(web_info);
}

String get_ds_values(){
  ds_info["slider_d"] = slider_d;
  ds_info["sw_ctrl"] = checkbox;

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
      Serial.printf("incoming message");
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
  // Defino el boton como pullup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ONOFF_PIN, INPUT_PULLUP);

  pinMode(LEDGN_PIN, OUTPUT);
  pinMode(LEDRD_PIN, OUTPUT);
  pinMode(LEDSS_PIN, OUTPUT);

  // Attach the channel to the GPIO to be controlled
  ledcAttachPin(PWMPin1, ledChannel);
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
      //Serial.print("Slider_f value: ");
      //Serial.println(slider_f);
      
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
    //Serial.print("checkbox inputmessage: ");
    //Serial.println(inputMessage);
    if(request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      checkbox = inputMessage;
      //Serial.print("checkbox value: ");
      //Serial.println(checkbox);

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
  notifyClients(get_ds_values());
}


/***************************************************************
 * Funcionamiento del programa
 ***************************************************************/
void loop() {   
  OnOffState = digitalRead(BUTTON_ONOFF_PIN);
  if (OnOffState == LOW){
    digitalWrite(LEDSS_PIN, HIGH);
    // Pote
    int analogValue = analogRead(36); // Read the input on analog pin GIOP36
    String slider_d = String(floor(analogValue));
    float duty = floatMap(analogValue, 0, 4095, 0, 255); // Rescale to potentiometer's voltage (from 0V to 3.3V)
    //*********************************************
    
    // PWM
    int Webfrec = slider_f.toInt();  // changing the PWM from webpage

    //*********************************************

    // Button
    // read the state of the switch/button:
    currentState = digitalRead(BUTTON_PIN);

    if (lastState == HIGH && currentState == LOW)
      count = 100; 
    else if (lastState == LOW && currentState == HIGH)
      count = 100;

    // save the the last state
    lastState = currentState;
    //*********************************************

    digitalWrite(LEDGN_PIN, HIGH); // turn the LED GREEN on
    // Timer
    //TIMER ENCENDIDO
    if (count > 0) {
      count--;
      ledcSetup(ledChannel, Webfrec, resolution); //REVISAR: Anda ~bien pero no actualiza la frecuencia al instante
      ledcWrite(ledChannel, (int) duty);
      digitalWrite(LEDRD_PIN, LOW); // turn the LED RED off
      checkbox = true;
    }
    //TIMER EXPIRADOs
    else {
      count = 0;
      ledcSetup(ledChannel, freq1, resolution);
      ledcWrite(ledChannel, 0);
      digitalWrite(LEDRD_PIN, HIGH); // turn the LED RED on
      checkbox = false;
      //Serial.println("Timer expired");
    }
    notifyClients(get_ds_values());

    delay(20); //Para no loopear tan rápido 
  }
  else {
    digitalWrite(LEDGN_PIN, LOW); // turn the LED GREEN off
    digitalWrite(LEDRD_PIN, LOW); // turn the LED GREEN off
    digitalWrite(LEDSS_PIN, LOW); // turn the LEDs off
  }
}
