#include <SPI.h>
#include <TinyPICO.h>
#include <Adafruit_NeoPixel.h>
#include "VoiceRecognitionV3.h"
#include "DFRobot_SpeechSynthesis.h"

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define LED_COUNT 90

#define BOSS_TALK_SDA 5
#define BOSS_TALK_SCL 22
#define BOSS_LISTEN 15 // indicator light that our boss is listening
#define BOSS_SERIAL_RX 21  // receive commands from the boss
#define BOSS_SERIAL_TX 32  // send commands to the boss

#define BOSS_BUTTON 4 // since speech rec kinda sucks this is a button
#define BOSS_STATUS 25 // so we can get visual feedback as boss understands are instructions

#define RESET_BUTTON 27
#define LIGHT_POTENT 4
#define STATUS_LIGHT_OUT 33
#define DO_EXPAND(VAL)  VAL ## 1
#define EXPAND(VAL)     DO_EXPAND(VAL)

TinyPICO tp = TinyPICO();
DFRobot_SpeechSynthesis_I2C ss;
Adafruit_NeoPixel *pixels;
Adafruit_NeoPixel *pixelStatus;

void setWhite(int index, short mag=255)   { pixels->setPixelColor(index, pixels->Color(mag, mag, mag)); }
void setGreen(int index)   { pixels->setPixelColor(index, pixels->Color(0, 250, 0)); }
void setRed(int index) { pixels->setPixelColor(index, pixels->Color(250, 0, 0)); }
void setBlue(int index)  { pixels->setPixelColor(index, pixels->Color(0, 0, 250)); }
void setPurple(int index)  { pixels->setPixelColor(index, pixels->Color(0, 250, 250)); }
void setOrange(int index)  { pixels->setPixelColor(index, pixels->Color(250, 250, 0)); }
void setError(int index)  { pixels->setPixelColor(index, pixels->Color(55,200,90)); }
void setFromPot();

void blinkGreen();
void blinkOrange();
void blinkBlue();

void lightTestCycle();

void setOff(int index)  { pixels->setPixelColor(index, pixels->Color(0, 0, 0)); }
void setErrorAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setError(i);
  }
  pixels->show(); 
}
void setOffAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setOff(i);
  }
  pixels->show(); 
}
void setBlueAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setBlue(i);
  }
  pixels->show(); 
}
void setWhiteAll(short mag=255) {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    //if (i % 2 == 0) {
    setWhite(i, mag);
    //}
  }
  pixels->show(); 
}
void setRedAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setRed(i);
  }
  pixels->show(); 
}
void setGreenAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setGreen(i);
  }
  pixels->show(); 
}


VR myVR(BOSS_SERIAL_RX,BOSS_SERIAL_TX);    // 2:RX 3:TX, you can choose your favourite pins.

uint8_t bossListenStart = 0;
bool bossListening = false;
int lightMode = 0;
uint8_t buf[64];

bool isTouched = false;
bool didFive = false;
uint64_t touchStart = 0;

void setup() {
  Serial.begin(115200);
//  Serial2.begin(9600, SERIAL_8N1, BOSS_SERIAL_RX, BOSS_SERIAL_TX);

  delay(3000);
  pinMode(RESET_BUTTON, INPUT_PULLDOWN);
  pinMode(LIGHT_POTENT, INPUT);
  pinMode(BOSS_LISTEN, OUTPUT);
  pinMode(BOSS_BUTTON, INPUT_PULLDOWN);

  pixels = new Adafruit_NeoPixel(LED_COUNT, STATUS_LIGHT_OUT, NEO_GRB + NEO_KHZ800);
  pixels->begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  pixelStatus = new Adafruit_NeoPixel(1, BOSS_STATUS, NEO_GRB + NEO_KHZ800);
  pixelStatus->begin();
  pixelStatus->setPixelColor(0, pixels->Color(0, 100, 0));
  pixelStatus->show();   // Send the updated pixel colors to the hardware.

  Serial.println("start serial2\n");
  myVR.begin(9600, BOSS_SERIAL_TX, BOSS_SERIAL_RX);

  for (int i = 0; i < 6; ++i) {
    if (myVR.load((uint8_t)i) < 0) {
      Serial.println("Error loading command!");
    }
  }
  Serial.println("All commands loaded and we're listening");

  ss.begin(true, BOSS_TALK_SDA, BOSS_TALK_SCL);
  delay(2000);
  Serial.println("Let everyone know we're ready");
  ss.speak(F("Hello, i'm here to help.  You can wake me up by saying 'hello'."));
  Serial.println("Did you hear that?");
  delay(1000);
  lightMode  = -1;

  pixelStatus->clear();
  pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
  pixelStatus->show();   // Send the updated pixel colors to the hardware.
}

