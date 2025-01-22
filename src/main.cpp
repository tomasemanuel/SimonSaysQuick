#include <Arduino.h>

/* Constants - define pin numbers for LEDs and buttons: */
const uint8_t ledPins[] = {9, 10, 11, 12};
const uint8_t buttonPins[] = {2, 3, 4, 5};

// These are connected to 74HC595 shift register (used to show game score):
const int LATCH_PIN = A1;  // 74HC595 pin 12
const int DATA_PIN = A0;   // 74HC595 pin 14
const int CLOCK_PIN = A2;  // 74HC595 pin 11

#define MAX_GAME_LENGTH 100

/* Global variables - store the game state */
uint8_t gameSequence[MAX_GAME_LENGTH] = {0};
uint8_t gameIndex = 0;


#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4); 


/**
   Set up the Arduino board and initialize Serial communication
*/
void setup() {
  Serial.begin(9600);
  lcd.init();                 
  lcd.backlight();\
  for (byte i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  randomSeed(analogRead(A3));

  Serial.println("Simon Says Game Initialized");
}

/* Digit table for the 7-segment display */
const uint8_t digitTable[] = {
  0b11000000,  // 0
  0b11111001,  // 1
  0b10100100,  // 2
  0b10110000,  // 3
  0b10011001,  // 4
  0b10010010,  // 5
  0b10000010,  // 6
  0b11111000,  // 7
  0b10000000,  // 8
  0b10010000,  // 9
};
const uint8_t DASH = 0b10111111;

void sendScore(uint8_t high, uint8_t low) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, low);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, high);
  digitalWrite(LATCH_PIN, HIGH);
}

void displayScore() {
  int high = gameIndex % 100 / 10;
  int low = gameIndex % 10;
  sendScore(high ? digitTable[high] : 0xff, digitTable[low]);
}

/**
   Lights the given LED
*/
void lightLed(byte ledIndex) {
  digitalWrite(ledPins[ledIndex], HIGH);
  delay(300);  // LED stays on for 300ms
  digitalWrite(ledPins[ledIndex], LOW);
}

/**
   Plays the current sequence of LEDs that the user has to repeat
*/
void playSequence() {
  for (int i = 0; i < gameIndex; i++) {
    byte currentLed = gameSequence[i];
    lightLed(currentLed);
    delay(200);  // Short delay between LEDs
  }
}

/**
    Waits until the user presses one of the buttons,
    and returns the index of that button
*/
byte readButtons() {
  while (true) {
    for (byte i = 0; i < 4; i++) {
      byte buttonPin = buttonPins[i];
      if (digitalRead(buttonPin) == LOW) {
        // Wait until the button is released
        while (digitalRead(buttonPin) == LOW) {
          delay(1);
        }
        return i;
      }
    }
    delay(1);
  }
}

/**
  Play the game over sequence, and report the game score
*/
void gameOver() {
  lcd.clear();
  Serial.print("Game over! Your score: ");
  lcd.setCursor(0, 0);  
  lcd.print("Game Over!");
  
  lcd.setCursor(0, 1);  
  uint8_t score = gameIndex - 1;
  lcd.print("Score: " + String(score));
  Serial.println(gameIndex - 1);
  gameIndex = 0;
  delay(8000);
  sendScore(DASH, DASH);
  delay(4000);
}

/**
   Get the user's input and compare it with the expected sequence.
*/
bool checkUserSequence() {
  for (int i = 0; i < gameIndex; i++) {
    byte expectedButton = gameSequence[i];
    byte actualButton = readButtons();
    lightLed(actualButton); 
    if (expectedButton != actualButton) {
      return false;
    }
  }

  return true;
}

/**
   The main game loop
*/
void loop() {
  displayScore();
  lcd.clear();
  lcd.setCursor(0, 0); 
  if (gameIndex == 0) {
    lcd.print("Simon Says Game");
  }
  else {
    lcd.print("Good Job!");
    lcd.setCursor(0, 1);
    lcd.print("Level: " + String(gameIndex));
  }
  gameSequence[gameIndex] = random(0, 4);
  gameIndex++;
  if (gameIndex >= MAX_GAME_LENGTH) {
    gameIndex = MAX_GAME_LENGTH - 1;
  }

  playSequence();
  if (!checkUserSequence()) {
    gameOver();
    return;
  }

  delay(500);  
}
