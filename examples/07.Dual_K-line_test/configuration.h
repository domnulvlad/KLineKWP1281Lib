//Select your target module addresses:
#define connect_to_module1 0x01
#define connect_to_module2 0x17

//Select your target module speed:
#define module_baud_rate1 9600
#define module_baud_rate2 10400

//Enable/disable printing library debug information on the Serial Monitor.
#define debug_info false
//Enable/disable printing bus traffic on the Serial Monitor.
#define debug_traffic false

//Uncomment one of the following options:

//ESP32 (can use Serial1 and Serial2)
/*
  #define K_line1 Serial1
  #define RX1_pin 16
  #define TX1_pin 17
  
  #define K_line2 Serial2
  #define RX2_pin 26
  #define TX2_pin 27
*/

#if not defined(K_line1) or not defined(K_line2)
  #error Please select an option in configuration.h!
#endif
