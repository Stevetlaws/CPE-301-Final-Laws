// ==========================================
// STATUS LED CONTROLS
// ==========================================

// Global pointers for Port A registers
volatile unsigned char *portDDRA = (unsigned char *) 0x21; // Data Direction Register
volatile unsigned char *portA    = (unsigned char *) 0x22; // Port Data Register

// Pin 22 (Bit 0): System Power (White/Blue)
// Pin 23 (Bit 1): State OFF (Yellow)
// Pin 24 (Bit 2): State IDLE (Green)
// Pin 25 (Bit 3): State RUNNING (Blue)
// Pin 26 (Bit 4): State ERROR (Red)

void init_LEDs() 
{
  // Set the first 5 bits of Port A to 1 (Output)
  *portDDRA |= 0x1F;
  
  // Turn them all off
  *portA &= 0xE0;
}

void clearAllLEDs() 
{
  // Force bits 0-4 to zero
  *portA &= 0xE0; 
}

void set_state_off_led() 
{
  clearAllLEDs();
  // Turn on Power LED (Bit 0) and OFF LED (Bit 1)
  *portA |= 0x03; 
}

void set_state_idle_led() 
{
  clearAllLEDs();
  // Turn on Power LED (Bit 0) and IDLE LED (Bit 2)
  *portA |= 0x05;
}

void set_state_running_led() 
{
  clearAllLEDs();
  // Turn on Power LED (Bit 0) and RUNNING LED (Bit 3)
  *portA |= 0x09;
}

void trigger_error_led() 
{
  clearAllLEDs();
  // Turn on ERROR LED (Bit 4) only, turn off power light to show fault
  *portA |= 0x10;
}