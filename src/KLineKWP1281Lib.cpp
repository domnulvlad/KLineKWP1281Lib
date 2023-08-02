#include "KLineKWP1281Lib.h"

/**
  Function:
    KLineKWP1281Lib(beginFunction, endFunction, sendFunction, receiveFunction, tx_pin, full_duplex, (debug_port))
  
  Parameters:
    beginFunction   -> [void (unsigned long baud)] function to begin communication on the serial port
    endFunction     -> [void                   ()] function to end communication on the serial port
    sendFunction    -> [void       (uint8_t data)] function to write a byte to the serial port
    receiveFunction -> [bool      (uint8_t &data)] function to receive a byte from the serial port
    tx_pin          -> digital pin, coincides with the selected serial port's transmit pin
    full_duplex     -> flag indicating whether or not the selected serial interface can also read while sending
    (debug_port)    -> optionally provide the address of a HardwareSerial object (e.g. &Serial) to send debug information on
  
  Description:
    Creates an instance of the library.
  
  Notes:
    *The return type and parameters taken by each callback function are explained above in brackets.
    *The receiveFunction must return false if no byte is available. If a byte is available, it is stored in the byte passed as reference and it returns true.
    *If, for some reason, the K-line does not echo back the bytes received, full_duplex should be set to false even if the serial interface is capable of
    receiving while sending.
  
  Callback function examples (using hardware serial port Serial1):
    ||void beginFunction(unsigned long baud) {
    ||  Serial1.begin(baud);
    ||}
    ||
    ||void endFunction() {
    ||  Serial1.end();
    ||}
    ||
    ||void sendFunction(uint8_t data) {
    ||  Serial1.write(data);
    ||}
    ||
    ||bool receiveFunction(uint8_t &data) {
    ||  if (Serial1.available()) {
    ||    data = Serial1.read();
    ||    return true;
    ||  }
    ||  return false;
    ||}
*/
KLineKWP1281Lib::KLineKWP1281Lib(beginFunction_type beginFunction, endFunction_type endFunction, sendFunction_type sendFunction, receiveFunction_type receiveFunction, uint8_t tx_pin, bool full_duplex, Stream* debug_port) :
  //Use the initializer list to set the private variables.
  _beginFunction    (beginFunction),
  _endFunction      (endFunction),
  _sendFunction     (sendFunction),
  _receiveFunction  (receiveFunction),
  _tx_pin           (tx_pin),
  _full_duplex      (full_duplex),
  _debug_port       (debug_port)
{}

/**
  Function:
    KWP1281debugFunction(KWP1281debugFunction_type debug_function)
  
  Parameters:
    debug_function -> function to execute after sending/receiving a KWP1281 message, to show it
  
  Description:
    Attaches a function to be called for debugging KWP1281 messages.
    
  Notes:
    *The function given here must return void and take the following parameters in the following order:
      - bool type        -> true for inbound frames (received), false for outbound (sent)
      - uint8_t sequence -> message sequence
      - uint8_t command  -> message type
      - uint8_t* data    -> message parameters (uint8_t pointer = byte array)
      - uint16_t length  -> amount of message parameters (amount of bytes in the data array)
    
    *For example, the function may be defined as such:
      ||void KWP1281debugFunction(bool type, uint8_t sequence, uint8_t command, uint8_t* data, uint8_t length) {
      ||  Serial.println();
      ||
      ||  Serial.println(type ? "RECEIVE:" : "SEND:");
      ||  
      ||  Serial.print("*command: ");
      ||  if (command < 0x10) Serial.print(0);
      ||  Serial.println(command, HEX);
      ||  
      ||  Serial.print("*sequence: ");
      ||  if (sequence < 0x10) Serial.print(0);
      ||  Serial.println(sequence, HEX);
      ||  
      ||  if (length) {
      ||    Serial.print("*data bytes: ");
      ||    Serial.println(length);
      ||  
      ||    Serial.print("*data: ");
      ||    for (uint16_t i = 0; i < length; i++) { //iterate through the message's contents
      ||      if (data[i] < 0x10) Serial.print(0);  //print a leading 0 where necessary to display 2-digit HEX
      ||      Serial.print(data[i], HEX);           //print the byte in HEX
      ||      Serial.print(' ');
      ||    }
      ||    Serial.println();
      ||  }
      ||}
    
    *Then, it will be attached like:
      ||instanceName.KWP1281debugFunction(KWP1281debugFunction);
*/
void KLineKWP1281Lib::KWP1281debugFunction(KWP1281debugFunction_type debug_function)
{
  _debugFunction = debug_function;
}

/**
  Function:
    custom5baudWaitFunction(callBack_type function)
  
  Parameters:
    function -> routine to execute after sending each bit during the 5-baud-init procedure
  
  Description:
    Attaches a custom function to be executed while initializing the control module.
  
  Notes:
    *To achieve a baud rate of 5, the custom function must take 200 milliseconds.
    *It will be called a total of 10 times during initialization.
    *If no custom function is defined, a regular delay of 200ms will be used.
*/
void KLineKWP1281Lib::custom5baudWaitFunction(callBack_type function)
{
  custom_5baud_wait_function_pointer = function;
}

/**
  Function:
    customErrorFunction(callBack_type function)
  
  Parameters:
    function -> routine to execute when a communication error occurs
  
  Description:
    Attaches a custom function to be executed if an error occurs.
  
  Notes:
    *It will be called once, after a timeout occurs while waiting for data.
    *If no custom function is defined, the library will attempt to reconnect to the previously connected module.
*/
void KLineKWP1281Lib::customErrorFunction(callBack_type function)
{
  custom_error_function_pointer = function;
}

/**
  Function:
    attemptConnect(uint8_t module, unsigned long baud_rate)
  
  Parameters:
    module -> logical ID of the target module (engine-0x01, cluster-0x17 etc.)
    baud_rate -> communication speed (10400, 9600, 4800 etc.)
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Attempts to connect to a module.
  
  Notes:
    *The module's baud rate must be known beforehand. Trying to connect at an incorrect speed will always fail.
    *If a blocking delay of one second (if the connection fails) is not a problem, use connect(), which will run until successful.
    *To run custom code if a connection attempt fails, use attemptConnect() and check its return value.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::attemptConnect(uint8_t module, unsigned long baud_rate)
{
  //If there is a module connected, disconnect from it first.
  if (_current_module) {
    disconnect();
  }
  
  //Wake the target module up.
  bitbang_5baud(module);
  
  //Initialize the serial port at the requested speed.
  _beginFunction(baud_rate);
  
  //Set the timeout to the appropriate value for receiving an initialization response (protocol parameters).
  _timeout = initResponseTimeout;
  
  //If initialization is successful, the module will send exactly 3 bytes (0x55 = sync, then 0x01 0x8A or 0x01 0x0A = KWP1281 designator).
  
  //Try to receive the sync byte without sending a complement, or exit if receiving times out.
  if (read_byte(_receive_byte, false) == ERROR_TIMEOUT) {
    return ERROR;
  }
  
  //If the sync byte was received, try to receive the remaining two bytes of the protocol parameters.
  if (_receive_byte == 0x55) {
    #ifdef KWP1281_DEBUG_SUPPORTED
      if (_debug_port) {
        _debug_port->println(F("Got sync byte"));
      }
    #endif
    
    //Try to read the two bytes of protocol identification, or exit if it fails (times out).
    if (!receive_parameters(_parameter_buffer)) {
      show_debug_info(NO_RESPONSE);
      return ERROR;
    }
    
    //Send a complement to the last byte after a delay.
    delay(initComplementDelay);
    send_complement(_parameter_buffer[1]);
    
    //Determine the protocol.
    uint16_t protocol = ((_parameter_buffer[1] & 0x7F) << 7) | _parameter_buffer[0];
    
    #ifdef KWP1281_DEBUG_SUPPORTED
      if (_debug_port) {
        _debug_port->print(F("Protocol="));
        _debug_port->println(protocol);
      }
    #endif
      
    if (protocol != 1281) {
      show_debug_info(INCORRECT_PROTOCOL);
    }
    
    show_debug_info(CONNECTED);
    
    //Save the current parameters for reconnecting in case of an error.
    _current_baud_rate = baud_rate;
    _current_module = module;
    
    //After initialization, before sending its identification strings (part number etc.), the module might send these protocol bytes again.
    //This special case will be handled in the first receive() function called by read_identification().
    _may_send_protocol_parameters_again = true;
    
    //Set the timeout to the appropriate value for receiving regular responses.
    _timeout = responseTimeout;
    
    //Populate the private struct with the module's identification.
    return read_identification();
  }
  //If something other than the sync byte was received, the baud rate may be incorrect.
  else {
    show_debug_info(CHECK_BAUD);
    return ERROR;
  }
}

/**
  Function:
    connect(uint8_t module, unsigned long baud_rate)
  
  Parameters:
    module -> logical ID of the target module (engine-0x01, cluster-0x17 etc.)
    baud_rate -> communication speed (10400, 9600, 4800 etc.)
  
  Description:
    Connects to a module.
  
  Notes:
    *If establishing the connection fails, this function waits one second before retrying (blocking).
    *To avoid the blocking delay, use attemptConnect() and check its return value in order to execute custom code in case of failure.
    *If the connection cannot be established (i.e. the target module is unavailable), it will run indefinitely.
*/
void KLineKWP1281Lib::connect(uint8_t module, unsigned long baud_rate)
{
  while (attemptConnect(module, baud_rate) != SUCCESS) {
    //If connection fails, wait one second before retying.
    delay(1000);
  }
}

/**
  Function:
    update()
  
  Description:
    Maintains the connection to the control module.
  
  Notes:
    *After ~500ms of inactivity without update(), the connection breaks.
*/
void KLineKWP1281Lib::update()
{
  //To maintain the connection, send an acknowledgement, to which the module also sends an acknowledgement.
  if (!acknowledge()) {
    show_debug_info(SEND_ERROR);
  }
  
  //Check the response.
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_ACK:
      break;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      break;
  }
}

/**
  Function:
    disconnect()
  
  Description:
    Disconnects from the control module.
*/
void KLineKWP1281Lib::disconnect()
{
  //Request to disconnect.
  send(KWP_DISCONNECT);
  
  //Indicate that there is no module connected anymore, so the following read will not trigger the error function if it times out.
  _current_module = 0;
  
  show_debug_info(DISCONNECT_INFO);
  
  //Some modules may send a "refuse" block, don't leave it un-complemented.
  receive(_receive_buffer, sizeof(_receive_buffer));
  
  //Clear the strings in the module identification struct.
  memset(identification_data.part_number, 0, sizeof(identification_data.part_number));
  memset(identification_data.identification, 0, sizeof(identification_data.identification));
  memset(identification_data.extra_identification, 0, sizeof(identification_data.extra_identification));
}

/**
  Function:
    getPartNumber()
  
  Returns:
    char* -> pointer to a character array
  
  Description:
    Provides a string containing the module's VAG part number.
*/
char* KLineKWP1281Lib::getPartNumber()
{
  return identification_data.part_number;
}

/**
  Function:
    getIdentification()
  
  Returns:
    char* -> pointer to a character array
  
  Description:
    Provides a string containing the module's model name.
*/
char* KLineKWP1281Lib::getIdentification()
{
  return identification_data.identification;
}

/**
  Function:
    getExtraIdentification()
  
  Returns:
    char* -> pointer to a character array
  
  Description:
    Provides a string containing the module's extra identification (such as a VIN).
  
  Notes:
    *If the module does not have any extra identification, the string will have a null size.
*/
char* KLineKWP1281Lib::getExtraIdentification()
{
  return identification_data.extra_identification;
}

/**
  Function:
    getCoding()
  
  Returns:
    uint16_t -> the coding value
  
  Description:
    Provides the module's coding value.
*/
uint16_t KLineKWP1281Lib::getCoding()
{
  return identification_data.coding;
}

/**
  Function:
    getWorkshopCode()
  
  Returns:
    uint16_t -> the workshop code
  
  Description:
    Provides the module's WSC.
*/
uint32_t KLineKWP1281Lib::getWorkshopCode()
{
  return identification_data.workshop_code;
}

