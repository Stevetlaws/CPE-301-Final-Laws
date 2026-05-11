// ==========================================
// CPE 301 FINAL - MASTER UNIFIED BARE-METAL FILE
// ==========================================

#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 rtc;

// ==========================================
// MASTER MEMORY MAP (BARE METAL POINTERS)
// ==========================================

// LCD 1602 (Port H - Pins 16, 17, 6, 7, 8, 9)
volatile unsigned char *portDDRH = (unsigned char *) 0x101; 
volatile unsigned char *portH    = (unsigned char *) 0x102; 

// Water Sensor (ADC - Pin A0)
volatile unsigned char *my_ADMUX    = (unsigned char *) 0x7C;
volatile unsigned char *my_ADCSRA   = (unsigned char *) 0x7A;
volatile unsigned char *my_ADC_Low  = (unsigned char *) 0x78; 
volatile unsigned char *my_ADC_High = (unsigned char *) 0x79; 

// DHT11 Temp Sensor (Port B - Pin 12 / Bit 6)
volatile unsigned char *myDDRB  = (unsigned char *) 0x24;
volatile unsigned char *myPORTB = (unsigned char *) 0x25;
volatile unsigned char *myPINB  = (unsigned char *) 0x23;

// LEDs (Port A - Pins 22, 23, 24, 25, 26)
volatile unsigned char *portDDRA = (unsigned char *) 0x21; 
volatile unsigned char *portA    = (unsigned char *) 0x22; 

// Buttons (Port C Pin 36, Port D Pin 18)
volatile unsigned char *portDDRC = (unsigned char *) 0x27; 
volatile unsigned char *pinC     = (unsigned char *) 0x26; 
volatile unsigned char *portDDRD = (unsigned char *) 0x2A; 
volatile unsigned char *pinD     = (unsigned char *) 0x29; 

// L293D Motor Driver (Port L - Pins 47, 48, 49)
// PL0 (49), PL1 (48), PL2 (47)
volatile unsigned char *portDDRL = (unsigned char *) 0x10A;
volatile unsigned char *portL    = (unsigned char *) 0x10B;

// Timer 1 (2-Second Heartbeat)
volatile unsigned char *my_TCCR1A = (unsigned char *) 0x80;
volatile unsigned char *my_TCCR1B = (unsigned char *) 0x81;
volatile unsigned char *my_TCCR1C = (unsigned char *) 0x82;
volatile unsigned char *my_TIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *my_TCNT1  = (unsigned int *)  0x84;

// ==========================================
// GLOBAL STATE VARIABLES
// ==========================================
int current_temp = 0;
int current_humidity = 0;
int water_raw = 0;
bool is_wet = false;
volatile int timer_heartbeat = 0;

// State Machine Variables
enum SystemState { OFF, IDLE, RUNNING, ERROR };
SystemState current_state = OFF;

// Thresholds
const int TEMP_THRESHOLD = 25; // Degrees Celsius to turn fan on

// ==========================================
// BARE METAL SUBROUTINES
// ==========================================

// --- Timer 1 ---
void timer1_init() {
  *my_TCCR1A = 0x00; *my_TCCR1B = 0x00; *my_TCCR1C = 0x00;
  *my_TCCR1B = 0x05; // 1024 prescaler
  *my_TIMSK1 = 0x01;
  *my_TCNT1 = 34286; // Preload for 2 seconds
  asm("sei");
}

ISR(TIMER1_OVF_vect) {
  *my_TCNT1 = 34286; 
  timer_heartbeat = 1; 
}

// --- Motor Control (Port L) ---
void init_motor() {
  *portDDRL |= 0x07; // Set bits 0, 1, 2 to output
  *portL &= 0xF8;    // Turn motor off initially
}

void motor_on() {
  // Pin 49 (Enable) HIGH, Pin 48 (In 1) HIGH, Pin 47 (In 2) LOW
  *portL |= 0x03; 
  *portL &= 0xFB; 
}

void motor_off() {
  *portL &= 0xF8; // All motor pins LOW
}

// --- LEDs (Port A) ---
void init_LEDs() {
  *portDDRA |= 0x1F;
  *portA &= 0xE0; // All off
}
void set_state_off_led() { *portA &= 0xE0; *portA |= 0x03; }     // Power + Yellow
void set_state_idle_led() { *portA &= 0xE0; *portA |= 0x05; }    // Power + Green
void set_state_running_led() { *portA &= 0xE0; *portA |= 0x09; } // Power + Blue
void trigger_error_led() { *portA &= 0xE0; *portA |= 0x10; }     // Red Only (Power off to show fault)

