//Change to correspond to the target module:
#define module 0x01
#define module_baud_rate 10400

//Enable/disable printing bus traffic on the Serial Monitor:
#define has_debug false

//Select whether or not your serial interface can receive at the same time as sending (or whether or not your K-line interface has echo as it should):
#define is_full_duplex true

//Uncomment one of the following options:

//Arduino UNO (no additional hardware serial ports, must use software serial)
/*
  #include <AltSoftSerial.h>
  AltSoftSerial software_serial_port;
  #define K_line software_serial_port
  #define TX_pin 9
*/

//Arduino Mega (can use Serial1-Serial3)
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

//ESP32 (can use Serial2)
/*
  #define K_line Serial2
  #define TX_pin 17
*/

//ESP8266 can not be used with this sketch (explained in the main file)
#ifdef ESP8266
  #error This demo sketch is not compatible with the ESP8266!
#endif

#ifndef K_line
  #error Please select an option in configuration.h!
#endif
