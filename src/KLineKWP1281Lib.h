#ifndef KLineKWP1281Lib_H
#define KLineKWP1281Lib_H

//If the following line is commented out, all debug procedures are removed from the library (to save memory):
#define KWP1281_DEBUG_SUPPORTED

//There are some formulas whose units string comes from a large table. If the following line is commented out, that table is removed:
#define KWP1281_TEXT_TABLE_SUPPORTED
//(if disabled, the string given to getMeasurementUnits() will contain "EN_f25" if that formula is encountered)

//If the following line is commented out, fault code description strings are removed from the library (to save memory):
//#define KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED
//(if disabled, the string given to getFaultDescription() will contain "EN_dsc")

//If the following line is commented out, OBD fault code description strings are removed from the library (to save memory):
//#define KWP1281_OBD_FAULT_CODE_DESCRIPTION_SUPPORTED
//(if disabled, the string given to getFaultDescription() will contain "EN_obd")

//If the following line is commented out, fault code elaboration strings are removed from the library (to save memory):
#define KWP1281_FAULT_CODE_ELABORATION_SUPPORTED
//(if disabled, the string given to getFaultElaboration() will contain "EN_elb")

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "units.h" //measurement unit strings
#include "decimals.h" //measurement decimals
#include "keyed_struct.h" //storage type for text

#ifdef KWP1281_TEXT_TABLE_SUPPORTED
  //choose text table language, if enabled:
  
  #include "text_table_EN.h"
  //#include "text_table_DE.h"
  //#include "text_table_PL.h"
  //#include "text_table_RO.h"
#endif

#ifdef KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED
  //choose fault code description text language, if enabled:
  
  #include "fault_code_description_EN.h"
  //#include "fault_code_description_DE.h"
  //#include "fault_code_description_PL.h"
  //#include "fault_code_description_RO.h"
#endif

#ifdef KWP1281_OBD_FAULT_CODE_DESCRIPTION_SUPPORTED
  //choose OBD fault code description text language, if enabled:
  
  #include "OBD_fault_code_description_EN.h"
  //#include "OBD_fault_code_description_DE.h"
  //#include "OBD_fault_code_description_PL.h"
  //#include "OBD_fault_code_description_RO.h"
#endif

#ifdef KWP1281_FAULT_CODE_ELABORATION_SUPPORTED
  //choose fault code elaboration text language, if enabled:
  
  #include "fault_code_elaboration_EN.h"
  //#include "fault_code_elaboration_DE.h"
  //#include "fault_code_elaboration_PL.h"
  //#include "fault_code_elaboration_RO.h"
#endif

//AVR microcontrollers use 2-byte (word) pointers. Others use 4-byte (dword) pointers.
#if defined(__AVR__)
  #define READ_POINTER_FROM_PROGMEM pgm_read_word_near
  
  #if defined(KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED) || defined(KWP1281_OBD_FAULT_CODE_DESCRIPTION_SUPPORTED)
    #warning Enabling fault code descriptions on AVR platforms may cause problems.
  #endif
#else
  #define READ_POINTER_FROM_PROGMEM pgm_read_dword_near
#endif

//Helper for determining the amount of elements in an array
#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

class KLineKWP1281Lib
{  
  public:
    ///VARIABLES/TYPES

    //Function pointer types for callbacks
    using callBack_type             = void (*)(void);                                                                        //generic callback (for custom functions)
    using beginFunction_type        = void (*)(unsigned long baud);                                                          //beginFunction
    using endFunction_type          = void (*)(void);                                                                        //endFunction
    using sendFunction_type         = void (*)(uint8_t data);                                                                //sendFunction
    using receiveFunction_type      = bool (*)(uint8_t &data);                                                               //receiveFunction
    using KWP1281debugFunction_type = void (*)(bool type, uint8_t sequence, uint8_t command, uint8_t* data, uint8_t length); //debugFunction
    
    //Return types for functions
    enum executionStatus {
      FAIL,
      SUCCESS,
      ERROR
    };
    
    //Measurement types
    enum measurementType {
      UNKNOWN,
      VALUE,
      UNITS
    };
    
