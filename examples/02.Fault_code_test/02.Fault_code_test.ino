/*
  Title:
    02.Fault_code_test.ino

  Description:
    Demonstrates how to read a module's fault codes and their elaborations.

  Notes:
    *The connection will be stopped after the fault codes are read.
    *If you have fault code descriptions enabled (KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED/KWP1281_OBD_FAULT_CODE_DESCRIPTION_SUPPORTED),
    you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h".
      *Please only choose one option.
      *If they are disabled, the descriptions will be replaced by "EN_dsc"/"EN_obd".
      *This feature doesn't fit on the AVR platform, but you can take a look at the "fault_code_description_xx.h" and "OBD_fault_code_description_xx.h"
      files if you need these descriptions.
    *If you have fault code elaborations enabled (KWP1281_FAULT_CODE_ELABORATION_SUPPORTED), you can choose from a few different languages
    a bit further below in "KLineKWP1281Lib.h".
      *Please only choose one option.
      *If they are disabled, the elaborations will be replaced by "EN_elb".
*/

/*
  *This sketch displays information in the Serial Monitor, so another serial port is required for the K-line.
  *If another port is not available on the selected board, the regular port "Serial" can be used, although no information can be printed to it (good for
  projects with external displays or for data logging).

  ***Uncomment the appropriate options in the "configuration.h" file!

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

// Include the library.
#include <KLineKWP1281Lib.h>

// Include the two files containing configuration options and the functions used for communication.
#include "configuration.h"
#include "communication.h"

// Debugging can be enabled in "configuration.h" in order to print connection-related info on the Serial Monitor.
#if debug_info
  KLineKWP1281Lib diag(beginFunction, endFunction, sendFunction, receiveFunction, TX_pin, is_full_duplex, &Serial);
#else
  KLineKWP1281Lib diag(beginFunction, endFunction, sendFunction, receiveFunction, TX_pin, is_full_duplex);
#endif

// Each fault code takes up 3 bytes.
// You can increase the value below if you get "Too many faults for the given buffer size" during the test.
#define DTC_BUFFER_MAX_FAULT_CODES 16
uint8_t faults[3 * DTC_BUFFER_MAX_FAULT_CODES];

void setup()
{
  // Initialize the Serial Monitor.
  Serial.begin(115200);
  delay(500);
  Serial.println("Sketch started.");
  
  // If debugging bus traffic was enabled, attach the debugging function.
#if debug_traffic
  diag.KWP1281debugFunction(KWP1281debugFunction);
#endif
  
  // Connect to the module.
  /*
    -There is an optional argument for connect()/attemptConnect(), which, if set to false, will make it so extra identification will not be requested
    if the module supports it.
    -This would save almost one second of the connection time, with the downside being that getExtraIdentification() will return an empty string.
    -It has no effect if the module doesn't support extra identification.
  */
  diag.connect(connect_to_module, module_baud_rate, false);
  
  // Show the module's fault codes (procedure moved to a function).
  Serial.println("Requesting fault codes.");
  showDTCs();

  // Disconnect from the module.
  /*
    -There is an optional argument for disconnect(), which, if set to false, will make it so the library doesn't wait for a response.
    -When disconnecting, some modules send a response, some don't.
    -If the module doesn't send a response, not waiting saves [diag.responseTimeout] milliseconds, by default 2 seconds.
    -If the module sends a response, not waiting for it shouldn't really have consequences.
  */
  diag.disconnect(false);
  Serial.println("Disconnected.");
}

void loop()
{
  
}

