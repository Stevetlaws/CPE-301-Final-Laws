// ==========================================
// MOTOR CONTROL (L293D) - Port L: Pins 47-49
// ==========================================

// Pointers for Port L (Extended I/O memory addresses on the Mega)
volatile unsigned char *portDDRL = (unsigned char *) 0x10A; 
volatile unsigned char *portL    = (unsigned char *) 0x10B; 

// Breadboard Wiring Map:
// Pin 49 (Bit 0) -> Connect to Enable 1 (EN1) on L293D
// Pin 48 (Bit 1) -> Connect to Input 1 (IN1) on L293D
// Pin 47 (Bit 2) -> Connect to Input 2 (IN2) on L293D

void initMotorSetup() 
{
  // Set bits 0, 1, and 2 to Output mode (1)
  // 0000 0111 in binary = 0x07 in hex
  *portDDRL |= 0x07;
  
  // Force the motor completely off during boot so it doesn't jump
  // 1111 1000 = 0xF8
  *portL &= 0xF8; 
}

void turn_fan_on() 
{
  // To spin the fan forward: EN1 must be HIGH, IN1 must be HIGH, IN2 must be LOW
  // Bit 0 = 1, Bit 1 = 1, Bit 2 = 0
  // 0000 0011 = 0x03
  
  // First clear the 3 bits completely just in case there's leftover garbage voltage
  *portL &= 0xF8; 
  
  // Now set the specific bits to start the fan
  *portL |= 0x03;
}

void turn_fan_OFF() 
{
  // To stop the motor safely, we drop the Enable pin to LOW. 
  // I am pulling IN1 and IN2 low as well just to be completely safe against voltage spikes.
  // 1111 1000 = 0xF8
  *portL &= 0xF8;
}