    //Create an instance of the library
    KLineKWP1281Lib(
      beginFunction_type beginFunction, endFunction_type endFunction, sendFunction_type sendFunction, receiveFunction_type receiveFunction,
      uint8_t tx_pin, bool full_duplex = true, Stream* debug_port = nullptr
    );
    
    //How many milliseconds to wait before sending a complement to the initialization bytes received from the module
    uint16_t initComplementDelay = 40;
    //How many milliseconds to wait before sending a complement to a byte received from the module
    uint16_t complementDelay = 2;
    //How many milliseconds to wait before sending a byte in a block, after receiving a complement from the module
    uint16_t byteDelay = 2;
    //How many milliseconds to wait before sending a block (request)
    uint16_t blockDelay = 10;
    
    //How many milliseconds to wait for receiving a response during initialization
    uint16_t initResponseTimeout = 1000;
    //How many milliseconds to wait for receiving a complement when sending bytes in a block
    uint16_t complementResponseTimeout = 50;
    //How many milliseconds to wait for receiving a response to a request
    uint16_t responseTimeout = 2000;
    
    ///FUNCTIONS
    
    //Attach a function to be called for debugging KWP1281 messages
    void KWP1281debugFunction(KWP1281debugFunction_type debug_function);
    
    //Define a custom function to execute while initializing at 5-baud (must take 200ms)
    void custom5baudWaitFunction(callBack_type function);
    //Define a custom function to execute if an error occurs and the connection is terminated
    void customErrorFunction(callBack_type function);
    
    //Attempt to connect to a module
    executionStatus attemptConnect(uint8_t module, unsigned long baud_rate);
    //Connect to a module
    void connect(uint8_t module, unsigned long baud_rate);
    //Maintain the connection
    void update();
    //Stop the connection
    void disconnect();
    
    //Get the module's VAG part number
    char* getPartNumber();
    //Get the module's name
    char* getIdentification();
    //Get the module's extra identification
    char* getExtraIdentification();
    //Get the module's coding value
    uint16_t getCoding();
    //Get the module's workshop code
    uint32_t getWorkshopCode();
    
    //Perform a login operation
    executionStatus login(uint16_t login_code, uint32_t workshop_code);
    
    //Change the coding of a module
    executionStatus recode(uint16_t coding, uint32_t workshop_code);
    