void loop() {
  if (digitalRead(BOSS_BUTTON) == HIGH) {
    // button is pressed
    if (touchStart) {
      uint64_t now = millis();
      int elasped = (now - touchStart) / 1000;
      if (elasped > 5 && elasped < 10) {
        if (didFive) {
        } else {
          Serial.println("touched for 5 seconds");
          didFive = true;
          pixelStatus->clear();
          pixelStatus->setPixelColor(0, pixels->Color(0, 200, 100));
          pixelStatus->show();   // Send the updated pixel colors to the hardware.
          lightMode  = 2; // hold press turn lights off
        }
      }
    } else {
      touchStart = millis();
      isTouched = true;
      Serial.println("button is pressed");
      pixelStatus->clear();
      pixelStatus->setPixelColor(0, pixels->Color(0, 100, 100));
      pixelStatus->show();   // Send the updated pixel colors to the hardware.
      lightMode  = 1; // single press enable lights
    }
  } else if (myVR.recognize(buf, 50) > 0) {
    uint64_t now = millis();
    pixelStatus->clear();
    pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
    pixelStatus->show();   // Send the updated pixel colors to the hardware.
    isTouched = false;
    didFive = false;
    touchStart = 0;

    if (bossListening) {
      switch (buf[1]) {
      case 1: // lights on
        lightMode  = 1;
        bossListenStart = 0;
        bossListening = false;
        break;
      case 2: // lights off
        lightMode  = 2;
        bossListenStart = 0;
        bossListening = false;
        break;
      case 3: // rainbows
        lightMode  = 3;
        bossListenStart = 0;
        bossListening = false;
        break;
      case 4: // dim 
        lightMode  = 4;
        bossListenStart = 0;
        bossListening = false;
        break;
      case 5: // bright 
        lightMode  = 4;
        bossListenStart = 0;
        bossListening = false;
        break;
      default:
        bossListenStart = 0;
        bossListening = false;
        lightMode = 10; // don't understand
      }
      digitalWrite(BOSS_LISTEN, LOW);
      pixelStatus->clear();
      pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
      pixelStatus->show();   // Send the updated pixel colors to the hardware.
 
    } else {
      switch (buf[1]) {
      case 0: // wake word boss on
        lightMode       = 0;
        bossListenStart = now;
        bossListening   = true;
        digitalWrite(BOSS_LISTEN, HIGH);
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(0, 200, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
        break;
      default:
        bossListenStart = 0;
        bossListening = false;
        lightMode = 11; // don't understand
        digitalWrite(BOSS_LISTEN, LOW);
        Serial.printf("not in boss mode you said: %d\n", buf[1]);
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(200, 0, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
      }
    }
    memset(buf, 0, sizeof(buf));
  } else {
    if (bossListening) {
      uint64_t now = millis();
      int delta = (now - (now - bossListenStart));
      if (delta > 5000) {
        Serial.println("boss timeout");
        // over 10 seconds no input reset stop boss mode
        bossListenStart = 0;
        bossListening = false;
        digitalWrite(BOSS_LISTEN, LOW);
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(200, 0, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
      }
    }
    if (lightMode > 5 || isTouched) { // unknown mode clear it out
      lightMode = -1;
      pixelStatus->clear();
      pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
      pixelStatus->show();   // Send the updated pixel colors to the hardware.
      isTouched = false;
      didFive = false;
      touchStart = 0;
    } else {
        // keep the mode 
    }

  }

  if (lightMode > -1) {
    switch (lightMode) {
    case 0: // wake mode
      digitalWrite(BOSS_LISTEN, HIGH);
      if (!isTouched) {
        ss.speak(F("How can I help?"));
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(0, 200, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
      }
      break;
    case 5: // bright - but not sure yet
    case 1: // turn the lights on
      digitalWrite(BOSS_LISTEN, LOW);
      setWhiteAll();
      if (!isTouched) {
        ss.speak(F("Ok Lights On"));
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
      }
      break;
    case 4: // dim - which i was gonna just use as off
    case 2: // turn the lights off
      digitalWrite(BOSS_LISTEN, LOW);
      setOffAll();
      if (!isTouched) {
        ss.speak(F("Ok Lights Off"));
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
      }
      break;
    case 3: // rainboes
      digitalWrite(BOSS_LISTEN, LOW);
      lightTestCycle();
      if (!isTouched) {
        ss.speak(F("Ooh Rainbows so Fun!"));
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
      }
      break;
    default:
      digitalWrite(BOSS_LISTEN, LOW);
      if (bossListening) {
        ss.speak(F("I didn't understand you!"));
      } else {
        Serial.printf("I'm not in boss mode -- %d\n", lightMode);
      }
      for (int i = 0; i < 3; ++i) {
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(0, 100, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
        delay(400);
        pixelStatus->clear();
        pixelStatus->setPixelColor(0, pixels->Color(0, 0, 0));
        pixelStatus->show();   // Send the updated pixel colors to the hardware.
        delay(400);
      }

      break;
    }
    lightMode = -1;
  }
}

void lightTestCycle() {
  pixels->clear();
  delay(5); // Pause before next pass through loop
	int counter = 0;
  for(int i=0; i< LED_COUNT; ++i) { // For each pixel...
    delay(5); // Pause before next pass through loop
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
    pixels->show();   // Send the updated pixel colors to the hardware.
    delay(5); // Pause before next pass through loop
		counter++;
		// rainbow -> red -> orange -> yellow -> green -> blue -> purple
		if (counter == 1) { // red
      pixels->setPixelColor(i, pixels->Color(150, 0, 0));
		} else if (counter == 2) { // orange
      pixels->setPixelColor(i, pixels->Color(250, 140, 0));
		} else if (counter == 3) { // yellow
      pixels->setPixelColor(i, pixels->Color(254, 215, 0));
		} else if (counter == 4) { // green
      pixels->setPixelColor(i, pixels->Color(0, 150, 0));
		} else if (counter == 5) { // blue
      pixels->setPixelColor(i, pixels->Color(0, 0, 150));
		} else if (counter == 6) { // purple
      pixels->setPixelColor(i, pixels->Color(0, 250, 100));
		} else {
      pixels->setPixelColor(i, pixels->Color(250, 150, 150));
			counter = 0;
		}
		if (counter > 0) {
			delay(5);
      pixels->show();   // Send the updated pixel colors to the hardware.
		}
		continue;
  }
}
void blinkGreen() {
  pixels->clear();
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
  for(int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(150, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
}
void blinkOrange() {
  pixels->clear();
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
  for(int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(150, 150, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
}
void blinkBlue() {
  pixels->clear();
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 150));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
}

float lastVolts = 0;

void setFromPot() {
  const short cycles = 50;
  float volts = analogRead(LIGHT_POTENT);
  // greater then or less than by 100
  if ( (volts > lastVolts+100) || (volts < lastVolts-100) || (volts < 10) ) {
    float voltAverage = 0;
    for (int i = 0; i < cycles; ++i) {
      voltAverage += analogRead(LIGHT_POTENT);
    }
    voltAverage /= cycles;
    if ( ((voltAverage > lastVolts+80) || (voltAverage < lastVolts-80)) ) {
      printf("volts: %f and average: %f\n", volts, voltAverage);
      lastVolts = volts;
      int mag = (int)(voltAverage / 10); // with resistor to ensure max voltage of 2.5
      if (mag > 255) { mag = 255; }
      else if (mag > 240) { mag = 255; }
      else if (mag < 14) { mag = 0; }
      printf("change brightness: %d\n", mag);
      if (mag == 0) {
        setOffAll();
      } else {
        setWhiteAll(mag);
      }
    } else if (voltAverage < 10) {
      setOffAll();
    }
  }
}

