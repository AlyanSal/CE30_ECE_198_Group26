#include <DFRobot_MAX30102.h>
#include <Wire.h>
#include "rgb_lcd.h"

#define AVG_WINDOW 8
#define SAFE_HR_MAX 110
#define SAFE_HR_MIN 40

// Global Scope Variables
int touchSensorPin{4};
int hrBuffer[AVG_WINDOW]{};
int hrIndex{};
bool bufferFilled{};
uint32_t avgHR{};
DFRobot_MAX30102 oximeter;
bool oximeterState{};
rgb_lcd lcd;
const int colorR{255};
const int colorG{255};
const int colorB{255};

// Function Declarations
void addHeartRate(int hr);
uint32_t getAverageHR();
void configureSensor();

// Runs Once On Startup
void setup() {
  Serial.begin(115200);
  Wire.begin();

  configureSensor(true);

  while (!oximeter.begin()) {
    Serial.println("MAX20102 not found!");
    delay(1000);
  }

  pinMode(touchSensorPin, INPUT);

  lcd.begin(16,2);
  lcd.setRGB(colorR, colorG, colorB);

  delay(1000);
}

// Runs Continously During Operation
void loop() {
  int touchState{digitalRead(touchSensorPin)};

  if (touchState == HIGH && !oximeterState) {
    configureSensor(true);
    lcd.clear();
    lcd.print("Measuring...");
    oximeterState = true;
    Serial.println("Oximeter ON");
  } else if (touchState == LOW && oximeterState) {
    configureSensor(false);
    lcd.print("Standby");
    oximeterState = false;
    Serial.println("Oximeter OFF");
  }

  if (oximeterState) {
    readAndDisplayData();
  }

  delay(100);
}

// Add Measured Heart Rate Into the Heart Rate Buffer
void addHeartRate(int hr) {
  hrBuffer[hrIndex] = hr;
  hrIndex = (hrIndex + 1) % AVG_WINDOW;
  if (hrIndex == 0) bufferFilled = true;

  return;
}

// Get The Average Of All Valid Values In The Buffer
uint32_t getAverageHR() {
  int count{bufferFilled  ? AVG_WINDOW : hrIndex};
  if (count == 0) return 0;
  uint32_t sum{};
  for (int i{}; i<count; i++) sum += hrBuffer[i];
  return sum/count;
}

void configureSensor(bool onState) {
  if (onState) {
    oximeter.sensorConfiguration(
        128, SAMPLEAVG_8, MODE_MULTILED, SAMPLERATE_400, 
        PULSEWIDTH_411, ADCRANGE_16384);
  } else {
    oximeter.sensorConfiguration(
      0, SAMPLEAVG_1, MODE_REDONLY, SAMPLERATE_50, 
      PULSEWIDTH_69, ADCRANGE_2048);
  }
}

void readAndDisplayData() {
    int32_t spo2, heartRate;
  int8_t spo2Valid, heartRateValid;

  oximeter.heartrateAndOxygenSaturation(&spo2, &spo2Valid, &heartRate, &heartRateValid);

  if (heartRateValid) {
    addHeartRate(heartRate);
    avgHR = getAverageHR();
    Serial.print("HR: "); Serial.print(heartRate);
    Serial.print("Smoothed: "); Serial.println(avgHR);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HeartRate:");
    if (avgHR % 10 < 10) lcd.print(" ");
    lcd.print(avgHR);
    lcd.print("bpm");

    if (spo2Valid) {
      lcd.setCursor(0, 1);
      lcd.print("SpO2: ");
      lcd.print(spo2);
      lcd.print(" %");
    }
  } 
}