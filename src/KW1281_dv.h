#ifndef KW1281_dv_h
#define KW1281_dv_h

#include <inttypes.h>
#include <Arduino.h>

#include "NewSoftwareSerial.h"

// #define DEBUG
// #define DEBUG_SHOW_SENT
// #define DEBUG_SHOW_RECEIVED
// #define DEBUG_SHOW_CURRENT_FUNCTION
// #define DEBUG_SHOW_GROUP_VALUES_RAW
// #define DEBUG_SHOW_ERRORS
// #define DEBUG_SHOW_INTRO_ID

#define KWP_READ_IDENT          '\x00'  /* Read Identification                   */
//#define KWP_READ_RAM          '\x01'  /* Read RAM                              */
//#define KWP_READ_ROM_EEPROM   '\x03'  /* Read ROM or EEPROM                    */
//#define KWP_OUTPUT_TESTS      '\x04'  /* Actuator/Output Tests                 */
#define KWP_CLEAR_FAULTS        '\x05'  /* Clear Faults                          */
#define KWP_DISCONNECT          '\x06'  /* Disconnect                            */
#define KWP_READ_FAULTS         '\x07'  /* Read Faults                           */
//#define KWP_SINGLE_READING    '\x08'  /* Single Reading                        */
#define KWP_ACK                 '\x09'  /* Request or Response to Acknowledge    */
#define KWP_NAK                 '\x0A'  /* Request or Response to No Acknowledge */
//#define KWP_WRITE_EEPROM      '\x0C'  /* Write EEPROM                          */
#define KWP_RECODING            '\x10'  /* RECODING                              */
//#define KWP_READ_EEPROM       '\x19'  /* Read EEPROM                           */
#define KWP_ADAPTATION          '\x21'  /* Adaptation                            */
#define KWP_TEST_ADAPTATION     '\x22'  /* Test Adaptation                       */
//#define KWP_BASIC_SETTING     '\x28'  /* Basic Setting                         */
#define KWP_GROUP_READING       '\x29'  /* Group Reading                         */
#define KWP_SAVE_ADAPTATION     '\x2A'  /* Save Adaptation                       */
#define KWP_LOGIN               '\x2B'  /* Login                                 */
#define KWP_R_GROUP_READING     '\xE7'  /* Response to Group Reading             */
#define KWP_R_ADAPTATION        '\xE6'  /* Response to Adaptation                */
//#define KWP_R_OUTPUT_TEST     '\xF5'  /* Response to Actuator/Output Tests     */
#define KWP_R_ASCII_DATA        '\xF6'  /* Response with ASCII Data/ID code      */
//#define KWP_R_WRITE_EEPROM    '\xF9'  /* Response to Write EEPROM              */
#define KWP_R_FAULTS            '\xFC'  /* Response to Read or Clear Faults      */
//#define KWP_R_READ_ROM_EEPROM '\xFD'  /* Response to Read ROM or EEPROM        */
//#define KWP_R_READ_RAM        '\xFE'  /* Response to Read RAM                  */

#define RETURN_SUCCESS                 1
#define RETURN_TIMEOUT_ERROR          -1
#define RETURN_COMPLEMENT_ERROR       -2
#define RETURN_MAXMSGSIZE_ERROR       -3
#define RETURN_BLOCKCOUNT_ERROR       -4
#define RETURN_NULLSIZE_ERROR         -5
#define RETURN_RECEIVE_ERROR          -6
#define RETURN_SEND_ERROR             -7
#define RETURN_UNEXPECTEDANSWER_ERROR -8
#define RETURN_LOGIN_ERROR            -9
#define RETURN_INVALID_ERROR          -10

#define FORMULA_OIL_PRESSURE 37
#define FORMULA_TIME 44
#define FORMULA_TEXT1 16
#define FORMULA_TEXT2 17

#define bitcount 10 //do not change, used in the init sequence

class KW1281_dv {
    using m_cb = void (*)(void); //function pointer for callbacks
    
  public:
    uint8_t currAddr = 0;
    
    KW1281_dv(uint8_t receivePin, uint8_t transmitPin);
    ~KW1281_dv();
    
    void define_reset_function(m_cb functionPointer);
    void define_wait_5baud_function(m_cb functionPointer);
    
    char connect(uint8_t address, int baud, char id[], uint16_t &coding, uint32_t &wsc);
    char connect(uint8_t address, int baud, uint16_t &coding, uint32_t &wsc);
    char connect(uint8_t address, int baud);
    void disconnect();
    char VIN(char vin[]);
    char Login(int code, uint32_t wsc);
    char Coding(uint16_t &coding, uint32_t wsc);
    char readFaults(uint16_t faults[], uint8_t startFrom, uint8_t amount);
    char clearFaults();
    uint16_t readAdaptation(uint8_t channel);
    uint16_t testAdaptation(uint8_t channel, uint16_t value);
    uint16_t Adaptation(uint8_t channel, uint16_t value, uint32_t wsc);
    char MeasBlocks(uint8_t group, float &result1, float &result2, float &result3, float &result4);
    char MeasBlocksWithUnits(uint8_t group, float &result1, float &result2, float &result3, float &result4, uint8_t &t1, uint8_t &t2, uint8_t &t3, uint8_t &t4);
    char SingleReading(uint8_t group, uint8_t index, float &result);
    char SingleReadingWithUnits(uint8_t group, uint8_t index, uint8_t &t, float &result);
    void getUnits(uint8_t formula, char units[]);
    char keepAlive();

  private:
    m_cb res;
    m_cb wt;
    uint8_t _OBD_RX_PIN;
    uint8_t _OBD_TX_PIN;
    NewSoftwareSerial *obd;
    uint8_t blockCounter = 0;
    
    uint8_t errorTimeout = 0;
    uint8_t errorData = 0;
    bool connected = false;
    
    uint8_t dtcCounter = 0;
    
    void bitBang5Baud(uint8_t address);
    
    char obdRead();
    
    void reset();
    char KWPSendBlock(char *sendMessage, int sendMessageSize);
    char KWPReceiveBlock(char receivedMessage[], int maxSize, int &receivedMessageSize);
    char KWPSendAckBlock();
    char KWPReadAscii(char id[], uint16_t &coding, uint32_t &wsc);
    char KWPReadAscii(uint16_t &coding, uint32_t &wsc);
    char KWPReadAscii();
    float calculateResult(uint8_t formula, uint8_t a, uint8_t b);
    
    void if_has_reset();
    void if_wait_5baud();
    void defaultReset();
    void defaultWait();
    
    bool usingCustomReset = false;
    bool usingCustomWait = false;
};

#endif
