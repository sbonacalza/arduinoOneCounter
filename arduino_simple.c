// Arduino Spede
// a Reaction Time Tester
//
// Copyright (c) 2013 Petri Häkkinen
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <EEPROM.h>

// Arduino pins connected to latch, clock and data pins of the 74HC595 chip
const int latchPin = 7;
const int clockPin = 8;
const int dataPin = 9;

// Arduino pin connected to the speaker
//const int tonePin = 2;

// Arduino pins connected to transistors controlling the digits
const int enableDigits[] = { 13,10,11,12 };

// Arduino pins connected to leds
const int leds[] = { 3,4,5,6 };

// Arduino pins connected to buttons
const int buttons[] = { 14,15,16,17 };

// Frequencies of tones played when buttons are pressed
const int toneFreq[] = { 277, 311, 370, 415 };  // CS4, DS4, FS4, GS4

// Segment bits for numbers 0-9
int digits[10] = {
  B1111101, // 0  ABCDEF-
  B1000001, // 1  -BC----
  B1011110, // 2  AB-DE-G
  B1010111, // 3  ABCD--G  
  B1100011, // 4  -BC--FG
  B0110111, // 5  A-CD-EF
  B0111111, // 6  A-BCDEF
  B1010001, // 7  ABC----
  B1111111, // 8  ABCDEFG
  B1110111, // 9  ABCD-FG
};

enum {
  STATE_START_MENU,
  STATE_GAME,
  STATE_GAME_OVER
};

int score = 0;
int score2 = 0;
int led = 0;
int prevLed = 0;
int nextTimer = 0;
int level = 0;
int hiscore = 0;
int startMenuTimer = 0;
int prevButtonState[] = { HIGH, HIGH, HIGH, HIGH };
int state = STATE_START_MENU;
int count = 0;
boolean pasoInter = false;

// Read hiscore value from EEPROM
void readHiscore() {
  hiscore = (EEPROM.read(0) << 8) + EEPROM.read(1);
  
  // EEPROM initial value is FFFF
  if(hiscore == 0xffff)
    hiscore = 0;
}

// Write hiscore value to EEPROM
void writeHiscore() {
  EEPROM.write(0, hiscore >> 8);
  EEPROM.write(1, hiscore & 0xff);
}

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  pinMode(2, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  digitalWrite(1, HIGH);
  
  
  for(int i = 0; i < 4; i++)
    pinMode(enableDigits[i], OUTPUT);
  
  for(int i = 0; i < 4; i++)
    pinMode(leds[i], OUTPUT);

  // enable pull up resistors for button pins
  for(int i = 0; i < 4; i++)
    pinMode(buttons[i], INPUT_PULLUP);
    
  readHiscore();
}

// Updates display with current score.
// Flashes 4 digits quickly on the display.
// Display is turned off if enable is false.
void updateDisplay(int score, boolean enable) {
  int s = score;
  
  for(int pos = 0; pos < 4; pos++) {
    int digit = s % 10;
    s /= 10;
      
    // turn off all digits
    
    delayMicroseconds(500);
    
    // set latch low so digits won't change while sending bits 
    

    // shift out the bits
      
  
    // set latch high so the LEDs will light up
    
  
    delayMicroseconds(500);

    delayMicroseconds(2000);
  }
}

// Updates the start menu. Switch between previous score and hiscore on the display.
// Start a new game when a button is pressed. Clear the hiscore is all buttons are held down for 2 seconds.
void startMenu() {
  int s = score;

  // flick between previous score and hiscore display
  startMenuTimer = (startMenuTimer + 1) % 2000;
  if(startMenuTimer >= 1000)
    s = hiscore;
    
  updateDisplay(s, startMenuTimer < 975 || (startMenuTimer > 1000 && startMenuTimer < 1975));
  
  // read button state
  int buttonState = 0;
  for(int i = 0; i < 4; i++)
    if(digitalRead(buttons[i]) == LOW)
      buttonState |= 1<<i;
  
  // reset hiscore if all buttons are held down for 2 seconds
  static long resetHiscoreTimer = 0;
  if(buttonState == 15) {
    if(resetHiscoreTimer == 0)
      resetHiscoreTimer = millis();
    if(millis() - resetHiscoreTimer > 2000) {
      updateDisplay(0, false);
      //tone(tonePin, 500, 500);
      hiscore = 0;
      writeHiscore();
      delay(700);
      resetHiscoreTimer = 0;
    }
  } else {
    resetHiscoreTimer = 0;
  }
  
  // start new game if a single button is pressed for 100ms
  static int startNewGameTimer = 0;
  if(buttonState == 1 || buttonState == 2 || buttonState == 4 || buttonState == 8) {
    if(startNewGameTimer == 0)
      startNewGameTimer = millis();
    if(millis() - startNewGameTimer > 50) {  
      // start new game
      updateDisplay(score, false);
      delay(2000);
      startNewGame();
      startNewGameTimer = 0;
    }
  } else {
    startNewGameTimer = 0;
  }
}