// --- Buttons (Ports C & D) ---
void init_buttons() {
  *portDDRC &= 0xFD; // Pin 36 input
  *portDDRD &= 0xF7; // Pin 18 input
}
void debounce() { for(volatile long int i = 0; i < 30000; i++); }

int readStartButton() {
  if ((*pinD & 0x08) == 0x08) {
    debounce(); 
    if ((*pinD & 0x08) == 0x08) return 1;
  }
  return 0;
}
int readResetButton() {
  if ((*pinC & 0x02) == 0x02) {
    debounce(); 
    if ((*pinC & 0x02) == 0x02) return 1;
  }
  return 0;
}

// --- Water Sensor ---
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

// --- DHT11 (Interrupt-Protected Version) ---
void read_DHT11() {
  unsigned char dht_data[5] = {0, 0, 0, 0, 0};
  int timeout = 0;
  
  // DISABLE GLOBAL INTERRUPTS so background timers don't ruin our counting
  asm("cli");
  
  // STEP 1: WAKE UP SENSOR 
  *myDDRB |= 0x40;  
  *myPORTB &= 0xBF; 
  for (volatile long i = 0; i < 100000; i++); 
  *myPORTB |= 0x40; 
  *myDDRB &= 0xBF;  
  
  // STEP 2: WAIT FOR ACKNOWLEDGE
  while ((*myPINB & 0x40) != 0) { if(++timeout > 30000) { asm("sei"); return; } }
  timeout = 0; 
  while ((*myPINB & 0x40) == 0) { if(++timeout > 30000) { asm("sei"); return; } }
  timeout = 0; 
  while ((*myPINB & 0x40) != 0) { if(++timeout > 30000) { asm("sei"); return; } }
  
  // STEP 3: READ THE 40 BITS
  for (int byte_idx = 0; byte_idx < 5; byte_idx++) {
    for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
      timeout = 0; 
      while ((*myPINB & 0x40) == 0) { if(++timeout > 30000) { asm("sei"); return; } }
      
      int pulse_length = 0;
      while ((*myPINB & 0x40) != 0) {
        pulse_length++;
        if(pulse_length > 30000) { asm("sei"); return; } 
      }
      if (pulse_length > 100) dht_data[byte_idx] |= (1 << bit_idx); 
    }
  }

  // ENABLE GLOBAL INTERRUPTS AGAIN
  asm("sei");

// ENABLE GLOBAL INTERRUPTS AGAIN
  asm("sei");

  // =======================================
  // STEP 4: VERIFY CHECKSUM & DIAGNOSE
  // =======================================
  
  // PRINT THE RAW BYTES TO SEE THE NOISE
  Serial.print(">> RAW DHT DATA: ");
  Serial.print(dht_data[0]); Serial.print(" ");
  Serial.print(dht_data[1]); Serial.print(" ");
  Serial.print(dht_data[2]); Serial.print(" ");
  Serial.print(dht_data[3]); Serial.print(" ");
  Serial.println(dht_data[4]);

  if ((dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) == dht_data[4]) {
     current_humidity = dht_data[0]; 
     current_temp = dht_data[2]; 
  } else {
     Serial.println(">> DHT11 FAULT: Checksum failed!");
  }
}

// --- LCD Subroutines ---
void lcd_delay() { for(volatile int i = 0; i < 1000; i++); }
void pulse_enable() { *portH |= 0x01; lcd_delay(); *portH &= 0xFE; lcd_delay(); }
void lcd_write_4bit(unsigned char val) { *portH &= 0x87; *portH |= (val << 3); pulse_enable(); }
void lcd_send(unsigned char data, int is_data) {
  if(is_data) *portH |= 0x02; else *portH &= 0xFD;  
  lcd_write_4bit(data >> 4); lcd_write_4bit(data & 0x0F); 
}
void init_LCD() {
  *portDDRH |= 0x7B; 
  lcd_delay(); lcd_write_4bit(0x03); lcd_delay(); lcd_write_4bit(0x03);
  lcd_delay(); lcd_write_4bit(0x03); lcd_write_4bit(0x02); 
  lcd_send(0x28, 0); lcd_send(0x0C, 0); lcd_send(0x01, 0); lcd_send(0x06, 0); 
}
void lcd_print(char* str) { while(*str) lcd_send(*str++, 1); }
void lcd_set_cursor(unsigned char row, unsigned char col) {
  unsigned char addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
  lcd_send(addr, 0);
}
void lcd_print_int(int num) {
  char buffer[10]; itoa(num, buffer, 10); lcd_print(buffer);
}

