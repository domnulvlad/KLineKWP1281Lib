#ifndef KLineKWP1281Lib_H
#define KLineKWP1281Lib_H

//If the following line is commented out, all debug procedures are removed from the library (to save memory):
#define KWP1281_DEBUG_SUPPORTED

//There are some formulas whose units string comes from a large table. If the following line is commented out, that table is removed:
#define KWP1281_TEXT_TABLE_SUPPORTED
//(if disabled, the string given to getMeasurementUnits() will contain "EN_f25" if that formula is encountered)

//If the following line is commented out, fault code elaboration strings are removed from the library (to save memory):
//#define KWP1281_FAULT_CODE_ELABORATION_SUPPORTED
//(if disabled, the string given to getFaultElaboration() will contain "EN_elb")

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "units.h" //measurement unit strings

#ifdef KWP1281_TEXT_TABLE_SUPPORTED
  #include "text_table.h" //special text table, if enabled
#endif

#ifdef KWP1281_FAULT_CODE_ELABORATION_SUPPORTED
  #include "fault_code_elaboration.h" //fault code elaboration text table, if enabled
#endif

//AVR microcontrollers use 2-byte (word) pointers. Others, such as the ESP32, use 4-byte (dword) pointers.
#if defined(__AVR__)
  #define READ_POINTER_FROM_PROGMEM pgm_read_word_near
#else
  #define READ_POINTER_FROM_PROGMEM pgm_read_dword_near
#endif

class KLineKWP1281Lib
{  
  public:
    ///VARIABLES/TYPES

    //Function pointer types for callbacks
    using callBack_type             = void (*)(void);                                                                        //generic callback (for custom wait functions)
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
    executionStatus recode(uint16_t coding, uint32_t workshop_code = 0);
    
    //Get the module's fault codes
    executionStatus readFaults(uint8_t &amount_of_fault_codes, uint8_t* fault_code_buffer = nullptr, size_t fault_code_buffer_size = 0);
    //Get a fault code from a fault code reading
    static uint16_t getFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
    //Get a fault elaboration code from a fault code reading
    static uint8_t getFaultElaborationCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
    //Get a fault elaboration string from a fault code reading
    static char* getFaultElaboration(bool &is_intermittent, uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size);
    //Clear the module's fault codes
    executionStatus clearFaults();
    
    //Read the value of an adaptation channel
    executionStatus readAdaptation(uint8_t channel, uint16_t &value);
    //Test if a value would work on an adaptation channel
    executionStatus testAdaptation(uint8_t channel, uint16_t value);
    //Change the value of an adaptation channel
    executionStatus adapt(uint8_t channel, uint16_t value, uint32_t workshop_code);
    
    //Perform a basic setting
    executionStatus basicSetting(uint8_t group, uint8_t* basic_setting_buffer = nullptr, size_t basic_setting_buffer_size = 0);
    
    //Read a group measurement
    executionStatus readGroup(uint8_t &amount_of_measurements, uint8_t group, uint8_t* measurement_buffer = nullptr, size_t measurement_buffer_size = 0);
    //Get a measurement's type from a group reading (whether the value or the units is significant)
    static measurementType getMeasurementType(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
    //Get the calculated value of a measurement from a group reading
    static float getMeasurementValue(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
    //Get the units of a measurement from a group reading
    static char* getMeasurementUnits(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size, char* str, size_t string_size);
    
    //Read a chunk of ROM/EEPROM
    executionStatus readROM(uint8_t chunk_size, uint16_t start_address, uint8_t* memory_buffer = nullptr, uint8_t memory_buffer_size = 0);
    
    //Perform output tests
    executionStatus outputTests(uint16_t &returned_ID);
    
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
    static const uint8_t KWP_REQUEST_READ_ROM        = 0x03; //response: KWP_RECEIVE_ROM (ok) / KWP_REFUSE (reading ROM not supported or invalid parameters)
    static const uint8_t KWP_REQUEST_OUTPUT_TEST     = 0x04; //response: KWP_RECEIVE_OUTPUT_TEST (ok) / KWP_REFUSE (output tests not supported)
    static const uint8_t KWP_REQUEST_BASIC_SETTING   = 0x28; //response: KWP_RECEIVE_BASIC_SETTING (ok) / KWP_ACKNOWLEDGE (empty group) / KWP_REFUSE (invalid channel or not supported)
    
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
    RETURN_TYPE receive(uint8_t* buffer, size_t buffer_size);
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
    
    /*
      Fault elaboration code: 7-bit + 1 bit; high-bit=Intermittent
        00 - -
        01 - Signal Shorted to Plus
        02 - Signal Shorted to Ground
        03 - No Signal
        04 - Mechanical Malfunction
        05 - Input Open
        06 - Signal too High
        07 - Signal too Low
        08 - Control Limit Surpassed
        09 - Adaptation Limit Surpassed
        10 - Adaptation Limit Not Reached
        11 - Control Limit Not Reached
        12 - Adaptation Limit (Mul) Exceeded
        13 - Adaptation Limit (Mul) Not Reached
        14 - Adaptation Limit (Add) Exceeded
        14 - Adaptation Limit (Add) Exceeded
        15 - Adaptation Limit (Add) Not Reached
        16 - Signal Outside Specifications
        17 - Control Difference
        18 - Upper Limit
        19 - Lower Limit
        20 - Malfunction in Basic Setting
        21 - Front Pressure Build-up Time too Long
        22 - Front Pressure Reducing Time too Long
        23 - Rear Pressure Build-up Time too Long
        24 - Rear Pressure Reducing Time too Long
        25 - Unknown Switch Condition
        26 - Output Open
        27 - Implausible Signal
        28 - Short to Plus
        29 - Short to Ground
        30 - Open or Short to Plus
        31 - Open or Short to Ground
        32 - Resistance Too High
        33 - Resistance Too Low
        34 - No Elaboration Available
        35 - -
        36 - Open Circuit
        37 - Faulty
        38 - Output won't Switch or Short to Plus
        39 - Output won't Switch or Short to Ground
        40 - Short to Another Output
        41 - Blocked or No Voltage
        42 - Speed Deviation too High
        43 - Closed
        44 - Short Circuit
        45 - Connector
        46 - Leaking
        47 - No Communications or Incorrectly Connected
        48 - Supply voltage
        49 - No Communications
        50 - Setting (Early) Not Reached
        51 - Setting (Late) Not Reached
        52 - Supply Voltage Too High
        53 - Supply Voltage Too Low
        54 - Incorrectly Equipped
        55 - Adaptation Not Successful
        56 - In Limp-Home Mode
        57 - Electric Circuit Failure
        58 - Can't Lock
        59 - Can't Unlock
        60 - Won't Safe
        61 - Won't De-Safe
        62 - No or Incorrect Adjustment
        63 - Temperature Shut-Down
        64 - Not Currently Testable
        65 - Unauthorized
        66 - Not Matched
        67 - Set-Point Not Reached
        68 - Cylinder 1
        69 - Cylinder 2
        70 - Cylinder 3
        71 - Cylinder 4
        72 - Cylinder 5
        73 - Cylinder 6
        74 - Cylinder 7
        75 - Cylinder 8
        76 - Terminal 30 missing
        77 - Internal Supply Voltage
        78 - Missing Messages
        79 - Please Check Fault Codes
        80 - Single-Wire Operation
        81 - Open
        82 - Activated
    */
};

#endif
