/*
  Title:
    06.Output_tests

  Description:
    Demonstrates how to perform output tests.

  Notes:
    *The connection will be stopped after the output tests are performed.
    *If you are using the ESP32 platform, feel free to uncomment the line "#define KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED" in "KLineKWP1281Lib.h",
    in order to get descriptive names associated with each output test. This feature doesn't fit on the AVR platform, but you can take a look at the
    "fault_code_description_xx.h" files if you need these descriptions.
    *If you have descriptions enabled, you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h". Please only choose
    one option.
    *If this is left disabled, the descriptions will be replaced by "EN_dsc".
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

// How many milliseconds to idle on each output test
#define WAIT_TIME_MS 5000

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
  diag.connect(connect_to_module, module_baud_rate, false);
  
  // Perform all output tests (procedure moved to a function).
  Serial.println("Performing output tests.");
  performOutputTests();

  // Disconnect from the module.
  diag.disconnect(false);
  Serial.println("Disconnected.");
}

void loop()
{
  
}

void performOutputTests()
{
  // Output tests are always performed in a predefined sequence that cannot be changed.
  // It is possible to determine which output test is currently running, as they return a value.
  // This value is technically a fault code, so its description can be retrieved.
  uint16_t current_output_test;
  
  /*
    The outputTests() function can return:
      KLineKWP1281Lib::SUCCESS - performed next output test
      KLineKWP1281Lib::FAIL    - the module doesn't support output tests / reached end of output tests
      KLineKWP1281Lib::ERROR   - communication error
    
    Technically, there are 3 more values defined for the KLineKWP1281Lib::executionStatus data type,
    but they only apply to the readGroup() function, so they will never be returned by other functions.
  */
  
  // Run the first output test and store the return value.
  KLineKWP1281Lib::executionStatus outputTests_status = diag.outputTests(current_output_test);
  
  // Continue running output tests until something other than SUCCESS is returned.
  while (outputTests_status == KLineKWP1281Lib::SUCCESS)
  {
    // Declare a character array and use it to store the description string.
    char description_string[32];
    KLineKWP1281Lib::getOutputTestDescription(current_output_test, description_string, sizeof(description_string));
    
    // Display the output test description.
    Serial.print("Performing output test: ");
    Serial.print(description_string);
    
    // Get the full length of the description string, to warn the user if the provided buffer wasn't large enough to store the entire string.
    size_t description_string_length = KLineKWP1281Lib::getOutputTestDescriptionLength(current_output_test);
    
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
    
    // Wait as many milliseconds as specified by WAIT_TIME_MS.
    // The communication is kept alive by update() while waiting.
    unsigned long start_time = millis();
    while (millis() - start_time <= WAIT_TIME_MS)
    {
      diag.update();
    }
    
    // Run the next output test.
    outputTests_status = diag.outputTests(current_output_test);
  }
  
  // At this point, the loop above was stopped because either FAIL or ERROR was returned.
  
  // If FAIL was returned, the output test sequence is complete.
  if (outputTests_status == KLineKWP1281Lib::FAIL)
  {
    Serial.println("End of output tests");
  }
  // If ERROR was returned, a communication error probably occurred.
  else
  {
    Serial.println("Error performing output tests");
  }
}