// ==========================================
// MAIN SETUP AND LOOP
// ==========================================
void setup() {
  Serial.begin(9600); // Standard Library (For now)
  
  // Initialize all bare-metal subsystems
  init_LEDs();
  init_buttons();
  init_motor();
  setup_water_sensor();
  init_LCD();
  timer1_init();

  // Initialize RTC (Standard Library)
  if (!rtc.begin()) {
    Serial.println("RTC ERROR");
  } else if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Set initial state
  current_state = OFF;
  set_state_off_led();
  lcd_send(0x01, 0);
  lcd_print("SYSTEM OFF");
}

void loop() {
  // 1. READ BUTTONS (Instantly responsive!)
  if (readStartButton() == 1) {
    if (current_state == OFF) {
      current_state = IDLE;
      Serial.println("START BUTTON PRESSED: System IDLE");
    } else {
      current_state = OFF;
      Serial.println("STOP BUTTON PRESSED: System OFF");
    }
    // Very lazy software debounce wait so it doesn't flutter
    for(volatile long int i = 0; i < 100000; i++); 
  }

  if (readResetButton() == 1 && current_state == ERROR) {
    if (is_wet) { // Only allow reset if water is fixed
      current_state = IDLE;
      Serial.println("RESET BUTTON PRESSED: Error Cleared");
    } else {
      Serial.println("CANNOT RESET: Water still low!");
    }
  }

  // 2. READ SENSORS (Only every 2 seconds via Timer)
  if (timer_heartbeat == 1) {
      if (current_state != OFF) {
        read_DHT11();
        water_raw = readWaterLevel();
        is_wet = (water_raw > 100); 
      }
      timer_heartbeat = 0; // Lower the flag
  }

  // 3. STATE MACHINE LOGIC
  switch(current_state) {
    
    case OFF:
      set_state_off_led();
      motor_off();
      // Only update screen if heartbeat fired to prevent flickering
      if(timer_heartbeat == 0) { 
        lcd_set_cursor(0,0); lcd_print("SYSTEM OFF      ");
        lcd_set_cursor(1,0); lcd_print("                ");
      }
      break;

    case IDLE:
      set_state_idle_led();
      motor_off();
      
      if (!is_wet) {
        current_state = ERROR; // Water ran out!
      } else if (current_temp > TEMP_THRESHOLD) {
        current_state = RUNNING; // It got hot!
      }
      break;

    case RUNNING:
      set_state_running_led();
      motor_on(); // FIRE UP THE L293D!
      
      if (!is_wet) {
        current_state = ERROR; // Water ran out while running!
      } else if (current_temp <= TEMP_THRESHOLD) {
        current_state = IDLE; // Cooled down
      }
      break;

    case ERROR:
      trigger_error_led();
      motor_off(); // Safely shut down motor
      // Stays in ERROR until reset button is pressed
      break;
  }

  // 4. LCD & SERIAL OUTPUT (If not OFF)
  if (current_state != OFF && timer_heartbeat == 0) {
      DateTime now = rtc.now();
      
      // Top Row: Time and Temp
      lcd_set_cursor(0, 0);
      if (now.hour() < 10) lcd_print("0"); lcd_print_int(now.hour()); lcd_print(":");
      if (now.minute() < 10) lcd_print("0"); lcd_print_int(now.minute());
      lcd_print(" T:"); lcd_print_int(current_temp); lcd_print("C ");

      // Bottom Row: Status and Water
      lcd_set_cursor(1, 0);
      if (current_state == IDLE) lcd_print("IDLE    ");
      if (current_state == RUNNING) lcd_print("RUNNING ");
      if (current_state == ERROR) lcd_print("ERROR   ");
      
      if (is_wet) lcd_print(" W:WET ");
      else lcd_print(" W:DRY ");

      // Serial Logging
      Serial.print("State: "); Serial.print(current_state);
      Serial.print(" | Temp: "); Serial.print(current_temp);
      Serial.print(" | Water: "); Serial.println(water_raw);
  }
}

// ==========================================
// UART SERIAL LOGGING (USART0)
// ==========================================

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *) 0x00C6;

// Function to initialize UART at a specific baud rate (usually 9600)
void U0init(unsigned long baud)
{
  unsigned int bittimer = (16000000 / (16 * baud)) - 1;
  
  // Set the baud rate into the high and low registers
  *myUBRR0 = bittimer;
  
  // Enable the receiver and transmitter
  // 0001 1000 = 0x18
  *myUCSR0B = 0x18;
  
  // Set frame format: 8 data bits, 1 stop bit
  // 0000 0110 = 0x06
  *myUCSR0C = 0x06;
}

