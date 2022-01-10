#include <TinyPICO.h>
#include <Adafruit_NeoPixel.h>
#include "VoiceRecognitionV3.h"

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define LED_COUNT 90

#define BOSS_LISTEN 15 // indicator light that our boss is listening
#define BOSS_SERIAL_RX 32  // receive commands from the boss
#define BOSS_SERIAL_TX 21  // send commands to the boss
#define BOSS_AWAKE 33 // our boss is awake and we should be reading from the serial connection

#define RESET_BUTTON 27
#define LIGHT_POTENT 4
#define STATUS_LIGHT_OUT 25
#define DO_EXPAND(VAL)  VAL ## 1
#define EXPAND(VAL)     DO_EXPAND(VAL)

TinyPICO tp = TinyPICO();
Adafruit_NeoPixel *pixels;

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
    if (i % 2 == 0) {
      setWhite(i, mag);
    }
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

void setup() {
  Serial.begin(115200);
//  Serial2.begin(9600, SERIAL_8N1, BOSS_SERIAL_RX, BOSS_SERIAL_TX);

  delay(3000);
  pinMode(RESET_BUTTON, INPUT_PULLDOWN);
  pinMode(LIGHT_POTENT, INPUT);
  pinMode(BOSS_LISTEN, OUTPUT);

  pixels = new Adafruit_NeoPixel(LED_COUNT, STATUS_LIGHT_OUT, NEO_GRB + NEO_KHZ800);
  pixels->begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  //setFromPot();
//  lightTestCycle();
 // delay(3000);
  //myVR.begin(9600);
  Serial.println("start serial2\n");
  myVR.begin(9600, 21, 32);

  for (int i = 0; i < 6; ++i) {
    if (myVR.load((uint8_t)i) < 0) {
      Serial.println("Error loading command!");
    }
  }
  Serial.println("All commands loaded and we're listening");
}

uint8_t bossListenStart = 0;
bool bossListening = false;
int lightMode = 0;
uint8_t buf[64];

void loop() {
  if (myVR.recognize(buf, 50) > 0) {
    uint64_t now = millis();

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
      
    } else {
      switch (buf[1]) {
      case 0: // wake word boss on
        lightMode       = 0;
        bossListenStart = now;
        bossListening   = true;
        digitalWrite(BOSS_LISTEN, HIGH);
        break;
      default:
        bossListenStart = 0;
        bossListening = false;
        lightMode = 11; // don't understand
        digitalWrite(BOSS_LISTEN, LOW);
        Serial.printf("not in boss mode you said: %d\n", buf[1]);
      }
    }
    int delta = (now - (now - bossListenStart));
    Serial.println(delta);
    if ( delta > 5000) {
      Serial.println("boss timeout");
      // over 10 seconds no input reset stop boss mode
      bossListenStart = 0;
      bossListening = false;
      digitalWrite(BOSS_LISTEN, LOW);
    }
  } else {
    lightMode = -1;
  }

  if (lightMode > -1) {
    switch (lightMode) {
    case 0: // wake mode
      digitalWrite(BOSS_LISTEN, HIGH);
      break;
    case 1: // turn the lights on
      digitalWrite(BOSS_LISTEN, LOW);
      setWhiteAll();
      break;
    case 2: // turn the lights off
      digitalWrite(BOSS_LISTEN, LOW);
      setOffAll();
      break;
    case 3: // rainboes
      digitalWrite(BOSS_LISTEN, LOW);
      lightTestCycle();
      break;
    case 4: // dim - which i was gonna just use as off
      digitalWrite(BOSS_LISTEN, LOW);
      setOffAll();
      break;
    case 5: // bright - but not sure yet
      digitalWrite(BOSS_LISTEN, LOW);
      setWhiteAll();
      break;
    default:
      digitalWrite(BOSS_LISTEN, LOW);
      Serial.printf("I didnt' understand you! %d\n", lightMode);
      break;
    }
    // send mode data and clear
    /*Serial.println("");
    if (lightMode == 0) { // send moss mode
      Serial.print(255);
    } else {
      Serial.print(lightMode);
    }
    Serial.println("");
    */
  }

/*  String data = Serial2.readString();
  if (data.length()) {
    int cmd = data.toInt();
    if (cmd > 0) {
      Serial.printf("command: %d\n", cmd);

      switch (cmd) {
      case 255:
        digitalWrite(BOSS_LISTEN, HIGH);
        break;
      case 1:
        setWhiteAll();
        digitalWrite(BOSS_LISTEN, LOW);
        break;
      case 4: // dim
      case 2: // off
        setOffAll();
        digitalWrite(BOSS_LISTEN, LOW);
        break;
      case 3:
        lightTestCycle();
        digitalWrite(BOSS_LISTEN, LOW);
        break;
      default:
        Serial.printf("unknown: %d\n", cmd);
        digitalWrite(BOSS_LISTEN, LOW);
        break;
      }


    } else {
      Serial.printf("unknown: %d: %s\n", cmd, data.c_str());
    }
  }
  */
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
      pixels->setPixelColor(i, pixels->Color(100, 150, 100));
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

