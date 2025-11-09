#include <DFRobot_MAX30102.h>
#include <Wire.h>
#include "rgb_lcd.h"

#define AVG_WINDOW 8
#define SAFE_HR_MAX 110
#define SAFE_HR_MIN 40

// Global Scope Variables
int hrBuffer[AVG_WINDOW]{};
int hrIndex{};
bool bufferFilled{};
uint32_t avgHR{};
DFRobot_MAX30102 oximeter;
rgb_lcd lcd;
const int colorR{255};
const int colorG{255};
const int colorB{255};

// Function Declarations
void addHeartRate(int hr);
uint32_t getAverageHR();
void readAndDisplayData();

// Runs Once On Startup
void setup() {
  Serial.begin(115200);
  Wire.begin();

  lcd.begin(16,2);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.setCursor(0, 0);

  while (!oximeter.begin()) {
    Serial.println("MAX30102 not found!");
    lcd.print("Sensor Not Found!");
    delay(1000);
  }

  oximeter.sensorConfiguration(
    128, SAMPLEAVG_8, MODE_MULTILED, SAMPLERATE_400, 
    PULSEWIDTH_411, ADCRANGE_16384
  );

  delay(100);
}

// Runs Continously During Operation
void loop() {
  readAndDisplayData();

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

void readAndDisplayData() {
  int32_t spo2, heartRate;
  int8_t spo2Valid, heartRateValid;

  oximeter.heartrateAndOxygenSaturation(&spo2, &spo2Valid, &heartRate, &heartRateValid);

  if (heartRateValid) {
    addHeartRate(heartRate);
    avgHR = getAverageHR();
    Serial.print("HR: "); Serial.print(heartRate);
    Serial.print(" | Smoothed: "); Serial.println(avgHR);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HeartRate:");
    lcd.print(avgHR);
    lcd.print("bpm");

    lcd.setCursor(0, 1);
    if (spo2Valid) {
      lcd.print("SpO2: ");
      lcd.print(spo2);
      lcd.print(" %");
    } else {
      lcd.print("Measuring SpO2%");
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Finger Not Found");
  } 
}