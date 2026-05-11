Component,Arduino Mega Pin,Port / Bit,Function
Start/Stop Button,Digital 18,Port D (Bit 3),Input (Hardware Interrupt)
Reset Button,Digital 36,Port C (Bit 1),Input
Water Sensor (Signal),Analog A0,ADC Multiplexer,Input (Analog to Digital)
DHT11 Temp Sensor,Digital 12,Port B (Bit 6),Input/Output (1-Wire Data)
RTC DS1307 (SDA),Digital 20,TWI / I2C,Data Line
RTC DS1307 (SCL),Digital 21,TWI / I2C,Clock Line
System OFF LED (Yellow),Digital 22,Port A (Bit 0),Output
System IDLE LED (Green),Digital 23,Port A (Bit 1),Output
System RUNNING LED (Blue),Digital 24,Port A (Bit 2),Output
System ERROR LED (Red),Digital 25,Port A (Bit 3),Output
Power Status LED (White),Digital 26,Port A (Bit 4),Output
L293D Motor (IN 2),Digital 47,Port L (Bit 2),Output (Direction)
L293D Motor (IN 1),Digital 48,Port L (Bit 1),Output (Direction)
L293D Motor (Enable),Digital 49,Port L (Bit 0),Output (PWM / Power)
LCD (RS),Digital 16,Port H (Bit 1),Output (Register Select)
LCD (Enable),Digital 17,Port H (Bit 0),Output
LCD (D4 - D7),"Digital 6, 7, 8, 9",Port H (Bits 3-6),Output (4-bit Data Bus)