/*
  Title:
    03.Full_measurement_test.ino

  Description:
    Demonstrates how to read a module's measuring blocks.

  Notes:
    *Measuring blocks 0-255 will be read, after which the connection will be stopped.
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
  
  //Read all groups (000-255).
  Serial.println("Requesting measuring blocks 000-255.");
  for (uint16_t i = 0; i <= 255; i++) {
    showMeasurements(i);
  }
  
  //Disconnect from the module.
  diag.disconnect();
  Serial.println("Disconnected.");
}

void loop() {
  
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
    
    //You can retrieve the "formula" byte for a measurement, to avoid giving all these parameters to the other functions.
    uint8_t formula = KLineKWP1281Lib::getFormula(i, amount_of_measurements, measurements, sizeof(measurements));
    
    /*
      The getMeasurementType() function can return:
        *KLineKWP1281Lib::UNKNOWN - index out of range (measurement doesn't exist in block)
        *KLineKWP1281Lib::VALUE   - regular measurement, with a value and units
        *KLineKWP1281Lib::TEXT    - text measurement
    */
    
    //Get the current measurement's type.
    //If you don't want to extract the "formula" byte as shown above with the getFormula() function, getMeasurementType() can also take the same parameters
    //like the other functions (index, amount, buffer, buffer_size).
    KLineKWP1281Lib::measurementType measurement_type = KLineKWP1281Lib::getMeasurementType(formula);
    
    //Check the return value.
    switch (measurement_type) {
      //"Value and units" type
      case KLineKWP1281Lib::VALUE:
      {
        //You can retrieve the other significant bytes for a measurement, to avoid giving all these parameters to the other functions.
        uint8_t NWb = KLineKWP1281Lib::getNWb(i, amount_of_measurements, measurements, sizeof(measurements));
        uint8_t MWb = KLineKWP1281Lib::getMWb(i, amount_of_measurements, measurements, sizeof(measurements));
        
        //This will hold the measurement's units.
        char units_string[16];
        
        //Get the current measurement's value and units.
        //If you don't want to extract the "formula", "NWb" and "MWb" bytes as shown above with the getFormula(), getNWb() and getMWb() functions,
        //getMeasurementValue() and getMeasurementUnits() can also take the same parameters like the other functions (index, amount, buffer, buffer_size).
        double value = KLineKWP1281Lib::getMeasurementValue(formula, NWb, MWb);
        KLineKWP1281Lib::getMeasurementUnits(formula, NWb, MWb, units_string, sizeof(units_string));
        //The getMeasurementUnits() function returns the same string it's given, units_string in this case.
        
        //Determine how many decimal places are best suited to this measurement.
        //This function only needs to know the "formula", but you can also give it all parameters as with all other functions.
        uint8_t decimals = KLineKWP1281Lib::getMeasurementDecimals(formula);
        
        //Display the calculated value, with the recommended amount of decimals.
        Serial.print(value, decimals);
        Serial.print(' ');
        Serial.println(units_string);
        
        /*
          //If you prefer the C-style method with the printf() family of functions, you may use the recommended decimals like so:
          printf("Value: %.*lf %s", decimals, value, units_string);
        */
      }
      break;
      
      //"Text" type
      case KLineKWP1281Lib::TEXT:
      {
        //This will hold the measurement's text.
        char text_string[16];
        
        //The only important values are stored in the text string.
        //The getMeasurementUnits() function needs more data than just those 3 bytes.
        //An example of extracting this data is given in the "Dual_K-line_test" demo.
        //It's easier to just give it all parameters, like done here.
        KLineKWP1281Lib::getMeasurementText(i, amount_of_measurements, measurements, sizeof(measurements), text_string, sizeof(text_string));
        
        //Display the text.
        Serial.println(text_string);
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
