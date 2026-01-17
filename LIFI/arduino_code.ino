#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#define LDR_PIN 8
const int UNIT_DURATION = 100;  // 1 Morse unit in ms
unsigned long signalStart = 0;
unsigned long signalEnd = 0;
bool signalOn = false;
 String morseLetter = "";
String fullMessage = "";
 // Morse-to-English lookup
String morseChars[] = {
 ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", // A-I
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", // J-R
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..",	 // S-Z
  "-----", ".----", "..---", "...--", "....-", ".....",    	  // 0-5
  "-....", "--...", "---..", "----." 
};
char alphabet[] = {
  'A','B','C','D','E','F','G','H','I',
  'J','K','L','M','N','O','P','Q','R',
  'S','T','U','V','W','X','Y','Z',
  '0','1','2','3','4','5',
  '6','7','8','9'
};
 
void setup() {
  Serial.begin(9600);
  pinMode(LDR_PIN, INPUT_PULLUP);
  lcd.begin(16, 2);
 
  lcd.clear();
  lcd.print("LiFi Project");
  delay(1000);
 
  lcd.clear();
  lcd.print("Send any message");
  lcd.setCursor(0, 1);
  lcd.print("from LiFi App..");
  delay(1000);
 
  lcd.clear();
  lcd.print("Ready to decode");
  lcd.setCursor(0, 1);
  lcd.print("Morse input...");
}
 
void loop() {
  int ldrVal = digitalRead(LDR_PIN);
  unsigned long now = millis();
 
  if (ldrVal == LOW && !signalOn) {
    signalOn = true;
    signalStart = now;
     Serial.print("[");
     Serial.print(now);
     Serial.println(" ms] LIGHT ON");
        	
    unsigned long gap = now - signalEnd;
 
        	if (gap >= 5 * UNIT_DURATION) {
        	if (morseLetter.length() > 0) {
        decodeAndAppendLetter();
        	}
      fullMessage += ' ';
       Serial.println("Word gap detected: Adding space");
      printToLCD(fullMessage);
        	}
        	else if (gap >= 2.3 * UNIT_DURATION && gap <= 3.8 * UNIT_DURATION) {
        	if (morseLetter.length() > 0) {
        decodeAndAppendLetter();
        	}
        	}
        	// else intra-letter: no action
  }
 
  else if (ldrVal == HIGH && signalOn) {
    signalOn = false;
    signalEnd = now;
 
    unsigned long duration = signalEnd - signalStart;
 
        	if (duration >= 0.4 * UNIT_DURATION && duration <= 1.6 * UNIT_DURATION) {
      morseLetter += ".";
      Serial.println("Dot detected");
        	}
        	else if (duration >= 2.4 * UNIT_DURATION && duration <= 3.6 * UNIT_DURATION) {
      morseLetter += "-";
      Serial.println("Dash detected");
        	}
        	// else ignore noisy or undefined signals
  }
 
  if (!signalOn && morseLetter.length() > 0 && (now - signalEnd > 2000)) {
    decodeAndAppendLetter();
    printToLCD(fullMessage);
    fullMessage = "";
  }
}
void decodeAndAppendLetter() {
  for (int i = 0; i < 36; i++) {
        	if (morseLetter == morseChars[i]) {
      fullMessage += alphabet[i];
      break;
        	}
  }
  morseLetter = "";
}


void printToLCD(String msg) {
  lcd.clear();
  lcd.print(msg.substring(0, 16));
  lcd.setCursor(0, 1);
  if (msg.length() > 16) {
    lcd.print(msg.substring(16, 32));
  }
}
