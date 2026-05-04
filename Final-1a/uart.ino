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