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

#define I_HR_FACTOR 130
#define NORM_F 6/10

// Global Scope Variables
int lightSensorPin{A0};
int soundSensorPin{A1};

int hrBuffer[HR_AVG_WINDOW]{};
int hrIndex{};
bool hrBufferFilled{};
int32_t avgHR{};

int spo2Buffer[SPO2_AVG_WINDOW]{};
int spo2Index{};
bool spo2BufferFilled{};
int32_t avgSpO2{};

int soundBuffer[SOUND_AVG_WINDOW]{};
int soundIndex{};
bool SoundBufferFilled{};
int32_t avgSound{};

int lightBuffer[LIGHT_AVG_WINDOW];
int lightIndex{};
bool lightBufferFilled{};
int32_t avgLight{};

DFRobot_MAX30102 oximeter;
rgb_lcd lcd;
const int colorR{255};
const int colorG{255};
const int colorB{255};

// Function Declarations
void addValueTo(const int32_t& val, int buffer[], int size, int& index, bool& isFull);
void addValues(int32_t& hr, const int8_t& hrV, int32_t& sp, const int32_t& spV, int32_t& sound, int32_t& light);
void getAverageFrom(const int buffer[], int size, const int& index, const bool& isFull, int32_t& output);
void readAndDisplayData();
void readData(int32_t& hr, const int8_t& hrV, int32_t& sp, const int32_t& spV, int32_t& sound, int32_t& light);
void displayData();
void readLight(int32_t &a);
void readSound(int32_t &a);

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
  int32_t hr, spo2, light, sound;
  int8_t hrV, spV;

  readData(hr, hrV, spo2, spV, sound, light);

  if (hrV) avgHR = getAverageFrom(hrBuffer, HR_AVG_WINDOW, hrIndex, hrBufferFilled);
  if (spV) avgSpO2 = getAverageFrom(spo2Buffer, SPO2_AVG_WINDOW, spo2Index, spo2BufferFilled);
  avgLight = getAverageFrom(lightBuffer, LIGHT_AVG_WINDOW, lightIndex, lightBufferFilled);
  avgSound = getAverageFrom(soundBuffer, SOUND_AVG_WINDOW, soundIndex, SoundBufferFilled);

  displayData(hrV, spV);
  // unsafe funcion that just returns true based off "Logical" numbers
    
  if (SAFE_HR_MIN <= avgHR || avgHR <= SAFE_HR_MAX){
    Serial.print("The heart rate is at a dangerous level, "); Serial.print(avgHR);
  }
  if (avgSpO2 <= SAFE_SPO2_MIN){
    Serial.print("The SPO2 is too low, "); Serial.print(avgSpO2); 
  }
  if (avgSound >= SAFE_SOUND_MAX){
    Serial.print("The sound is too loud, "); Serial.print(avgSound); 
  }
  if (avgLight >= SAFE_LIGHT_MAX ){
    Serial.print("Its too bright, "); Serial.print(avgLight);
  }

  delay(100);
}

// Add Measured Heart Rate Into the Heart Rate Buffer
void addValues(int32_t& hr, const int8_t& hrV, int32_t& sp, const int32_t& spV, int32_t& sound, int32_t& light) {
  // Heartrate
  if (hrV)
    if (hr > I_HR_FACTOR){
      addValueTo(hr*NORM_F, hrBuffer, HR_AVG_WINDOW, hrIndex, hrBufferFilled);
    }
    else{
    addValueTo(hr, hrBuffer, HR_AVG_WINDOW, hrIndex, hrBufferFilled);
    }

  // SpO2
  if (spV)
    addValueTo(sp, spo2Buffer, SPO2_AVG_WINDOW, spo2Index, spo2BufferFilled);

  // Light
  addValueTo(light, lightBuffer, LIGHT_AVG_WINDOW, lightIndex, lightBufferFilled);

  // Sound
  addValueTo(sound, soundBuffer, SOUND_AVG_WINDOW, soundIndex, SoundBufferFilled);

  return;
}

void addValueTo(const int32_t& val, int buffer[], int size, int& index, bool& isFull) {
  buffer[index] = val;
  index = (index + 1) % size;
  if (index == 0) isFull = true;

  return;
}

// Get The Average Of All Valid Values In The Buffer
int32_t getAverageFrom(const int buffer[], int size, const int& index, const bool& isFull) {
  int count{isFull ? size : index};
  if (count == 0) return 0;

  int32_t output{};
  
  for (int i{}; i<count; i++) {
    output += buffer[i];
  }

  return output/count;
}

void readData(int32_t& hr, int8_t& hrV, int32_t& sp, int8_t& spV, int32_t& sound, int32_t& light) {
  // Heartrate and SPo2
  oximeter.heartrateAndOxygenSaturation(&sp, &spV, &hr, &hrV);

  // Sound
  sound = analogRead(soundSensorPin);

  // Light
  light = analogRead(lightSensorPin);

  return;
}

void displayData(const int8_t& hrV, const int8_t& spV) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("L: "); lcd.print(avgLight); lcd.print(" | S: "); lcd.println(avgSound);

  if (hrV) {
    avgHR = getAverageFrom(hrBuffer, HR_AVG_WINDOW, hrIndex, hrBufferFilled);

    Serial.print("HR: "); Serial.print(avgHR); Serial.print(" & "); Serial.print(hrBuffer[hrIndex]); Serial.print(" | ");

    lcd.print("HR:"); lcd.print(avgHR);

    if (spV) {
      avgSpO2 = getAverageFrom(spo2Buffer, SPO2_AVG_WINDOW, spo2Index, spo2BufferFilled);

      Serial.print("SpO2: "); Serial.print(avgHR); Serial.print(" & "); Serial.print(hrBuffer[hrIndex]); Serial.print(" | ");

      lcd.print(" | O2:"); lcd.print(avgSpO2);
    } else {
      lcd.print(" | O2...");
    }
  } else {
    lcd.print("No Finger Found!");
  }
}