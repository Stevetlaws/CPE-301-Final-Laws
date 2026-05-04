// ==========================================
// PUSH BUTTON INPUTS
// ==========================================

volatile unsigned char *portDDRC = (unsigned char *) 0x27; // Data Direction Register
volatile unsigned char *portC    = (unsigned char *) 0x28; // Port Data Register
volatile unsigned char *pinC     = (unsigned char *) 0x26; // PIN register to actually read the voltage


// Pin 37 (Bit 0) = Start/Stop Button
// Pin 36 (Bit 1) = Reset Button

void init_buttons() 
{
  // We want bits 0 and 1 to be 0 for INPUT
  *portDDRC &= 0xFC; 
  
  *portC &= 0xFC; 
}

void debounce() 
{

  for(volatile long int i = 0; i < 300; i++) 
  {
     
  }
}

int readStartButton() 
{
  // read the PINC register and mask everything but bit 0
  if ((*pinC & 0x01) == 0x01) 
  {
    debounce();
    
    if ((*pinC & 0x01) == 0x01) {
      return 1; // button pressed
    }
  }
  return 0;
}

int read_reset_button() 
{
  //read PINC register and mask everything but bit 1
  if ((*pinC & 0x02) == 0x02) 
  {
    debounce(); 
    if ((*pinC & 0x02) == 0x02) {
      return 1;
    }
  }
  return 0;
}