/**
  Function:
    login(uint16_t login_code, uint32_t workshop_code)
  
  Parameters:
    login_code -> the login code
    workshop_code -> the CURRENT workshop code
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Performs a login operation.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::login(uint16_t login_code, uint32_t workshop_code)
{
  uint8_t login_code_high_byte = login_code >> 8;
  uint8_t login_code_low_byte = login_code & 0xFF;
  
  uint8_t workshop_code_top_bit = (workshop_code >> 16) & 0x01;
  uint8_t workshop_code_high_byte = (workshop_code >> 8) & 0xFF;
  uint8_t workshop_code_low_byte = workshop_code & 0xFF;
  
  uint8_t parameters[] = {login_code_high_byte, login_code_low_byte, workshop_code_top_bit, workshop_code_high_byte, workshop_code_low_byte};
  if (!send(KWP_REQUEST_LOGIN, parameters, sizeof(parameters))) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(LOGIN_NOT_ACCEPTED);
      return FAIL;
    
    case TYPE_ACK:
      show_debug_info(LOGIN_ACCEPTED);
      return SUCCESS;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    recode(uint16_t coding, uint32_t workshop_code)
  
  Parameters:
    coding -> the desired coding
    workshop_code -> the desired workshop code
  
  Default parameters:
    workshop_code = 0 (00000)
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Recodes the module.
  
  Notes:
    *The new workshop code may only be applied if currently logged in with the previous code.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::recode(uint16_t coding, uint32_t workshop_code)
{
  //The coding value is shifted left by 1.
  coding <<= 1;
  
  uint8_t coding_high_byte = coding >> 8;
  uint8_t coding_low_byte_workshop_code_top_bit = (coding & 0xFF) | ((workshop_code >> 16) & 0x01);
  uint8_t workshop_code_high_byte = (workshop_code >> 8) & 0xFF;
  uint8_t workshop_code_low_byte = workshop_code & 0xFF;
  
  uint8_t parameters[] = {coding_high_byte, coding_low_byte_workshop_code_top_bit, workshop_code_high_byte, workshop_code_low_byte};
  if (!send(KWP_REQUEST_RECODE, parameters, sizeof(parameters))) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  //After recoding, the module automatically sends its identification like when initializing.
  return read_identification();
}

/**
  Function:
    readFaults(uint8_t &amount_of_fault_codes, uint8_t fault_code_buffer[], size_t fault_code_buffer_size)
  
  Parameters:
    amount_of_fault_codes  -> will contain the total number of fault codes
    fault_code_buffer[]    -> array into which to store the fault codes
    fault_code_buffer_size -> total (maximum) length of the given array
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Reads the module's fault codes.
  
  Notes:
    *The DTCs are stored in the following order: DTC_H, DTC_L, DTC_ELABORATION.
    *The functions getFaultCode() and getFaultElaborationCode() can be used to easily get the fault+elaboration codes stored in the buffer by readFaults().
    *getFaultCode() and getFaultElaborationCode() are static functions so they do not require an instance to be used (useful in multi-instance applications).
    
    *A possible implementation for displaying all fault+elaboration codes, after the buffer is filled by readFaults(), would be:
      ||//Navigate the list of fault codes.
      ||for (uint8_t i = 0; i < amount_of_fault_codes; i++) {
      ||  //Get the "i"-th fault code and elaboration code from the buffer.
      ||  //The "buffer" parameter represents the same array that was given to readFaults().
      ||  uint16_t DTC = KLineKWP1281Lib::getFaultCode(i, amount_of_fault_codes, buffer, sizeof(buffer));
      ||  uint8_t DTC_elaboration = KLineKWP1281Lib::getFaultElaborationCode(i, amount_of_fault_codes, buffer, sizeof(buffer));
      ||  
      ||  //Declare a character array and use it to store the fault code.
      ||  char DTC_string[8];
      ||  sprintf(DTC_string, "%05d", DTC); //pad with zeroes to reach 5 characters
      ||
      ||  //Declare a character array and use it to store the elaboration code.
      ||  char DTC_elaboration_string[8];
      ||  sprintf(DTC_elaboration_string, "%02d", DTC_elaboration); //pad with zeroes to reach 2 characters
      ||  
      ||  //Print the fault and elaboration code.
      ||  Serial.print(DTC_string);
      ||  Serial.print(" - ");
      ||  Serial.println(DTC_elaboration_string);
      ||
      ||  //Declare a character array and use it to store the elaboration string.
      ||  char elaboration_string[32];
      ||  
      ||  //Declare a bool that indicates whether or not the fault is intermittent.
      ||  bool is_intermittent;
      ||  
      ||  //Get the elaboration string.
      ||  KLineKWP1281Lib::getFaultElaboration(is_intermittent, i, amount_of_fault_codes, buffer, sizeof(buffer), elaboration_string, sizeof(elaboration_string));
      ||  
      ||  //Print the elaboration.
      ||  Serial.print(elaboration_string);
      ||
      ||  if (is_intermittent) {
      ||    Serial.print(" - Intermittent");
      ||  }
      ||  
      ||  Serial.println();
      ||}
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readFaults(uint8_t &amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size)
{
  if (!send(KWP_REQUEST_FAULT_CODES)) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  uint8_t faults = 0; //will contain the total amount of fault codes
  uint8_t fault_buffer_index = 0; //for saving fault codes in the array
  uint8_t fault_codes_in_current_block; //will contain the amount of fault codes in the currently received message
  uint8_t max_fault_codes_in_array = fault_code_buffer_size / 3; //how many fault codes would fit in the array
  
  //Loop until no more fault codes can be read.
  while (true) {
    switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
      case TYPE_REFUSE:
        show_debug_info(FAULT_CODES_NOT_SUPPORTED);
        return FAIL;
      
      case TYPE_ACK:
        //Store how many DTCs have been read after receiving the last block.
        amount_of_fault_codes = faults;
        return SUCCESS;
      
      case TYPE_FAULT_CODES:
      {
        //Determine how many fault codes have just been received.
        fault_codes_in_current_block = _receive_buffer[0] / 3;
        
        //A single stored DTC, with code 0xFFFF and elaboration 0x88, means no DTCs present.
        if (fault_codes_in_current_block == 1) {
          if (_receive_buffer[1] == 0xFF && _receive_buffer[2] == 0xFF) {
            if (_receive_buffer[3] == 0x88) {
              show_debug_info(NO_FAULT_CODES);
              amount_of_fault_codes = 0;
              return SUCCESS;
            }
          }
        }
        
        //If fault codes were received, increment the counter.
        faults += fault_codes_in_current_block;
        
        if (fault_codes_in_current_block > max_fault_codes_in_array) {
          show_debug_info(ARRAY_NOT_LARGE_ENOUGH);
        }
        
        //Calculate how many fault codes will be copied in the given array (how many will fit).
        uint8_t fault_codes_to_copy = min(fault_codes_in_current_block, max_fault_codes_in_array);
      
        //Each code takes up 3 bytes.
        memcpy(fault_code_buffer + fault_buffer_index, _receive_buffer + 1, fault_codes_to_copy * 3);
        
        //Increment the buffer index by how many bytes were copied.
        fault_buffer_index += fault_codes_to_copy * 3;
        
        //Decrement the available space in the array.
        max_fault_codes_in_array -= fault_codes_to_copy;
        
        //Confirm receipt and request the next response.
        acknowledge();
        break;
      }
      
      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }
  }
}

/**
  Function:
    getFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t fault_code_buffer[], size_t fault_code_buffer_size)
  
  Parameters:
    fault_code_index       -> index of the fault code that needs to be retrieved
    amount_of_fault_codes  -> total number of fault codes stored in the array (value passed as reference to readFaults())
    fault_code_buffer[]    -> array in which fault codes have been stored by readFaults()
    fault_code_buffer_size -> total size of the given array (provided with the sizeof() operator)
  
  Returns:
    uint16_t -> fault code
  
  Description:
    Provides a fault code from a buffer filled by readFaults().
  
  Notes:
    *It is a static function so it does not require an instance to be used (useful in multi-instance applications).
*/
uint16_t KLineKWP1281Lib::getFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size)
{
  //Check if the array is large enough to contain as many fault codes as "declared" by the value given.
  if (fault_code_buffer_size < amount_of_fault_codes * 3) {
    amount_of_fault_codes = fault_code_buffer_size / 3;
  }
  
  //If the requested index is out of range (or the buffer doesn't contain fault codes), return 0.
  if (fault_code_index >= amount_of_fault_codes) {
    return 0;
  }
  
  //Combine the two bytes to form the code and return it.
  uint16_t fault_code = (fault_code_buffer[3 * fault_code_index] << 8) | fault_code_buffer[3 * fault_code_index + 1];
  return fault_code;
}

/**
  Function:
    getFaultElaborationCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t fault_code_buffer[], size_t fault_code_buffer_size)
  
  Parameters:
    fault_code_index       -> index of the fault code whose elaboration code needs to be retrieved
    amount_of_fault_codes  -> total number of fault codes stored in the array (value passed as reference to readFaults())
    fault_code_buffer[]    -> array in which fault codes have been stored by readFaults()
    fault_code_buffer_size -> total size of the given array (provided with the sizeof() operator)
  
  Returns:
    uint8_t -> fault elaboration code
  
  Description:
    Provides a fault elaboration code from a buffer filled by readFaults().
  
  Notes:
    *It is a static function so it does not require an instance to be used (useful in multi-instance applications).
*/
uint8_t KLineKWP1281Lib::getFaultElaborationCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size)
{
  //Check if the array is large enough to contain as many fault codes as "declared" by the value given.
  if (fault_code_buffer_size < amount_of_fault_codes * 3) {
    amount_of_fault_codes = fault_code_buffer_size / 3;
  }
  
  //If the requested index is out of range (or the buffer doesn't contain fault codes), return 0.
  if (fault_code_index >= amount_of_fault_codes) {    
    return 0;
  }
  
  //Return the elaboration code.
  return fault_code_buffer[3 * fault_code_index + 2];
}


/**
  Function:
    getFaultElaboration(bool &is_intermittent, uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size)
  
  Parameters:
    is_intermittent        -> whether or not the elaboration code indicated that the fault is "Intermittent"
    fault_code_index       -> index of the fault code whose elaboration needs to be retrieved
    amount_of_fault_codes  -> total number of fault codes stored in the array (value passed as reference to readFaults())
    fault_code_buffer[]    -> array in which fault codes have been stored by readFaults()
    fault_code_buffer_size -> total size of the given array (provided with the sizeof() operator)
    str[]                  -> string (character array) into which to copy the elaboration string
    string_size            -> total size of the given array (provided with the sizeof() operator)
  
  Returns:
    char* -> the same character array provided (str), to be able to use it like "Serial.println(getFaultElaboration(...))"
  
  Description:
    Provides a fault elaboration string from a buffer filled by readFaults().
  
  Notes:
    *It is a static function so it does not require an instance to be used (useful in multi-instance applications).
*/
char* KLineKWP1281Lib::getFaultElaboration(bool &is_intermittent, uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size)
{
  //Determine the elaboration code.
  uint8_t elaboration_code = getFaultElaborationCode(fault_code_index, amount_of_fault_codes, fault_code_buffer, fault_code_buffer_size);
  
  //The fault is considered "Intermittent" if the highest bit in the elaboration code is set.
  is_intermittent = elaboration_code & 0x80;
  
  //Ensure the high bit is not set.
  elaboration_code &= ~0x80;
  
  //If the feature is enabled, copy the elaboration into the given string.
#ifdef KWP1281_FAULT_CODE_ELABORATION_SUPPORTED
  //If the elaboration code is valid, copy its string into the given array.
  if (elaboration_code < (sizeof(fault_elaborations) / sizeof(fault_elaborations[0]))) {
    //Copy the elaboration string into the given array.
    strncpy_P(str, (const char*)READ_POINTER_FROM_PROGMEM(fault_elaborations + elaboration_code), string_size);
    
    //Ensure the string is null-terminated.
    if (string_size) {
      str[string_size - 1] = '\0';
    }
  }
  
  //Otherwise, clear the string by writing a null on the first position.
  else {
    if (string_size) {
      str[0] = '\0';
    }
  }
#else
  //Otherwise, copy the "warning" into the given string.
  strncpy(str, "EN_elb", string_size);
  
  //Ensure the string is null-terminated.
  if (string_size) {
    str[string_size - 1] = '\0';
  }
#endif
  
  //Return a pointer to the given array.
  return str;
}

