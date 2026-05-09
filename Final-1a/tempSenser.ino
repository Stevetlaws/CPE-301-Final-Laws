// ==========================================
// BARE METAL DHT11 SENSOR (Port B - Pin 12)
// ==========================================
volatile unsigned char *myDDRB  = (unsigned char *) 0x24;
volatile unsigned char *myPORTB = (unsigned char *) 0x25;
volatile unsigned char *myPINB  = (unsigned char *) 0x23;

// Globals to hold the last good reading
int current_temp = 0;
int current_humidity = 0;

void read_DHT11() {
  unsigned char dht_data[5] = {0, 0, 0, 0, 0};
  
  // WAKE UP SENSOR (Pull Low for 18ms)
  *myDDRB |= 0x40; 
  *myPORTB &= 0xBF; 
  for (volatile long i = 0; i < 30000; i++) {} // clunky delay
  *myPORTB |= 0x40;
  
  // LISTEN
  *myDDRB &= 0xBF; 
  int timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
  timeout = 0;
  while ((*myPINB & 0x40) != 0) { timeout++; if(timeout > 10000) return; }

  // READ 40 BITS
  for (int i = 0; i < 5; i++) {
    for (int j = 7; j >= 0; j--) {
      timeout = 0;
      while ((*myPINB & 0x40) == 0) { timeout++; if(timeout > 10000) return; }
      
      int pulse_length = 0;
      while ((*myPINB & 0x40) != 0) {
        pulse_length++;
        if(pulse_length > 10000) return; 
      }
      if (pulse_length > 40) dht_data[i] |= (1 << j);
    }
  }
  
  // CHECKSUM VERIFICATION
  // BUG NOTE: If timing is off, this fails and we just keep the old temp.
  if (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3] == dht_data[4]) {
     current_humidity = dht_data[0];
     current_temp = dht_data[2]; 
  }
}

// --- Pin Definitions ---
const int waterSensorPin = 24;
const int ledRed = 47, ledYellow = 49, ledGreen = 51, ledBlue = 53, ledWhite = 6;
const int button1 = 18, button2 = 19;

// --- Library Objects ---
RTC_DS1307 rtc;