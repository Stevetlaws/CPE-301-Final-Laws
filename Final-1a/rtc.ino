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