void showDTCs()
{
  // This will contain the amount of fault codes, after calling the readFaults() function.
  uint8_t amount_of_fault_codes = 0;
  
  /*
    Always check the return value of functions!
    
    The readFaults() function can return:
      KLineKWP1281Lib::SUCCESS - received fault codes
      KLineKWP1281Lib::FAIL    - the module doesn't support fault codes
      KLineKWP1281Lib::ERROR   - communication error
    
    Technically, there are 3 more values defined for the KLineKWP1281Lib::executionStatus data type,
    but they only apply to the readGroup() function, so they will never be returned by other functions.
  */

  // If fault codes were read successfully, display them.
  if (diag.readFaults(amount_of_fault_codes, faults, sizeof(faults)) == KLineKWP1281Lib::SUCCESS)
  {
    // Display how many fault codes were received.
    Serial.print("DTCs: ");
    Serial.println(amount_of_fault_codes);

    // Each fault code takes up 3 bytes.
    // Check if the buffer was large enough for all fault codes.
    // If it wasn't, only attempt to display as many as fit.
    uint8_t available_fault_codes = amount_of_fault_codes;
    if (sizeof(faults) < amount_of_fault_codes * 3)
    {
      Serial.println("Too many faults for the given buffer size");
      available_fault_codes = sizeof(faults) / 3;
    }

    // Print each fault code.
    for (uint8_t i = 0; i < available_fault_codes; i++)
    {
      // Retrieve the "i"-th fault code and the elaboration code from the buffer.
      uint16_t fault_code = KLineKWP1281Lib::getFaultCode(i, available_fault_codes, faults, sizeof(faults));
      uint8_t fault_elaboration_code = KLineKWP1281Lib::getFaultElaborationCode(i, available_fault_codes, faults, sizeof(faults));
      
      // Store the fault code in a string (5 digits, padded with zero on the left).
      char fault_code_string[8];
      sprintf(fault_code_string, "%05u", fault_code);

      // Print the fault code.
      Serial.print("  ");
      Serial.print(fault_code_string);

      // Declare a character array and use it to store the description string of the fault code.
      char description_string[32];
      KLineKWP1281Lib::getFaultDescription(fault_code, description_string, sizeof(description_string));

      // Print the description string.
      Serial.print(" - ");
      Serial.print(description_string);

      // Get the full length of the description string, to warn the user if the provided buffer wasn't large enough to store the entire string.
      size_t description_string_length = KLineKWP1281Lib::getFaultDescriptionLength(fault_code);

      // If the buffer was too small, display an ellipsis and indicate how many characters would have been needed for the entire string.
      if (description_string_length > (sizeof(description_string) - 1))
      {
        Serial.print("... (");
        Serial.print(sizeof(description_string) - 1);
        Serial.print("/");
        Serial.print(description_string_length);
        Serial.print(")");
      }
      Serial.println();
      
      // Store the elaboration code of the fault (without the high bit) in a string.
      char fault_elaboration_code_string[8];
      sprintf(fault_elaboration_code_string, "%02u-%02u", fault_elaboration_code & ~0x80, ((fault_elaboration_code & 0x80) ? 10 : 0));
      
      // This will indicate whether or not the fault is intermittent, after calling the getFaultElaboration() function.
      bool is_intermittent;

      // Declare a character array and use it to store the elaboration string.
      char elaboration_string[32];
      KLineKWP1281Lib::getFaultElaboration(is_intermittent, fault_elaboration_code, elaboration_string, sizeof(elaboration_string));
      
      // A fault code may or may not be of "OBD" type.
      // OBD fault codes should be displayed formatted according to the standard, for example "P0001" instead of "16385".
      if (KLineKWP1281Lib::isOBDFaultCode(fault_code))
      {
        // Declare a character array and use it to store the formatted fault code.
        char OBD_fault_code_string[6];
        KLineKWP1281Lib::getOBDFaultCode(fault_code, OBD_fault_code_string, sizeof(OBD_fault_code_string));
        
        // Print the formatted fault code and elaboration code.
        Serial.print("    ");
        Serial.print(OBD_fault_code_string);
        Serial.print(" - ");
        Serial.print(fault_elaboration_code_string);
        
        // The elaboration produced by getFaultElaboration() may not be accurate for OBD fault codes.
        // The proper elaboration is contained in the description string, after the last ':' character.
      }
      else
      {
        // Print the fault elaboration code and elaboration text.
        Serial.print("    ");
        Serial.print(fault_elaboration_code_string);
        Serial.print(" - ");
        Serial.print(elaboration_string);
        
        // Get the full length of the elaboration string, to warn the user if the provided buffer wasn't large enough to store the entire string.
        size_t elaboration_string_length = KLineKWP1281Lib::getFaultElaborationLength(i, available_fault_codes, faults, sizeof(faults));
  
        // If the buffer was too small, display an ellipsis and indicate how many characters would have been needed for the entire string.
        if (elaboration_string_length > (sizeof(elaboration_string) - 1))
        {
          Serial.print("... (");
          Serial.print(sizeof(elaboration_string) - 1);
          Serial.print("/");
          Serial.print(elaboration_string_length);
          Serial.print(")");
        }
      }

      // If the fault is intermittent, display this info.
      if (is_intermittent)
      {
        Serial.print(" - Intermittent");
      }
      Serial.println();
    }
  }
  // If fault codes were not read successfully, show an error.
  else
  {
    Serial.println("Error reading DTCs");
  }
}
