#include <DFRobot_MAX30102.h>
#include <Wire.h>
#include "rgb_lcd.h"

#define HR_AVG_WINDOW 8
#define SPO2_AVG_WINDOW 4
#define SAFE_HR_MAX 110
#define SAFE_HR_MIN 40

// Global Scope Variables
int hrBuffer[HR_AVG_WINDOW]{};
int hrIndex{};
int spo2Buffer[SPO2_AVG_WINDOW]{};
int spo2Index{};
bool hrBufferFilled{};
bool spo2BufferFilled{};
uint32_t avgHR{};
uint32_t avgSpO2{};
DFRobot_MAX30102 oximeter;
rgb_lcd lcd;
const int colorR{255};
const int colorG{255};
const int colorB{255};

// Function Declarations
void addHeartRate(int hr);
void addSpO2(int spo2);
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
    255, SAMPLEAVG_8, MODE_MULTILED, SAMPLERATE_200, 
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
  hrIndex = (hrIndex + 1) % HR_AVG_WINDOW;
  if (hrIndex == 0) hrBufferFilled = true;

  return;
}

void addSpO2(int spo2) {
  spo2Buffer[spo2Index] = spo2;
  spo2Index = (spo2Index + 1) % SPO2_AVG_WINDOW;
  if (spo2Index == 0) spo2BufferFilled = true;

  return;
}

// Get The Average Of All Valid Values In The Buffer
uint32_t getAverageHR() {
  int count{hrBufferFilled  ? HR_AVG_WINDOW : hrIndex};
  if (count == 0) return 0;
  uint32_t sum{};
  for (int i{}; i<count; i++) sum += hrBuffer[i];
  return sum/count;
}

uint32_t getAverageSpO2() {
  int count{spo2BufferFilled ? SPO2_AVG_WINDOW : spo2Index};
  if (count == 0) return 0;
  uint32_t sum{};
  for (int i{};i<count; i++) sum += spo2Buffer[i];
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
      addSpO2(spo2);
      avgSpO2 = getAverageSpO2();

      Serial.print("SpO2: "); Serial.print(spo2);
      Serial.print(" | Smoothed SpO2: "); Serial.println(avgSpO2);

      lcd.print("SpO2: ");
      lcd.print(avgSpO2);
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