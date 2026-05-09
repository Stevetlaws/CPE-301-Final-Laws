// ==========================================
// LCD 1602 CONTROL (4-Bit Mode) - Port H
// ==========================================

volatile unsigned char *portDDRH = (unsigned char *) 0x101; 
volatile unsigned char *portH    = (unsigned char *) 0x102; 

// A very clunky micro-delay for the LCD hardware to keep up
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