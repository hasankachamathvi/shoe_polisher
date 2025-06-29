#include <Arduino.h>

#define Trig_Clean   7
#define Echo_Clean   6
#define Trig_Polish  5
#define Echo_Polish  4

#define Relay_Duster   8   
#define Relay_Buffer   9   
#define Relay_Sprayer  10  

#define Range_Max     10.0
#define Range_Min     1.0
#define Time_Limit    20000

#define Relay_Action_Low
#ifdef Relay_Action_Low
  #define Relay_On  LOW
  #define Relay_Off HIGH
#else
  #define Relay_On  HIGH
  #define Relay_Off LOW
#endif

bool dusterRunning = false;
int count = 0;

enum PolishPhase { idle, spraying, buffing };
PolishPhase polishStatus = idle;
unsigned long sprayStartTime = 0;

//read distance from sensor
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

  pinMode(Relay_Duster, OUTPUT);
  pinMode(Relay_Buffer, OUTPUT);
  pinMode(Relay_Sprayer, OUTPUT);

  digitalWrite(Relay_Duster, Relay_Off);
  digitalWrite(Relay_Buffer, Relay_Off);
  digitalWrite(Relay_Sprayer, Relay_Off);

  Serial.begin(9600);
  Serial.println("success!");
}

//control cleaning
void controlDuster(float range) {
  switch (dusterRunning) {
    case false:
      switch ((range > Range_Min && range <= Range_Max)) {
        case true:
          digitalWrite(Relay_Duster, Relay_On);
          dusterRunning = true;
          count++;
          Serial.print("Duster working");
          Serial.print("count: ");
          Serial.println(count);
          break;
      }
      break;

    case true:
      switch ((range <= Range_Min || range > Range_Max)) {
        case true:
          digitalWrite(Relay_Duster, Relay_Off);
          dusterRunning = false;
          Serial.println("Duster Off");
          break;
      }
      break;
  }
}

//control polishing
void controlBuffer(float range) {
  switch (polishStatus) {
    case idle:
      if (range > Range_Min && range <= Range_Max) {
        if (count > 0) {
          digitalWrite(Relay_Sprayer, Relay_On);
          sprayStartTime = millis();
          polishStatus = spraying;
          count--;
          Serial.print("Spraying Started");
          Serial.print("count: ");
          Serial.println(count);
        } else {
          Serial.println("Buffer Locked");
        }
      }
      break;

    case spraying:
      if (millis() - sprayStartTime >= 5000) {
        digitalWrite(Relay_Sprayer, Relay_Off);
        digitalWrite(Relay_Buffer, Relay_On);
        polishStatus = buffing;
        Serial.println("Spray Done");//buffer on
      }
      break;

    case buffing:
      if (range <= Range_Min || range > Range_Max) {
        digitalWrite(Relay_Buffer, Relay_Off);
        polishStatus = idle;
        Serial.println("Buffer Off");
      }
      break;
  }

  //if shoe removed too early reset spray 
  if ((range <= Range_Min || range > Range_Max) && polishStatus == spraying) {
    digitalWrite(Relay_Sprayer, Relay_Off);
    polishStatus = idle;
    Serial.println("reset");
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