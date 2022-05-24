/*
  Wilson Medeiros
  https://github.com/Mortanius
*/
#include <math.h>
#include "projeto.h"

#define PIN_US_TRIGGER 12
#define PIN_US_ECHO 11
#define PIN_RELAY 13

#define MS_TO_CM 58

typedef struct {
  // minimum time measured
  uint16_t minTime;
  // maximum time measured
  uint16_t maxTime;
  // height relative to water tank's top
  uint8_t height;
} UltrasonicSensor;

const UltrasonicSensor us = {.minTime = 116, .maxTime = 23200, .height = 51};

uint16_t measureDistanceCm(measure_error* err) {
  digitalWrite(PIN_US_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIGGER, LOW);
  
  while(digitalRead(PIN_US_ECHO) == LOW);
  auto t = micros();
  while(digitalRead(PIN_US_ECHO) == HIGH);
  auto delta = (uint16_t) (micros() - t);
  
  uint16_t distance;
  measure_error merr;
  if (delta < us.minTime) {
    merr = BELOW_MIN;
    distance = us.minTime / MS_TO_CM;
  } else if (delta > us.maxTime) {
    merr = ABOVE_MAX;
    distance = us.maxTime / MS_TO_CM;
  } else {
    merr = NONE;
    distance = delta / MS_TO_CM;
  }
  if (err != NULL) {
    *err = merr;
  }
  return distance;
}

bool isPumpOn = false;

void turnOnPump() {
  Serial.println("TURNING ON PUMP");
  digitalWrite(PIN_RELAY, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  isPumpOn = true;
}

void turnOffPump() {
  Serial.println("TURNING OFF PUMP");
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  isPumpOn = false;
}

// Truncated Cone Volume in Liters
// useful reference: https://www.omnicalculator.com/math/cone-volume#truncated-cone-volume-volume-of-frustum
float truncatedConeVolumeLiters(float h, float r, float R) {
  return 0.001 * (1.0 / 3.0) * M_PI * h * (r*r + r*R + R*R);
}

WaterTank newWaterTank(uint16_t capacity, uint16_t height, uint16_t diameterMin, uint16_t diameterMax) {
  WaterTank wt = {
    .capacity = capacity,
    .capacityError = 0,
    .height = height,
    .diameterMin = diameterMin,
    .diameterMax = diameterMax
  };
  wt.capacityError = truncatedConeVolumeLiters(float(wt.height), float(wt.diameterMin) / 2.0, float(wt.diameterMax) / 2.0) - float(wt.capacity);
  return wt;
}

const WaterTank wt = newWaterTank(15000, 313, 267, 315);


float diameterAtLevel(uint16_t level) {
  auto deltaDiameter = float(int32_t(wt.diameterMax) - int32_t(wt.diameterMin));
  return float(wt.diameterMin) + deltaDiameter * float(level) / float(wt.height);
}

float capacityErrorAtLevel(uint16_t level) {
  return float(level) / float(wt.height) * wt.capacityError;
}

uint16_t waterVolumeAtLevelL(uint16_t level) {
  auto diameter = diameterAtLevel(level);
  Serial.print("Diameter in cm: ");
  Serial.println(diameter);
  auto capacityError = capacityErrorAtLevel(level);
  Serial.print("Capacity Error in L: ");
  Serial.println(capacityError);
  auto volume = truncatedConeVolumeLiters(float(level), float(wt.diameterMin) / 2.0, diameter / 2.0);
  Serial.print("Trunc Cone Volume in L: ");
  Serial.println(volume);
  return uint16_t(round(volume - capacityError));
}

float volumeAsCapacityPerc(uint16_t volume) {
  return float(volume) / float(wt.capacity) * 100.0;
}

const uint16_t measureInterval = 5000;

// thresholds in percentage
const uint8_t thresholdOn = 50;
const uint8_t thresholdOff = 95;

int distance;
measure_error err;

void setup() {
  pinMode(PIN_US_TRIGGER, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_US_TRIGGER, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);
}

void loop() {
  Serial.println("calculating distance...");
  distance = measureDistanceCm(&err);
  Serial.print("Distance in CM: ");
  Serial.println(distance);
  if (err != NONE) {
    Serial.print("inaccurate measurement: ");
    switch(err) {
      case BELOW_MIN: {
        Serial.println("below minimum");
        break;
      }
      case ABOVE_MAX: {
        Serial.println("above maximum");
        break;
      }
    }
  }
  auto waterLevel = int32_t(wt.height) + int32_t(us.height) - int32_t(distance);
  Serial.print("Level in cm: ");
  Serial.println(waterLevel);
  if (waterLevel < 0) {
    Serial.println("WARN: Calculated water level was negative. Setting to 0. Make sure the water tank parameters are correct");
    waterLevel = 0;
  }
  auto waterVolume = waterVolumeAtLevelL(waterLevel);
  if (waterVolume > wt.capacity) {
    Serial.println("WARN: Calculated water level exceeds capacity. Make sure the water tank and ultrasonic sensor parameters are correct");
  }
  auto waterPercentage = volumeAsCapacityPerc(waterVolume);
  Serial.print("Volume in L: ");
  Serial.print(waterVolume);
  Serial.print(" (");
  Serial.print(waterPercentage);
  Serial.println("%)");
  auto waterPercentageFloor = uint8_t(floor(waterPercentage));
  if (isPumpOn && waterPercentageFloor > thresholdOff) {
    turnOffPump();
  } else if (!isPumpOn && waterPercentageFloor < thresholdOn) {
    turnOnPump();
  }
  delay(measureInterval);
}