// Basic function to send a single character
void putChar(unsigned char data)
{
  // Wait for the transmit buffer to be empty (UDRE0 bit in UCSR0A)
  // 0010 0000 = 0x20
  while ((*myUCSR0A & 0x20) == 0x00) 
  {
     // Wait...
  }
  
  // Shove the data into the buffer
  *myUDR0 = data;
}

// Clunky string printer using a pointer
void printString(char* StringPtr)
{
  while (*StringPtr != '\0') 
  {
    putChar(*StringPtr);
    StringPtr++;
  }
}

// Helper to print numbers (because we can't use Serial.print(int))
void printInt(int n) 
{
  char buf[10];
  itoa(n, buf, 10); // Standard C function to turn int to string
  printString(buf);
}

// ==========================================
// RTC DS1307 CONTROL (I2C / TWI) - Pins 20 & 21
// ==========================================

// TWI (I2C) Register Pointers
volatile unsigned char *myTWBR = (unsigned char *) 0xB8; // Bit Rate Register
volatile unsigned char *myTWSR = (unsigned char *) 0xB9; // Status Register
volatile unsigned char *myTWDR = (unsigned char *) 0xBB; // Data Register
volatile unsigned char *myTWCR = (unsigned char *) 0xBC; // Control Register

// Global variables to hold the time so we don't have to deal with returning pointers
unsigned int current_sec = 0;
unsigned int current_min = 0;
unsigned int current_hr = 0;

void init_I2C() 
{
  // set the I2C clock to 100kHz. The formula is F_CPU / (16 + 2 * TWBR * Prescaler)
  // 16MHz / (16 + 2 * 72 * 1) = 100,000. So TWBR needs to be 72 (which is 0x48 in hex)
  *myTWBR = 0x48; 
  *myTWSR &= 0xFC; // force prescaler to 1 just to be safe
}

// I hate I2C. We have to wait for the TWINT flag (bit 7) to become 1 before doing anything else
void wait_for_twi() 
{
  while ((*myTWCR & 0x80) == 0x00) 
  {
     // do absolutely nothing until the hardware says it is ready
  }
}

void i2c_start() 
{
  // send START condition: TWINT=1, TWSTA=1, TWEN=1 
  // 1010 0100 = 0xA4
  *myTWCR = 0xA4;
  wait_for_twi();
}

void i2c_stop() 
{
  // send STOP condition: TWINT=1, TWSTO=1, TWEN=1
  // 1001 0100 = 0x94
  *myTWCR = 0x94;
  // you don't wait for the flag after a stop
}

void i2c_write(unsigned char data) 
{
  *myTWDR = data;
  // clear TWINT to start transmission: TWINT=1, TWEN=1
  // 1000 0100 = 0x84
  *myTWCR = 0x84; 
  wait_for_twi();
}

unsigned char i2c_read_ACK() 
{
  // read and send an Acknowledge (ACK) so the chip knows we want more data
  // TWINT=1, TWEA=1, TWEN=1
  // 1100 0100 = 0xC4
  *myTWCR = 0xC4;
  wait_for_twi();
  return *myTWDR;
}

unsigned char i2c_read_NACK() 
{
  // read but DON'T acknowledge (NACK) so the chip knows we are done reading
  // TWINT=1, TWEN=1
  // 1000 0100 = 0x84
  *myTWCR = 0x84;
  wait_for_twi();
  return *myTWDR;
}

// The DS1307 gives us weird BCD format numbers. This fixes it.
int bcd_to_decimal(unsigned char bcd) 
{
  return (bcd / 16 * 10) + (bcd % 16);
}

// The main function to grab the time
void get_rtc_time() 
{
  // DS1307 I2C address is 0x68. 
  // Shifted left 1 bit for writing = 0xD0
  i2c_start();
  i2c_write(0xD0); 
  
  // tell the RTC we want to start reading from register 0x00 (Seconds)
  i2c_write(0x00); 
  i2c_stop();

  // Now actually read the data
  // Shifted left 1 bit for reading (+1) = 0xD1
  i2c_start();
  i2c_write(0xD1); 
  
  // read seconds and minutes with ACK, read hours with NACK to end it
  unsigned char raw_sec = i2c_read_ACK();
  unsigned char raw_min = i2c_read_ACK();
  unsigned char raw_hr  = i2c_read_NACK();
  i2c_stop();

  // Convert the garbage BCD data into normal numbers and save to our global variables
  // Mask the seconds to ignore the CH bit (bit 7)
  current_sec = bcd_to_decimal(raw_sec & 0x7F); 
  current_min = bcd_to_decimal(raw_min);
  // Mask the hours to ignore the 12/24 hour format bits
  current_hr  = bcd_to_decimal(raw_hr & 0x3F); 
}