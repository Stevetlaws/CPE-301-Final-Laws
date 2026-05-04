#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>

// --- Pin Definitions ---
const int tempPin = A0;
const int waterSensorPin = 24;

// LEDs
const int ledRed = 47;
const int ledYellow = 49;
const int ledGreen = 51;
const int ledBlue = 53;
const int ledWhite = 6;

// Buttons
const int button1 = 18;
const int button2 = 19;

// --- Library Objects ---
// RS=7, E=8, D4=9, D5=10, D6=11, D7=12
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
RTC_DS1307 rtc; // Change to RTC_DS3231 if that is the module you have

void setup() {
  Serial.begin(9600);
  Serial.println("--- CPE 301 Hardware Diagnostics ---");

  // Set LED pins as outputs
  pinMode(ledRed, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);
  pinMode(ledWhite, OUTPUT);

  // Set Buttons as inputs with internal pull-up resistors
  // (Prevents floating pins; buttons should wire to GND when pressed)
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);

  // Set Water Sensor as input
  pinMode(waterSensorPin, INPUT);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("System Boot...");
  delay(1000);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    lcd.setCursor(0, 1);
    lcd.print("RTC ERROR");
  } else {
    Serial.println("RTC Initialized.");
    if (!rtc.isrunning()) {
      Serial.println("RTC is NOT running, setting time...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Sets RTC to compile time
    }
  }

  // Quick LED Test (Flashes all on, then off)
  digitalWrite(ledRed, HIGH); digitalWrite(ledYellow, HIGH);
  digitalWrite(ledGreen, HIGH); digitalWrite(ledBlue, HIGH);
  digitalWrite(ledWhite, HIGH);
  delay(500);
  digitalWrite(ledRed, LOW); digitalWrite(ledYellow, LOW);
  digitalWrite(ledGreen, LOW); digitalWrite(ledBlue, LOW);
  digitalWrite(ledWhite, LOW);
}

void loop() {
  // 1. Read Sensors
  int rawTemp = analogRead(tempPin);
  int waterState = digitalRead(waterSensorPin);

  // 2. Read Buttons (LOW means pressed if using INPUT_PULLUP)
  bool b1Pressed = (digitalRead(button1) == LOW);
  bool b2Pressed = (digitalRead(button2) == LOW);

  // 3. Test LED Logic via Buttons
  // Button 1 turns on Red/Yellow, Button 2 turns on Green/Blue/White
  if (b1Pressed) {
    digitalWrite(ledRed, HIGH);
    digitalWrite(ledYellow, HIGH);
  } else {
    digitalWrite(ledRed, LOW);
    digitalWrite(ledYellow, LOW);
  }

  if (b2Pressed) {
    digitalWrite(ledGreen, HIGH);
    digitalWrite(ledBlue, HIGH);
    digitalWrite(ledWhite, HIGH);
  } else {
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledBlue, LOW);
    digitalWrite(ledWhite, LOW);
  }

  // 4. Print RTC and Temp Data to LCD
  DateTime now = rtc.now();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(now.hour(), DEC); lcd.print(':');
  if (now.minute() < 10) lcd.print('0'); // Leading zero
  lcd.print(now.minute(), DEC);

  lcd.setCursor(0, 1);
  lcd.print("T-Raw:");
  lcd.print(rawTemp);
  lcd.print(" W:");
  lcd.print(waterState == HIGH ? "WET" : "DRY");

  // 5. Print to Serial Monitor
  Serial.print("Temp Raw: "); Serial.print(rawTemp);
  Serial.print(" | Water: "); Serial.print(waterState);
  Serial.print(" | B1: "); Serial.print(b1Pressed ? "ON" : "OFF");
  Serial.print(" | B2: "); Serial.println(b2Pressed ? "ON" : "OFF");

  delay(250); // Small delay to prevent LCD flickering
}