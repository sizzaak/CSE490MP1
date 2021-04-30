#include <PushButton.h>
#include "src/RGBConverter/RGBConverter.h"

// LEDs
const int TRIPLE_STAR_PIN = 3;
const int RIGHT_STAR_PIN = 5;
const int LEFT_STAR_PIN = 6;
const int RGB_BLUE_PIN = 9;
const int RGB_GREEN_PIN = 10;
const int RGB_RED_PIN = 11;

// Buttons
const int MODE_BUTTON_PIN = 2;
const int RIGHT_BUTTON_PIN = 12;
const int LEFT_BUTTON_PIN = 13;

// Variable inputs
const int PHOTORESISTOR_PIN = A0;
const int TIME_DIAL_PIN = A1;

// Mode 0 constants
const bool COMMON_ANODE = false;
const byte MAX_RGB_VALUE = 255;
const float HUE_MIN = 0.010f;
const float HUE_MAX = 0.100f;
const float HUE_STEP_START = 0.002f;

// Mode 1 constants
// Use another script to read and callibrate these levels
const int DAWN_LEVEL = 360;
const int DAY_LEVEL = 570;
const int DUSK_LEVEL = 740;
const int NIGHT_LEVEL = 950;

RGBConverter _rgbConverter;

PushButton _modeButton(MODE_BUTTON_PIN);
PushButton _rightButton(RIGHT_BUTTON_PIN);
PushButton _leftButton(LEFT_BUTTON_PIN);

int _mode = 0;

float _hue = HUE_MIN;
float _hueStep = HUE_STEP_START;
float _hueGameStep = 0.001f;

int _gameStage = 1;

void setup() {
  pinMode(TRIPLE_STAR_PIN, OUTPUT);
  pinMode(RIGHT_STAR_PIN, OUTPUT);
  pinMode(LEFT_STAR_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_RED_PIN, OUTPUT);

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);

  //Serial.begin(9600);
  
  initMode0();  
}

void loop() {
  updateButtons();
  if (_modeButton.isClicked()) {
    if (_mode == 0) {
      _mode = 1;
      initMode1();
      exitMode0();
    } else if (_mode == 1) {
      _mode = 2;
      initMode2();
      exitMode1();
    } else {
      _mode = 0;
      initMode0();
      exitMode2();
    }
  }
  if (_mode == 0) {
    runMode0();
  } else if (_mode == 1) {
    runMode1();
  } else {
    runMode2();
  }
}

void runMode0() {
  // Run crossfade
  _hue += _hueStep;
  if (_hue >= HUE_MAX || _hue <= HUE_MIN) {
    _hueStep = -_hueStep;
  }

  // Calculate brightness based on photoresistor
  int phrVal = analogRead(PHOTORESISTOR_PIN);
  float brightness = (1023 - phrVal) / 1023.0;
  int starBrightness = brightness * 255;

  // Convert from HSL to RGB and light up the LEDs
  byte rgb[3];
  _rgbConverter.hslToRgb(_hue, 1, brightness / 2, rgb);
  setRGB(rgb[0], rgb[1], rgb[2]);
  analogWrite(TRIPLE_STAR_PIN, starBrightness);
  analogWrite(RIGHT_STAR_PIN, starBrightness);
  analogWrite(LEFT_STAR_PIN, starBrightness);
  delay(50);
}

void initMode0() {
  _hue = HUE_MIN;
  _hueStep = HUE_STEP_START;
}

void exitMode0() {
  
}

void runMode1() {
  int dialVal = analogRead(TIME_DIAL_PIN);
  int lightLevel = calculateLight(dialVal);
  updateSky(lightLevel);
  delay(50);
}

void initMode1() {
  
}

void exitMode1() {
  
}

void runMode2() {
  byte rgb[3];
  _rgbConverter.hslToRgb(_hue, 1, 0.5, rgb);
  setRGB(rgb[0], rgb[1], rgb[2]);
  
  // update hue based on step size
  _hue += _hueGameStep;

  // hue ranges between 0-1, so if > 1, reset to 0
  if(_hue > 1.0){
    _hue = 0;
  }

  // Start a round
  if (_leftButton.isClicked() || _rightButton.isClicked()) {
    if (playRound(_gameStage)) {
      _gameStage++;
    } else {
      _gameStage = 1;
    }
  }
}

void initMode2() {
  _gameStage = 1;
  digitalWrite(TRIPLE_STAR_PIN, LOW);
  digitalWrite(RIGHT_STAR_PIN, LOW);
  digitalWrite(LEFT_STAR_PIN, LOW);
}

void exitMode2() {
  
}

