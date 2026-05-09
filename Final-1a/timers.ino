// ==========================================
// TIMER 1 CODE (Repurposed from Lab 9)
// ==========================================

// Register pointers from Lab 9
volatile unsigned char *my_TCCR1A = (unsigned char *) 0x80;
volatile unsigned char *my_TCCR1B = (unsigned char *) 0x81;
volatile unsigned char *my_TCCR1C = (unsigned char *) 0x82;
volatile unsigned char *my_TIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *my_TCNT1  = (unsigned int *)  0x84;

// Global flag to trigger sensor reads in the main loop
volatile int timer_heartbeat = 0;

// Counter to slow down the heartbeat 
// (Timer 1 overflows fast, but we only need sensors every second or so)
volatile unsigned int overflow_counter = 0;

void timer1_init() 
{
  // Clear control registers (from Lab 9 setup)
  *my_TCCR1A = 0x00;
  *my_TCCR1B = 0x00;
  *my_TCCR1C = 0x00;
  
  // Set Timer 1 to Normal Mode (same as Lab 9)
  // Prescaler set to 1024 to get the slowest possible ticks
  // CS12=1, CS11=0, CS10=1 -> 0x05
  *my_TCCR1B = 0x05;
  
  // Enable the Overflow Interrupt (TOIE1)
  *my_TIMSK1 = 0x01;
  
  // Initialize the counter to 0
  *my_TCNT1 = 0x0000;
  
  // Make sure global interrupts are on!
  asm("sei");
}

// The Interrupt Service Routine (ISR)
// This is exactly where your Lab 9 piano frequency logic used to live
ISR(TIMER1_OVF_vect) 
{
  // Since we don't need to change notes, we just increment a counter
  overflow_counter++;
  
  // At 1024 prescaler, Timer 1 overflows about every 4 seconds.
  // If we want to check temperature/water every 4 seconds, we flip the flag now.
  if (overflow_counter >= 1) 
  {
    timer_heartbeat = 1; 
    overflow_counter = 0; // Reset for the next cycle
  }
}

// ==========================================
// TIMER 1 (2-Second Heartbeat Interrupt)
// ==========================================

volatile unsigned char *my_TCCR1A = (unsigned char *) 0x80;
volatile unsigned char *my_TCCR1B = (unsigned char *) 0x81;
volatile unsigned char *my_TCCR1C = (unsigned char *) 0x82;
volatile unsigned char *my_TIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *my_TCNT1  = (unsigned int *)  0x84;

// Flag to trigger the main loop
volatile int timer_heartbeat = 0;

void timer1_init() 
{
  *my_TCCR1A = 0x00;
  *my_TCCR1B = 0x00;
  *my_TCCR1C = 0x00;
  
  // Prescaler 1024 (CS12=1, CS11=0, CS10=1 -> 0x05)
  *my_TCCR1B = 0x05;
  
  // Enable Overflow Interrupt
  *my_TIMSK1 = 0x01;
  
  // Preload timer for exactly 2 seconds
  // 65536 - (16MHz / 1024 * 2s) = 34286 (0x85EE)
  *my_TCNT1 = 34286; 
}

// The Interrupt Service Routine
ISR(TIMER1_OVF_vect) 
{
  // 1. Immediately reset the timer to 34286 so the next 2 seconds can start counting
  *my_TCNT1 = 34286; 
  
  // 2. Raise the flag for the main loop
  timer_heartbeat = 1; 
}