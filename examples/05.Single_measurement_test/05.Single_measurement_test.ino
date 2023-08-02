/*
  Title:
    05.Single_measurement_test.ino

  Description:
    Demonstrates how to read a single measurement from a module's measuring blocks continuously.

  Notes:
    *You can change which block and which measurement to read by modifying the #defines below.
    *This measurement will be read continuously while the sketch is running.
    *It is not necessary to maintain the connection with "diag.update();" in the loop, because any request will have the same effect (update() must be used in
    periods of inactivity).
*/

//Change which measurement to read, from which block.
#define BLOCK_TO_READ 1
#define MEASUREMENT_TO_READ 0 //valid range: 0-3

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
  
  *This sketch will not fit on an Arduino UNO by default! To fix this, open the library's /src/KLineKWP1281Lib.h file in a text editor and comment out the line
  "#define KWP1281_TEXT_TABLE_SUPPORTED" at the top, as explained in the comments.
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

uint8_t measurements[3 * 4]; //buffer to store the measurements; each measurement takes 3 bytes; one block contains 4 measurements

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
  Serial.print(", measurement index ");
  Serial.print(MEASUREMENT_TO_READ);
  Serial.println(" continuously.");
}

void loop() {
  showSingleMeasurement(BLOCK_TO_READ, MEASUREMENT_TO_READ);
}

void showSingleMeasurement(uint8_t block, uint8_t measurement_index) {
  /*
    The readGroup() function can return:
      *KLineKWP1281Lib::SUCCESS - received measurements
      *KLineKWP1281Lib::FAIL    - the requested block does not exist
      *KLineKWP1281Lib::ERROR   - communication error
  */
  uint8_t amount_of_measurements = 0;
  switch (diag.readGroup(amount_of_measurements, block, measurements, sizeof(measurements))) {
    case KLineKWP1281Lib::ERROR:
      Serial.println("Error reading measurements!");
      break;
    
    case KLineKWP1281Lib::FAIL:
      Serial.print("Block ");
      Serial.print(block);
      Serial.println(" does not exist!");
      break;
    
    case KLineKWP1281Lib::SUCCESS:
      //Will hold the measurement's units
      char units_string[16];
      
      //Display the selected measurement.
      
      /*
        The getMeasurementType() function can return:
          *KLineKWP1281Lib::UNKNOWN - index out of range (measurement doesn't exist in block)
          *KLineKWP1281Lib::UNITS   - the measurement contains human-readable text in the units string
          *KLineKWP1281Lib::VALUE   - "regular" measurement, with a value and units
      */
      switch (KLineKWP1281Lib::getMeasurementType(measurement_index, amount_of_measurements, measurements, sizeof(measurements))) {
        //Value and units
        case KLineKWP1281Lib::VALUE:
          Serial.print(KLineKWP1281Lib::getMeasurementValue(measurement_index, amount_of_measurements, measurements, sizeof(measurements)));
          Serial.print(' ');
          Serial.println(KLineKWP1281Lib::getMeasurementUnits(measurement_index, amount_of_measurements, measurements, sizeof(measurements), units_string, sizeof(units_string)));
          break;
        
        //Units string containing text
        case KLineKWP1281Lib::UNITS:
          Serial.println(KLineKWP1281Lib::getMeasurementUnits(measurement_index, amount_of_measurements, measurements, sizeof(measurements), units_string, sizeof(units_string)));
          break;
        
        //Invalid measurement index
        case KLineKWP1281Lib::UNKNOWN:
          Serial.println("N/A");
          break;
      }
      break;
  }
}
