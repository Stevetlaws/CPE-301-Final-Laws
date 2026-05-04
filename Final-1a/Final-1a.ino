#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>

// --- Pin Definitions ---
const int tempPin = 2;
#define DHTTYPE DHT11
DHT dht(tempPin, DHTTYPE); // Initialize DHT sensor

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
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
RTC_DS1307 rtc;

void setup() {
  Serial.begin(9600);
  Serial.println("--- CPE 301 Unified Diagnostics ---");

  // Setup LEDs
  pinMode(ledRed, OUTPUT); pinMode(ledYellow, OUTPUT);
  pinMode(ledGreen, OUTPUT); pinMode(ledBlue, OUTPUT);
  pinMode(ledWhite, OUTPUT);

  // Setup Inputs
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(waterSensorPin, INPUT);

  // Init LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("System Boot...");

  // Init RTC
  if (!rtc.begin()) {
    Serial.println("RTC ERROR");
  } else if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Init Temp Sensor
  dht.begin();
}

void loop() {
  // 1. Read Sensors
  float tempC = dht.readTemperature();
  int waterState = digitalRead(waterSensorPin);

  // 2. Read Buttons
  bool b1Pressed = (digitalRead(button1) == LOW);
  bool b2Pressed = (digitalRead(button2) == LOW);

  // 3. LED Logic
  if (b1Pressed) {
    digitalWrite(ledRed, HIGH); digitalWrite(ledYellow, HIGH);
  } else {
    digitalWrite(ledRed, LOW); digitalWrite(ledYellow, LOW);
  }

  if (b2Pressed) {
    digitalWrite(ledGreen, HIGH); digitalWrite(ledBlue, HIGH); digitalWrite(ledWhite, HIGH);
  } else {
    digitalWrite(ledGreen, LOW); digitalWrite(ledBlue, LOW); digitalWrite(ledWhite, LOW);
  }

  // 4. LCD Print
  DateTime now = rtc.now();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(now.hour(), DEC); lcd.print(':');
  if (now.minute() < 10) lcd.print('0');
  lcd.print(now.minute(), DEC);

  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  if (isnan(tempC)) {
    lcd.print("ERR"); // Prints ERR if sensor wiring is loose
  } else {
    lcd.print(tempC, 1); // Prints with 1 decimal place
    lcd.print("C");
  }
  lcd.print(" W:");
  lcd.print(waterState == HIGH ? "WET" : "DRY");

  // 5. Serial Print
  Serial.print("Temp (C): "); Serial.print(tempC);
  Serial.print(" | Water: "); Serial.print(waterState);
  Serial.print(" | B1: "); Serial.print(b1Pressed ? "ON" : "OFF");
  Serial.print(" | B2: "); Serial.println(b2Pressed ? "ON" : "OFF");

  // CRITICAL NOTE: The DHT11 is slow. This 2-second delay is required.
  // Because of this delay, you will have to hold the buttons down for a 
  // full 2 seconds before the LEDs register the change!
  delay(2000); 
}