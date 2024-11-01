/*
  Title:
    04.Continuous_measurement_test.ino

  Description:
    Demonstrates how to read a measurement group continuously.

  Notes:
    *You can change which group to read by modifying the #define below.
    *This group will be read continuously while the sketch is running.
    *It is not necessary to maintain the connection with "diag.update();" in the loop, because any request will have the same effect (update() must
    be used in periods of inactivity).
    *This sketch will not fit on an Arduino UNO by default!
    *To fix this, open the library's /src/KLineKWP1281Lib.h file in a text editor and comment out the line "#define KWP1281_TEXT_TABLE_SUPPORTED"
    at the top, as explained in the comments.
    *After manually disabling the text table, the value for some parameters will be displayed as "EN_f25".
    *If you have the text table enabled, you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h". Please only choose
    one option.
*/

// Change which group to read.
#define GROUP_TO_READ 1

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

// Please find more information on the topic of KWP1281 measurement groups in the "03.Full_measurement_test" example.
uint8_t measurement_buffer[80];
uint8_t measurement_body_buffer[4];

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
  
  Serial.print("Requesting group ");
  Serial.print(GROUP_TO_READ);
  Serial.println(" continuously.");
}

void loop()
{
  showMeasurements(GROUP_TO_READ);
}

/*

  -The variables used for "header+body" groups are stored globally.
  -Normally, the flag `received_group_header` should be reset to false when requesting a new group.
  -This example only requests one group, so it isn't necessary; if the group was "header+body" once, it will always be like that.
  
  -You could also keep `received_group_header` local to the showMeasurements() function, and do diag.update() before each request,
  to ensure you receive the [Header].
  -Of course, this would negatively impact performance, since you need to receive more data from the module each time you read the group.

*/

// This flag keeps track if a [Header] was received for the current group, meaning it's of the "header+body" type.
bool received_group_header = false;

// This will contain the amount of measurements received in the [Header] of a "header+body" group.
uint8_t amount_of_measurements_in_header = 0;