/**
  Function:
    clearFaults()
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Clears the module's diagnostic trouble codes.
  
  Notes:
    *Run readFaults() afterwards to check if it was effective.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::clearFaults()
{
  if (!send(KWP_REQUEST_CLEAR_FAULTS)) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(CLEARING_FAULT_CODES_NOT_SUPPORTED);
      return FAIL;
    
    case TYPE_ACK:
      show_debug_info(CLEARING_FAULT_CODES_ACCEPTED);
      return SUCCESS;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    readAdaptation(uint8_t channel, uint16_t &value)
  
  Parameters:
    channel -> adaptation channel to read
    value   -> will store the value read from the channel
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Reads the value of an adaptation channel.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readAdaptation(uint8_t channel, uint16_t &value)
{
  uint8_t parameters[] = {channel};
  if (!send(KWP_REQUEST_ADAPTATION, parameters, sizeof(parameters))) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(INVALID_ADAPTATION_CHANNEL);
      return FAIL;
    
    case TYPE_ADAPTATION:
      show_debug_info(ADAPTATION_RECEIVED);
      
      //Combine the two bytes to get the adaptation value.
      value = (_receive_buffer[2] << 8) | _receive_buffer[3];
      return SUCCESS;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    testAdaptation(uint8_t channel, uint16_t value)
  
  Parameters:
    channel -> adaptation channel to test the value on
    value   -> value to test
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Tests whether or not a value would work for an adaptation channel.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::testAdaptation(uint8_t channel, uint16_t value)
{
  uint8_t value_high_byte = value >> 8;
  uint8_t value_low_byte = value & 0xFF;
  
  uint8_t parameters[] = {channel, value_high_byte, value_low_byte};
  if (!send(KWP_REQUEST_ADAPTATION_TEST, parameters, sizeof(parameters))) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(INVALID_ADAPTATION_CHANNEL);
      return FAIL;
    
    case TYPE_ADAPTATION:
      if (uint16_t((_receive_buffer[2] << 8) | _receive_buffer[3]) == value) {
        show_debug_info(ADAPTATION_ACCEPTED);
        return SUCCESS;
      }
      else {
        show_debug_info(ADAPTATION_NOT_ACCEPTED);
        return FAIL;
      }
      break;
      
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    adapt(uint8_t channel, uint16_t value, uint32_t workshop_code)
  
  Parameters:
    channel       -> adaptation channel to modify
    value         -> value to store in the channel
    workshop_code -> WSC to use
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Changes the value of an adaptation channel.
  
  Notes:
    *Use testAdaptation() before adapt(), to ensure it's possible to use the value on the channel.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::adapt(uint8_t channel, uint16_t value, uint32_t workshop_code)
{
  uint8_t value_high_byte = value >> 8;
  uint8_t value_low_byte = value & 0xFF;
  
  uint8_t workshop_code_top_bit = (workshop_code >> 16) & 0x01;
  uint8_t workshop_code_high_byte = (workshop_code >> 8) & 0xFF;
  uint8_t workshop_code_low_byte = workshop_code & 0xFF;
  
  uint8_t parameters[] = {channel, value_high_byte, value_low_byte, workshop_code_top_bit, workshop_code_high_byte, workshop_code_low_byte};
  if (!send(KWP_REQUEST_ADAPTATION_SAVE, parameters, sizeof(parameters))) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(INVALID_ADAPTATION_CHANNEL_OR_VALUE);
      return FAIL;
    
    case TYPE_ADAPTATION:
      show_debug_info(ADAPTATION_RECEIVED);
      return SUCCESS;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    basicSetting(uint8_t group, uint8_t basic_setting_buffer[], size_t basic_setting_buffer_size)
  
  Parameters:
    amount_of_values          -> will contain the total number of values read from the basic setting group
    group                     -> basic setting group to activate/read
    basic_setting_buffer[]    -> array into which to store the received values
    basic_setting_buffer_size -> total (maximum) length of the given array
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Performs a basic setting.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::basicSetting(uint8_t &amount_of_values, uint8_t group, uint8_t* basic_setting_buffer, size_t basic_setting_buffer_size)
{
  if (group) {
    uint8_t parameters[] = {group};
    if (!send(KWP_REQUEST_BASIC_SETTING, parameters, sizeof(parameters))) {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }
  else {
    if (!send(KWP_REQUEST_BASIC_SETTING_0)) {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(INVALID_BASIC_SETTING_GROUP);
      return FAIL;
    
    case TYPE_ACK:
      show_debug_info(RECEIVED_EMPTY_BASIC_SETTING_GROUP);
      return FAIL;
    
    case TYPE_BASIC_SETTING:
      {
        show_debug_info(RECEIVED_BASIC_SETTING);
        
        //Determine how many values were received.
        amount_of_values = _receive_buffer[0];
        
        //Determine how many values to copy into the given buffer.
        uint8_t max_values_in_receive_buffer = sizeof(_receive_buffer); //how many values would fit in the receive buffer
        uint8_t max_values_in_array = basic_setting_buffer_size; //how many values would fit in the given array
        uint8_t values_to_copy = min(amount_of_values, min(max_values_in_receive_buffer, max_values_in_array));
        
        //Copy the values from the RX buffer into the given array.
        memcpy(basic_setting_buffer, _receive_buffer + 1, values_to_copy);
        return SUCCESS;
      }
      break;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    getBasicSettingValue(uint8_t value_index, uint8_t amount_of_values, uint8_t basic_setting_buffer[], size_t basic_setting_buffer_size)
  
  Parameters:
    value_index               -> index of the value that needs to be retrieved
    amount_of_values          -> total number of values stored in the array (value passed as reference to basicSetting())
    basic_setting_buffer[]    -> array in which fault codes have been stored by basicSetting()
    basic_setting_buffer_size -> total size of the given array (provided with the sizeof() operator)
  
  Returns:
    uint8_t -> value
  
  Description:
    Provides a measured value from a buffer filled by basicSetting().
  
  Notes:
    *It is a static function so it does not require an instance to be used (useful in multi-instance applications).
*/
uint8_t KLineKWP1281Lib::getBasicSettingValue(uint8_t value_index, uint8_t amount_of_values, uint8_t* basic_setting_buffer, size_t basic_setting_buffer_size)
{
  //Check if the array is large enough to contain as many values as "declared" by the value given.
  if (basic_setting_buffer_size < amount_of_values) {
    amount_of_values = basic_setting_buffer_size;
  }
  
  //If the requested index is out of range (or the buffer doesn't contain fault codes), return 0.
  if (value_index >= amount_of_values) {
    return 0;
  }
  
  //Determine the value and return it.
  uint8_t value = basic_setting_buffer[value_index];
  return value;
}

/**
  Function:
    readGroup(uint8_t &amount_of_measurements, uint8_t group, uint8_t measurement_buffer[], size_t measurement_buffer_size)
  
  Parameters:
    amount_of_measurements  -> will contain the total number of measurements in the group
    group                   -> measurement group to read
    measurement_buffer[]    -> array into which to store the measurements
    measurement_buffer_size -> total (maximum) length of the given array
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Reads a measurement group.
  
  Notes:
    *The measurements are stored in the following order: FORMULA, BYTE_A, BYTE_B.
    *The getMeasurementValue() function can be used to easily get the calculated values stored in the buffer by readGroup().
    *getMeasurementValue() is a static function so it does not require an instance to be used (useful in multi-instance applications).
    
    *A possible implementation for displaying all measurements from a group would be:
      ||//Create a character array which will hold the measurement's units
      ||char units_string[16];
      ||
      ||//Navigate the list of measurements.
      ||for (uint8_t i = 0; i < amount_of_measurements; i++) {
      ||  //Print the measurement index.
      ||  Serial.print(i);
      ||  Serial.print(": ");
      ||  
      ||  //Print the "i"-th measurement differently based on its type.
      ||  //The "buffer" parameter represents the same array that was given to readGroup().
      ||  switch (KLineKWP1281Lib::getMeasurementType(i, amount_of_measurements, buffer, sizeof(buffer))) {
      ||    case KLineKWP1281Lib::VALUE: //Value and units
      ||      Serial.print(KLineKWP1281Lib::getMeasurementValue(i, amount_of_measurements, buffer, sizeof(buffer)));
      ||      Serial.print(' ');
      ||      Serial.println(KLineKWP1281Lib::getMeasurementUnits(i, amount_of_measurements, buffer, sizeof(buffer), units_string, sizeof(units_string)));
      ||      break;
      ||    
      ||    case KLineKWP1281Lib::UNITS: //Units string containing text
      ||      Serial.println(KLineKWP1281Lib::getMeasurementUnits(i, amount_of_measurements, buffer, sizeof(buffer), units_string, sizeof(units_string)));
      ||      break;
      ||    
      ||    case KLineKWP1281Lib::UNKNOWN: //Invalid measurement index
      ||      Serial.println("N/A");
      ||      break;
      ||  }
      ||}
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readGroup(uint8_t &amount_of_measurements, uint8_t group, uint8_t* measurement_buffer, size_t measurement_buffer_size)
{
  if (group) {
    uint8_t parameters[] = {group};
    if (!send(KWP_REQUEST_GROUP_READING, parameters, sizeof(parameters))) {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }
  else {
    if (!send(KWP_REQUEST_GROUP_READING_0)) {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(INVALID_MEASUREMENT_GROUP);
      return FAIL;
    
    case TYPE_ACK:
      show_debug_info(RECEIVED_EMPTY_GROUP);
      return FAIL;
    
    case TYPE_GROUP_READING:
      {
        show_debug_info(RECEIVED_GROUP);
        
        //Determine how many measurements were received.
        amount_of_measurements = _receive_buffer[0] / 3;
        
        //Determine how many measurements to copy into the given buffer.
        uint8_t max_measurements_in_receive_buffer = sizeof(_receive_buffer)  / 3; //how many measurements would fit in the receive buffer
        uint8_t max_measurements_in_array = measurement_buffer_size  / 3; //how many measurements would fit in the given array
        uint8_t measurements_to_copy = min(amount_of_measurements, min(max_measurements_in_receive_buffer, max_measurements_in_array));
        
        //Copy the measurements from the RX buffer into the given array.
        memcpy(measurement_buffer, _receive_buffer + 1, measurements_to_copy * 3);
      }
      return SUCCESS;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    getMeasurementType(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t measurement_buffer[], size_t measurement_buffer_size)
  
  Parameters:
    measurement_index       -> index of the measurement whose type must be determined (0-4)
    amount_of_measurements  -> total number of measurements stored in the array (value passed as reference to readGroup())
    measurement_buffer[]    -> array in which measurements have been stored by readGroup()
    measurement_buffer_size -> total size of the given array (provided with the sizeof() operator)
  
  Returns:
    measurementType ->
      UNKNOWN - requested measurement outside range
      VALUE   - the value is significant (measurement has value+units)
      UNITS   - the units are significant (human-readable text is contained in the units string)
  
  Description:
    Determines whether the value or the units string is significant for a measurement from a buffer filled by readGroup().
  
  Notes:
    *Even if the measurement is of type UNITS, its value still contains the "origin" of the units string, like a code or 16-bit value so it can be used without
    necessarily checking the units string.
    *For example, for a clock measurement, the units string will contain "hh:mm", but the value will also be hh.mm (hours before decimal, minutes after).
    *It is a static function so it does not require an instance to be used (useful in multi-instance applications).
*/
KLineKWP1281Lib::measurementType KLineKWP1281Lib::getMeasurementType(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size)
{
  //Check if the array is large enough to contain as many measurements as "declared" by the value given.
  if (measurement_buffer_size < amount_of_measurements * 3) {
    amount_of_measurements = measurement_buffer_size / 3;
  }
  
  //If the requested index is out of range (or the buffer doesn't contain measurements), return UNKNOWN.
  if (measurement_index >= amount_of_measurements) {
    return UNKNOWN;
  }
  
  //Determine the formula based on the given index.
  uint8_t formula = measurement_buffer[3 * measurement_index];
  
  //Only a few measurement formulas are of the "UNITS" type.
  switch (formula) {
    case 0x0A:
    case 0x10:
    case 0x11:
    case 0x1D:
    case 0x25:
    case 0x2C:
    case 0x6B:
    case 0x7B:
    case 0x7F:
    case 0x88:
    case 0x8E:
    case 0xA1:
      return UNITS;
    
    default:
      return VALUE;
  }
}

