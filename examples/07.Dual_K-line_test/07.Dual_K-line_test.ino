/*
  Title:
    07.Dual_K-line_test.ino

  Description:
    Demonstrates how to connect to 2 K-lines at once and read measurements.

  Notes:
    *This demo is made specifically for the ESP32, as native multitasking is needed. Also, it has 2 hardware serial ports.
    *It doesn't make sense to have 2 transceivers on the same K-line! This is only useful if you decide to separate a specific module from the
    car's K-line in order to be able to access it asyncronously.

    *You can change which blocks and which measurements to read by modifying the #defines below.
    *These measurements will be read continuously while the sketch is running.
    *It is not necessary to maintain the connection with "diag.update();" in the loop, because any request will have the same effect (update() must
    be used in periods of inactivity).
    *If you have manually disabled the text table by commenting out the line "#define KWP1281_TEXT_TABLE_SUPPORTED" in "KLineKWP1281Lib.h", the value
    for some parameters will be displayed as "EN_f25".
    *If you have the text table enabled, you can choose from a few different languages a bit further below in "KLineKWP1281Lib.h". Please only choose
    one option.
*/

/*
  ***Uncomment the appropriate options in the configuration.h file!

  ESP32
     has two additional serial ports
     pins (they can be remapped, this is what they are configured to in this example):
       K-line 1 TX -> RX pin 16
       K-line 1 RX -> TX pin 17
       K-line 2 TX -> RX pin 26
       K-line 2 RX -> TX pin 27
*/

//Change which measurement to read from K-line 1, from which block.
#define BLOCK_TO_READ_KLINE1 1
#define MEASUREMENT_TO_READ_KLINE1 0 //valid range: 0-3

//Change which measurement to read from K-line 2, from which block.
#define BLOCK_TO_READ_KLINE2 1
#define MEASUREMENT_TO_READ_KLINE2 0 //valid range: 0-3

//Include the library.
#include <KLineKWP1281Lib.h>

//Include the two files containing configuration options and the functions used for communication.
#include "configuration.h"
#include "communication.h"

//Debugging can be enabled in configuration.h in order to print connection-related info on the Serial Monitor.
#if debug_info
  KLineKWP1281Lib diag1(beginFunction1, endFunction1, sendFunction1, receiveFunction1, TX1_pin, true, &Serial);
  KLineKWP1281Lib diag2(beginFunction2, endFunction2, sendFunction2, receiveFunction2, TX2_pin, true, &Serial);
#else
  KLineKWP1281Lib diag1(beginFunction1, endFunction1, sendFunction1, receiveFunction1, TX1_pin, true);
  KLineKWP1281Lib diag2(beginFunction2, endFunction2, sendFunction2, receiveFunction2, TX2_pin, true);
#endif

//Normally, each measurement takes 3 bytes, and a block can store up to 4 measurements.
#define MEASUREMENT_BUFFER_SIZE 3 * 4

//Structure to hold all data necessary for processing a measurement
struct kline_measurement_structure {
  uint8_t formula;
  uint8_t NWb;
  uint8_t MWb;
  uint8_t measurement_data[MEASUREMENT_BUFFER_SIZE];
  uint8_t measurement_data_length;
};

//Queues which will contain structures like the one defined above
QueueHandle_t kline1_measurement_queue, kline2_measurement_queue;

