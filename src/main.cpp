#include <Arduino.h>

enum Mode { idle, start, run, shut};
const uint8_t IGN_SENSOR_PIN = A0;
const uint8_t ILL_SENSOR_PIN = A1;
const uint8_t RELAY_PIN = 9;
const uint8_t SHUT_PIN = 10;
const uint8_t ILL_PIN = 11;
const unsigned short MAX_RETRY = 50;
const bool REVERSE_HIGH_LOW = false;
const uint8_t QUEUE_SIZE = 4;

float ILLUMINATION_LIMIT = 0.9;
float IGNITION_LIMIT = 3.1;
unsigned short i = 0;
unsigned short a = 0;
float voltageILL = 0;
float voltageIGN = 0;
bool isLedActivated = false;
Mode mode = idle;

float illQueue[QUEUE_SIZE];


float readInputVoltage(uint8_t pin) {
  return (analogRead(pin) * 5.0)/1024;
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

// Reverse the level if REVERSE_HIGH_LOW is true else keep the same level(used for reversed relay action)
int getLevel(int level) {
  if (!REVERSE_HIGH_LOW) {
    return level;
  }
  else {
    return level == HIGH ? LOW : HIGH;
  }
}

void addToIllQueue(float number) {
  for (int i = QUEUE_SIZE; i>0; i--) {
    illQueue[i] = illQueue[i-1];
  }
  illQueue[0] = number;
}

float getIllQueueMean() {
  float d = 0;
  for (float i:illQueue) {
    d += i;
  }
  return d/QUEUE_SIZE;
}

void updateIllVoltageAndApplyBrightness() {
  voltageILL = readInputVoltage(ILL_SENSOR_PIN);
  Serial.print("Last ILL voltage: ");
  Serial.println(voltageILL);
  addToIllQueue(voltageILL);
  voltageILL = getIllQueueMean();
  Serial.print(" Mean ILL voltage: ");
  Serial.println(voltageILL);
  digitalWrite(ILL_PIN, getLevel(voltageILL < ILLUMINATION_LIMIT ? HIGH: LOW));
}

void waitForStartComplete() {
  digitalWrite(RELAY_PIN, getLevel(HIGH));
  Serial.println("Wait for complete start");
  a = 0;
  for (a=0; a<200; a++) {
    updateIllVoltageAndApplyBrightness();
    delay(400);
  }
  //delay(80000);
  Serial.println("Start complete");
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SHUT_PIN, OUTPUT);
  pinMode(ILL_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  if (REVERSE_HIGH_LOW) {
    digitalWrite(ILL_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(SHUT_PIN, HIGH);
  }
}

void loop() {
  printMode();

  switch (mode)
  {
  case idle:
    voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
    Serial.print("IGN voltage: ");
    Serial.println(voltageIGN);

    if (voltageIGN > IGNITION_LIMIT) {
      delay(200);
      voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
      Serial.println("Test start confirmation");
      Serial.print("IGN voltage: ");
      Serial.println(voltageIGN);

      if (voltageIGN > IGNITION_LIMIT) {
        mode = start;
      }
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
        waitForStartComplete();
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
        waitForStartComplete();
        mode = run;
      }
    }
    break;

  case run:
    digitalWrite(RELAY_PIN, getLevel(HIGH));
    updateIllVoltageAndApplyBrightness();
  
    voltageIGN = readInputVoltage(IGN_SENSOR_PIN);
    Serial.print("IGN voltage: ");
    Serial.println(voltageIGN);
    delay(150);

    if (voltageIGN < IGNITION_LIMIT) {
      delay(200);
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
    digitalWrite(SHUT_PIN, getLevel(HIGH));
    Serial.println("Waiting for shutdown");
    delay(15000);
    digitalWrite(ILL_PIN, getLevel(LOW));
    digitalWrite(RELAY_PIN, getLevel(LOW));
    digitalWrite(SHUT_PIN, getLevel(LOW));
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Shutdown complete");
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