/**
  Function:
    getMeasurementValue(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t measurement_buffer[], size_t measurement_buffer_size)
  
  Parameters:
    measurement_index       -> index of the measurement that needs to be calculated (0-3)
    amount_of_measurements  -> total number of measurements stored in the array (value passed as reference to readGroup())
    measurement_buffer[]    -> array in which measurements have been stored by readGroup()
    measurement_buffer_size -> total size of the given array (provided with the sizeof() operator)
  
  Returns:
    float -> calculated value
  
  Description:
    Calculates the actual value of a measurement from a buffer filled by readGroup().
  
  Notes:
    *It is a static function so it does not require an instance to be used (useful in multi-instance applications).
*/
float KLineKWP1281Lib::getMeasurementValue(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size)
{
  //Check if the array is large enough to contain as many measurements as "declared" by the value given.
  if (measurement_buffer_size < amount_of_measurements * 3) {
    amount_of_measurements = measurement_buffer_size / 3;
  }
  
  //If the requested index is out of range (or the buffer doesn't contain measurements), return 0/0 (nan).
  if (measurement_index >= amount_of_measurements) {
    return 0.0 / 0.0;
  }
  
  //Get the three important values from the buffer based on the requested index.
  uint8_t formula  = measurement_buffer[3 * measurement_index];
  uint8_t a_raw    = measurement_buffer[3 * measurement_index + 1];
  uint8_t b_raw    = measurement_buffer[3 * measurement_index + 2];
  
  //Convert the values to floating point for use in formulas.
  float a = a_raw, b = b_raw;
  
  /*
    Slight inaccuracy on mass flow - 19, 35, 96.
    Text table not yet retrieved - 25, 7B.
    Formula unknown - 48, 6E, 8B, 8C, 8D, 93.
    Meaning unknown - A0.
  */
  
  //Calculate the real value depending on the formula.
  float result = 0;
  switch (formula) {
    //0x00 - doesn't exist
    case 0x01: result = a * b * 0.2;                                                                           break; //RPM                 [/min]
    case 0x02: result = a * b * 0.002;                                                                         break; //Load                [%]
    case 0x03: result = a * b * 0.002;                                                                         break; //Throttle angle      [deg]
    case 0x04: result = a * abs(b - 0x7F) * 0.01;                                                              break; //Ignition angle      [deg]
    case 0x05: result = a * (b - 100) * 0.1;                                                                   break; //Temperature         [degC]
    case 0x06: result = a * b * 0.001;                                                                         break; //Voltage             [V]
    case 0x07: result = a * b * 0.01;                                                                          break; //Speed               [km/h]
    case 0x08: result = a * b * 0.1;                                                                           break; //N/A                 [N/A]
    case 0x09: result = a * (b - 0x7F) * 0.02;                                                                 break; //Steering angle      [deg]
    case 0x0A: result = b ? 1 : 0;                                                                             break; //Text                [Warm/Cold]
    case 0x0B: result = a * (b - 0x80) * 0.0001 + 1;                                                           break; //Lambda factor       [N/A]
    case 0x0C: result = a * b * 0.001;                                                                         break; //Resistance          [Ohm]
    case 0x0D: result = a * (b - 0x7F) * 0.001;                                                                break; //Distance            [mm]
    case 0x0E: result = a * b * 0.005;                                                                         break; //Pressure            [bar]
    case 0x0F: result = a * b * 0.01;                                                                          break; //Injection time      [ms]
    case 0x10: result = b_raw & a_raw;                                                                         break; //Binary bits         [bin(b&a)]
    case 0x11: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //ASCII               [char(a)char(b)]
    case 0x12: result = a * b * 0.04;                                                                          break; //Pressure            [mbar]
    case 0x13: result = a * b * 0.01;                                                                          break; //Fuel volume         [l]
    case 0x14: result = a * (b - 0x80) / 128.0;                                                                break; //Deviation           [%]
    case 0x15: result = a * b * 0.001;                                                                         break; //Voltage             [V]
    case 0x16: result = a * b * 0.001;                                                                         break; //Time                [ms]
    case 0x17: result = a * b / 256.0;                                                                         break; //Duty cycle          [%]
    case 0x18: result = a * b * 0.001;                                                                         break; //Current             [A]
    case 0x19: result = a / 180.0 + b * 1.422;                                                                 break; //Mass flow           [g/s]
    case 0x1A: result = b - a;                                                                                 break; //Temperature         [degC]
    case 0x1B: result = a * abs(b - 0x80) * 0.01;                                                              break; //Ignition angle      [deg]
    case 0x1C: result = b - a;                                                                                 break; //N/A                 [N/A]
    case 0x1D: result = (b < a) ? 1 : 2;                                                                       break; //Map number          [Map1/2]
    case 0x1E: result = a * b / 12.0;                                                                          break; //Knock correction    [deg]
    case 0x1F: result = a * b / 2560.0;                                                                        break; //N/A                 [N/A]
    case 0x20: result = (int8_t)b;                                                                             break; //N/A                 [N/A]
    case 0x21: result = a ? (100.0 * b / float(a)) : 0;                                                        break; //Load                [%]
    case 0x22: result = a * (b - 0x80) * 0.01;                                                                 break; //Idle correction     [KW]
    case 0x23: result = a * b * 0.01;                                                                          break; //Consumption         [l/h]
    case 0x24: result = uint16_t((a_raw << 8) | b_raw) * 10.0;                                                 break; //Mileage             [km]
    case 0x25: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //Text                [strings from table]
    case 0x26: result = a * (b - 0x80) * 0.001;                                                                break; //Segment correction  [degKW]
    case 0x27: result = a * b / 255.0;                                                                         break; //Injection quantity  [mg/h]
    case 0x28: result = a * 25.5 + b * 0.1 - 400.0;                                                            break; //Current             [A]
    case 0x29: result = uint16_t(a * 255 + b);                                                                 break; //Capacity            [Ah]
    case 0x2A: result = ((a * 255.0 + b) - 4000.0) * 0.1;                                                      break; //Power               [kW]
    case 0x2B: result = uint16_t(a * 255 + b) * 0.1;                                                           break; //Battery voltage     [V]
    case 0x2C: result = b / 100.0 + a;                                                                         break; //Clock               [a:b]
    case 0x2D: result = a * b * 0.1;                                                                           break; //Consumption         [l/100km]
    case 0x2E: result = (a - 0x80) * 0.704 + b * 0.00275;                                                      break; //Segment correction  [degKW]
    case 0x2F: result = a * (b - 0x80);                                                                        break; //Time correction     [ms]
    case 0x30: result = uint16_t(a * 255 + b);                                                                 break; //N/A                 [N/A]
    case 0x31: result = a * b / 40.0;                                                                          break; //Air mass per stroke [mg/h]
    case 0x32: result = a ? ((b - 0x80) * 100.0 / a) : 0;                                                      break; //Pressure            [mbar]
    case 0x33: result = a * (b - 0x80) / 255.0;                                                                break; //Injection quantity  [mg/h]
    case 0x34: result = a * b * 0.02 - a;                                                                      break; //Torque              [Nm]
    case 0x35: result = a * 5.58 / 1000.0 + (b - 0x80) * 1.422;                                                break; //Mass flow           [g/s]
    case 0x36: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //Count               [N/A]
    case 0x37: result = a * b / 200.0;                                                                         break; //Time                [s]
    case 0x38: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //Workshop code       [N/A]
    case 0x39: result = uint16_t((a_raw << 8) | b_raw) + 65536;                                                break; //Workshop code       [N/A]
    case 0x3A: result = b * 260.9 / 255.0;                                                                     break; //Misfires            [/s]
    case 0x3B: result = uint16_t((a_raw << 8) | b_raw) * 2.0 / 65535.0;                                        break; //N/A                 [N/A]
    case 0x3C: result = uint16_t((a_raw << 8) | b_raw) * 0.01;                                                 break; //Time                [s]
    case 0x3D: result = a ? ((b - 0x80) / float(a)) : 0;                                                       break; //Difference          [N/A]
    case 0x3E: result = a * b * 0.256;                                                                         break; //Time                [s]
    //0x3F - doesn't exist
    case 0x40: result = a + b;                                                                                 break; //Resistance          [Ohm]
    case 0x41: result = a * (b - 0x7F) * 0.01;                                                                 break; //Distance            [mm]
    case 0x42: result = (a * b) / 512.0;                                                                       break; //Voltage             [V]
    case 0x43: result = ((a_raw % 4) - 4 * (a >= 0x80)) * 640.0 + b * 2.5;                                     break; //Steering angle      [deg]
    case 0x44: result = ((a_raw % 4) - 4 * (a >= 0x80)) * 34.763 + b * 0.1358;                                 break; //Turn rate           [deg/s]
    case 0x45: result = ((a_raw % 4) - 4 * (a >= 0x80)) * 83.33 + b * 0.3254;                                  break; //Pressure            [bar]
    case 0x46: result = ((a_raw % 4) - 4 * (a >= 0x80)) * 49.15 + b * 0.192;                                   break; //Acceleration        [m/s^2]
    case 0x47: result = a * b;                                                                                 break; //Distance            [cm]
    //0x48 - unknown formula (Voltage [V])
    case 0x49: result = a * b * 0.01;                                                                          break; //Resistance          [Ohm]
    case 0x4A: result = a * b * 0.1;                                                                           break; //Time                [months]
    case 0x4B: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //Error code          [N/A]
    case 0x4C: result = uint16_t((a_raw << 8) | b_raw) - a;                                                    break; //Resistance          [kOhm]
    case 0x4D: result = a * 0.062745 + b * 0.0147;                                                             break; //Voltage             [V]
    case 0x4E: result = int8_t(b) * 1.819;                                                                     break; //Misfires            [/s]
    case 0x4F: result = b;                                                                                     break; //Channel number      [N/A]
    case 0x50: result = uint16_t((a_raw << 8) | b_raw) * 0.01;                                                 break; //Resistance          [kOhm]
    case 0x51: result = ((a_raw << 8) | b_raw) * 4375.0 * 0.00001;                                             break; //Steering angle      [deg]
    case 0x52: result = ((a_raw << 8) | b_raw) * 981.0 * 0.00001;                                              break; //Acceleration        [m/s^2]
    case 0x53: result = ((a_raw << 8) | b_raw) * 0.01;                                                         break; //Pressure            [bar]
    case 0x54: result = (b * 973.0 * 0.0001) - ((a < 0x80) ? 0 : 24.909);                                      break; //Acceleration        [m/s^2]
    case 0x55: result = ((a_raw << 8) | b_raw) / 349.0;                                                        break; //Turn rate           [deg/s]
    case 0x56: result = a * b * 0.1;                                                                           break; //Current             [A]
    case 0x57: result = a * (b - 0x80) * 0.1;                                                                  break; //Turn rate           [deg/s]
    case 0x58: result = a * b * 0.01;                                                                          break; //Resistance          [kOhm]
    case 0x59: result = uint16_t(a_raw << 8) | b_raw;                                                          break; //On-time             [h]
    case 0x5A: result = a * b * 0.1;                                                                           break; //Weight              [kg]
    case 0x5B: result = a * (b - 0x80) * 0.1;                                                                  break; //Steering angle      [deg]
    case 0x5C: result = a * b;                                                                                 break; //Mileage             [km]
    case 0x5D: result = a * (b - 0x80) * 0.001;                                                                break; //Torque              [Nm]
    case 0x5E: result = a * (b - 0x80) * 0.1;                                                                  break; //Torque              [Nm]
    //0x5F - shows min(a, length) bytes from the entire contents of block response as ASCII characters
    case 0x60: result = a * b * 0.1;                                                                           break; //Pressure            [mbar]
    case 0x61: result = (b - a) * 5;                                                                           break; //Cat-temperature     [degC]
    case 0x62: result = a * b * 0.1;                                                                           break; //Impulses            [/km]
    case 0x63: result = (a_raw << 8) | b_raw;                                                                  break; //N/A                 [N/A]
    case 0x64: result = a * b * 0.1;                                                                           break; //Pressure            [bar]
    case 0x65: result = a * b * 0.001;                                                                         break; //Fuel level factor   [l/mm]
    case 0x66: result = a * b * 0.1;                                                                           break; //Fuel level          [mm]
    case 0x67: result = b * 5.0 * 0.01 + a;                                                                    break; //Voltage             [V]
    case 0x68: result = a * (b - 0x80) / 5.0;                                                                  break; //Volume              [ml]
    case 0x69: result = a * (b - 0x80) * 0.01;                                                                 break; //Distance            [m]
    case 0x6A: result = a * (b - 0x80) * 0.1;                                                                  break; //Speed               [km/h]
    case 0x6B: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //Hex bytes           [hex(a)hex(b)]
    //0x6C - doesn't exist
    //0x6D - doesn't exist
    //0x6E - unknown formula (Workshop ID [N/A])
    case 0x6F: result = a * 0x10000 + b * 0x100;                                                               break; //Mileage             [km]
    case 0x70: result = a * (b - 0x80) * 0.001;                                                                break; //Angle               [deg]
    case 0x71: result = a * (b - 0x80) * 0.01;                                                                 break; //N/A                 [N/A]
    case 0x72: result = a * (b - 0x80);                                                                        break; //Altitude            [m]
    case 0x73: result = (a_raw << 8) | b_raw;                                                                  break; //Output power        [W]
    case 0x74: result = (a_raw << 8) | b_raw;                                                                  break; //RPM                 [/min]
    case 0x75: result = a * (b - 64) * 0.01;                                                                   break; //Temperature         [degC]
    //0x76 - shows min(a, length) bytes from the entire contents of block response as HEX values
    case 0x77: result = a * b * 0.01;                                                                          break; //ACC-Amplitude       [%]
    case 0x78: result = a * b * 1.411752;                                                                      break; //Angle               [deg]
    case 0x79: result = a * 5.0 * 0.1 + b * 128.0;                                                             break; //N/A                 [N/A]
    case 0x7A: result = a * 0.01 + (b - 0x80) * 256.0 * 0.01;                                                  break; //N/A                 [N/A]
    case 0x7B: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //Text                [strings from table]
    case 0x7C: result = a * b * 0.1;                                                                           break; //Current             [mA]
    case 0x7D: result = a - b;                                                                                 break; //Attenuation         [dB]
    case 0x7E: result = a * b * 0.1;                                                                           break; //N/A                 [N/A]
    case 0x7F: result = 200000 + (b_raw & 0x7F) * 100                                                                 //Date                [yyyy:mm:dd]
                       + (((a_raw & 0x07) << 1) | ((b_raw & 0x80) >> 7))
                       + ((a_raw & 0xF8) >> 3) * 0.01;                                                         break; 
    case 0x80: result = a * b;                                                                                 break; //RPM                 [/min]
    case 0x81: result = a * b / 256.0;                                                                         break; //Load                [%]
    case 0x82: result = a * b / 2560.0;                                                                        break; //Current             [A]
    case 0x83: result = abs(a * b / 2.0 - 30.0);                                                               break; //Ignition angle      [deg]
    case 0x84: result = a * b / 2.0;                                                                           break; //Throttle angle      [deg]
    case 0x85: result = a * b / 256.0;                                                                         break; //Voltage             [V]
    case 0x86: result = a * b;                                                                                 break; //Speed               [km/h]
    case 0x87: result = a * b;                                                                                 break; //N/A                 [N/A]
    case 0x88: result = b_raw & a_raw;                                                                         break; //Binary bits         [bin(b&a)]
    case 0x89: result = a * b * 0.01;                                                                          break; //Injection time      [ms]
    case 0x8A: result = a * b * 0.001;                                                                         break; //Knock sensor        [V]
    //0x8B - unknown formula (RPM [/min])
    //0x8C - unknown formula (Temperature [degC])
    //0x8D - text from unknown source
    case 0x8E: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //ASCII               [char(a)char(b)]
    case 0x8F: result = a * (b - 0x80) * 0.01;                                                                 break; //Ignition angle      [deg]
    case 0x90: result = a * b * 0.01;                                                                          break; //Consumption         [l/h]
    case 0x91: result = a * b * 0.01;                                                                          break; //Consumption         [l/100km]
    case 0x92: result = a * (b - 0x80) * 0.0001 + 1;                                                           break; //Lambda factor       [N/A]
    //0x93 - unknown formula (Inclination [%])
    case 0x94: result = a * (b - 0x80) / 4.0;                                                                  break; //Slip RPM            [N/A]
    case 0x95: result = a * (b - 100) * 0.1;                                                                   break; //Temperature         [degC]
    case 0x96: result = a * 0.005568 + b * 1.422;                                                              break; //Mass flow           [g/s]
    case 0x97: result = a * (b - 0x80) * 0.01;                                                                 break; //Idle correction     [degKW]
    case 0x98: result = a * b / 40.0;                                                                          break; //Air mass per stroke [mg/h]
    case 0x99: result = a * (b - 0x80) / 256.0;                                                                break; //Injection quantity  [mg/h]
    case 0x9A: result = a * b;                                                                                 break; //Angle               [deg]
    case 0x9B: result = a * b * 0.01 - 90.0;                                                                   break; //Angle               [deg]
    case 0x9C: result = uint16_t((a_raw << 8) | b_raw);                                                        break; //Distance            [cm]
    case 0x9D: result = int16_t((a_raw << 8) | b_raw);                                                         break; //Distance            [cm]
    case 0x9E: result = uint16_t((a_raw << 8) | b_raw) * 0.01;                                                 break; //Speed               [km/h]
    case 0x9F: result = (a  + (b - 0x7F) * 256.0) * 0.1;                                                       break; //Temperature         [degC]
    //0xA0 - unknown
    case 0xA1: result = uint16_t(a_raw << 8) | b_raw;                                                          break; //Binary bytes        [bin(a)bin(b)]
    case 0xA2: result = a * b * 0.448;                                                                         break; //Angle               [deg]
    case 0xA3: result = b / 100.0 + a;                                                                         break; //Clock               [a:b]
    case 0xA4: result = (b > 100 && b <= 200) ? (b - 100) : b;                                                 break; //Percentage          [%]
    case 0xA5: result = a + (b - 0x80) * 256.0;                                                                break; //Current             [mA]
    case 0xA6: result = ((a_raw % 4) - 4 * (a >= 0x80)) * 640.0 + b * 2.5;                                     break; //Turn rate           [deg/s]
    case 0xA7: result = a * b * 0.001;                                                                         break; //Resistance          [mOhm]
    case 0xA8: result = uint16_t((a_raw << 8) | b_raw) * 0.01;                                                 break; //N/A                 [%]
    case 0xA9: result = a * b + 200;                                                                           break; //N/A                 [mV]
    case 0xAA: result = a * b / 2560.0;                                                                        break; //N/A                 [g]
    case 0xAB: result = a * b / 256.0;                                                                         break; //N/A                 [g]
    case 0xAC: result = a * b;                                                                                 break; //N/A                 [mg/km]
    case 0xAD: result = a * b * 0.1;                                                                           break; //N/A                 [mg/s]
    case 0xAE: result = a * b * 0.01;                                                                          break; //N/A                 [l/1000km]
    case 0xAF: result = a * b * 0.005;                                                                         break; //N/A                 [bar]
    case 0xB0: result = a * b * 0.1;                                                                           break; //N/A                 [ppm]
    case 0xB1: result = uint16_t((a_raw << 8) | b_raw) - 32768.0;                                              break; //N/A                 [Nm]
    case 0xB2: result = a * 30.11796 + b * 0.118 - 3855.1;                                                     break; //N/A                 [deg/s]
    case 0xB3: result = uint16_t((a_raw << 8) | b_raw) * 10.0;                                                 break; //N/A                 [N/A]
    case 0xB4: result = a * b / 256.0;                                                                         break; //N/A                 [kg/h]
    case 0xB5: result = a * b * 0.001;                                                                         break; //N/A                 [mg/s]
  }
  
  //Return the calculated value.
  return result;
}

