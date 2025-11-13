#include <DFRobot_MAX30102.h>
#include <Wire.h>
#include "rgb_lcd.h"



#define HR_AVG_WINDOW 8
#define SPO2_AVG_WINDOW 2
#define SOUND_AVG_WINDOW 4
#define LIGHT_AVG_WINDOW 2

#define SAFE_HR_MAX 110
#define SAFE_HR_MIN 40

#define SAFE_SPO2_MIN 85
#define SAFE_SOUND_MAX 670
#define SAFE_LIGHT_MAX 800

#define LCD_HEIGHT 2
#define LCD_WIDTH 16


// Global Scope Variables
int lightSensorPin{A0};
int soundSensorPin{A1};

int hrBuffer[HR_AVG_WINDOW]{};
int hrIndex{};
bool hrBufferFilled{};
uint32_t avgHR{};

int spo2Buffer[SPO2_AVG_WINDOW]{};
int spo2Index{};
bool spo2BufferFilled{};
uint32_t avgSpO2{};

int soundBuffer[SOUND_AVG_WINDOW]{};
int soundIndex{};
bool SoundBifferFilled{};
uint32_t avgSound{};

int lightBuffer[LIGHT_AVG_WINDOW];
int lightIndex{};
bool lightBufferFilled{};
uint32_t avgLight{};

DFRobot_MAX30102 oximeter;
rgb_lcd lcd;
const int colorR{255};
const int colorG{255};
const int colorB{255};

// Function Declarations
void addValueTo(const uint32_t& val, int buffer[], int size, int& index, bool& isFull);
void addValues(uint32_t& hr, const uint8_t& hrV, uint32_t& sp, const uint32_t& spV, uint32_t& sound, uint32_t& light);
void assignAverageFrom(const int buffer[], int size, const int& index, const bool& isFull, uint32_t& output);
void readAndDisplayData();
void readData(uint32_t& hr, const uint8_t& hrV, uint32_t& sp, const uint32_t& spV, uint32_t& sound, uint32_t& light);
void displayData();
void readLight(uint32_t &a);
void readSound(uint32_t &a);

// Runs Once On Startup
void setup() {
  Serial.begin(115200);
  Wire.begin();

  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.setCursor(0, 0);

  while (!oximeter.begin()) {
    Serial.println("MAX30102 not found!");
    lcd.print("Sensor Not Found!");
    delay(1000);
  }

  pinMode(lightSensorPin, INPUT);
  pinMode(soundSensorPin, INPUT);


  oximeter.sensorConfiguration(
    0xff, SAMPLEAVG_8, MODE_MULTILED, SAMPLERATE_200, 
    PULSEWIDTH_411, ADCRANGE_16384
  );

  delay(100);
}

// Runs Continously During Operation
void loop() {
  uint32_t light, sound;

  readAndDisplayData();
  // unsafe funcion that just returns true based off "Logical" numbers
    
  if (SAFE_HR_MIN <= getAverageHR() <= SAFE_HR_MAX){
    Serial.print("The heart rate is at a dangerous level, "); Serial.print(avgHR);
  }
  if (getAverageSpO2() <= SAFE_SPO2_MIN){
    Serial.print("The SPO2 is too low, "); Serial.print(avgSpO2); 
  }
  if (getAverageSound() >= SAFE_SOUND_MAX){
    Serial.print("The sound is too loud, "); Serial.print(avgSound); 
  }

  if (getAverageLight() >= SAFE_LIGHT_MAX ){
    Serial.print("Its too bright, "); Serial.print(avgLight);
  }


  }

  delay(100);
}

void readLight(uint32_t& a) {
  a = analogRead(lightSensorPin);
  return;
}

void readSound(uint32_t& a) {
  a = analogRead(soundSensorPin);
  return;
}

// Add Measured Heart Rate Into the Heart Rate Buffer
void addValues(uint32_t& hr, const uint8_t& hrV, uint32_t& sp, const uint32_t& spV, uint32_t& sound, uint32_t& light) {
  // Heartrate
  if (hrV)
    addValueTo(hr, hrBuffer, HR_AVG_WINDOW, hrIndex, hrBufferFilled);

  // SpO2
  if (spV)
    addValueTo(sp, spo2Buffer, SPO2_AVG_WINDOW, spo2Index, spo2Buffer);

  // Light
  addValueTo(light, lightBuffer, LIGHT_AVG_WINDOW, lightIndex, lightBufferFilled);

  // Sound
  addValueTo(sound, soundBuffer, SOUND_AVG_WINDOW, soundIndex, SoundBifferFilled);

  return;
}

void addValueTo(const uint32_t& val, int buffer[], int size, int& index, bool& isFull) {
  buffer[index] = val;
  index = (index + 1) % size;
  if (index == 0) isFull = true;

  return;
}

// Get The Average Of All Valid Values In The Buffer
void assignAverageFrom(const int buffer[], int size, const int& index, const bool& isFull, uint32_t& output) {
  int count{isFull ? size : index};
  if (count == 0) return 0;

  output = 0;
  
  for (int i{}; i<count; i++) {
    output += buffer[i];
  }

  output /= count;

  return;
}


void readAndDisplayData() {
  int32_t spo2, heartRate;
  int8_t spo2Valid, heartRateValid;

  oximeter.heartrateAndOxygenSaturation(&spo2, &spo2Valid, &heartRate, &heartRateValid);

  readLight(light);
  readSound(sound);

  Serial.print(" | Sound: "); Serial.print(sound);
  Serial.print(" | Smoothed: "); Serial.println(avgSound);

  Serial.print(" | Light: "); Serial.println(light);
  Serial.print(" | Smoothed: "); Serial.println(avgLight);

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