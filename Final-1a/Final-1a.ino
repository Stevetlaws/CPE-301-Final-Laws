// #include <LiquidCrystal.h>
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

// ==========================================
// LCD 1602 CONTROL (4-Bit Mode) - Port H
// ==========================================

volatile unsigned char *portDDRH = (unsigned char *) 0x101; 
volatile unsigned char *portH    = (unsigned char *) 0x102; 

void lcd_delay() {
  for(volatile int i = 0; i < 1000; i++);
}

// The "Handshake": EN pin must go High then Low to lock in data
void pulse_enable() {
  *portH |= 0x01;  // EN High
  lcd_delay();
  *portH &= 0xFE;  // EN Low
  lcd_delay();
}

// Sends 4 bits to the screen
void lcd_write_4bit(unsigned char val) {
  // Clear bits 3,4,5,6 (the data pins)
  *portH &= 0x87; 
  // Shift the value to match pins 6-9 and shove it in
  *portH |= (val << 3); 
  pulse_enable();
}

// Sends a full byte (Command or Character)
void lcd_send(unsigned char data, int is_data) {
  if(is_data) *portH |= 0x02;  // RS High for text
  else        *portH &= 0xFD;  // RS Low for command
  
  lcd_write_4bit(data >> 4);   // Send top half
  lcd_write_4bit(data & 0x0F); // Send bottom half
}

void init_LCD() {
  *portDDRH |= 0x7B; // Set RS, EN, and D4-D7 as outputs
  
  // Hardcoded initialization sequence for 4-bit mode
  lcd_delay();
  lcd_write_4bit(0x03);
  lcd_delay();
  lcd_write_4bit(0x03);
  lcd_delay();
  lcd_write_4bit(0x03);
  lcd_write_4bit(0x02); // Set to 4-bit mode
  
  lcd_send(0x28, 0); // 2 lines, 5x8 font
  lcd_send(0x0C, 0); // Display ON, Cursor OFF
  lcd_send(0x01, 0); // Clear screen
  lcd_send(0x06, 0); // Entry mode
}

void lcd_print(char* str) {
  while(*str) {
    lcd_send(*str++, 1);
  }
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
  unsigned char addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
  lcd_send(addr, 0);
}