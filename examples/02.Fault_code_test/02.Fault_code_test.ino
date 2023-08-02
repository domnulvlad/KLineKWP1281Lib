/*
  Title:
    02.Fault_code_test.ino

  Description:
    Demonstrates how to read a module's fault codes and their elaborations.

  Notes:
    *The connection will be stopped after the fault codes are read.
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
    
  Serial.println("Requesting fault codes.");

  //Show the module's fault codes (procedure moved to a function).
  showDTCs();

  //Disconnect from the module.
  diag.disconnect();
  
  Serial.println("Disconnected.");
}

void loop() {
  
}

void showDTCs() {
  /*
    The readFaults() function can return:
      *KLineKWP1281Lib::SUCCESS - received fault codes
      *KLineKWP1281Lib::FAIL    - the module doesn't support fault codes
      *KLineKWP1281Lib::ERROR   - communication error
  */

  //If fault codes were read successfully (stored into the buffer defined at the top of the sketch), display them.
  uint8_t amount_of_fault_codes = 0;
  if (diag.readFaults(amount_of_fault_codes, faults, sizeof(faults)) == KLineKWP1281Lib::SUCCESS) {
    Serial.print("DTCs: ");
    Serial.println(amount_of_fault_codes);

    //Check if the buffer was large enough for all fault codes.
    //If it wasn't, only attempt to display as many as fit.
    uint8_t available_DTCs = amount_of_fault_codes;
    if (sizeof(faults) < amount_of_fault_codes * 3) {
      Serial.println("Too many faults for the given buffer size");
      available_DTCs = sizeof(faults) / 3;
    }

    //Print each fault code.
    for (uint8_t i = 0; i < available_DTCs; i++) {
      uint16_t dtc = KLineKWP1281Lib::getFaultCode(i, available_DTCs, faults, sizeof(faults));
      uint8_t dtc_elaboration_code = KLineKWP1281Lib::getFaultElaborationCode(i, available_DTCs, faults, sizeof(faults));
      
      //Store the fault code in a string.
      char dtc_str[8];
      sprintf(dtc_str, "%05u", dtc);
      
      //Store the elaboration code (without the high bit) in a string.
      char dtc_status_str[8];
      sprintf(dtc_status_str, "%02u-%02u", dtc_elaboration_code & ~0x80, ((dtc_elaboration_code & 0x80) ? 10 : 0));
      
      //Print the fault code and its elaboration code.
      Serial.print("  ");
      Serial.print(dtc_str);
      Serial.print(" - ");
      Serial.println(dtc_status_str);
      
      //Declare a character array and use it to store the elaboration string.
      char elaboration_string[32];

      //Declare a bool that indicates whether or not the fault is intermittent.
      bool is_intermittent;

      //Get the elaboration string.
      KLineKWP1281Lib::getFaultElaboration(is_intermittent, i, available_DTCs, faults, sizeof(faults), elaboration_string, sizeof(elaboration_string));

      //Print the elaboration string.
      Serial.print("    ");
      Serial.print(elaboration_string);
      if (is_intermittent) {
        Serial.print(" - Intermittent");
      }
      Serial.println();
    }
  }
  //If fault codes were not read successfully, show an error.
  else {
    Serial.println("Error reading DTCs");
  }
}
