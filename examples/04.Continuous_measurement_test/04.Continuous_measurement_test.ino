/*
  Title:
    04.Continuous_measurement_test.ino

  Description:
    Demonstrates how to read a measurement block continuously.

  Notes:
    *You can change which block to read by modifying the #define below.
    *This block will be read continuously while the sketch is running.
    *It is not necessary to maintain the connection with "diag.update();" in the loop, because any request will have the same effect (update() must
    be used in periods of inactivity).
    *This sketch will not fit on an Arduino UNO by default!
    *To fix this, open the library's /src/KLineKWP1281Lib.h file in a text editor and comment out the line "#define KWP1281_TEXT_TABLE_SUPPORTED"
    at the top, as explained in the comments.
    *After manually disabling the text table, the value for some parameters will be displayed as "EN_f25".
    *If you have the text table enabled, you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h". Please only choose
    one option.
*/

//Change which block to read.
#define BLOCK_TO_READ 1

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

//Normally, each measurement takes 3 bytes, and a block can store up to 4 measurements.
//There are some measurements which take more space.
//Consider increasing the size of this buffer if you get "Error reading measurements!":
uint8_t measurements[3 * 4];

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
  
  Serial.print("Requesting block ");
  Serial.print(BLOCK_TO_READ);
  Serial.println(" continuously.");
}

void loop() {
  showMeasurements(BLOCK_TO_READ);
}

void showMeasurements(uint8_t block) {
  //This will contain the amount of measurements in the current block, after calling the readGroup() function.
  uint8_t amount_of_measurements = 0;
  
  /*
    The readGroup() function can return:
      *KLineKWP1281Lib::SUCCESS - received measurements
      *KLineKWP1281Lib::FAIL    - the requested block does not exist
      *KLineKWP1281Lib::ERROR   - communication error
  */
  
  //Read the requested group and store the return value.
  KLineKWP1281Lib::executionStatus readGroup_status = diag.readGroup(amount_of_measurements, block, measurements, sizeof(measurements));
  
  //Check the return value.
  switch (readGroup_status) {
    case KLineKWP1281Lib::ERROR:
      Serial.println("Error reading measurements!");
      return;
    
    case KLineKWP1281Lib::FAIL:
      Serial.print("Block ");
      Serial.print(block);
      Serial.println(" does not exist!");
      return;
    
    //Execute the code after the switch().
    case KLineKWP1281Lib::SUCCESS:
      break;
  }
  
  //If the block was read successfully, display its measurements.
  Serial.print("Block ");
  Serial.print(block);
  Serial.println(':');
    
  //Display each measurement.
  for (uint8_t i = 0; i < 4; i++) {
    //Format the values with a leading tab.
    Serial.print('\t');
    
    /*
      The getMeasurementType() function can return:
        *KLineKWP1281Lib::UNKNOWN - index out of range (measurement doesn't exist in block)
        *KLineKWP1281Lib::VALUE   - regular measurement, with a value and units
        *KLineKWP1281Lib::TEXT    - text measurement
    */
    
    //Get the current measurement's type and check the return value.
    switch (KLineKWP1281Lib::getMeasurementType(i, amount_of_measurements, measurements, sizeof(measurements))) {
      //"Value and units" type
      case KLineKWP1281Lib::VALUE:
      {
        //This will hold the measurement's units.
        char units_string[16];
        
        //Display the calculated value, with the recommended amount of decimals.
        Serial.print(KLineKWP1281Lib::getMeasurementValue(i, amount_of_measurements, measurements, sizeof(measurements)),
          KLineKWP1281Lib::getMeasurementDecimals(i, amount_of_measurements, measurements, sizeof(measurements)));
        
        Serial.print(' ');
        
        //The function getMeasurementUnits() returns the same string that it's given. It's the same as units_string.
        Serial.println(KLineKWP1281Lib::getMeasurementUnits(i, amount_of_measurements, measurements, sizeof(measurements), units_string, sizeof(units_string)));
      }
      break;
      
      //"Text" type
      case KLineKWP1281Lib::TEXT:
      {
        //This will hold the measurement's text.
        char text_string[16];
        
        //The function getMeasurementText() returns the same string that it's given. It's the same as text_string.
        Serial.println(KLineKWP1281Lib::getMeasurementText(i, amount_of_measurements, measurements, sizeof(measurements), text_string, sizeof(text_string)));
      }
      break;
      
      //Invalid measurement index
      case KLineKWP1281Lib::UNKNOWN:
        Serial.println("N/A");
        break;
    }
  }

  //Leave an empty line.
  Serial.println();
}