void showMeasurements(uint8_t group)
{
  // This will contain the amount of measurements in the current group, after calling the readGroup() function.
  uint8_t amount_of_measurements = 0;
  
  /*
    -For modules which report measuring groups in the "header+body" mode, it is important to do update() when requesting a new group,
    so the module sends the [Header].
    -If it's the first request, it's not necessary; the module will always send the [Header] for the first time.
    -Since this example only requests one group, it would decrease performance (measurement rate) to do update() every time, since it would cause the module
    to send the [Header] over and over again, and it takes time to receive it, instead of only receiving it once, at the beginning.
  */
  // diag.update();
  
  /*
    The readGroup() function can return:
      KLineKWP1281Lib::FAIL                - the requested group does not exist
      KLineKWP1281Lib::ERROR               - communication error
      KLineKWP1281Lib::SUCCESS             - received measurements
      KLineKWP1281Lib::GROUP_BASIC_SETTING - received a [Basic settings] measurement; the buffer contains 10 raw values
      KLineKWP1281Lib::GROUP_HEADER        - received the [Header] for a "header+body" group; need to read again to get the [Body]
      KLineKWP1281Lib::GROUP_BODY          - received the [Body] for a "header+body" group
  */
  
  // Read the requested group and store the returneded value.
  KLineKWP1281Lib::executionStatus readGroup_status;
  // If the group is not of "header+body" type, or if it is and this is the first request, we don't have a [Header] (yet), so `received_group_header=false`.
  // The response to this request will be stored in the larger array.
  // If it is in fact of "header+body" type, the [Header] will be stored in this array.
  if (!received_group_header)
  {
    readGroup_status = diag.readGroup(amount_of_measurements, group, measurement_buffer, sizeof(measurement_buffer));
  }
  // If the group is of "header+body" type, and this is not the first request, it means we have a header, so `received_group_header=true`.
  // The response to this request will be stored in the smaller array, because it should be the [Body].
  else
  {
    readGroup_status = diag.readGroup(amount_of_measurements, group, measurement_body_buffer, sizeof(measurement_body_buffer));
  }
  
  // Check the returned value.
  switch (readGroup_status)
  {
    case KLineKWP1281Lib::ERROR:
      {
        Serial.println("Error reading group!");
      }
      // There is no reason to continue, exit the function.
      return;
    
    case KLineKWP1281Lib::FAIL:
      {
        Serial.print("Group ");
        Serial.print(group);
        Serial.println(" does not exist!");
      }
      // There is no reason to continue, exit the function.
      return;

    case KLineKWP1281Lib::GROUP_BASIC_SETTINGS:
      {
        // If we have a [Header], it means this group sends responses of "header+body" type.
        // So, at this point, it doesn't make sense to receive something other than a [Body].
        if (received_group_header)
        {
          Serial.println("Error reading body! (got basic settings)");
          return;
        }

        // We have 10 raw values in the `measurement_buffer` array.
        Serial.print("Basic settings in group ");
        Serial.print(group);
        Serial.print(": ");
        for (uint8_t i = 0; i < 10; i++)
        {
          Serial.print(measurement_buffer[i]);
          Serial.print(" ");
        }
        Serial.println();
      }
      // We have nothing else to display, exit the function.
      return;

    case KLineKWP1281Lib::GROUP_HEADER:
      {
        // If we have a [Header], it means this group sends responses of "header+body" type.
        // So, at this point, it doesn't make sense to receive something other than a [Body].
        if (received_group_header)
        {
          Serial.println("Error reading body! (got header)");
          return;
        }

        // Set the flag to indicate that a header was received, making this a "header+body" group response.
        received_group_header = true;

        // Store the number of measurements received in the header.
        amount_of_measurements_in_header = amount_of_measurements;
      }
      // We have nothing to display yet, the next readGroup() will get the actual data; exit the function.
      return;

    case KLineKWP1281Lib::GROUP_BODY:
      {
        // If we don't have a [Header], it doesn't make sense to receive a [Body].
        if (!received_group_header)
        {
          Serial.println("Error reading header! (got body)");
          return;
        }
      }
      // If we have the [Header], now we also have the [Body]; execute the code after the switch().
      break;
    
    // Execute the code after the switch().
    case KLineKWP1281Lib::SUCCESS:
      break;
  }
  
  // If the group was read successfully, display its measurements.
  Serial.print("Group ");
  Serial.print(group);
  Serial.println(':');
    
  // Display each measurement.
  for (uint8_t i = 0; i < 4; i++)
  {
    // Format the values with a leading tab.
    Serial.print("    ");
    
    /*
      The getMeasurementType() function can return:
        KLineKWP1281Lib::UNKNOWN - index out of range (the measurement doesn't exist in the group) or the formula is invalid/not-applicable
        KLineKWP1281Lib::VALUE   - regular measurement, with a value and units
        KLineKWP1281Lib::TEXT    - text measurement
    */

    // Get the current measurement's type.
    KLineKWP1281Lib::measurementType measurement_type;
    if (!received_group_header)
    {
      measurement_type = KLineKWP1281Lib::getMeasurementType(i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer));
    }
    // For "header+body" measurements, you need to use this other function that specifically parses headers instead of regular responses.
    else
    {
      measurement_type = KLineKWP1281Lib::getMeasurementTypeFromHeader(i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer));
    }
    
    // Check the returned value.
    switch (measurement_type)
    {
      // "Value and units" type
      case KLineKWP1281Lib::VALUE:
      {
        // The measurement's units will be copied into this array, so they can be displayed.
        char units_string[16];
        
        // Regular mode:
        if (!received_group_header)
        {
          // Display the calculated value, with the recommended amount of decimals.
          Serial.print(KLineKWP1281Lib::getMeasurementValue(i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer)),
                      KLineKWP1281Lib::getMeasurementDecimals(i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer)));
          
          // The function getMeasurementUnits() returns the same string that it's given. It's the same as units_string.
          Serial.print(' ');
          Serial.println(KLineKWP1281Lib::getMeasurementUnits(i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer), units_string, sizeof(units_string)));
        }
        // "Header+body" mode:
        else
        {
          // Display the calculated value, with the recommended amount of decimals.
          Serial.print(KLineKWP1281Lib::getMeasurementValueFromHeaderBody(i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer)),
                      KLineKWP1281Lib::getMeasurementDecimalsFromHeader(i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer)));
          
          // The function getMeasurementUnitsFromHeaderBody() returns the same string that it's given. It's the same as units_string.
          Serial.print(' ');
          Serial.println(KLineKWP1281Lib::getMeasurementUnitsFromHeaderBody(i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer), units_string, sizeof(units_string)));
        }
      }
      break;
      
      // "Text" type
      case KLineKWP1281Lib::TEXT:
      {
        // The measurement's text will be copied into this array, so it can be displayed.
        char text_string[16];

        if (!received_group_header)
        {
          // The function getMeasurementText() returns the same string that it's given. It's the same as text_string.
          Serial.println(KLineKWP1281Lib::getMeasurementText(i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer), text_string, sizeof(text_string)));
        }
        else
        {
          // The function getMeasurementTextFromHeaderBody() returns the same string that it's given. It's the same as text_string.
          Serial.println(KLineKWP1281Lib::getMeasurementTextFromHeaderBody(i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer), text_string, sizeof(text_string)));
        }
      }
      break;
      
      // Invalid measurement index
      case KLineKWP1281Lib::UNKNOWN:
        Serial.println("N/A");
        break;
    }
  }

  // Leave an empty line.
  Serial.println();
}