/**
  Function:
    getMeasurementUnits(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t measurement_buffer[], size_t measurement_buffer_size, char* str, size_t string_size)
  
  Parameters:
    measurement_index       -> index of the measurement whose units are needed (0-4)
    amount_of_measurements  -> total number of measurements stored in the array (value passed as reference to readGroup())
    measurement_buffer[]    -> array in which measurements have been stored by readGroup()
    measurement_buffer_size -> total size of the given array (provided with the sizeof() operator)
    str[]                   -> string (character array) into which to copy the units
    string_size             -> total size of the given array (provided with the sizeof() operator)
  
  Returns:
    char* -> the same character array provided (str), to be able to use it like "Serial.println(getMeasurementUnits(...))"
  
  Description:
    Provides a string containing the proper units for a measurement from a buffer filled by readGroup().
  
  Notes:
    *It is a static function so it does not require an instance to be used (useful in multi-instance applications).
*/
char* KLineKWP1281Lib::getMeasurementUnits(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size, char* str, size_t string_size)
{
  //Check if the array is large enough to contain as many measurements as "declared" by the value given.
  if (measurement_buffer_size < amount_of_measurements * 3) {
    amount_of_measurements = measurement_buffer_size / 3;
  }
  
  //If the requested index is out of range (or the buffer doesn't contain measurements), return an empty string.
  if (measurement_index >= amount_of_measurements) {
    if (string_size) {
      str[0] = '\0';
    }
    return str;
  }
  
  //Get the three important values from the buffer based on the requested index.
  uint8_t formula = measurement_buffer[3 * measurement_index];
  uint8_t a       = measurement_buffer[3 * measurement_index + 1];
  uint8_t b       = measurement_buffer[3 * measurement_index + 2];
  
  //Pointer which will point to one of the strings stored in PROGMEM from "units.h", which will be copied into the given string.
  const char* unit_pointer;
  
  switch (formula) {
    case 0x01:
    case 0x74:
    case 0x80:
    case 0x8B:
      unit_pointer = KWP_units_RPM;
      break;
    
    case 0x02:
    case 0x14:
    case 0x17:
    case 0x21:
    case 0x77:
    case 0x81:
    case 0x93:
    case 0xA4:
    case 0xA8:
      unit_pointer = KWP_units_Percentage;
      break;
    
    case 0x03:
    case 0x09:
    case 0x1E:
    case 0x43:
    case 0x51:
    case 0x5B:
    case 0x70:
    case 0x78:
    case 0x84:
    case 0x8F:
    case 0x9A:
    case 0x9B:
    case 0xA2:
      unit_pointer = KWP_units_Angle;
      break;
    
    case 0x04:
      unit_pointer = (b < 127) ? KWP_units_Ignition_BTDC : KWP_units_Ignition_ATDC;
      break;
    
    case 0x05:
    case 0x1A:
    case 0x61:
    case 0x75:
    case 0x8C:
    case 0x95:
    case 0x9F:
      unit_pointer = KWP_units_Temperature;
      break;
    
    case 0x06:
    case 0x15:
    case 0x2B:
    case 0x42:
    case 0x48:
    case 0x4D:
    case 0x67:
    case 0x85:
    case 0x8A:
      unit_pointer = KWP_units_Voltage;
      break;
    
    case 0x07:
    case 0x6A:
    case 0x86:
    case 0x9E:
      unit_pointer = KWP_units_Speed;
      break;
    
    case 0x0A:
      unit_pointer = b ? KWP_units_Warm : KWP_units_Cold;
      break;
    
    case 0x0C:
    case 0x40:
    case 0x49:
      unit_pointer = KWP_units_Resistance;
      break;
    
    case 0x0D:
    case 0x41:
    case 0x66:
      unit_pointer = KWP_units_Distance_m;
      break;
    
    case 0x0E:
    case 0x45:
    case 0x53:
    case 0x64:
    case 0xAF:
      unit_pointer = KWP_units_Pressure;
      break;
    
    case 0x0F:
    case 0x16:
    case 0x2F:
    case 0x89:
      unit_pointer = KWP_units_Time_m;
      break;
    
    case 0x10:
    case 0x88:
      for (uint8_t i = 0; i < ((string_size < 8) ? string_size : 8); i++) {
        if (a & (1 << (7 - i))) {
          if (b & (1 << (7 - i))) {
            str[i] = '1';
          }
          else {
            str[i] = '0';
          }
        }
        else {
          str[i] = ' ';
        }
      }
      
      if (string_size > 8) {
        str[8] = '\0';
      }
      else if (string_size) {
        str[string_size - 1] = '\0';
      }
      return str;
    
    case 0x11:
    case 0x8E:
      if (string_size > 0) {
        str[0] = a;
      }
      
      if (string_size > 1) {
        str[1] = b;
      }
      
      if (string_size > 2) {
        str[2] = '\0';
      }
      else if (string_size) {
        str[string_size - 1] = '\0';
      }
      return str;
    
    case 0x2C:
      snprintf(str, string_size, "%02d:%02d", a, b);
      
      if (string_size) {
        str[string_size - 1] = '\0';
      }
      return str;
    
    case 0x12:
    case 0x32:
    case 0x60:
      unit_pointer = KWP_units_Pressure_m;
      break;
    
    case 0x13:
      unit_pointer = KWP_units_Volume;
      break;
    
    case 0x18:
    case 0x28:
    case 0x56:
    case 0x82:
      unit_pointer = KWP_units_Current;
      break;
    
    case 0x19:
    case 0x35:
    case 0x96:
      unit_pointer = KWP_units_Mass_Flow;
      break;
    
    case 0x1B:
      unit_pointer = (b < 128) ? KWP_units_Ignition_ATDC : KWP_units_Ignition_BTDC;
      break;
    
    case 0x1D:
      unit_pointer =  (b < a) ? KWP_units_Map1 : KWP_units_Map2;
      break;
    
    case 0x22:
      unit_pointer = KWP_units_Correction;
      break;
    
    case 0x23:
    case 0x90:
      unit_pointer = KWP_units_Consumption_h;
      break;
    
    case 0x24:
    case 0x5C:
    case 0x6F:
      unit_pointer = KWP_units_Distance_k;
      break;
    
    case 0x26:
    case 0x2E:
    case 0x97:
      unit_pointer = KWP_units_Segment_Correction;
      break;
    
    case 0x27:
    case 0x31:
    case 0x33:
    case 0x98:
    case 0x99:
      unit_pointer = KWP_units_Mass_Per_Stroke_m;
      break;
    
    case 0x29:
      unit_pointer = KWP_units_Capacity;
      break;
    
    case 0x2A:
      unit_pointer = KWP_units_Power_k;
      break;
    
    case 0x2D:
    case 0x91:
      unit_pointer = KWP_units_Consumption_100km;
      break;
    
    case 0x34:
    case 0x5D:
    case 0x5E:
    case 0xB1:
      unit_pointer = KWP_units_Torque;
      break;
    
    case 0x37:
    case 0x3C:
    case 0x3E:
      unit_pointer = KWP_units_Time;
      break;
    
    case 0x3A:
    case 0x4E:
      unit_pointer = KWP_units_Misfires;
      break;
    
    case 0x44:
    case 0x55:
    case 0x57:
    case 0xA6:
    case 0xB2:
      unit_pointer = KWP_units_Turn_Rate;
      break;
    
    case 0x46:
    case 0x52:
    case 0x54:
      unit_pointer = KWP_units_Acceleration;
      break;
    
    case 0x47:
    case 0x9C:
    case 0x9D:
      unit_pointer = KWP_units_Distance_c;
      break;
    
    case 0x4A:
      unit_pointer = KWP_units_Time_mo;
      break;
    
    case 0x4C:
    case 0x50:
    case 0x58:
      unit_pointer = KWP_units_Resistance_k;
      break;
    
    case 0x59:
    case 0xA3:
      unit_pointer = KWP_units_Time_h;
      break;
    
    case 0x5A:
      unit_pointer = KWP_units_Mass_k;
      break;
    
    case 0x62:
      unit_pointer = KWP_units_Impulses;
      break;
    
    case 0x65:
      unit_pointer = KWP_units_Fuel_Level_Factor;
      break;
    
    case 0x68:
      unit_pointer = KWP_units_Volume_m;
      break;
    
    case 0x69:
    case 0x72:
      unit_pointer = KWP_units_Distance;
      break;
    
    case 0x6B:
      snprintf(str, string_size, "%02X %02X", a, b);
      
      if (string_size) {
        str[string_size - 1] = '\0';
      }
      return str;
    
    case 0x73:
      unit_pointer = KWP_units_Power;
      break;
    
    case 0x7C:
    case 0xA5:
      unit_pointer = KWP_units_Current_m;
      break;
    
    case 0x7D:
      unit_pointer = KWP_units_Attenuation;
      break;
    
    case 0x83:
      unit_pointer = ((float(a) * float(b) / 2.0 - 30.0) < 0) ?  KWP_units_Ignition_BTDC : KWP_units_Ignition_ATDC;
      break;
    
    case 0xA1:
      snprintf(str, string_size, BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(a), BYTE_TO_BINARY(b));
      
      if (string_size) {
        str[string_size - 1] = '\0';
      }
      return str;
    
    case 0xA7:
      unit_pointer = KWP_units_Resistance_m;
      break;
    
    case 0xA9:
      unit_pointer = KWP_units_Voltage_m;
      break;
    
    case 0xAA:
    case 0xAB:
      unit_pointer = KWP_units_Mass;
      break;
    
    case 0xAC:
      unit_pointer = KWP_units_Mass_Flow_km;
      break;
    
    case 0xAD:
    case 0xB5:
      unit_pointer = KWP_units_Mass_Flow_m;
      break;
    
    case 0xAE:
      unit_pointer = KWP_units_Consumption_1000km;
      break;
    
    case 0xB0:
      unit_pointer = KWP_units_Parts_Per_Million;
      break;
    
    case 0xB4:
      unit_pointer = KWP_units_Mass_Per_Stroke_k;
      break;
    
    //Text from table
    case 0x25:
    case 0x7B:
      {
        #ifdef KWP1281_TEXT_TABLE_SUPPORTED
          uint16_t code = (a << 8) | b;
          
          if (code >= 1 && code <= 0x065E) {
            unit_pointer = (const char*)READ_POINTER_FROM_PROGMEM(formula_string_table + code - 1);
          }
          else if (code >= 0x0668 && code <= 0x066C) {
            unit_pointer = (const char*)READ_POINTER_FROM_PROGMEM(formula_string_table + code - 10);
          }
          else {
            if (string_size) {
              str[0] = '\0';
            }
            return str;
          }
          
          strncpy_P(str, unit_pointer, string_size);
          
          if (string_size) {
            str[string_size - 1] = '\0';
          }
        #else
          strncpy(str, "EN_f25", string_size);
          
          if (string_size) {
            str[string_size - 1] = '\0';
          }
        #endif
      }
      return str;
    
    //Date
    case 0x7F:
      snprintf(str, string_size, "%04d.%02d.%02d", 2000 + (b & 0x7F), ((a & 0x07) << 1) | ((b & 0x80) >> 7), (a & 0xF8) >> 3);
      
      if (string_size) {
        str[string_size - 1] = '\0';
      }
      return str;
    
    default:
      if (string_size)  {
        str[0] = '\0';
      }
      return str;
  }
  
  strncpy_P(str, unit_pointer, string_size);
  if (string_size) {
    str[string_size - 1] = '\0';
  }
  
  return str;
}