void setup() {
  //Initialize the Serial Monitor.
  Serial.begin(115200);
  delay(500);
  Serial.println("Sketch started.");

  //If debugging bus traffic was enabled, attach the debugging functions.
#if debug_traffic
  diag1.KWP1281debugFunction(KWP1281debugFunction1);
  diag2.KWP1281debugFunction(KWP1281debugFunction2);
#endif
  
  //Create the two queues, each able to hold 10 measurement structures.
  kline1_measurement_queue = xQueueCreate(10, sizeof(kline_measurement_structure));
  kline2_measurement_queue = xQueueCreate(10, sizeof(kline_measurement_structure));
  
  //Create the task which will display the data coming from the two modules.
  xTaskCreate(receiver_task, "recv", 4096, NULL, 1, NULL);

  //Create a task for each K-line.
  //You can find more details about these functions in Espressif's documentation.
  //Here, both tasks have the same priority, higher than the receiver task as they are more important.
  xTaskCreate(kline1_task, "K1", 4096, NULL, 2, NULL);
  xTaskCreate(kline2_task, "K2", 4096, NULL, 2, NULL);

  Serial.printf("Requesting block %d, measurement %d from K-line 1 and block %d, measurement %d from K-line 2 continuously.\n",
                BLOCK_TO_READ_KLINE1, MEASUREMENT_TO_READ_KLINE1,
                BLOCK_TO_READ_KLINE2, MEASUREMENT_TO_READ_KLINE2);
}

void loop() {
  //The classic Arduino loop will not be used. Delete the task responsible for it.
  vTaskDelete(NULL);
}

void receiver_task(void *arg) {
  //This structure will hold data, either from K-line 1 or K-line 2.
  kline_measurement_structure measurement_structure;
  
  //A task must have an infinite loop.
  while (true) {
    //Only display the data when it is available from both modules.
    //The timeout of portMAX_DELAY means this will wait however necessary (possibly indefinitely) for data to be available.
    if (xQueuePeek(kline1_measurement_queue, &measurement_structure, portMAX_DELAY) && xQueuePeek(kline2_measurement_queue, &measurement_structure, portMAX_DELAY)) {
      //At this point, there is definitely something available in the queue for K-line 1, retrieve it.
      xQueueReceive(kline1_measurement_queue, &measurement_structure, 0);
  
      //Display the measurement from K-line 1.
      displayMeasurement(measurement_structure);
      Serial.print(" | ");
  
      //At this point, there is definitely something available in the queue for K-line 2, retrieve it.
      xQueueReceive(kline2_measurement_queue, &measurement_structure, 0);
  
      //Display the measurement from K-line 2.
      displayMeasurement(measurement_structure);
      Serial.println();
    }
  }
}

void kline1_task(void *arg) {
  //When the task starts, connect to the desired module.
  //Change these according to your needs, in configuration.h.
  diag1.connect(connect_to_module1, module_baud_rate1);
  uint8_t measurements[MEASUREMENT_BUFFER_SIZE];

  //A task must have an infinite loop.
  while (true) {
    getSingleMeasurement(BLOCK_TO_READ_KLINE1, MEASUREMENT_TO_READ_KLINE1, diag1, kline1_measurement_queue, measurements, sizeof(measurements));
  }
}

void kline2_task(void *arg) {
  //When the task starts, connect to the desired module.
  //Change these according to your needs, in configuration.h.
  diag2.connect(connect_to_module2, module_baud_rate2);
  uint8_t measurements[MEASUREMENT_BUFFER_SIZE];

  //A task must have an infinite loop.
  while (true) {
    getSingleMeasurement(BLOCK_TO_READ_KLINE2, MEASUREMENT_TO_READ_KLINE2, diag2, kline2_measurement_queue, measurements, sizeof(measurements));
  }
}

