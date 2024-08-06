// Select your target module address:
#define connect_to_module 0x01

// Select your target module speed:
#define module_baud_rate 10400

// Enable/disable printing library debug information on the Serial Monitor.
// You may change the debug level in "KLineKWP1281Lib.h".
#define debug_info false
// Enable/disable printing bus traffic on the Serial Monitor.
#define debug_traffic false

// Select whether or not your serial interface can receive at the same time as sending (or whether or not your K-line interface has echo as it should).
// Most software serial libraries for AVR microcontrollers are half-duplex (can't receive while sending), so if using such library this should be false.
#define is_full_duplex true

// Uncomment one of the following options:

// Arduino UNO (no additional hardware serial ports, must use software serial; AltSoftSerial was chosen as it is full-duplex)
/*
  #include <AltSoftSerial.h>
  AltSoftSerial software_serial_port; //pins cannot be configured
  #define K_line software_serial_port
  #define TX_pin 9
*/

// Arduino Mega (can use Serial1-Serial3)
/*
  #define K_line Serial1
  #define TX_pin 18
*/
/*
  #define K_line Serial2
  #define TX_pin 16
*/
/*
  #define K_line Serial3
  #define TX_pin 14
*/

// ESP32 (can use Serial1/Serial2)
/*
  #define K_line Serial2
  #define TX_pin 17
  #define RX_pin 16
*/

// ESP32-C6 (can use Serial1)
/*
  #define K_line Serial1
  #define TX_pin 17
  #define RX_pin 16
*/

// ESP8266 (no additional hardware serial ports, must use software serial)
// You can change the RX and TX pins to any unused pin, don't forget to also change the TX_pin define below.
/*
  #include "SoftwareSerial.h"
  SoftwareSerial software_serial_port(4, 5); //D2, D1
  #define K_line software_serial_port
  #define TX_pin 5 //D1
*/

// Raspberry Pi Pico (can use Serial1/Serial2)
/*
  #define K_line Serial1
  #define TX_pin 0
*/
/*
  #define K_line Serial2
  #define TX_pin 8
*/

#ifndef K_line
  #error Please select an option in configuration.h!
#endif