/**
  Function:
    readROM(uint8_t chunk_size, uint16_t start_address, uint8_t memory_buffer[], uint8_t memory_buffer_size)
  
  Parameters:
    chunk_size         -> how many bytes to read
    memory_buffer[]    -> array into which to store the bytes read
    memory_buffer_size -> total (maximum) length of the given array
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Reads a chunk of ROM/EEPROM.
  
  Notes:
    *If reading is successful, the first byte in the buffer represents the total amount of bytes of memory.
    *After the first byte, the bytes of memory are stored.
    *It is possible to call the function without parameters, for debugging purposes (nothing will be stored anywhere).
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readROM(uint8_t chunk_size, uint16_t start_address, uint8_t* memory_buffer, uint8_t memory_buffer_size)
{
  uint8_t start_address_high_byte = start_address >> 8;
  uint8_t start_address_low_byte = start_address & 0xFF;
  
  uint8_t parameters[] = {chunk_size, start_address_high_byte, start_address_low_byte};
  if (!send(KWP_REQUEST_READ_ROM, parameters, sizeof(parameters))) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  switch (receive(memory_buffer, memory_buffer_size)) {
    case TYPE_ACK:
    case TYPE_REFUSE:
      show_debug_info(READ_ROM_NOT_SUPPORTED);
      return FAIL;
    
    case TYPE_ROM:
      show_debug_info(RECEIVED_ROM);
      return SUCCESS;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

/**
  Function:
    outputTests(uint16_t &returned_ID)
  
  Parameters:
    &returned_ID -> will contain the currently running output test ID
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Performs an output test.
  
  Notes:
    *Output tests are always performed in a sequenced that cannot be changed.
    *To go through them all, use multiple calls of outputTests() until FAIL is returned.
    *The currently running test ID is stored in the variable passed by reference.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::outputTests(uint16_t &returned_ID)
{
  uint8_t parameters[] = {0x00};
  send(KWP_REQUEST_OUTPUT_TEST, parameters, sizeof(parameters));
  
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_REFUSE:
      show_debug_info(OUTPUT_TESTS_NOT_SUPPORTED);
      return FAIL;
    
    case TYPE_OUTPUT_TEST:
      show_debug_info(RECEIVED_OUTPUT_TEST);
      returned_ID = (_receive_buffer[1] << 8) | _receive_buffer[2];
      return SUCCESS;
    
    case TYPE_ACK:
      show_debug_info(END_OF_OUTPUT_TESTS);
      returned_ID = 0;
      return FAIL;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

///PRIVATE

/**
  Function:
    bitbang_5baud(uint8_t module)
  
  Parameters:
    module -> logical ID of the target module (engine-0x01, cluster-0x17 etc.)
  
  Description:
    Initializes the requested module.
*/
void KLineKWP1281Lib::bitbang_5baud(uint8_t module)
{
  //Stop serial communication on the K-line.
  _endFunction();
  
  //Configure the transmit pin for sending the 5-baud initialization manually.
  pinMode(_tx_pin, OUTPUT);
  
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      _debug_port->print(F("Connecting to module "));
      if (module < 0x10) _debug_port->print(0);
      _debug_port->println(module, HEX);
      _debug_port->print(F("5-baud-init: "));
    }
  #endif
  
  //Construct the wake-up byte, bit by bit.
  bool bit, bits[10], parity = 1;
  for (uint8_t i = 0; i < sizeof(bits); i++) {
    bit = 0;
    
    switch (i) {
      case 0: //start bit
        bit = 0;
        break;
      
      case 8: //odd parity bit
        bit = parity;
        break;
      
      case 9: //stop bit
        bit = 1;
        break;
      
      default: //7 data bits
        bit = module & bit(i - 1); //address of module, LSB first
        parity ^= bit; //XOR to calculate parity
        break;
    }
    
    bits[i] = bit; //save the current bit in the array
  }
  
  //Send the constructed byte by sending each bit from the array.
  for (uint8_t i = 0; i < sizeof(bits); i++) {
    //Set the transmit line according to the current bit.
    digitalWrite(_tx_pin, bits[i]);
    
    #ifdef KWP1281_DEBUG_SUPPORTED
      if (_debug_port) {
        _debug_port->print(bits[i]);
      }
    #endif
    
    //Wait around 200ms after each bit for 5Bd.
    if (custom_5baud_wait_function_pointer) {
      custom_5baud_wait_function_pointer();
    }
    else {
      delay(200); 
    }
  }
  
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      _debug_port->println();
    }
  #endif
}

/**
  Function:
    read_identification()
  
  Returns:
    executionStatus -> whether or not the operation executed successfully
      *SUCCESS
      *FAIL
      *ERROR
  
  Description:
    Receives the module's identification and stores it in the private struct's strings.
*/
KLineKWP1281Lib::executionStatus KLineKWP1281Lib::read_identification()
{
  bool extra_info_available = false;
  uint8_t string_length;
  
  //1. VAG part number
  RETURN_TYPE ret_val = receive(_receive_buffer, sizeof(_receive_buffer));
  
  //In the receive() function above, if the module has just been initialized, it send the protocol identification bytes again.
  //After that, it will not happen again, so reset this flag.
  _may_send_protocol_parameters_again = false;
  
  switch (ret_val) {
    case TYPE_ACK:
      show_debug_info(NO_PART_NUMBER_AVAILABLE);
      return SUCCESS;
      
    case TYPE_ID:
      show_debug_info(RECEIVED_PART_NUMBER);
      break;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
  
  //The top bit in the first byte indicates whether or not there is extra identification available.
  if (_receive_buffer[1] & 0x80) {
    extra_info_available = true;
    
    //Strip that bit away, leaving only the 7-bit ASCII character.
    _receive_buffer[1] &= ~(1 << 7);
  }
  
  //Copy the string from the buffer to the struct.
  string_length = _receive_buffer[0];
  memcpy(identification_data.part_number, _receive_buffer + 1, string_length);
  
  //Confirm receiving of the message and request the next.
  if (!acknowledge()) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  //2. Regular identification
  uint8_t identification_string_index = 0;
  
  //Part 1
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_ACK:
      show_debug_info(NO_ID_PART1_AVAILABLE);
      return SUCCESS;
      
    case TYPE_ID:
      show_debug_info(RECEIVED_ID_PART1);
      break;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
  
  //Copy the string from the buffer to the struct.
  string_length = _receive_buffer[0];
  memcpy(identification_data.identification, _receive_buffer + 1, string_length);
  
  identification_string_index += string_length;
  
  //Confirm receiving of the message and request the next.
  if (!acknowledge()) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  //Part 2
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_ACK:
      show_debug_info(NO_ID_PART2_AVAILABLE);
      return SUCCESS;
      
    case TYPE_ID:
      show_debug_info(RECEIVED_ID_PART2);
      break;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
  
  //Copy the string from the buffer to the struct, without overwriting the first part.
  string_length = _receive_buffer[0];
  memcpy(identification_data.identification + identification_string_index, _receive_buffer + 1, string_length);
  
  //Confirm receiving of the message and request the next.
  if (!acknowledge()) {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }
  
  //3. Coding, WSC
  switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
    case TYPE_ACK:
      show_debug_info(NO_CODING_WSC_AVAILABLE);
      break;
      
    case TYPE_ID:
      show_debug_info(RECEIVED_CODING_WSC);
      break;
    
    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
  
  //Extract the received coding value and workshop code.
  if (_receive_buffer[0] == 5) {
    identification_data.coding = ((_receive_buffer[2] << 8) | _receive_buffer[3]) >> 1;
    identification_data.workshop_code = (uint32_t(_receive_buffer[3] & 0x01) << 16) | (_receive_buffer[4] << 8) | _receive_buffer[5];
  }
  //A control module may send a coding+WSC field with a data length of 1 instead of 5, which means coding is not supported, or may send an acknowledgement.
  else {
    identification_data.coding = identification_data.workshop_code = 0;
  }
  
  if (extra_info_available) {
    //4. Extra identification
    show_debug_info(EXTRA_ID_AVAILABLE);
    
    uint8_t extra_identification_string_index = 0;
    
    //Request extra identification.
    if (!send(KWP_REQUEST_EXTRA_ID)) {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
    
    //Part 1
    switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
      case TYPE_ACK:
        show_debug_info(NO_EXTRA_ID_PART1_AVAILABLE);
        return FAIL;
        
      case TYPE_ID:
        show_debug_info(RECEIVED_EXTRA_ID_PART1);
        break;
      
      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }
    
    //Copy the string from the buffer to the struct.
    string_length = _receive_buffer[0];
    memcpy(identification_data.extra_identification, _receive_buffer + 1, string_length);
    
    extra_identification_string_index += string_length;
    
    //Confirm receiving of the message and request the next.
    if (!acknowledge()) {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
    
    //Part 2
    switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
      case TYPE_ACK:
        show_debug_info(NO_EXTRA_ID_PART2_AVAILABLE);
        return SUCCESS;
        
      case TYPE_ID:
        show_debug_info(RECEIVED_EXTRA_ID_PART2);
        break;
      
      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }
    
    //Copy the string from the buffer to the struct, without overwriting the first part.
    string_length = _receive_buffer[0];
    memcpy(identification_data.extra_identification + extra_identification_string_index, _receive_buffer + 1, string_length);
    
    extra_identification_string_index += string_length;
    
    //Confirm receiving of the message and request the next.
    if (!acknowledge()) {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
    
    //Part 3
    switch (receive(_receive_buffer, sizeof(_receive_buffer))) {
      case TYPE_ACK:
        show_debug_info(NO_EXTRA_ID_PART3_AVAILABLE);
        return SUCCESS;
        
      case TYPE_ID:
        show_debug_info(RECEIVED_EXTRA_ID_PART3);
        break;
      
      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }
    
    //Copy the string from the buffer to the struct, without overwriting the second part.
    string_length = _receive_buffer[0];
    memcpy(identification_data.extra_identification + extra_identification_string_index, _receive_buffer + 1, string_length);
  }
  else {
    show_debug_info(NO_EXTRA_ID_AVAILABLE);
  }
  
  //All identification data has been received.
  return SUCCESS;
}

/**
  Function:
    receive_parameters(uint8_t buffer[], uint8_t amount)
  
  Parameters:
    buffer[] -> array in which to store the received bytes
  
  Returns:
    bool -> whether or not the parameters were received successfully
  
  Description:
    Receives the 2 bytes sent by the module during initialization after the sync byte.
*/
bool KLineKWP1281Lib::receive_parameters(uint8_t* buffer)
{
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      _debug_port->print(F("Init bytes: "));
    }
  #endif
    
  //Receive the two bytes.
  for (uint8_t i = 0; i < 2; i++) {
    //Attempt to receive a byte, without sending a complement, or exit if it times out.
    if (read_byte(_receive_byte, false) == ERROR_TIMEOUT) {
      return false;
    }
    
    //Save the received byte in the given buffer.
    buffer[i] = _receive_byte;
    
    #ifdef KWP1281_DEBUG_SUPPORTED
      if (_debug_port) {
        if (_receive_byte < 0x10) _debug_port->print(0);
        _debug_port->print(_receive_byte, HEX);
      }
    #endif
  }
  
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      _debug_port->println();
    }
  #endif
  
  //Received both bytes.
  return true;
}