// Prepares game state for a new game.
void startNewGame() {
  state = STATE_GAME;
  level = -1;
  led = -1;
  prevLed = -1;
  nextTimer = 5;
 
  for(int i = 0; i < 4; i++) 
    prevButtonState[i] = HIGH;

  // set random seed, so that every game has a different sequence
  randomSeed(millis());
}

void playGame() {
  // update time
  nextTimer--;
 
  if(nextTimer < 0) {
    // game over if player is too slow
    if(led >= 0) {
      gameOver(score2);
      return;
    }

    led = random(4);
    
    // make consequent same leds less probable
    if(led == prevLed && random(10) < 6) {
      led = random(4);
    }
      
    prevLed = led;
    
    nextTimer = max(150 * pow(1.6, -level*0.05), 10);
    
    //tone(tonePin, toneFreq[led], nextTimer * 8);
  }
  
  // update leds
  for(int i = 0; i < 4; i++)
    digitalWrite(leds[i], led == i || (digitalRead(buttons[i]) == LOW && nextTimer > 5) ? HIGH : LOW);
  
  updateDisplay(score, true);
  // read input   
  for(int i = 0; i < 4; i++) {
    int but = digitalRead(buttons[i]);
    if(but == LOW && prevButtonState[i] == HIGH) {
      // ignore button press if time since last press is too short
      if( led >= 0 ) { //&& millis() - lastButtonPress > 50 ) { 
        // correct button pressed?
        if( i == led ) {
          score2++;
          keepScore(score2);          
          led = -1;  // turn off led
        } else {
          if (score2 > 0)
          score2--;          
          gameOver(score2);
        }
        //lastButtonPress = millis();
        //noTone(tonePin);
      }
    }
    prevButtonState[i] = but;
  }
}

// Game over. Play a game over sound and blink score.
void gameOver(int score2) {
  //tone(tonePin, 250, 2500);

  // new hiscore?
  if(score > hiscore) {
    hiscore = score;
    writeHiscore();
  }
  
  // turn on leds
  for(int i = 0; i < 4; i++)
    digitalWrite(leds[i], HIGH);
    
  // flash display
  for(int i = 0; i < 70*5; i++) {
    //if(i == 70*2)
      //tone(tonePin, 200, 2000);    
    boolean enable = 1 - (i/60) & 1;
    updateDisplay(score, enable);
  }
  
  // turn off leds
  for(int i = 0; i < 4; i++)
    digitalWrite(leds[i], LOW);
   updateDisplay(score, false);
  
  // enter menu
  //delay(1000);
   if (score2 >= 0)
   keepScore(score2);
  state = STATE_START_MENU;
  startMenuTimer = 0;
}

// Main loop. Update menu, game or game over depending on current state.
void loop() {
  if(state == STATE_START_MENU)
    startMenu();
  else if(state == STATE_GAME)
    playGame();
  else
    gameOver(score2);
}

void keepScore(int score2) {
  if (score2 == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score2 == 1) {
    digitalWrite(10, HIGH);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score2 == 2) {
    digitalWrite(10, LOW);
    digitalWrite(9, HIGH);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score2 == 3) {
    if (pasoInter == true) {
      digitalWrite(1, HIGH);
      digitalWrite(0, LOW);
    }
    digitalWrite(10, HIGH);
    digitalWrite(9, HIGH);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score2 == 4) {
    pasoInter = true;
    digitalWrite(0, HIGH);
    digitalWrite(1, LOW);
    digitalWrite(2, LOW);
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
    digitalWrite(13, LOW);
  }
  if (score2 == 5) {
    digitalWrite(2, HIGH);
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
    digitalWrite(13, LOW);
  }
  if (score2 == 6) {
    digitalWrite(2, HIGH);
    digitalWrite(11, HIGH);
    digitalWrite(12, LOW);
    digitalWrite(13, LOW);
  }
  /*if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }
  if (score == 0) {
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
    digitalWrite(7, LOW);
  }*/
}