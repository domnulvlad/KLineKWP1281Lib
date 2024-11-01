/*
  Title:
    03.Full_measurement_test.ino

  Description:
    Demonstrates how to read a module's measuring groups.

  Notes:
    *Measuring groups 0-255 will be read, after which the connection will be stopped.
    *Each group will be read 5 times.
    *This sketch will not fit on an Arduino UNO by default!
    *To fix this, open the library's /src/KLineKWP1281Lib.h file in a text editor and comment out the line "#define KWP1281_TEXT_TABLE_SUPPORTED"
    at the top, as explained in the comments.
    *After manually disabling the text table, the value for some parameters will be displayed as "EN_f25".
    *If you have the text table enabled, you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h". Please only choose
    one option.
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

/*
  -The KWP1281 protocol may respond to a "get measurement group" request in 6 ways:
  1. NAK - the group does not exist
    (readGroup() returns KLineKWP1281Lib::FAIL)
  2. ACK - the group exists, but it's empty
    (readGroup() returns KLineKWP1281Lib::FAIL)
  3. Basic settings - the group contains 10 raw values
    (readGroup() returns KLineKWP1281Lib::GROUP_BASIC_SETTINGS)
  4. Standard - this is how almost all modules report measurement groups
    (readGroup() returns KLineKWP1281Lib::SUCCESS)
  5. Header - the group is of, what I call, "header+body" type, this header only contains a sort of template
    (readGroup() returns KLineKWP1281Lib::GROUP_HEADER)
  6. Body - this is the part of a "header+body" group that contains the actual data read from sensors
    (readGroup() returns KLineKWP1281Lib::GROUP_BODY)

  ---
  -You can treat cases 1 and 2 as meaning that the requested group doesn't exist.
  ---

  ---
  -For case 3 [Basic settings], the buffer which you used for readGroup() contains 10 bytes starting from byte 0.
  -The amount_of_measurements variable which you passed to readGroup() contains 0.
  -There aren't any functions that use this buffer, you should just access the array like normal.
  ---

  ---
  -For case 4 [Standard], the buffer which you used for readGroup() contains the group.
  -The amount_of_measurements variable which you passed to readGroup() contains the amount of measurements received, usually between 0 and 4.

  -You can interpret the data using the following functions:
    +getMeasurementType() - tells if the measurement is a VALUE or TEXT
    +getMeasurementValue() - get the value for a measurement of type VALUE
      (for some measurements of type TEXT, it might return a sort of code, but usually it returns nan)
    +getMeasurementUnits() - get the units of the measurement of type VALUE
      (for measurements of type TEXT, or upon other errors, it clears the given character array and returns an invalid string (nullptr))
    +getMeasurementText() - get the value for a measurement of type TEXT
      (for measurements of type VALUE, or upon other errors, it clears the given character array and returns an invalid string (nullptr))
    +getMeasurementTextLength() - get the character length of the value for a measurement of type TEXT
      (for measurements of type VALUE, or upon other errors, it returns 0)
    +getMeasurementDecimals() - get the recommended amount of decimal places for a measurement

  -For more advanced use, the functions mentioned above can also take values extracted from the measurement buffer, instead of being given the entire thing.
  -There are also functions that extract these important bytes from the measurement buffer:
    +getFormula() - gets the FORMULA byte
    +getNWb() - gets the BYTE_A
    +getMWb() - gets the BYTE_B
    +getMeasurementData() - returns an array containing a measurement's bytes
      (most often, this array just contains BYTE_A and BYTE_B, but there are some measurements with more bytes)
      (technically, what it returns is the same measurement buffer, but indexed so it starts right at the measurement's data)
    +getMeasurementDataLength() - returns the length of the array containing a measurement's bytes
  - It is very rarely necessary to use these advanced functions, so don't overcomplicate your code with them.
  ---

  ---
  -For case 5 [Header], the buffer which you used for readGroup() contains the group's header.
  -You are supposed to save this response somewhere, or at least protect it from being overwritten.
  -When you get the [Body] response, you will use it together with the [Header] response for calculating measurements.

  -The amount_of_measurements variable which you passed to readGroup() contains the amount of measurements in the header.
  -Although not exactly necessary, the functions used for calculating values from "header+body" responses also needs this from the header.
  -Just save it somewhere, and use it when you get the [Body].

  -Take special care, because, for modules which respond in this way, you have to call update() right before you request a new group!
  -Otherwise, you will only get a [Body], and it's useless without a [Header].
  ---

  ---
  -For case 6 [Body], the buffer which you used for readGroup() contains the group's body.
  -You should already have this group's header.
  -The amount_of_measurements variable which you passed to readGroup() contains the amount of measurements received, usually between 0 and 4.

  -You can interpret the data using the following functions:
    +getMeasurementTypeFromHeader() - needs only header
    +getMeasurementValueFromHeaderBody() - needs header and body
    +getMeasurementUnitsFromHeaderBody() - needs header and body
    +getMeasurementTextFromHeaderBody() - needs header and body
    +getMeasurementTextLengthFromHeaderBody() - needs header and body
    +getMeasurementDecimalsFromHeader() - needs only header
  -You can see that some functions only need the header, most also need the body.
  -Apart from that, they work exactly like the functions for [Standard] responses.

  -There are also functions that extract important bytes from the header buffer, but they are not very useful, because you can't use them properly.
  -They are mainly used internally.
    +getFormulaFromHeader() - gets the FORMULA byte
    +getNWbFromHeader() - gets the BYTE_A, which stays constant
    +getDataTableFromHeader() - returns an array containing a measurement's attached table
    +getDataTableLengthFromHeader() - returns the length of the array containing a measurement's attached table
  - It is pretty much never necessary to use these advanced functions, so don't overcomplicate your code with them.
  ---
*/

