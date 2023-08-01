/*
  Title:
    01.Connection_test.ino

  Description:
    Demonstrates how to establish a connection with a module and read its identification.

  Notes:
    *The connection will be maintained while the sketch is running.
*/

/*
  *This sketch displays information in the Serial Monitor, so another serial port is required for the K-line.
  *If another port is not available on the selected board, the regular port "Serial" can be used, although no information can be printed to it (good for
  projects with external displays or for data logging).

  ***Uncomment the appropriate options in the configuration.h file!

  Arduino UNO
    *no additional serial port, software serial is used
    *library of choice: AltSoftSerial (better than SoftwareSerial because it can also receive while sending)
    *pins:
      *K-line TX -> RX pin 8
      *K-line RX -> TX pin 9

  Arduino MEGA
    *has three additional serial ports
    *pins:
      *K-line TX -> Serial1 - RX pin 19 / Serial2 - RX pin 17 / Serial3 - RX pin 15
      *K-line RX -> Serial1 - TX pin 18 / Serial2 - TX pin 16 / Serial3 - TX pin 14

  ESP32
    *has one additional serial port
    *pins:
      *K-line TX -> RX pin 16
      *K-line RX -> TX pin 17

  ESP8266
    *has no additional serial ports
    *library of choice: SoftwareSerial (the ESP version of SoftwareSerial can also receive while sending)
    *pins: any unused
      *chosen for this demo:
        *K-line TX -> RX pin 4 (D2)
        *K-line RX -> TX pin 5 (D1)

  ***If using the first hardware serial port (Serial) (with other sketches), the interface must be disconnected during code upload, and no "Serial.print"s
  should be used.
*/

//Include the library.
#include <KLineKWP1281Lib.h>

//Include the two files containing configuration options and the functions used for communication.
#include "configuration.h"
#include "communication.h"

//Debugging can be enabled in configuration.h in order to print connection-related info on the Serial Monitor.
#if debug_info
  KLineKWP1281Lib diag(beginFunction, endFunction, sendFunction, receiveFunction, TX_pin, is_full_duplex, &Serial);
#else
  KLineKWP1281Lib diag(beginFunction, endFunction, sendFunction, receiveFunction, TX_pin, is_full_duplex);
#endif

//Debugging can be enabled in configuration.h in order to print bus traffic on the Serial Monitor.
#if debug_traffic
void KWP1281debugFunction(bool type, uint8_t sequence, uint8_t command, uint8_t* data, uint8_t length) {
  Serial.println();
  
  Serial.println(type ? "RECEIVE:" : "SEND:");

  Serial.print("*command: ");
  if (command < 0x10) Serial.print(0);
  Serial.println(command, HEX);

  Serial.print("*sequence: ");
  if (sequence < 0x10) Serial.print(0);
  Serial.println(sequence, HEX);

  if (length) {
    Serial.print("*data bytes: ");
    Serial.println(length);

    Serial.print("*data: ");
    for (uint16_t i = 0; i < length; i++) { //iterate through the message's contents
      if (data[i] < 0x10) Serial.print(0);  //print a leading 0 where necessary to display 2-digit HEX
      Serial.print(data[i], HEX);           //print the byte in HEX
      Serial.print(' ');
    }
    Serial.println();
  }
}
#endif

//You can increase the value below if you get "Too many faults for the given buffer size" during the DTC test.
#define DTC_BUFFER_MAX_FAULT_CODES 16
uint8_t faults[3 * DTC_BUFFER_MAX_FAULT_CODES]; //buffer to store the fault codes; each code takes 3 bytes

void setup() {
  //Initialize the Serial Monitor.
  Serial.begin(115200);
  delay(500);
  Serial.println("Sketch started.");
  
  //If debugging bus traffic was enabled, attach the debugging function.
#if debug_traffic
  diag.KWP1281debugFunction(KWP1281debugFunction);
#endif
  
  //Change these according to your module, in configuration.h.
  diag.connect(connect_to_module, module_baud_rate);
  
  //Leave an empty line.
  Serial.println();

  //Print the module's part number.
  Serial.println(diag.getPartNumber());

  //Print the module's regular identification (name).
  Serial.println(diag.getIdentification());

  //If it is available, print the module's extra identification.
  if (strlen(diag.getExtraIdentification())) {
    Serial.println(diag.getExtraIdentification());
  }
  
  //Put the module's coding value into a string and pad with 0s to show 5 characters.
  char coding_str[6];
  sprintf(coding_str, "%05u", diag.getCoding());
  
  //Put the module's wokshop code into a string and pad with 0s to show 5 characters.
  char WSC_str[6];
  #if defined(__AVR__)
    sprintf(WSC_str, "%05lu", diag.getWorkshopCode());
  #else
    sprintf(WSC_str, "%05u", diag.getWorkshopCode()); //uint32_t is not a "long" on the ESP platform
  #endif
  
  //Leave an empty line.
  Serial.println();
  
  //Print the module's coding value.
  Serial.print("Coding: ");
  Serial.println(coding_str);
  
  //Print the module's wokshop code.
  Serial.print("WSC: ");
  Serial.println(WSC_str);
  
  //Leave an empty line.
  Serial.println();
  
  Serial.println("Maintaining the connection active.");
}

void loop() {
  //Maintain the connection.
  diag.update();
}
