// ==========================================
// CPE 301 FINAL - 100% BARE-METAL MASTER FILE
// ==========================================

// No Libraries

// ==========================================
// MASTER MEMORY MAP (BARE METAL POINTERS)
// ==========================================

// UART SERIAL LOGGING (USART0)
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *) 0x00C6;

// RTC DS1307 CONTROL (I2C / TWI)
volatile unsigned char *myTWBR = (unsigned char *) 0xB8; 
volatile unsigned char *myTWSR = (unsigned char *) 0xB9; 
volatile unsigned char *myTWDR = (unsigned char *) 0xBB; 
volatile unsigned char *myTWCR = (unsigned char *) 0xBC; 

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

unsigned int current_sec = 0;
unsigned int current_min = 0;
unsigned int current_hr = 0;

enum SystemState { OFF, IDLE, RUNNING, ERROR };
SystemState current_state = OFF;

const int TEMP_THRESHOLD = 25; 

// ==========================================
// BARE METAL SUBROUTINES
// ==========================================

// --- UART (Serial Print) ---
void U0init(unsigned long baud) {
  unsigned int bittimer = (16000000 / (16 * baud)) - 1;
  *myUBRR0 = bittimer;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
}
void putChar(unsigned char data) {
  while ((*myUCSR0A & 0x20) == 0x00) {}
  *myUDR0 = data;
}
void printString(char* StringPtr) {
  while (*StringPtr != '\0') { putChar(*StringPtr); StringPtr++; }
}
void printInt(int n) {
  char buf[10]; itoa(n, buf, 10); printString(buf);
}
void printLine(char* str) {
  printString(str); putChar('\n');
}

// --- I2C (RTC Time) ---
void init_I2C() {
  *myTWBR = 0x48; 
  *myTWSR &= 0xFC; 
}
void wait_for_twi() {
  while ((*myTWCR & 0x80) == 0x00) {}
}
void i2c_start() {
  *myTWCR = 0xA4; wait_for_twi();
}
void i2c_stop() {
  *myTWCR = 0x94;
}
void i2c_write(unsigned char data) {
  *myTWDR = data; *myTWCR = 0x84; wait_for_twi();
}
unsigned char i2c_read_ACK() {
  *myTWCR = 0xC4; wait_for_twi(); return *myTWDR;
}
unsigned char i2c_read_NACK() {
  *myTWCR = 0x84; wait_for_twi(); return *myTWDR;
}
int bcd_to_decimal(unsigned char bcd) {
  return (bcd / 16 * 10) + (bcd % 16);
}
void get_rtc_time() {
  i2c_start(); i2c_write(0xD0); i2c_write(0x00); i2c_stop();
  i2c_start(); i2c_write(0xD1); 
  unsigned char raw_sec = i2c_read_ACK();
  unsigned char raw_min = i2c_read_ACK();
  unsigned char raw_hr  = i2c_read_NACK();
  i2c_stop();
  current_sec = bcd_to_decimal(raw_sec & 0x7F); 
  current_min = bcd_to_decimal(raw_min);
  current_hr  = bcd_to_decimal(raw_hr & 0x3F); 
}

// --- Timer 1 ---
void timer1_init() {
  *my_TCCR1A = 0x00; *my_TCCR1B = 0x00; *my_TCCR1C = 0x00;
  *my_TCCR1B = 0x05; *my_TIMSK1 = 0x01; *my_TCNT1 = 34286; 
  asm("sei");
}
ISR(TIMER1_OVF_vect) {
  *my_TCNT1 = 34286; timer_heartbeat = 1; 
}

// --- Motor Control ---
void init_motor() {
  *portDDRL |= 0x07; *portL &= 0xF8; 
}
void motor_on() {
  *portL |= 0x03; *portL &= 0xFB; 
}
void motor_off() {
  *portL &= 0xF8; 
}

// --- LEDs ---
void init_LEDs() {
  *portDDRA |= 0x1F; *portA &= 0xE0; 
}
void set_state_off_led() { *portA &= 0xE0; *portA |= 0x03; }     
void set_state_idle_led() { *portA &= 0xE0; *portA |= 0x05; }    
void set_state_running_led() { *portA &= 0xE0; *portA |= 0x09; } 
void trigger_error_led() { *portA &= 0xE0; *portA |= 0x10; }     

// --- Buttons ---
void init_buttons() {
  *portDDRC &= 0xFD; *portDDRD &= 0xF7; 
}
void debounce() { for(volatile long int i = 0; i < 30000; i++); }
int readStartButton() {
  if ((*pinD & 0x08) == 0x08) { debounce(); if ((*pinD & 0x08) == 0x08) return 1; }
  return 0;
}
int readResetButton() {
  if ((*pinC & 0x02) == 0x02) { debounce(); if ((*pinC & 0x02) == 0x02) return 1; }
  return 0;
}