/**
  Function:
    read_byte(uint8_t &_byte, bool complement)
  
  Parameters:
    &_byte -> variable into which to store the received byte
    complement -> whether or not to send a complement to the received byte
  
  Default parameters:
    complement = true
  
  Returns:
    RETURN_TYPE -> whether or not a byte was received
      ERROR_TIMEOUT -> not received
      ERROR_OK -> received
  
  Description:
    Receives a single byte.
  
  Notes:
    *If receiving a byte times out, the error function is called.
*/
KLineKWP1281Lib::RETURN_TYPE KLineKWP1281Lib::read_byte(uint8_t &_byte, bool complement)
{
  //Store the current time to detect timeouts.
  unsigned long timeout_timer = millis();
  
  //Wait for data to be available, checking for timeouts.
  while (!_receiveFunction(_byte)) {
    //If the timeout period passes without receiving any messages, exit.
    if (millis() - timeout_timer > _timeout) {
      show_debug_info(TIMED_OUT);
      
      //Call the error function only if currently connected to a module.
      if (_current_module) {
        error_function();
      }
      
      return ERROR_TIMEOUT;
    }
  }
  
  //Store the current time after receiving a byte, to delay properly when sending.
  _last_received_byte_time = millis();
  
  //If a complement was requested, send it.
  if (complement) {
    //Send the complement after a small delay.
    delay(complementDelay);
    send_complement(_byte);
  }
  
  //Done reading the byte.
  return ERROR_OK;
}

/**
  Function:
    send_complement(uint8_t _byte)
  
  Parameters:
    _byte -> byte to send the complement to
  
  Description:
    Sends a complement to a byte.
*/
void KLineKWP1281Lib::send_complement(uint8_t _byte)
{
  //The complement is the difference between 0xFF and the received byte or the byte XORd by 0xFF.
  uint8_t complement = _byte ^ 0xFF;
  _sendFunction(complement);
  
  //Because the K-line is bidirectional, an "echo" will appear.
  //Read that echoed byte to get rid of it from the serial buffer.
  //If the serial interface cannot receive while sending, this is not necessary.
  if (_full_duplex) {
    uint8_t dummy;
    while (!_receiveFunction(dummy));
  }
}

/**
  Function:
    receive(uint8_t buffer[], size_t buffer_size)
  
  Parameters:
    buffer[] -> array into which to store the received block
    buffer_size -> total (maximum) length of the given array
  
  Returns:
    RETURN_TYPE -> what was received
      ERROR_TIMEOUT -> nothing (timed out)
      ERROR_OK -> unknown
      TYPE_... -> specific message
  
  Description:
    Receives a block.
  
  Notes:
    *Executes the error_function() function if receiving times out.
*/
KLineKWP1281Lib::RETURN_TYPE KLineKWP1281Lib::receive(uint8_t* buffer, size_t buffer_size)
{  
  uint8_t buffer_index = 0; //keep track of how many bytes have been saved in the buffer so far
  
  //At the beginning of communication, after the modules sends protocol parameters, but before sending its identification strings, it might send those
  //parameters again.
  //In this case, the 0x55 sync byte should not receive an automatic complement (and should not be considered a block length).
  //Instead, two more bytes should be read (protocol description), and a manual complement sent to the second byte.
  //If a byte different than 0x55 is received, or if 0x55 is received but the module already sent its identification strings, it will be considered the block
  //length and complemented.
  while (true) {
    //Try to receive the length of the block.
    //Do not send an automatic complement in case the event described above takes place.
    //Exit if receiving times out.
    if (read_byte(_receive_byte, false) == ERROR_TIMEOUT) {
      return ERROR_TIMEOUT;
    }
    
    //If the event described above takes place, proceed accordingly, after which wait for a byte again, to get the block length of the module's response,
    if (_receive_byte == 0x55 && _may_send_protocol_parameters_again) {
      #ifdef KWP1281_DEBUG_SUPPORTED
        if (_debug_port) {
          _debug_port->println(F("Got sync byte again"));
        }
      #endif
      
      //Try to receive the remaining two bytes of the protocol parameters.
      //If this times out, it could mean the 0x55 is waiting for a complement, so it's actually the block length (although improbable).
      if (!receive_parameters(_parameter_buffer)) {
        break;
      }
      
      //Send a complement to the second byte after a delay.
      delay(initComplementDelay);
      send_complement(_parameter_buffer[1]);
      
      //uint16_t protocol = ((_parameter_buffer[1] & 0x7F) << 7) | _parameter_buffer[0];
      
      //Now, the loop will repeat, causing another byte read attempt.
      //If the received byte will not qualify for the special repeated protocol response, the loop will stop, the byte will be considered the block length and
      //complemented, and the block will continue to be read.
    }
    else {
      break;
    }
  }
  
  //The block length was received, complement it manually.
  delay(complementDelay);
  send_complement(_receive_byte);
  
  //Determine how much data the message will contain (length without the block title, the sequence, and the end byte).
  uint8_t data_bytes = _receive_byte - 3;
  
  //If the given buffer has a size of at least one byte, store the message size on the first position.
  if (buffer_size) {
    buffer[buffer_index++] = data_bytes;
  }
  else {
    show_debug_info(NULL_ARRAY_SIZE);
  }
  
  if (buffer_size - buffer_index < data_bytes) {
    show_debug_info(ARRAY_NOT_LARGE_ENOUGH);
  }
  
  //Try to receive the block's sequence number, or exit if receiving times out.
  if (read_byte(_sequence) == ERROR_TIMEOUT) {
    return ERROR_TIMEOUT;
  }
  
  //Try to receive the block's title (message type), or exit if receiving times out.
  uint8_t command;
  if (read_byte(command) == ERROR_TIMEOUT) {
    return ERROR_TIMEOUT;
  }
  
  //If the block contains data bytes, attempt to read them.
  if (data_bytes) {
    
    //Attempt to read each data byte.
    for (uint8_t i = 0; i < data_bytes; i++) {
      //Try to receive a data byte, or exit if receiving times out.
      if (read_byte(_receive_byte) == ERROR_TIMEOUT) {
        return ERROR_TIMEOUT;
      }
      
      //If there is still space in the given buffer, save the received byte.
      if (buffer_size - buffer_index) {
        buffer[buffer_index++] = _receive_byte;
      }
    }
  }
  
  //Try to receive the block end byte, don't send a complement to it, or exit if receiving times out.
  if (read_byte(_receive_byte, false) == ERROR_TIMEOUT) {
    return ERROR_TIMEOUT;
  }
  
  if (_debugFunction) {
    _debugFunction(1, _sequence, command, buffer + 1, data_bytes);
  }
  
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      _debug_port->print(F("Info: received "));
      show_command_description(command);
    }
  #endif
  
  //Return the message type based on the block title byte.
  switch (command) {
    case KWP_RECEIVE_ID_DATA:
      return TYPE_ID;
    
    case KWP_ACKNOWLEDGE:
      return TYPE_ACK;
    
    case KWP_REFUSE:
      return TYPE_REFUSE;
    
    case KWP_RECEIVE_FAULT_CODES:
      return TYPE_FAULT_CODES;
    
    case KWP_RECEIVE_ADAPTATION:
      return TYPE_ADAPTATION;
    
    case KWP_RECEIVE_GROUP_READING:
      return TYPE_GROUP_READING;
    
    case KWP_RECEIVE_ROM:
      return TYPE_ROM;
    
    case KWP_RECEIVE_OUTPUT_TEST:
      return TYPE_OUTPUT_TEST;
    
    case KWP_RECEIVE_BASIC_SETTING:
      return TYPE_BASIC_SETTING;
    
    default:
      return ERROR_OK;
  }
}

/**
  Function:
    acknowledge()
  
  Returns:
    bool -> whether or not the acknowledgement block was sent successfully
  
  Description:
    Tells the module that a block was received and that it can continue sending more data.
*/
bool KLineKWP1281Lib::acknowledge()
{
  //The acknowledgement message contains no data bytes.
  return send(KWP_ACKNOWLEDGE); 
}

/**
  Function:
    send_byte(uint8_t _byte, bool wait_for_complement)
  
  Parameters:
    _byte -> byte to send
    wait_for_complement -> whether or not to expect to receive a complement to the sent byte
  
  Returns:
    RETURN_TYPE -> whether or not the byte was sent successfully (if a complement was received when needed)
      ERROR_TIMEOUT -> not send successfully
      ERROR_OK -> send successfully
  
  Default parameters:
    wait_for_complement = true
  
  Description:
    Sends a single byte.
*/
KLineKWP1281Lib::RETURN_TYPE KLineKWP1281Lib::send_byte(uint8_t _byte, bool wait_for_complement)
{
  //If the byte delay time has not passed already, wait.
  unsigned long time_passed_since_last_received_message = millis() - _last_received_byte_time;
  if (time_passed_since_last_received_message < byteDelay) {
    unsigned long byte_delay_adjusted = byteDelay - time_passed_since_last_received_message;
    delay(byte_delay_adjusted);
  }
  
  _sendFunction(_byte);
  
  //If the serial interface can receive while sending, read the echoed byte to get rid of it.
  if (_full_duplex) {
    uint8_t dummy;
    while (!_receiveFunction(dummy));
  }
  
  if (wait_for_complement) {    
    _timeout = complementResponseTimeout;
  
    uint8_t complement;
    if (read_byte(complement, false) == ERROR_TIMEOUT) {
      return ERROR_TIMEOUT;
    }
    
    _timeout = responseTimeout;
    
    return ERROR_OK;
  }
  
  return ERROR_OK;
}

