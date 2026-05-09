// --- LIBRARIES (WIP) ---
#include <Wire.h>
#include <RTClib.h>
// #include <LiquidCrystal.h>
// #include <DHT.h>           

// ==========================================
// MASTER MEMORY MAP (BARE METAL POINTERS)
// ==========================================

// LCD 1602 (Port H)
volatile unsigned char *portDDRH = (unsigned char *) 0x101; 
volatile unsigned char *portH    = (unsigned char *) 0x102; 

// Water Sensor (ADC)
volatile unsigned char *my_ADMUX    = (unsigned char *) 0x7C;
volatile unsigned char *my_ADCSRA   = (unsigned char *) 0x7A;
volatile unsigned char *my_ADC_Low  = (unsigned char *) 0x78; 
volatile unsigned char *my_ADC_High = (unsigned char *) 0x79; 

// DHT11 Temp Sensor (Port B - Pin 12)
volatile unsigned char *myDDRB  = (unsigned char *) 0x24;
volatile unsigned char *myPORTB = (unsigned char *) 0x25;
volatile unsigned char *myPINB  = (unsigned char *) 0x23;

// --- Remaining Standard Pins & Objects ---
const int ledRed = 47, ledYellow = 49, ledGreen = 51, ledBlue = 53, ledWhite = 6;
const int button1 = 18, button2 = 19;
RTC_DS1307 rtc;

// Global Variables
int current_temp = 0;
int current_humidity = 0;

// ==========================================
// BARE METAL SUBROUTINES
// ==========================================

// --- LCD Subroutines ---
void lcd_delay() { for(volatile int i = 0; i < 1000; i++); }

void pulse_enable() {
  *portH |= 0x01;  lcd_delay();
  *portH &= 0xFE;  lcd_delay();
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
  lcd_delay(); lcd_write_4bit(0x03);
  lcd_delay(); lcd_write_4bit(0x03);
  lcd_delay(); lcd_write_4bit(0x03);
  lcd_write_4bit(0x02); 
  lcd_send(0x28, 0); lcd_send(0x0C, 0); 
  lcd_send(0x01, 0); lcd_send(0x06, 0); 
}

void lcd_print(char* str) {
  while(*str) lcd_send(*str++, 1);
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
  unsigned char addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
  lcd_send(addr, 0);
}

void lcd_print_int(int num) {
  char buffer[10];
  itoa(num, buffer, 10); 
  lcd_print(buffer);
}

// --- Water Sensor Subroutines ---
void setup_water_sensor() {
  *my_ADMUX = 0x40;
  *my_ADCSRA = 0x87;
}

int readWaterLevel() {
  *my_ADCSRA |= 0x40;
  while ((*my_ADCSRA & 0x40) != 0x00) {}
  unsigned int low_byte = *my_ADC_Low;
  unsigned int high_byte = *my_ADC_High;
  return low_byte + (high_byte * 256);
}

// --- DHT11 Subroutines ---
void read_DHT11() {
  unsigned char dht_data[5] = {0, 0, 0, 0, 0};
  
  *myDDRB |= 0x40; 
  *myPORTB &= 0xBF;
  for (volatile long i = 0; i < 30000; i++) {} 
  *myPORTB |= 0x40;
  
  *myDDRB &= 0xBF; 
  int timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }

  for (int i = 0; i < 5; i++) {
    for (int j = 7; j >= 0; j--) {
      timeout = 0;
      while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
      int pulse_length = 0;
      while ((*myPINB & 0x40) != 0) {
        pulse_length++;
        if(pulse_length > 10000) return; 
      }
      if (pulse_length > 40) dht_data[i] |= (1 << j);
    }
  }
  
  if (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3] == dht_data[4]) {
     current_humidity = dht_data[0];
     current_temp = dht_data[2]; 
  }
}

// ==========================================
// MAIN SETUP AND LOOP
// ==========================================
void setup() {
  Serial.begin(9600);
  Serial.println("--- CPE 301 Unified Diagnostics (Phase 2) ---");

  // Setup Standard Pins
init_LEDs();
  set_state_off_led(); // Start the system in the OFF state by default
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);

  // Initialize Bare Metal Components
  init_LCD();
  lcd_send(0x01, 0); // Clear screen
  lcd_print("System Boot...");
  
  setup_water_sensor();

  // Init RTC
  if (!rtc.begin()) {
    Serial.println("RTC ERROR");
  } else if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  // 1. Read Bare Metal Sensors
  read_DHT11(); // Updates current_temp global variable
  int rawWater = readWaterLevel(); 
  bool isWet = (rawWater > 100); // Simple threshold to replace digitalRead

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
  lcd_send(0x01, 0); // Clear screen
  
  lcd_set_cursor(0,