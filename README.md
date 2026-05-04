# CPE 301 Final Project: Automated Evaporative Cooling System
**Author:** Steve Laws  
**Course:** CPE 301 - Embedded Systems Design  
**Semester:** Spring 2026  

## Video Demonstration
[**Click here to view the hardware demonstration video**](INSERT_YOUR_YOUTUBE_OR_DRIVE_LINK_HERE)

## Project Overview
This project is a register-level control system for an evaporative cooler utilizing an ATmega2560. The system monitors ambient temperature and water reservoir levels to autonomously regulate a cooling fan. It features a four-state operational machine (OFF, IDLE, RUNNING, ERROR) designed to optimize power consumption and prevent hardware damage.

## Bare-Metal Hardware Implementation
To satisfy the strict requirements of CPE 301, standard Arduino libraries (`analogRead`, `digitalWrite`, `LiquidCrystal`, `delay`) were completely bypassed. 

Key register-level achievements in this codebase include:
* **Custom ADC Polling:** The water level sensor is read by directly manipulating the `ADMUX` and `ADCSRA` registers, manually setting the 128 prescaler and triggering the conversion flag.
* **UART Serial Communication:** Standard `Serial.print()` was replaced with custom pointer-based `putChar` and `printString` functions interacting directly with `UCSR0A` and `UDR0`.
* **Hardware Timers:** System timing and polling intervals are handled via asynchronous Timer Overflow Interrupt Service Routines (`ISR(TIMER1_OVF_vect)`).

## Hardware Components
* ATmega2560 Microcontroller
* DHT11 Temperature & Humidity Sensor
* Water Level Sensor
* DS1307 RTC Module (I2C)
* DC Motor driven by L293D IC
* LCD 1602 Display (Custom 4-bit initialization)
* LEDs, Push Buttons, and 10k/220 Ohm Resistors

## State Machine Logic
1. **OFF:** System disabled, monitoring for the Start interrupt.
2. **IDLE:** System active. Ambient temperature is below the cooling threshold. Fan is disabled.
3. **RUNNING:** Ambient temperature exceeds threshold; Water levels are nominal. Fan is active.
4. **ERROR:** Water level falls below the safe threshold. Motor is immediately halted to prevent dry-running. System requires manual reset.
