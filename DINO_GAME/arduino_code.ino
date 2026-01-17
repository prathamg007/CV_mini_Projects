#include <LiquidCrystal.h>

// LCD Pins: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

byte birdChar[8] = {
  B00100,
  B01110,
  B11111,
  B10101,
  B11111,
  B01110,
  B00100,
  B00000
};

const int screenWidth = 16;
char topRow[screenWidth];
char bottomRow[screenWidth];
int birdY = 1;
int frameCount = 0;
bool gameOver = false;
bool birdUp = false;
int score = 0;

// Pipe generation system
int gapCountdown = 0;
bool nextPipeTop = true;

// Utility
bool randomChance(int percent) {
  return random(0, 100) < percent;
}

void waitForStart() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting to");
  lcd.setCursor(0, 1);
  lcd.print("scream...");
  while (true) {
    if (Serial.available() > 0) {
      if (Serial.read() == 'J') break;
    }
  }
  lcd.clear();
}

void setup() {
  delay(100);
  lcd.begin(16, 2);
  lcd.createChar(0, birdChar);
  Serial.begin(115200);
  randomSeed(analogRead(A0));  // Initialize randomness
  waitForStart();

  for (int i = 0; i < screenWidth; i++) {
    topRow[i] = ' ';
    bottomRow[i] = ' ';
  }

  lcd.clear();
  lcd.setCursor(0, birdY);
  lcd.write(byte(0));
}

void loop() {
  if (gameOver) {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Game Over");
    lcd.setCursor(1, 1);
    lcd.print("Score: ");
    lcd.print(score);
    while (true);
  }

  // Handle input
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'J') birdUp = true;
    else if (c == 'D') birdUp = false;
  }

  // Bird movement
  birdY = birdUp ? 0 : 1;

  // Shift screen
  for (int i = 0; i < screenWidth - 1; i++) {
    topRow[i] = topRow[i + 1];
    bottomRow[i] = bottomRow[i + 1];
  }

  // Clear last column
  topRow[screenWidth - 1] = ' ';
  bottomRow[screenWidth - 1] = ' ';

  // Pipe generation
  if (gapCountdown == 0) {
    // Place a pipe on either row
    if (nextPipeTop) {
      topRow[screenWidth - 1] = '|';
    } else {
      bottomRow[screenWidth - 1] = '|';
    }

    // Randomize next pipe location
    nextPipeTop = randomChance(40); // 40% chance top, 60% bottom

    // Set a random gap duration between 2–6 frames
    gapCountdown = random(2, 7); // 2 to 6
  } else {
    gapCountdown--;
  }

  // Collision detection
  if ((birdY == 0 && topRow[0] == '|') || (birdY == 1 && bottomRow[0] == '|')) {
    gameOver = true;
  }

  // Draw everything
  for (int i = 0; i < screenWidth; i++) {
    lcd.setCursor(i, 0);
    lcd.write(topRow[i]);
    lcd.setCursor(i, 1);
    lcd.write(bottomRow[i]);
  }

  // Draw bird
  lcd.setCursor(0, birdY);
  lcd.write(byte(0));

  // Score update
  if (frameCount % 2 == 0) score++;

  delay(200);
  frameCount++;
}