/**
  Function:
    send(uint8_t command, uint8_t parameters[], uint8_t parameter_count)
  
  Parameters:
    command -> opcode to send in the block
    parameters[] -> array of parameters to provide in the block
    uint8_t parameter_count -> how many parameters the given array contains
  
  Returns:
    bool -> whether or not the block was sent successfully
  
  Default parameters:
    parameters[] = {}
    parameter_count = 0
  
  Description:
    Sends a block.
*/
bool KLineKWP1281Lib::send(uint8_t command, uint8_t* parameters, uint8_t parameter_count)
{
  //If the byte delay time has not passed already, wait.
  unsigned long time_passed_since_last_received_message = millis() - _last_received_byte_time;
  if (time_passed_since_last_received_message < byteDelay) {
    unsigned long byte_delay_adjusted = byteDelay - time_passed_since_last_received_message;
    delay(byte_delay_adjusted);
  }
  
  uint8_t block_size = 3 + parameter_count; //the block size includes the sequence counter, the command byte, the parameter bytes and the final byte
    
  if (send_byte(block_size) == ERROR_TIMEOUT) {
    return false;
  }
  
  if (send_byte(++_sequence) == ERROR_TIMEOUT) { //send the sequence number and also increment it
    return false;
  }
  
  if (send_byte(command) == ERROR_TIMEOUT) {
    return false;
  }
  
  if (parameters) {    
    for (uint8_t i = 0; i < parameter_count; i++) { //send each byte in the array
      if (send_byte(parameters[i]) == ERROR_TIMEOUT) {
        return false;
      }
    }
  }
  
  send_byte(0x03, false); //send the constant final byte and do not wait to receive a complement
    
  if (_debugFunction) {
    _debugFunction(0, _sequence, command, parameters, parameter_count);
  }
  
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      _debug_port->print(F("Info: sent "));
      show_command_description(command);
    }
  #endif
  
  return true;
}

/**
  Function:
    error_function()
  
  Description:
    Reconnects to the module if a timeout occurs.
*/
void KLineKWP1281Lib::error_function()
{
  show_debug_info(TERMINATED);
  
  //If there is a custom function defined, execute it.
  if (custom_error_function_pointer) {
    custom_error_function_pointer();
  }
  
  //Otherwise, automatically reconnect to the previously connected module.
  else {
    //The module is already disconnected, ensure the disconnect() function is not executed again.
    uint8_t reconnect_to_module =  _current_module;
    _current_module = 0;
    
    //Reconnect using the previous values.
    connect(reconnect_to_module, _current_baud_rate);
  }
}

/**
  Function:
    show_debug_info(DEBUG_TYPE type)
  
  Parameters:
    type -> event to print information about
  
  Description:
    Prints debugging information.
*/
void KLineKWP1281Lib::show_debug_info(DEBUG_TYPE type)
{
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      switch (type) {
        case CHECK_BAUD:
          _debug_port->println(F("Error: wrong response; check baud rate!"));
          break;
        
        case INCORRECT_PROTOCOL:
          _debug_port->println(F("Warning: may not use KWP1281"));
          break;
        
        case CONNECTED:
          _debug_port->println(F("Info: connected"));
          break;
        
        case UNEXPECTED_RESPONSE:
          _debug_port->println(F("Error: unexpected response"));
          break;
        
        case NO_RESPONSE:
          _debug_port->println(F("Error: no response"));
          break;
        
        case DISCONNECTED:
          _debug_port->println(F("Info: disconnected"));
          break;
        
        case TERMINATED:
          _debug_port->println(F("Error: connection terminated"));
          break;
        
        case TIMED_OUT:
          _debug_port->println(F("Error: timed out"));
          break;
        
        case SEND_ERROR:
          _debug_port->println(F("Error: error sending request"));
          break;
        
        case ARRAY_NOT_LARGE_ENOUGH:
          _debug_port->println(F("Warning: not enough space in the given/internal array"));
          break;
        
        case NULL_ARRAY_SIZE:
          _debug_port->println(F("Warning: given array has a size of 0"));
          break;
        
        case LOGIN_ACCEPTED:
          _debug_port->println(F("Info: login accepted"));
          break;
        
        case LOGIN_NOT_ACCEPTED:
          _debug_port->println(F("Info: login not accepted"));
          break;
        
        case FAULT_CODES_NOT_SUPPORTED:
          _debug_port->println(F("Info: fault codes not supported"));
          break;
        
        case NO_FAULT_CODES:
          _debug_port->println(F("Info: no fault codes stored"));
          break;
        
        case RECEIVED_FAULT_CODES:
          _debug_port->println(F("Info: received fault codes"));
          break;
        
        case END_OF_FAULT_CODES:
          _debug_port->println(F("Info: no more fault codes available"));
          break;
        
        case CLEARING_FAULT_CODES_NOT_SUPPORTED:
          _debug_port->println(F("Info: clearing fault codes not supported"));
          break;
        
        case CLEARING_FAULT_CODES_ACCEPTED:
          _debug_port->println(F("Info: cleared fault codes"));
          break;
        
        case INVALID_ADAPTATION_CHANNEL:
          _debug_port->println(F("Error: invalid adaptation channel"));
          break;
        
        case INVALID_ADAPTATION_CHANNEL_OR_VALUE:
          _debug_port->println(F("Error: invalid adaptation channel or value"));
          break;
        
        case ADAPTATION_RECEIVED:
          _debug_port->println(F("Info: received adaptation channel"));
          break;
        
        case ADAPTATION_ACCEPTED:
          _debug_port->println(F("Info: adaptation accepted"));
          break;
        
        case ADAPTATION_NOT_ACCEPTED:
          _debug_port->println(F("Info: adaptation not accepted"));
          break;
        
        case INVALID_BASIC_SETTING_GROUP:
          _debug_port->println(F("Error: invalid basic setting group (or not supported)"));
          break;
        
        case RECEIVED_EMPTY_BASIC_SETTING_GROUP:
          _debug_port->println(F("Error: empty basic setting group"));
          break;
        
        case RECEIVED_BASIC_SETTING:
          _debug_port->println(F("Info: received basic setting"));
          break;
        
        case INVALID_MEASUREMENT_GROUP:
          _debug_port->println(F("Error: invalid measurement group"));
          break;
        
        case RECEIVED_EMPTY_GROUP:
          _debug_port->println(F("Error: empty measurement group"));
          break;
        
        case RECEIVED_GROUP:
          _debug_port->println(F("Info: received measurement group"));
          break;
        
        case READ_ROM_NOT_SUPPORTED:
          _debug_port->println(F("Error: reading ROM not supported, or invalid parameters given"));
          break;
        
        case RECEIVED_ROM:
          _debug_port->println(F("Info: received ROM reading"));
          break;
        
        case NO_PART_NUMBER_AVAILABLE:
          _debug_port->println(F("Info: no part number available"));
          break;
        
        case RECEIVED_PART_NUMBER:
          _debug_port->println(F("Info: received part number"));
          break;
        
        case NO_ID_PART1_AVAILABLE:
          _debug_port->println(F("Info: no identification available (part 1)"));
          break;
        
        case RECEIVED_ID_PART1:
          _debug_port->println(F("Info: received identification (part 1)"));
          break;
        
        case NO_ID_PART2_AVAILABLE:
          _debug_port->println(F("Info: no identification available (part 2)"));
          break;
        
        case RECEIVED_ID_PART2:
          _debug_port->println(F("Info: received identification (part 2)"));
          break;
        
        case NO_CODING_WSC_AVAILABLE:
          _debug_port->println(F("Info: no coding/WSC available"));
          break;
        
        case RECEIVED_CODING_WSC:
          _debug_port->println(F("Info: received coding/WSC"));
          break;
        
        case EXTRA_ID_AVAILABLE:
          _debug_port->println(F("Info: extra identification available"));
          break;
        
        case NO_EXTRA_ID_AVAILABLE:
          _debug_port->println(F("Info: no extra identification available"));
          break;
        
        case NO_EXTRA_ID_PART1_AVAILABLE:
          _debug_port->println(F("Info: no extra identification available (part 1)"));
          break;
        
        case RECEIVED_EXTRA_ID_PART1:
          _debug_port->println(F("Info: received extra identification (part 1)"));
          break;
        
        case NO_EXTRA_ID_PART2_AVAILABLE:
          _debug_port->println(F("Info: no extra identification available (part 2)"));
          break;
        
        case RECEIVED_EXTRA_ID_PART2:
          _debug_port->println(F("Info: received extra identification (part 2)"));
          break;
        
        case NO_EXTRA_ID_PART3_AVAILABLE:
          _debug_port->println(F("Info: no extra identification available (part 3)"));
          break;
        
        case RECEIVED_EXTRA_ID_PART3:
          _debug_port->println(F("Info: received extra identification (part 3)"));
          break;
        
        case OUTPUT_TESTS_NOT_SUPPORTED:
          _debug_port->println(F("Info: output tests not supported"));
          break;
        
        case RECEIVED_OUTPUT_TEST:
          _debug_port->println(F("Info: performing output test"));
          break;
        
        case END_OF_OUTPUT_TESTS:
          _debug_port->println(F("Info: end of output tests"));
          break;
        
        case DISCONNECT_INFO:
          _debug_port->println(F("Info: a \"timed out\" error is normal here"));
          break;
      }
    }
  #else
    (void) type;
  #endif
} 
  
/**
  Function:
    show_command_description(uint8_t command)
  
  Parameters:
    command -> command which to describe
  
  Description:
    Describes the received command if debugging traffic+info is enabled.
*/  
void KLineKWP1281Lib::show_command_description(uint8_t command)
{
  #ifdef KWP1281_DEBUG_SUPPORTED
    if (_debug_port) {
      switch (command) {
        ///SPECIAL
        
        case KWP_ACKNOWLEDGE:
          _debug_port->println(F("\"Acknowledgement\""));
          break;
        
        case KWP_REFUSE:
          _debug_port->println(F("\"Refuse\""));
          break;
        
        case KWP_DISCONNECT:
          _debug_port->println(F("\"Disconnect\""));
          break;
          
        ///SENT
        
        case KWP_REQUEST_EXTRA_ID:
          _debug_port->println(F("\"Request extra identification\""));
          break;
        
        case KWP_REQUEST_LOGIN:
          _debug_port->println(F("\"Login\""));
          break;
        
        case KWP_REQUEST_RECODE:
          _debug_port->println(F("\"Recode\""));
          break;
        
        case KWP_REQUEST_FAULT_CODES:
          _debug_port->println(F("\"Request fault codes\""));
          break;
        
        case KWP_REQUEST_CLEAR_FAULTS:
          _debug_port->println(F("\"Clear fault codes\""));
          break;
        
        case KWP_REQUEST_ADAPTATION:
          _debug_port->println(F("\"Request adaptation value\""));
          break;
        
        case KWP_REQUEST_ADAPTATION_TEST:
          _debug_port->println(F("\"Test adaptation value\""));
          break;
        
        case KWP_REQUEST_ADAPTATION_SAVE:
          _debug_port->println(F("\"Save adaptation value\""));
          break;
        
        case KWP_REQUEST_BASIC_SETTING:
        case KWP_REQUEST_BASIC_SETTING_0:
          _debug_port->println(F("\"Request basic setting\""));
          break;
        
        case KWP_REQUEST_GROUP_READING:
        case KWP_REQUEST_GROUP_READING_0:
          _debug_port->println(F("\"Request group reading\""));
          break;
        
        case KWP_REQUEST_READ_ROM:
          _debug_port->println(F("\"Request ROM reading\""));
          break;
        
        case KWP_REQUEST_OUTPUT_TEST:
          _debug_port->println(F("\"Request output test\""));
          break;
          
        ///RECEIVED
        
        case KWP_RECEIVE_ID_DATA:
          _debug_port->println(F("\"Provide identification\""));
          break;
        
        case KWP_RECEIVE_FAULT_CODES:
          _debug_port->println(F("\"Provide fault codes\""));
          break;
        
        case KWP_RECEIVE_ADAPTATION:
          _debug_port->println(F("\"Provide adaptation value\""));
          break;
        
        case KWP_RECEIVE_BASIC_SETTING:
          _debug_port->println(F("\"Provide basic setting\""));
          break;
        
        case KWP_RECEIVE_GROUP_READING:
          _debug_port->println(F("\"Provide group reading\""));
          break;
        
        case KWP_RECEIVE_ROM:
          _debug_port->println(F("\"Provide ROM reading\""));
          break;
        
        case KWP_RECEIVE_OUTPUT_TEST:
          _debug_port->println(F("\"Execute output test\""));
          break;
        
        default:
          _debug_port->println(F("\"Unknown\""));
          break;
      }
    }
  #else
    (void) command;
  #endif
}