    //Get the module's fault codes
    executionStatus readFaults(uint8_t &amount_of_fault_codes, uint8_t* fault_code_buffer = nullptr, size_t fault_code_buffer_size = 0);
    //Get a fault code from a fault code reading
    static uint16_t getFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
    //Determine whether or not a fault is of standard OBD type
    static bool isOBDFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
    static bool isOBDFaultCode(uint16_t fault_code);
    //Get a string containing the formatted code of a standard OBD fault code
    static char* getOBDFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size);
    static char* getOBDFaultCode(uint16_t fault_code, char* str, size_t string_size);
    //Get the fault description string from a fault code reading
    static char* getFaultDescription(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size);
    static char* getFaultDescription(uint16_t fault_code, char* str, size_t string_size);
    //Get the length of the description string from a fault code reading
    static size_t getFaultDescriptionLength(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
    static size_t getFaultDescriptionLength(uint16_t fault_code);
    //Get a fault elaboration code from a fault code reading
    static uint8_t getFaultElaborationCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
    //Get the fault elaboration string from a fault code reading
    static char* getFaultElaboration(bool &is_intermittent, uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size);
    static char* getFaultElaboration(bool &is_intermittent, uint8_t elaboration_code, char* str, size_t string_size);
    //Get the length of the elaboration string from a fault code reading
    static size_t getFaultElaborationLength(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
    static size_t getFaultElaborationLength(uint8_t elaboration_code);
    //Clear the module's fault codes
    executionStatus clearFaults();
    
    //Read the value of an adaptation channel
    executionStatus readAdaptation(uint8_t channel, uint16_t &value);
    //Test if a value would work on an adaptation channel
    executionStatus testAdaptation(uint8_t channel, uint16_t value);
    //Change the value of an adaptation channel
    executionStatus adapt(uint8_t channel, uint16_t value, uint32_t workshop_code);
    
    //Perform a basic setting
    executionStatus basicSetting(uint8_t &amount_of_values, uint8_t group, uint8_t* basic_setting_buffer = nullptr, size_t basic_setting_buffer_size = 0);
    //Get a value from a basic setting reading
    static uint8_t getBasicSettingValue(uint8_t value_index, uint8_t amount_of_values, uint8_t* basic_setting_buffer, size_t basic_setting_buffer_size);
    
    //Read a group (block) of measurements
    executionStatus readGroup(uint8_t &amount_of_measurements, uint8_t group, uint8_t* measurement_buffer, size_t measurement_buffer_size);
    
    //Get the 3 significant bytes of a measurement
    static uint8_t getFormula(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
    static uint8_t getNWb(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
    static uint8_t getMWb(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
    static uint8_t *getMeasurementData(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
    static uint8_t getMeasurementDataLength(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
    
    //Get a measurement's type from a group reading (whether the value or the units is significant)
    static measurementType getMeasurementType(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
    static measurementType getMeasurementType(uint8_t formula);
    //Get the calculated value of a measurement from a group reading
    static double getMeasurementValue(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
    static double getMeasurementValue(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length);
    static double getMeasurementValue(uint8_t formula, uint8_t NWb, uint8_t MWb);
    //Get the units of a measurement from a group reading
    static char* getMeasurementUnits(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size, char* str, size_t string_size);
    static char* getMeasurementUnits(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length, char* str, size_t string_size);
    //Get the recommended decimal places of a measurement from a group reading
    static uint8_t getMeasurementDecimals(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
    static uint8_t getMeasurementDecimals(uint8_t formula);
    
    //Read a chunk of ROM/EEPROM
    executionStatus readROM(uint8_t chunk_size, uint16_t start_address, size_t &bytes_received, uint8_t* memory_buffer, uint8_t memory_buffer_size);
    
    //Perform output tests
    executionStatus outputTests(uint16_t &current_output_test);
    //Get the description string for the currently running output test
    static char* getOutputTestDescription(uint16_t output_test, char* str, size_t string_size);
    //Get the length of the description string for the currently running output test
    static size_t getOutputTestDescriptionLength(uint16_t output_test);
    
  private:
    ///VARIABLES/TYPES
    static const uint8_t KWP_ACKNOWLEDGE             = 0x09; //the module has no more data to send
    static const uint8_t KWP_REFUSE                  = 0x0A; //the module can not fulfill a request
    static const uint8_t KWP_DISCONNECT              = 0x06; //the tester wants to disconnect from the module
    
    static const uint8_t KWP_REQUEST_EXTRA_ID        = 0x00; //response: KWP_RECEIVE_ID_DATA (data available) / KWP_ACKNOWLEDGE (no data available)
    static const uint8_t KWP_REQUEST_LOGIN           = 0x2B; //response: KWP_ACKNOWLEDGE (login successful) / KWP_REFUSE (login not successful)
    static const uint8_t KWP_REQUEST_RECODE          = 0x10; //response: KWP_REQUEST_EXTRA_ID...
    static const uint8_t KWP_REQUEST_FAULT_CODES     = 0x07; //response: KWP_RECEIVE_FAULT_CODES (ok) / KWP_REFUSE (fault codes not supported)
    static const uint8_t KWP_REQUEST_CLEAR_FAULTS    = 0x05; //response: KWP_ACKNOWLEDGE (ok) / KWP_REFUSE (clearing fault codes not supported)
    static const uint8_t KWP_REQUEST_ADAPTATION      = 0x21; //response: KWP_RECEIVE_ADAPTATION (ok) / KWP_REFUSE (invalid channel)
    static const uint8_t KWP_REQUEST_ADAPTATION_TEST = 0x22; //response: KWP_RECEIVE_ADAPTATION (ok) / KWP_REFUSE (invalid channel)
    static const uint8_t KWP_REQUEST_ADAPTATION_SAVE = 0x2A; //response: KWP_RECEIVE_ADAPTATION (ok) / KWP_REFUSE (invalid channel or value)
    static const uint8_t KWP_REQUEST_GROUP_READING   = 0x29; //response: KWP_RECEIVE_GROUP_READING (ok) / KWP_ACKNOWLEDGE (empty group) / KWP_REFUSE (invalid group)
    static const uint8_t KWP_REQUEST_GROUP_READING_0 = 0x12; //response: KWP_RECEIVE_GROUP_READING (ok) / KWP_ACKNOWLEDGE (empty group) / KWP_REFUSE (invalid group)
    static const uint8_t KWP_REQUEST_READ_ROM        = 0x03; //response: KWP_RECEIVE_ROM (ok) / KWP_REFUSE (reading ROM not supported or invalid parameters)
    static const uint8_t KWP_REQUEST_OUTPUT_TEST     = 0x04; //response: KWP_RECEIVE_OUTPUT_TEST (ok) / KWP_REFUSE (output tests not supported)
    static const uint8_t KWP_REQUEST_BASIC_SETTING   = 0x28; //response: KWP_RECEIVE_BASIC_SETTING (ok) / KWP_ACKNOWLEDGE (empty group) / KWP_REFUSE (invalid channel or not supported)
    static const uint8_t KWP_REQUEST_BASIC_SETTING_0 = 0x11; //response: KWP_RECEIVE_BASIC_SETTING (ok) / KWP_ACKNOWLEDGE (empty group) / KWP_REFUSE (invalid channel or not supported)
    
    static const uint8_t KWP_RECEIVE_ID_DATA         = 0xF6; //request: connect/KWP_REQUEST_EXTRA_ID/KWP_REQUEST_RECODE
    static const uint8_t KWP_RECEIVE_FAULT_CODES     = 0xFC; //request: KWP_REQUEST_FAULT_CODES
    static const uint8_t KWP_RECEIVE_ADAPTATION      = 0xE6; //request: KWP_REQUEST_ADAPTATION/KWP_REQUEST_ADAPTATION_TEST/KWP_REQUEST_ADAPTATION_SAVE
    static const uint8_t KWP_RECEIVE_GROUP_READING   = 0xE7; //request: KWP_REQUEST_GROUP_READING
    static const uint8_t KWP_RECEIVE_ROM             = 0xFD; //request: KWP_REQUEST_READ_ROM
    static const uint8_t KWP_RECEIVE_OUTPUT_TEST     = 0xF5; //request: KWP_REQUEST_OUTPUT_TEST
    static const uint8_t KWP_RECEIVE_BASIC_SETTING   = 0xF4; //request: KWP_REQUEST_BASIC_SETTING
    
    //Callback functions
    beginFunction_type        _beginFunction;
    endFunction_type          _endFunction;
    sendFunction_type         _sendFunction;
    receiveFunction_type      _receiveFunction;
    KWP1281debugFunction_type _debugFunction;
    
    //Selected transmit pin
    uint8_t _tx_pin;
    
    //Flag to indicate whether or not the used Serial port can receive at the same time as sending
    bool _full_duplex;
    
    //Optional debug port
    Stream* _debug_port;
    
    //Return types for some functions
    enum RETURN_TYPE {
      ERROR_TIMEOUT,
      ERROR_OK,
      TYPE_ID,
      TYPE_ACK,
      TYPE_REFUSE,
      TYPE_FAULT_CODES,
      TYPE_ADAPTATION,
      TYPE_GROUP_READING,
      TYPE_ROM,
      TYPE_OUTPUT_TEST,
      TYPE_BASIC_SETTING
    };
    
    //The currently connected module's identification strings and values
    struct module_identification {
      char part_number[13];
      char identification[25];
      char extra_identification[37];
      uint32_t coding;
      uint32_t workshop_code;
    } identification_data;
    
    //Types of debugging events for printing info
    enum DEBUG_TYPE {
      CHECK_BAUD,
      INCORRECT_PROTOCOL,
      CONNECTED,
      UNEXPECTED_RESPONSE,
      NO_RESPONSE,
      DISCONNECTED,
      TERMINATED,
      TIMED_OUT,
      SEND_ERROR,
      
      ARRAY_NOT_LARGE_ENOUGH,
      NULL_ARRAY_SIZE,
      
      NO_PART_NUMBER_AVAILABLE,
      RECEIVED_PART_NUMBER,
      NO_ID_PART1_AVAILABLE,
      RECEIVED_ID_PART1,
      NO_ID_PART2_AVAILABLE,
      RECEIVED_ID_PART2,
      NO_CODING_WSC_AVAILABLE,
      RECEIVED_CODING_WSC,
      EXTRA_ID_AVAILABLE,
      NO_EXTRA_ID_AVAILABLE,
      NO_EXTRA_ID_PART1_AVAILABLE,
      RECEIVED_EXTRA_ID_PART1,
      NO_EXTRA_ID_PART2_AVAILABLE,
      RECEIVED_EXTRA_ID_PART2,
      NO_EXTRA_ID_PART3_AVAILABLE,
      RECEIVED_EXTRA_ID_PART3,
      
      LOGIN_ACCEPTED,
      LOGIN_NOT_ACCEPTED,
      
      FAULT_CODES_NOT_SUPPORTED,
      NO_FAULT_CODES,
      RECEIVED_FAULT_CODES,
      END_OF_FAULT_CODES,
      CLEARING_FAULT_CODES_NOT_SUPPORTED,
      CLEARING_FAULT_CODES_ACCEPTED,
      
      INVALID_ADAPTATION_CHANNEL,
      INVALID_ADAPTATION_CHANNEL_OR_VALUE,
      ADAPTATION_RECEIVED,
      ADAPTATION_ACCEPTED,
      ADAPTATION_NOT_ACCEPTED,
      
      INVALID_BASIC_SETTING_GROUP,
      RECEIVED_EMPTY_BASIC_SETTING_GROUP,
      RECEIVED_BASIC_SETTING,
      
      INVALID_MEASUREMENT_GROUP,
      RECEIVED_EMPTY_GROUP,
      RECEIVED_GROUP,
      
      READ_ROM_NOT_SUPPORTED,
      RECEIVED_ROM,
      
      OUTPUT_TESTS_NOT_SUPPORTED,
      RECEIVED_OUTPUT_TEST,
      END_OF_OUTPUT_TESTS,
      
      DISCONNECT_INFO
    };
    
    //Current parameters
    long    _current_baud_rate;
    uint8_t _current_module;
    
    //Pointers to custom functions
    callBack_type custom_5baud_wait_function_pointer, custom_error_function_pointer;
    
    //Timers
    uint16_t _timeout;
    unsigned long _last_received_byte_time;
    
    //Buffers
    uint8_t _parameter_buffer[2];
    uint8_t _receive_buffer[16];
    uint8_t _receive_byte;
    
    //Flags
    bool _may_send_protocol_parameters_again;
    
    //Current message sequence number
    uint8_t _sequence;
    
    ///FUNCTIONS
    
    //Initialize the control module
    void bitbang_5baud(uint8_t module);
    
    //Read the module's identification strings and values
    executionStatus read_identification();
    
    //Receive parameters during initialization
    bool receive_parameters(uint8_t* buffer);
    //Receive a single byte
    RETURN_TYPE read_byte(uint8_t &_byte, bool complement = true);
    //Send the complement of a byte
    void send_complement(uint8_t _byte);
    //Receive a block
    RETURN_TYPE receive(size_t &bytes_received, uint8_t* buffer, size_t buffer_size);
    //Confirm receiving a block and request sending the next
    bool acknowledge();
    
    //Send a single byte
    RETURN_TYPE send_byte(uint8_t _byte, bool wait_for_complement = true);
    //Send a block
    bool send(uint8_t command, uint8_t* parameters = nullptr, uint8_t parameter_count = 0);
    
    //Reconnect in case of a timeout error
    void error_function();
    
    //Show debug information in the serial monitor
    void show_debug_info(DEBUG_TYPE type);
    
    //Describe the received command in a block
    void show_command_description(uint8_t command);
    
    //Helper functions
    static bool is_long_block(uint8_t formula);
    static double ToSigned(double MW);
    static double ToSigned(double NW, double MW);
    static double To16Bit(double NW, double MW);
    static int compare_keyed_structs(const void *a, const void *b);
};

#endif
