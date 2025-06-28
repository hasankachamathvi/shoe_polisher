#include <Arduino.h>

//Sensor Pins
#define Trig_Clean   7
#define Echo_Clean   6
#define Trig_Polish  5
#define Echo_Polish  4

//Relay Pins
#define Relay_Duster   8   
#define Relay_Buffer   9   
#define Relay_Sprayer  10  

#define Range_Max     10.0
#define Range_Min     1.0
#define Time_Limit    20000

#define Relay_Action_Low
#ifndef Relay_Action_Low
  #define Relay_On  HIGH
  #define Relay_Off LOW
#else
  #define Relay_On  LOW
  #define Relay_Off HIGH
#endif

//Variables
bool dusterRunning = false;
int count = 0;

enum PolishPhase { idle, spraying, buffing };
PolishPhase polishStatus = idle;
unsigned long sprayStartTime = 0;

//read distance
float readRange(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, Time_Limit);
  return (duration == 0) ? -1 : (duration * 0.034 / 2);
}

void initializeSystem() {
  pinMode(Trig_Clean, OUTPUT);
  pinMode(Echo_Clean, INPUT);
  pinMode(Trig_Polish, OUTPUT);
  pinMode(Echo_Polish, INPUT);

  for(int i=8; i<10; i++){
    pinMode(i, OUTPUT);
  }

  digitalWrite(Relay_Duster, Relay_Off);
  digitalWrite(Relay_Buffer, Relay_Off);
  digitalWrite(Relay_Sprayer, Relay_Off);

  Serial.begin(9600);
  Serial.println("Shoe Robot Initialization succes!");
}

//cleaning control motor
void controlDuster(float range) {
  switch (dusterRunning) {
    case false:
      switch ((range > Range_Min && range <= Range_Max)) {
        case true:
          digitalWrite(Relay_Duster, Relay_On);
          dusterRunning = true;
          count++;
          Serial.print("Duster ON Tokens: ");
          Serial.print("Count: ");
          Serial.println(count);
          break;
      }
      break;

    case true:
      switch ((range <= Range_Min || range > Range_Max)) {
        case true:
          digitalWrite(Relay_Duster, Relay_Off);
          dusterRunning = false;
          Serial.println("Duster OFF");
          break;
      }
      break;
  }
}

//polishing control
void controlBuffer(float range) {
  switch (polishStatus) {
    case idle:
      switch ((range > Range_Min && range <= Range_Max)) {
        case true:
          switch (count > 0) {
            case true:
              digitalWrite(Relay_Sprayer, Relay_On);
              sprayStartTime = millis();
              polishStatus = spraying;
              count--;
              Serial.print("Spraying Started ");
              Serial.print("Count: ");
              Serial.println(count);
              break;

            case false:
              Serial.println("Buffer Locked");
              break;
          }
          break;
      }
      break;

    case spraying:
      switch ((millis() - sprayStartTime >= 5000)) {
        case true:
          digitalWrite(Relay_Sprayer, Relay_Off);
          digitalWrite(Relay_Buffer, Relay_On);
          polishStatus = buffing;
          Serial.println("Spray Done Buffer ON");
          Serial.println("Buffer ON");
          break;
      }
      break;

    case buffing:
      switch ((range <= Range_Min || range > Range_Max)) {
        case true:
          digitalWrite(Relay_Buffer, Relay_Off);
          polishStatus = idle;
          Serial.println("Buffer OFF");
          break;
      }
      break;
  }

  switch ((range <= Range_Min || range > Range_Max)) {
    case true:
      switch (polishStatus) {
        case spraying:
          digitalWrite(Relay_Sprayer, Relay_Off);
          polishStatus = idle;
          Serial.println("Reset");
          break;
      }
      break;
  }
}

void setup() {
  initializeSystem();
}

void loop() {
  float rangeDust = readRange(Trig_Clean, Echo_Clean);
  float rangePolish = readRange(Trig_Polish, Echo_Polish);

  controlDuster(rangeDust);
  controlBuffer(rangePolish);

  delay(500);
}
