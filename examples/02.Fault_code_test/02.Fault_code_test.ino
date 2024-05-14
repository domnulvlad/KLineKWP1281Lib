/*
  Title:
    02.Fault_code_test.ino

  Description:
    Demonstrates how to read a module's fault codes and their elaborations.

  Notes:
    *The connection will be stopped after the fault codes are read.
    *If you are using the ESP32 platform, feel free to uncomment the line "#define KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED" in "KLineKWP1281Lib.h",
    in order to get descriptive names associated with each fault code. This feature doesn't yet work on the AVR platform, but you can take a look at
    the "fault_code_description_xx.h" files if you need these descriptions.
    *If you have descriptions enabled, you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h". Please only choose
    one option.
    *If you have manually disabled elaborations by commenting out the line "#define KWP1281_FAULT_CODE_ELABORATION_SUPPORTED" in "KLineKWP1281Lib.h",
    the elaborations will be replaced by "EN_elb".
    *If you have elaborations enabled, you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h". Please only choose
    one option.
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

  ESP32 / ESP32-C6
    *has one additional serial port
    *pins (they can be remapped, this is what they are configured to in these examples):
      *K-line TX -> RX pin 16
      *K-line RX -> TX pin 17

  ESP8266
    *has no additional serial ports
    *library of choice: SoftwareSerial (the ESP version of SoftwareSerial can also receive while sending)
    *pins: any unused
      *chosen for this demo:
        *K-line TX -> RX pin 4 (D2)
        *K-line RX -> TX pin 5 (D1)

  Raspberry Pi Pico
    *has two additional serial ports
    *pins: interchangeable
      *default:
        *K-line TX -> Serial1 - RX pin 1 / Serial2 - RX pin 9
        *K-line RX -> Serial1 - TX pin 0 / Serial2 - TX pin 8

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

      //Print the fault code.
      Serial.print("  ");
      Serial.print(dtc_str);

      // If fault code descriptions are enabled, display the description along with the fault code.
#ifdef KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED
      //Declare a character array and use it to store the description string.
      char description_string[32];
      KLineKWP1281Lib::getFaultDescription(i, available_DTCs, faults, sizeof(faults), description_string, sizeof(description_string));

      //Print the description string.
      Serial.print(" - ");
      Serial.print(description_string);

      //Get the full length of the description string, to warn the user if the provided buffer wasn't large enough to store the entire string.
      size_t description_string_length = KLineKWP1281Lib::getFaultDescriptionLength(i, available_DTCs, faults, sizeof(faults));

      //If the buffer was too small, display an ellipsis and indicate how many characters would have been needed for the entire string.
      if (description_string_length > (sizeof(description_string) - 1))
      {
        Serial.print("... (");
        Serial.print(sizeof(description_string) - 1);
        Serial.print("/");
        Serial.print(description_string_length);
        Serial.print(")");
      }
#endif
      Serial.println();
      
      //Store the elaboration code (without the high bit) in a string.
      char dtc_status_str[8];
      sprintf(dtc_status_str, "%02u-%02u", dtc_elaboration_code & ~0x80, ((dtc_elaboration_code & 0x80) ? 10 : 0));
      
      //Declare a bool that indicates whether or not the fault is intermittent, which will be changed accordingly by the getFaultElaboration() function.
      bool is_intermittent;

      //Declare a character array and use it to store the elaboration string.
      char elaboration_string[32];
      KLineKWP1281Lib::getFaultElaboration(is_intermittent, i, available_DTCs, faults, sizeof(faults), elaboration_string, sizeof(elaboration_string));

      //Print the fault elaboration code.
      Serial.print("    ");
      Serial.print(dtc_status_str);

      //Print the elaboration string.
      Serial.print(" ");
      Serial.print(elaboration_string);
      
      //Get the full length of the elaboration string, to warn the user if the provided buffer wasn't large enough to store the entire string.
      size_t elaboration_string_length = KLineKWP1281Lib::getFaultElaborationLength(i, available_DTCs, faults, sizeof(faults));

      //If the buffer was too small, display an ellipsis and indicate how many characters would have been needed for the entire string.
      if (elaboration_string_length > (sizeof(elaboration_string) - 1))
      {
        Serial.print("... (");
        Serial.print(sizeof(elaboration_string) - 1);
        Serial.print("/");
        Serial.print(elaboration_string_length);
        Serial.print(")");
      }

      // If the fault is intermittent, display this info.
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