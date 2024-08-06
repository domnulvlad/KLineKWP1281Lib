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

#ifdef module_baud_rate
  // If a specific baud rate was defined, connect with it.
  Serial.print("Connecting with baud rate ");
  Serial.println(module_baud_rate);
  diag.connect(connect_to_module, module_baud_rate);
#else
  // Otherwise, try each baud rate from the possible_baud_rates array.
  size_t current_index = 0;
  while (true)
  {
    // If connecting fails, try the next baud rate.
    Serial.print("Trying to connect with baud rate ");
    Serial.println(possible_baud_rates[current_index]);
    if (diag.attemptConnect(connect_to_module, possible_baud_rates[current_index]) != KLineKWP1281Lib::SUCCESS)
    {
      // If there are more baud rates to try, increment the index.
      if (current_index < (sizeof(possible_baud_rates) / sizeof(possible_baud_rates[0])) - 1)
      {
        current_index++;
      }
      // Otherwise, start over.
      else
      {
        current_index = 0;
      }
      
      // Delay 1 second before trying the next baud rate.
      delay(1000);
    }
    // If the connection is successful, continue.
    else
    {
      break;
    }
  }
#endif

  // Display the module's part number and identification.
  Serial.println();
  Serial.println(diag.getPartNumber());
  Serial.println(diag.getIdentification());

  // If it is available, display the module's extra identification.
  if (strlen(diag.getExtraIdentification()))
  {
    Serial.println(diag.getExtraIdentification());
  }
  
  // Put the module's coding value into a string and pad with 0s to show 5 characters.
  char coding_str[6];
  unsigned int coding = diag.getCoding();
  sprintf(coding_str, "%05u", coding);
  
  // Put the module's wokshop code into a string and pad with 0s to show 5 characters.
  char WSC_str[6];
  unsigned long WSC = diag.getWorkshopCode();
  sprintf(WSC_str, "%05lu", WSC);
  
  // Display the module's coding value and workshop code.
  Serial.println();
  Serial.print("Coding: ");
  Serial.println(coding_str);
  Serial.print("WSC: ");
  Serial.println(WSC_str);
  Serial.println();
  
  Serial.println("Maintaining the connection active.");
}

void loop()
{
  // Maintain the connection.
  diag.update();
}