void setRGB(int red, int green, int blue)
{
  // If a common anode LED, invert values
  if(COMMON_ANODE == true){
    red = MAX_RGB_VALUE - red;
    green = MAX_RGB_VALUE - green;
    blue = MAX_RGB_VALUE - blue;
  }
  analogWrite(RGB_RED_PIN, red);
  analogWrite(RGB_GREEN_PIN, green);
  analogWrite(RGB_BLUE_PIN, blue);  
}

// 100 = day, 50 = mid sunset/sunrise, 0 = night
void updateSky(int lightLevel) {
  float brightness = 1.0; // out of 1
  if (lightLevel > 50) {
    // Caculate hue on a scale from sunrise/sunset red to midday yellow
    _hue = (((lightLevel - 50) / 50.0f) * (HUE_MAX - HUE_MIN)) + HUE_MIN;
    // Stars are not visible
    digitalWrite(TRIPLE_STAR_PIN, LOW);
    digitalWrite(RIGHT_STAR_PIN, LOW);
    digitalWrite(LEFT_STAR_PIN, LOW);
  } else {
    _hue = HUE_MIN;
    if (lightLevel > 25) {
      brightness = (lightLevel - 25) / 25.0; // sun fades to dark at 25
    } else {
      brightness = 0; // sun gone below 25
    }
    int starBrightness = map(lightLevel, 50, 0, 0, 255); // brighter stars the darker it is
    analogWrite(TRIPLE_STAR_PIN, starBrightness);
    analogWrite(RIGHT_STAR_PIN, starBrightness);
    analogWrite(LEFT_STAR_PIN, starBrightness);
  }
  byte rgb[3];
  _rgbConverter.hslToRgb(_hue, 1, brightness / 2, rgb);
  setRGB(rgb[0], rgb[1], rgb[2]);
}

// Returns a light level between 0 and 100 for updateSky(int lightLevel)
int calculateLight(int dialVal) {
  if (dialVal < DAWN_LEVEL) { // early dawn
    return map(dialVal, 0, DAWN_LEVEL, 0, 50);
  } else if (dialVal >= DAWN_LEVEL && dialVal < DAY_LEVEL) { // morning
    return map(dialVal, DAWN_LEVEL, DAY_LEVEL, 50, 100);
  } else if (dialVal >= DAY_LEVEL && dialVal < DUSK_LEVEL) { // afternoon
    return map(dialVal, DAY_LEVEL, DUSK_LEVEL, 100, 50);
  } else if (dialVal >= DUSK_LEVEL && dialVal < NIGHT_LEVEL) { // evening
    return map(dialVal, DUSK_LEVEL, NIGHT_LEVEL, 50, 0);
  } else { // night
    return 0;
  }
}

// Returns true on victory and false on defeat
bool playRound(int difficulty) {
  startRound(difficulty);
  bool pattern[difficulty];
  for (int i = 0; i < difficulty; i++) {
    pattern[i] = rand() % 2;
    if (pattern[i]) {
      flashRight();
    } else {
      flashLeft();
    }
    delay(500);
  }
  int pos = 0;
  while (pos < difficulty) {
    updateButtons();
    if (_leftButton.isClicked() && !pattern[pos]) {
      flashLeft();
      pos++;
    } else if (_rightButton.isClicked() && pattern[pos]) {
      flashRight();
      pos++;
    } else if (_leftButton.isClicked() && pattern[pos] || _rightButton.isClicked() && !pattern[pos]) {
      for (int i = 0; i < 3; i++) {
        setRGB(255, 0, 0);
        delay(200);
        setRGB(0, 0, 0);
        delay(200);
      }
      delay(800);
      return false;
    }
  }
  for (int i = 0; i < 3; i++) {
    setRGB(0, 255, 0);
    delay(200);
    setRGB(0, 0, 0);
    delay(200);
  }
  delay(800);
  return true;
}

void startRound(int difficulty) {
  setRGB(0, 0, 0);
  delay(1000);
  for (int i = 0; i < difficulty; i++) {
    setRGB(255, 255, 255);
    delay(200);
    setRGB(0, 0, 0);
    delay(200);
  }
  delay(800);
}

void flashLeft() {
  digitalWrite(LEFT_STAR_PIN, HIGH);
  delay(500);
  digitalWrite(LEFT_STAR_PIN, LOW);
}

void flashRight() {
  digitalWrite(RIGHT_STAR_PIN, HIGH);
  delay(500);
  digitalWrite(RIGHT_STAR_PIN, LOW);
}

void updateButtons() {
  _modeButton.update();
  _rightButton.update();
  _leftButton.update();
}
