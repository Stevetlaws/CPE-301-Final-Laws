// #include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
// #include <DHT.h>

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
// LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// ==========================================
// BARE METAL LCD 1602 CONTROL (4-Bit Mode) 
// ==========================================
volatile unsigned char *portDDRH = (unsigned char *) 0x101; 
volatile unsigned char *portH    = (unsigned char *) 0x102; 

void lcd_delay() {
  for(volatile int i = 0; i < 1000; i++);
}

void pulse_enable() {
  *portH |= 0x01;  // EN High
  lcd_delay();
  *portH &= 0xFE;  // EN Low
  lcd_delay();
}

void lcd_write_4bit(unsigned char val) {
  *portH &= 0x87; 
  *portH |= (val << 3); 
  pulse_enable();
}

void lcd_send(unsigned char data, int is_data) {
  if(is_data) *portH |= 0x02;  
  else        *portH &= 0xFD;  
  
  lcd_write_4bit(data >> 4);   
  lcd_write_4bit(data & 0x0F); 
}

void init_LCD() {
  *portDDRH |= 0x7B; 
  lcd_delay();
  lcd_write_4bit(0x03);
  lcd_delay();
  lcd_write_4bit(0x03);
  lcd_delay();
  lcd_write_4bit(0x03);
  lcd_write_4bit(0x02); 
  
  lcd_send(0x28, 0); 
  lcd_send(0x0C, 0); 
  lcd_send(0x01, 0); // Clear screen
  lcd_send(0x06, 0); 
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

// Print the RTC and Temp numbers
void lcd_print_int(int num) {
  char buffer[10];
  itoa(num, buffer, 10); // converts int to string
  lcd_print(buffer);
}


// ==========================================
// MAIN SETUP AND LOOP
// ==========================================
void setup() {
  Serial.begin(9600);
  Serial.println("--- CPE 301 Unified Diagnostics (WIP) ---");

  // Setup LEDs
  pinMode(ledRed, OUTPUT); pinMode(ledYellow, OUTPUT);
  pinMode(ledGreen, OUTPUT); pinMode(ledBlue, OUTPUT);
  pinMode(ledWhite, OUTPUT);

  // Setup Inputs
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(waterSensorPin, INPUT);

  // Init BARE METAL LCD
  init_LCD();
  lcd_send(0x01, 0); // Clear screen
  lcd_print("System Boot...");

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

  // 4. BARE METAL LCD Print
  DateTime now = rtc.now();
  lcd_send(0x01, 0); // Bare metal clear screen
  
  lcd_set_cursor(0, 0);
  lcd_print("Time: ");
  lcd_print_int(now.hour()); // Using integer helper
  lcd_print(":");
  if (now.minute() < 10) lcd_print("0");
  lcd_print_int(now.minute());

  lcd_set_cursor(1, 0);
  lcd_print("Temp:");
  if (isnan(tempC)) {
    lcd_print("ERR"); 
  } else {
    // Cast to int, can't print floats yet
    lcd_print_int((int)tempC); 
    lcd_print("C");
  }
  
  lcd_print(" W:");
  if (waterState == HIGH) {
      lcd_print("WET");
  } else {
      lcd_print("DRY");
  }

  // 5. Serial Print
  Serial.print("Temp (C): "); Serial.print(tempC);
  Serial.print(" | Water: "); Serial.print(waterState);
  Serial.print(" | B1: "); Serial.print(b1Pressed ? "ON" : "OFF");
  Serial.print(" | B2: "); Serial.println(b2Pressed ? "ON" : "OFF");

  delay(2000); 
}

// ==========================================
// WATER LEVEL SENSOR (ADC) - Pin A0
// ==========================================

// Memory pointers for the Analog to Digital registers
volatile unsigned char *my_ADMUX    = (unsigned char *) 0x7C;
volatile unsigned char *my_ADCSRA   = (unsigned char *) 0x7A;
volatile unsigned char *my_ADC_Low  = (unsigned char *) 0x78; // ADCL
volatile unsigned char *my_ADC_High = (unsigned char *) 0x79; // ADCH

void setup_water_sensor() 
{
  // Set the reference voltage to AVCC (5V) and select Channel 0 (Pin A0)
  // REFS0 = 1, MUX = 0000. 
  // 0100 0000 = 0x40
  *my_ADMUX = 0x40;
  
  // Enable the ADC engine (ADEN) and set the prescaler to 128 so the clock isn't too fast
  // ADEN=1, ADPS2=1, ADPS1=1, ADPS0=1
  // 1000 0111 = 0x87
  *my_ADCSRA = 0x87;
}

int readWaterLevel() 
{
  // To start a reading, flip the ADSC bit (bit 6) to 1.
  // 0100 0000 = 0x40
  *my_ADCSRA |= 0x40;
  
  // flip bit 6 back to 0 when it is done.
  while ((*my_ADCSRA & 0x40) != 0x00) 
  {
    
  }
  
  // VERY IMPORTANT: You MUST read the low byte first. 
  // If you read the high byte first, the hardware locks the registers and ruins everything.
  unsigned int low_byte = *my_ADC_Low;
  unsigned int high_byte = *my_ADC_High;
  
  // The result is 10 bits spread across two 8-bit registers.
  // Multiply the high byte by 256 to shift it over, then add the low byte to smash them together.
  unsigned int finalResult = low_byte + (high_byte * 256);
  
  return finalResult;
}

// ==========================================
// DHT11 TEMP & HUMIDITY SENSOR - Pin 12 (Port B)
// ==========================================

// Pointers for Port B
volatile unsigned char *myDDRB  = (unsigned char *) 0x24;
volatile unsigned char *myPORTB = (unsigned char *) 0x25;
volatile unsigned char *myPINB  = (unsigned char *) 0x23;

// Global variables to store the final readings
int current_temp = 0;
int current_humidity = 0;

void read_DHT11() 
{
  unsigned char dht_data[5] = {0, 0, 0, 0, 0};
  
  // STEP 1: WAKE UP THE SENSOR
  // Set Pin 12 (Bit 6) to OUTPUT
  *myDDRB |= 0x40; 
  
  // Pull it LOW to send the start signal
  *myPORTB &= 0xBF;
  
  // We need to wait at least 18ms for the sensor to wake up. 
  // Since we can't use delay(), I just made this loop huge until it worked on the breadboard.
  for (volatile long i = 0; i < 30000; i++) {

  } 
  
  // Pull it HIGH
  *myPORTB |= 0x40;
  
  // STEP 2: LISTEN FOR THE SENSOR
  *myDDRB &= 0xBF; 
  
  // The sensor will pull low, then high, then low to acknowledge.
  // timeout limits to these loops so the whole Mega doesn't freeze if a wire falls out.
  int timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }

  // STEP 3: READ THE 40 BITS OF DATA
  for (int i = 0; i < 5; i++) 
  {
    for (int j = 7; j >= 0; j--) 
    {
      // Wait for the pin to go HIGH
      timeout = 0;
      while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
      
      // Now count exactly how long the pin STAYS high
      int pulse_length = 0;
      while ((*myPINB & 0x40) != 0) 
      {
        pulse_length++;
        if(pulse_length > 10000) return; // fail safe
      }
      
      // If it stayed high for a long time, the bit is a 1. If it was short, it's a 0.
            if (pulse_length > 40) 
      {
        // shove a 1 into the correct spot in array
        dht_data[i] |= (1 << j);
      }
    }
  }
  
  // STEP 4: VERIFY THE DATA
  // Add the first 4 bytes together. If they equal the 5th byte (checksum), the data is good!
  if (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3] == dht_data[4]) 
  {
     current_humidity = dht_data[0];
     current_temp = dht_data[2]; // This is in Celsius. 
  }
}

