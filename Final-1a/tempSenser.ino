// ==========================================
// DHT11 TEMP & HUMIDITY SENSOR - Pin 12 (Port B)
// ==========================================

// Pointers for Port B
volatile unsigned char *myDDRB  = (unsigned char *) 0x24;
volatile unsigned char *myPORTB = (unsigned char *) 0x25;
volatile unsigned char *myPINB  = (unsigned char *) 0x23;

// Global variables to store the final readings
int current_temp = 0;
int current_humidity = 0;

void read_DHT11() 
{
  unsigned char dht_data[5] = {0, 0, 0, 0, 0};
  
  // STEP 1: WAKE UP THE SENSOR
  // Set Pin 12 (Bit 6) to OUTPUT
  // 0100 0000 = 0x40
  *myDDRB |= 0x40; 
  
  // Pull it LOW to send the start signal
  *myPORTB &= 0xBF; // 1011 1111
  
  // We need to wait at least 18ms for the sensor to wake up. 
  // Since we can't use delay(), I just made this loop huge until it worked on the breadboard.
  for (volatile long i = 0; i < 30000; i++) {
      // waste time
  } 
  
  // Pull it HIGH
  *myPORTB |= 0x40;
  
  // STEP 2: LISTEN FOR THE SENSOR
  // Switch Pin 12 to INPUT mode
  *myDDRB &= 0xBF; 
  
  // The sensor will pull low, then high, then low to acknowledge.
  // I added arbitrary timeout limits to these loops so the whole Mega doesn't freeze if a wire falls out.
  int timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }

  // STEP 3: READ THE 40 BITS OF DATA
  for (int i = 0; i < 5; i++) 
  {
    for (int j = 7; j >= 0; j--) 
    {
      // Wait for the pin to go HIGH
      timeout = 0;
      while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
      
      // Now count exactly how long the pin STAYS high
      int pulse_length = 0;
      while ((*myPINB & 0x40) != 0) 
      {
        pulse_length++;
        if(pulse_length > 10000) return; // fail safe
      }
      
      // If it stayed high for a long time, the bit is a 1. If it was short, it's a 0.
      // I tested this with UART prints and the threshold on my board is around 40 loops.
      if (pulse_length > 40) 
      {
        // shove a 1 into the correct spot in our array
        dht_data[i] |= (1 << j);
      }
    }
  }
  
  // STEP 4: VERIFY THE DATA
  // Add the first 4 bytes together. If they equal the 5th byte (checksum), the data is good!
  if (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3] == dht_data[4]) 
  {
     current_humidity = dht_data[0];
     current_temp = dht_data[2]; // This is in Celsius. 
  }
}