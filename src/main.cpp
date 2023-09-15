#include <Arduino.h>

enum Mode { idle, start, run, shut};
const uint8_t IGN_SENSOR_PIN = A0;
const uint8_t ILL_SENSOR_PIN = A1;
const uint8_t SHUT_PIN = 7;
const uint8_t ILL_PIN = 8;
const uint8_t RELAY_PIN = 9;
const unsigned short MAX_RETRY = 50;
float ILLUMINATION_LIMIT = 0.9;
float IGNITION_LIMIT = 3.2;
unsigned short i = 0;
float voltageILL = 0;
float voltageIGN = 0;
bool isLedActivated = false;
Mode mode = idle;


float readInputVoltage(uint8_t pin) {
  return (analogRead(pin) * 3.3)/1024;
}

void printMode() {
  Serial.print("Mode: ");
  switch (mode)
  {
  case idle:
    Serial.println("idle");
    break;

  case start:
    Serial.println("start");
    break;

  case run:
    Serial.println("run");
    break;

  case shut:
    Serial.println("shut");
    break;
  
  default:
    Serial.println("unknown");
    break;
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SHUT_PIN, OUTPUT);
  pinMode(ILL_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  printMode();

  switch (mode)
  {
  case idle:
    voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
    Serial.print("IGN voltage: ");
    Serial.println(voltageIGN);

    if (voltageIGN > 3) {
      mode = start;
    }
    else {
      delay(1000);
    }
    break;

  case start:
    i = 0;
    Serial.println("Waiting for ignition shutdown");

    do{
      isLedActivated = !isLedActivated;
      digitalWrite(LED_BUILTIN, isLedActivated ? HIGH : LOW);
      delay(200);
      voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
      i++;
      Serial.print("IGN voltage: ");
      Serial.println(voltageIGN);
    }
    while(i < MAX_RETRY && voltageIGN > IGNITION_LIMIT);
    digitalWrite(LED_BUILTIN, LOW);

    if (i >= MAX_RETRY) {
      voltageIGN = readInputVoltage(IGN_SENSOR_PIN);

      if (voltageIGN > IGNITION_LIMIT) {
        mode = run;
      }
      break;
    }
    else {
      Serial.println("Waiting for 2nd ignition");
      i = 0;

      do{
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
        i++;
        Serial.print("IGN voltage: ");
        Serial.println(voltageIGN);
      }
      while(i < MAX_RETRY && voltageIGN <= IGNITION_LIMIT);
      
      digitalWrite(LED_BUILTIN, LOW);
      voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
      if (voltageIGN <= IGNITION_LIMIT) {
        Serial.print("i = ");
        Serial.println(i);

        Serial.println("Start aborted");
        mode = idle;
      }
      else {
        Serial.println("Start complete");
        mode = run;
      }
    }
    break;

  case run:
    digitalWrite(RELAY_PIN, HIGH);
    voltageILL = readInputVoltage(ILL_SENSOR_PIN);
    Serial.print("ILL voltage: ");
    Serial.println(voltageILL);

    digitalWrite(ILL_PIN, voltageILL < ILLUMINATION_LIMIT ? HIGH: LOW);
    voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
    Serial.print("IGN voltage: ");
    Serial.println(voltageIGN);
    delay(400);

    if (voltageIGN < IGNITION_LIMIT) {
      voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
      Serial.println("Test shut confirmation");
      Serial.print("IGN voltage: ");
      Serial.println(voltageIGN);

      if (voltageIGN < IGNITION_LIMIT) {
        mode = shut;
      }
    }

    break;

  case shut:
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(ILL_PIN, LOW);
    digitalWrite(SHUT_PIN, HIGH);
    delay(5000);
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(SHUT_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    mode = idle;
    break;
  
  default:
    delay(500);
    break;
  }

  Serial.println("------------");
  //float voltageA1 = (analogRead(A1) * 3.3)/1024;
  //int relayPosition = voltageA1 < 3 ? LOW : HIGH;

  //Serial.print("Voltage A1 : ");
  //Serial.println(voltageA1);

  //digitalWrite(LED_BUILTIN, relayPosition);
  //digitalWrite(10, relayPosition);
  delay(1500);
}