/*
  -Normally, each measurement takes 3 bytes, and a group can store up to 4 measurements.
  -This means that a good size for the measurement buffer is 12, and it will work for most modules.
  -There is a formula that takes 6 bytes, and some formulas that contain an arbitrary number of bytes.
  -Even if a group returns a [Basic settings], it will contain 10 bytes, which will fit.

  -There are also some modules which respond with "header+body", as mentioned in a comment above.
  -A realistic worst-case-scenario is 80 bytes, if all 4 measurements have a 17-byte table attached.
  -There is a formula for which this table can also be longer than 17 bytes, further complicating things.

  -If you see "Error reading group!" during runtime, please enable debugging.
  -If you see "The given buffer is too small for the incoming message" debug messages, you need to increase the size of the buffers below.
*/

// [Regular] measurements, [Basic settings] measurements and [Header] responses will be stored in this array.
uint8_t measurement_buffer[80];

// [Body] responses will be stored in this array.
// It must be separate from the other array, because the [Header] needs to remain intact in order to be able to calculate values.
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

  // Read all groups (000-255).
  Serial.println("Requesting measuring groups 000-255.");
  for (uint16_t i = 0; i <= 255; i++)
  {
    showMeasurements(i);
  }

  // Disconnect from the module.
  diag.disconnect(false);
  Serial.println("Disconnected.");
}

void loop()
{

}

void showMeasurements(uint8_t group)
{
  // This will contain the amount of measurements in the current group, after calling the readGroup() function.
  uint8_t amount_of_measurements = 0;

  // This flag keeps track if a [Header] was received for the current group, meaning it's of the "header+body" type.
  bool received_group_header = false;

  // In case of the "header+body" groups, the [Body] response is the one that actually contains live data.
  // In theory, the module should report the same number of measurements in the [Header] and in the [Body].
  // Still, the number of measurements received in the [Header] will be stored separately, for correctness.
  uint8_t amount_of_measurements_in_header = 0;

  // For modules which report measuring groups in the "header+body" mode, it is important to do update() when requesting a new group, so the module sends the [Header].
  diag.update();

  // The requested group will be read 5 times, if it exists and no error occurs.
  // If a "header+body" group is encountered, we will do one more step, so that the actual data requests are 5.
  for (uint8_t attempt = 1; attempt <= 5; attempt++)
  {
    /*
      The readGroup() function can return:
        KLineKWP1281Lib::FAIL                - the requested group does not exist
        KLineKWP1281Lib::ERROR               - communication error
        KLineKWP1281Lib::SUCCESS             - received measurements
        KLineKWP1281Lib::GROUP_BASIC_SETTING - received a [Basic settings] measurement; the buffer contains 10 raw values
        KLineKWP1281Lib::GROUP_HEADER        - received the [Header] for a "header+body" group; need to read again to get the [Body]
        KLineKWP1281Lib::GROUP_BODY          - received the [Body] for a "header+body" group
    */

    // Read the requested group and store the returned value.
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
        // We have nothing else to display (yet); skip this step of the loop.
        continue;

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

          // Add an extra step to the loop, because this one shouldn't count, as it doesn't contain live data.
          attempt--;
        }
        // We have nothing to display yet, the next readGroup() will get the actual data; skip this step of the loop.
        continue;

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
    for (uint8_t i = 0; i < amount_of_measurements; i++)
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

            // This variable will contain the calculated value of the measurement.
            double value;
            
            // This variable will contain the recommended amount of decimal places for the measurement.
            uint8_t decimals;
            
            // Regular mode:
            if (!received_group_header)
            {
              // Calculate the value.
              value = KLineKWP1281Lib::getMeasurementValue (i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer));

              // Get the units.
              // It's not necessary to use the returned value of this function (character pointer), the units will appear in the given character array.
              KLineKWP1281Lib::getMeasurementUnits (i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer), units_string, sizeof(units_string));

              // Get the recommended amount of decimal places.
              decimals = KLineKWP1281Lib::getMeasurementDecimals (i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer));
            }
            // "Header+body" mode:
            else
            {
              // Calculate the value; both the header and body are needed.
              value = KLineKWP1281Lib::getMeasurementValueFromHeaderBody (i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer));

              // Get the units; both the header and body are needed.
              // It's not necessary to use the returned value of this function (character pointer), the units will appear in the given character array.
              KLineKWP1281Lib::getMeasurementUnitsFromHeaderBody (i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer), units_string, sizeof(units_string));

              // Get the recommended amount of decimal places; only the header is needed.
              decimals = KLineKWP1281Lib::getMeasurementDecimalsFromHeader (i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer));
            }

            // Display the calculated value, with the recommended amount of decimals.
            Serial.print(value, decimals);
            if (strlen(units_string) != 0)
            {
              Serial.print(' ');
              Serial.println(units_string);
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
              // Get the text.
              // It's not necessary to use the returned value of this function (character pointer), the text will appear in the given character array.
              KLineKWP1281Lib::getMeasurementText(i, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer), text_string, sizeof(text_string));
            }
            else
            {
              // Get the text; both the header and body are needed.
              // It's not necessary to use the returned value of this function (character pointer), the text will appear in the given character array.
              KLineKWP1281Lib::getMeasurementTextFromHeaderBody(i, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer), text_string, sizeof(text_string));
            }

            // Display the text.
            if (strlen(text_string) != 0)
            {
              Serial.println(text_string);
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
}