// --- Water Sensor ---
void setup_water_sensor() {
  *my_ADMUX = 0x40; *my_ADCSRA = 0x87;
}
int readWaterLevel() {
  *my_ADCSRA |= 0x40; while ((*my_ADCSRA & 0x40) != 0x00) {}
  unsigned int low_byte = *my_ADC_Low; unsigned int high_byte = *my_ADC_High;
  return low_byte + (high_byte * 256);
}

// --- DHT11 ---
void read_DHT11() {
  unsigned char dht_data[5] = {0, 0, 0, 0, 0};
  int timeout = 0;
  
  asm("cli"); // Disable interrupts
  
  *myDDRB |= 0x40; *myPORTB &= 0xBF; 
  for (volatile long i = 0; i < 100000; i++); 
  *myPORTB |= 0x40; *myDDRB &= 0xBF;  
  
  while ((*myPINB & 0x40) != 0) { if(++timeout > 30000) { asm("sei"); return; } }
  timeout = 0; while ((*myPINB & 0x40) == 0) { if(++timeout > 30000) { asm("sei"); return; } }
  timeout = 0; while ((*myPINB & 0x40) != 0) { if(++timeout > 30000) { asm("sei"); return; } }
  
  for (int byte_idx = 0; byte_idx < 5; byte_idx++) {
    for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
      timeout = 0; while ((*myPINB & 0x40) == 0) { if(++timeout > 30000) { asm("sei"); return; } }
      int pulse_length = 0;
      while ((*myPINB & 0x40) != 0) {
        pulse_length++;
        if(pulse_length > 30000) { asm("sei"); return; } 
      }
      if (pulse_length > 100) dht_data[byte_idx] |= (1 << bit_idx); 
    }
  }

  asm("sei"); // Re-enable interrupts

  if ((dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) == dht_data[4]) {
     current_humidity = dht_data[0]; 
     current_temp = dht_data[2]; 
  } else {
     printLine(">> DHT11 FAULT: Checksum failed!");
  }
}

// --- LCD ---
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
  U0init(9600); 
  init_I2C();   
  
  init_LEDs();
  init_buttons();
  init_motor();
  setup_water_sensor();
  init_LCD();
  timer1_init();

  current_state = OFF;
  set_state_off_led();
  lcd_send(0x01, 0);
  lcd_print("SYSTEM OFF");
}

void loop() {
  // 1. READ BUTTONS
  if (readStartButton() == 1) {
    if (current_state == OFF) {
      current_state = IDLE;
      printLine("START BUTTON PRESSED: System IDLE");
    } else {
      current_state = OFF;
      printLine("STOP BUTTON PRESSED: System OFF");
    }
    for(volatile long int i = 0; i < 100000; i++); 
  }

  if (readResetButton() == 1 && current_state == ERROR) {
    if (is_wet) { 
      current_state = IDLE;
      printLine("RESET BUTTON PRESSED: Error Cleared");
    } else {
      printLine("CANNOT RESET: Water still low!");
    }
  }

  // 2. READ SENSORS 
  if (timer_heartbeat == 1) {
      if (current_state != OFF) {
        read_DHT11();
        water_raw = readWaterLevel();
        is_wet = (water_raw > 100); 
      }
      timer_heartbeat = 0; 
  }

  // 3. STATE MACHINE 
  switch(current_state) {
    case OFF:
      set_state_off_led();
      motor_off();
      if(timer_heartbeat == 0) { 
        lcd_set_cursor(0,0); lcd_print("SYSTEM OFF      ");
        lcd_set_cursor(1,0); lcd_print("                ");
      }
      break;

    case IDLE:
      set_state_idle_led();
      motor_off();
      if (!is_wet) current_state = ERROR; 
      else if (current_temp > TEMP_THRESHOLD) current_state = RUNNING; 
      break;

    case RUNNING:
      set_state_running_led();
      motor_on(); 
      if (!is_wet) current_state = ERROR; 
      else if (current_temp <= TEMP_THRESHOLD) current_state = IDLE; 
      break;

    case ERROR:
      trigger_error_led();
      motor_off(); 
      break;
  }

  // 4. LCD & SERIAL OUTPUT 
  if (current_state != OFF && timer_heartbeat == 0) {
      get_rtc_time();
      
      lcd_set_cursor(0, 0);
      if (current_hr < 10) lcd_print("0"); lcd_print_int(current_hr); lcd_print(":");
      if (current_min < 10) lcd_print("0"); lcd_print_int(current_min);
      lcd_print(" T:"); lcd_print_int(current_temp); lcd_print("C ");

      lcd_set_cursor(1, 0);
      if (current_state == IDLE) lcd_print("IDLE    ");
      if (current_state == RUNNING) lcd_print("RUNNING ");
      if (current_state == ERROR) lcd_print("ERROR   ");
      
      if (is_wet) lcd_print(" W:WET ");
      else lcd_print(" W:DRY ");

      // Serial Logging
      printString("State: "); printInt(current_state);
      printString(" | Temp: "); printInt(current_temp);
      printString(" | Water: "); printInt(water_raw);
      putChar('\n');
  }
}