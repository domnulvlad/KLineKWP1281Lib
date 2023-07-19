#include <KLineKWP1281Lib.h>

/*
  *This sketch displays information in the Serial Monitor, so another serial port is required for the K-line.
  *If another port is not available on the selected board, the regular port "Serial" can be used, although no information can be printed to it (good for
  projects with external displays or for data logging).

  ***Uncomment the appropriate options in the configuration.h file!

  Arduino UNO
    *no additional serial port, software serial is used
    *library of choice: AltSoftSerial
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
    *cannot be used with this sketch (software serial libraries do not support the baud rates necessary, mainly 10400)
    *can be used with sketches which display data on external hardware, by connecting the interface to the regular hardware serial

  ***If using the first hardware serial port (Serial) (with other sketches), the interface must be disconnected during code upload.
*/

#include "configuration.h"
#include "communication.h"

//Debugging can also be enabled in configuration.h in order to also print bus activity on the Serial Monitor.
#if has_debug
  KLineKWP1281Lib diag(beginFunction, endFunction, sendFunction, receiveFunction, TX_pin, is_full_duplex, &Serial);
#else
  KLineKWP1281Lib diag(beginFunction, endFunction, sendFunction, receiveFunction, TX_pin, is_full_duplex);
#endif

uint8_t measurements[3 * 4]; //buffer to store the measurements

void setup() {
  //Initialize the Serial Monitor.
  Serial.begin(115200);
  delay(500);
  Serial.println("Sketch started.");
  
  //Change these according to your module, in configuration.h.
  diag.connect(module, module_baud_rate);
  
  //Read all groups, 001-255.
  for (uint8_t i = 1; i != 0; i++) {
    showMeasurements(i);
  }
  
  //Disconnect from the module.
  diag.disconnect();
}

void loop() {
}

void showMeasurements(uint8_t block) {
  /*
    The readGroup() function can return:
      *KLineKWP1281Lib::SUCCESS - received measurements
      *KLineKWP1281Lib::FAIL    - the requested block does not exist
      *KLineKWP1281Lib::ERROR   - communication error
  */
  uint8_t amount_of_measurements = 0;
  switch (diag.readGroup(amount_of_measurements, block, measurements, sizeof(measurements))) {
    case KLineKWP1281Lib::ERROR:
      Serial.println(F("Error reading measurements!"));
      break;
    
    case KLineKWP1281Lib::FAIL:
      Serial.print(F("Block "));
      Serial.print(block);
      Serial.println(F(" does not exist!"));
      break;
    
    case KLineKWP1281Lib::SUCCESS:
      Serial.print(F("Block "));
      Serial.print(block);
      Serial.println(':');
      
      //Will hold the measurement's units
      char units_string[16];
      
      //Display each measurement.
      for (uint8_t i = 0; i < 4; i++) {
        //Format the values with a leading tab.
        Serial.print('\t');
        
        /*
          The getMeasurementType() function can return:
            *KLineKWP1281Lib::UNKNOWN - index out of range (measurement doesn't exist in block)
            *KLineKWP1281Lib::UNITS   - the measurement contains human-readable text in the units string
            *KLineKWP1281Lib::VALUE   - "regular" measurement, with a value and units
        */
        switch (KLineKWP1281Lib::getMeasurementType(i, amount_of_measurements, measurements, sizeof(measurements))) {
          //Value and units
          case KLineKWP1281Lib::VALUE:
            Serial.print(KLineKWP1281Lib::getMeasurementValue(i, amount_of_measurements, measurements, sizeof(measurements)));
            Serial.print(' ');
            Serial.println(KLineKWP1281Lib::getMeasurementUnits(i, amount_of_measurements, measurements, sizeof(measurements), units_string, sizeof(units_string)));
            break;
          
          //Units string containing text
          case KLineKWP1281Lib::UNITS:
            Serial.println(KLineKWP1281Lib::getMeasurementUnits(i, amount_of_measurements, measurements, sizeof(measurements), units_string, sizeof(units_string)));
            break;
          
          //Invalid measurement index
          case KLineKWP1281Lib::UNKNOWN:
            Serial.println("N/A");
            break;
        }
      }

      //Leave an empty line.
      Serial.println();
      break;
  }
}