void getSingleMeasurement(uint8_t block, uint8_t measurement_index, KLineKWP1281Lib &diag, QueueHandle_t &measurement_queue, uint8_t *measurement_buffer, size_t measurement_buffer_size) {
  //This will contain the amount of measurements in the current block, after calling the readGroup() function.
  uint8_t amount_of_measurements = 0;
  
  /*
    The readGroup() function can return:
       KLineKWP1281Lib::SUCCESS - received measurements
       KLineKWP1281Lib::FAIL    - the requested block does not exist
       KLineKWP1281Lib::ERROR   - communication error
  */
  
  //Read the requested group and store the return value.
  KLineKWP1281Lib::executionStatus readGroup_status = diag.readGroup(amount_of_measurements, block, measurement_buffer, measurement_buffer_size);
  
  switch (readGroup_status) {
    case KLineKWP1281Lib::ERROR:
      Serial.println("Error reading measurements!");
      return;

    case KLineKWP1281Lib::FAIL:
      Serial.printf("Block %d does not exist!\n", block);
      return;

    //Execute the code after the switch().
    case KLineKWP1281Lib::SUCCESS:
      break;
  }
  
  //This structure will contain the received measurement.
  kline_measurement_structure measurement_structure;
  
  /*
    *Most measurements only need the "formula" (type), "NWb" (byte A) and "MWb" (byte B).
    *To determine the type of a measurement with the getMeasurementType() function, only the formula is needed.
    
    *For measurements of type VALUE, you need to get the value and units.
    *You can call getMeasurementValue() with those 3 parameters mentioned above, instead of all (index, amount, buffer, buffer size).
    *The same applies to getMeasurementUnits().
    
    *There are a few measurements which may contain more than 2 data bytes (for example, if the module sends long text or a field of values).
    *These measurements will always be of the TEXT type, even if most of them still only use the 2 data bytes.
    
    *So the getMeasurementText() function can either be called with all parameters, or with the data buffer and its length.
    *In this case, the data buffer and its length can be retrieved with getMeasurementData() and getMeasurementDataLength().    
    *The function getMeasurementData() returns a pointer into the measurement buffer, so its contents must be copied to avoid loss!
    
    *You cannot call getMeasurementText() with only those 3 bytes!
  */
  
  
  //Retrieve the 3 bytes into the structure.
  measurement_structure.formula = KLineKWP1281Lib::getFormula(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  measurement_structure.NWb = KLineKWP1281Lib::getNWb(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  measurement_structure.MWb = KLineKWP1281Lib::getMWb(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  
  //Retrieve the data buffer and its length, because some measurements may need it.
  uint8_t measurement_data_length = KLineKWP1281Lib::getMeasurementDataLength(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  uint8_t *measurement_data = KLineKWP1281Lib::getMeasurementData(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  
  //Copy the data buffer and its length into the structure.
  measurement_structure.measurement_data_length = measurement_data_length;
  memcpy(measurement_structure.measurement_data, measurement_data, measurement_data_length);
  
  //Put the structure on the specified queue; don't wait for the queue to free up if it is full.
  xQueueSend(measurement_queue, &measurement_structure, 0);
}

void displayMeasurement(kline_measurement_structure &measurement_structure) {
  /*
    The getMeasurementType() function can return:
      *KLineKWP1281Lib::UNKNOWN - index out of range (measurement doesn't exist in block)
      *KLineKWP1281Lib::VALUE   - regular measurement, with a value and units
      *KLineKWP1281Lib::TEXT    - text measurement
  */
  
  //Get the measurement's type and check the return value.
  switch (KLineKWP1281Lib::getMeasurementType(measurement_structure.formula)) {
    //"Value and units" type
    case KLineKWP1281Lib::VALUE:
    {
      //This will hold the measurement's units.
      char units_string[16];
      
      //Determine how many decimal places are best suited to this measurement.
      uint8_t decimals = KLineKWP1281Lib::getMeasurementDecimals(measurement_structure.formula);
      
      //Display the calculated value, with the recommended amount of decimals.
      Serial.print(KLineKWP1281Lib::getMeasurementValue(measurement_structure.formula, measurement_structure.NWb, measurement_structure.MWb), decimals);
      Serial.print(' ');
      Serial.print(KLineKWP1281Lib::getMeasurementUnits(measurement_structure.formula, measurement_structure.NWb, measurement_structure.MWb, units_string, sizeof(units_string)));
    }
    break;

    //"Text" type
    case KLineKWP1281Lib::TEXT:
    {
      //This will hold the measurement's text.
      char text_string[16];
      
      //Display the text.
      Serial.print(KLineKWP1281Lib::getMeasurementText(measurement_structure.formula, measurement_structure.measurement_data, measurement_structure.measurement_data_length, text_string, sizeof(text_string)));
    }
    break;

    //Invalid measurement index
    case KLineKWP1281Lib::UNKNOWN:
      Serial.print("N/A");
      break;
  }
}
