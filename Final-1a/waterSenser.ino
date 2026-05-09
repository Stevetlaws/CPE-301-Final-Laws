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
  // To start a reading, we have to flip the ADSC bit (bit 6) to 1.
  // 0100 0000 = 0x40
  *my_ADCSRA |= 0x40;
  
  // Now we have to wait. The hardware will flip bit 6 back to 0 when it is done.
  while ((*my_ADCSRA & 0x40) != 0x00) 
  {
    // just wait around.
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