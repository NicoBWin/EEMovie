#include <Arduino.h>

#define CLIENT_NO 0
#define CLIENT_SI 1
#define HEAT_OFF  0
#define HEAT_ON   1

#define STATUS_LED_PIN  13

#define PERIOD_TICKS  8


unsigned long status_led_time = 0;
int client_state;
int power_state;
int status_led_state;


void init_status_led()
{
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  client_state = CLIENT_NO;
  power_state = HEAT_OFF;
  status_led_state = LOW;

  Serial.print("Client State: ");
  Serial.println(client_state);

  Serial.print("Power State: ");
  Serial.println(power_state);

  Serial.print("LED State: ");
  Serial.println(status_led_state);
}

void upd_client_status( int c_state)
{
  client_state = c_state;
}

void upd_power_status( int p_state)
{
  power_state = p_state;
}

void cycle_status_led()
{
  if(client_state == CLIENT_NO) {
    if(status_led_time == 0) {
      digitalWrite(STATUS_LED_PIN, HIGH);
    }
    if(status_led_time == 1) {
      digitalWrite(STATUS_LED_PIN, LOW);
    }
    if(status_led_time == 2) {
      digitalWrite(STATUS_LED_PIN, HIGH);
    }
    if(status_led_time == 3) {
      digitalWrite(STATUS_LED_PIN, LOW);
    }
  }

  if(client_state == CLIENT_SI) {
    if(power_state == HEAT_OFF) {
      if(status_led_time == 0) {
        digitalWrite(STATUS_LED_PIN, HIGH);
      }
      if(status_led_time == 3) {
        digitalWrite(STATUS_LED_PIN, LOW);
      }
    }
    else if(power_state == HEAT_ON) {
      digitalWrite(STATUS_LED_PIN, HIGH);
    }
  }
  
  status_led_time++;
  if(status_led_time == PERIOD_TICKS) {
    status_led_time = 0;
  }
  Serial.println(status_led_time);
}
