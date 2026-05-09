// ==========================================
// PUSH BUTTON INPUTS (Matches CirKit Schematic)
// ==========================================

// Port C Pointers (For Reset Button - Pin 36 / Bit 1)
volatile unsigned char *portDDRC = (unsigned char *) 0x27; 
volatile unsigned char *pinC     = (unsigned char *) 0x26; 

// Port D Pointers (For Start/Stop Button - Pin 18 / Bit 3)
volatile unsigned char *portDDRD = (unsigned char *) 0x2A; 
volatile unsigned char *pinD     = (unsigned char *) 0x29; 

void init_buttons() 
{
  // Set Port C, Bit 1 (Pin 36) to INPUT (0)
  *portDDRC &= 0xFD; 
  
  // Set Port D, Bit 3 (Pin 18) to INPUT (0)
  *portDDRD &= 0xF7; 
}

//debounce delay
void debounce() 
{
  for(volatile long int i = 0; i < 30000; i++) {} // bumped up zeroes so it actually works!
}

// Reads Pin 18 (Port D, Bit 3)
int readStartButton() 
{
  // 0000 1000 = 0x08
  if ((*pinD & 0x08) == 0x08) 
  {
    debounce(); 
    if ((*pinD & 0x08) == 0x08) {
      return 1; // Button is pressed (HIGH)
    }
  }
  return 0;
}

// Reads Pin 36 (Port C, Bit 1)
int read_reset_button() 
{
  if ((*pinC & 0x02) == 0x02) 
  {
    debounce(); 
    if ((*pinC & 0x02) == 0x02) {
      return 1; // Button is pressed (HIGH)
    }
  }
  return 0;
}