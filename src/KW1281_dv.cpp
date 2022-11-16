#include <Arduino.h>
#include "KW1281_dv.h"

KW1281_dv::KW1281_dv(uint8_t receivePin, uint8_t transmitPin) { //constructor
  _OBD_TX_PIN = transmitPin; //save pin connections globally to later use in the bitbanged function
  _OBD_RX_PIN = receivePin;

  pinMode(transmitPin, OUTPUT);
  digitalWrite(transmitPin, HIGH);

  obd = new NewSoftwareSerial(receivePin, transmitPin, false); // RX, TX, inverse logic
}

KW1281_dv::~KW1281_dv() { //deconstructor
  delete obd;
  obd = NULL;
}

void KW1281_dv::bitBang5Baud(uint8_t address) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("bitBang5Baud"));
#endif
  uint8_t bits[bitcount]; //bitcount=10
  uint8_t parity = 1;
  uint8_t bit;
  for (int i = 0; i < bitcount; i++) {
    bit = 0;
    if (i == 0)  bit = 0;
    else if (i == 8) bit = parity; //compute parity bit
    else if (i == 9) bit = 1;
    else {
      bit = (uint8_t) ((address & (1 << (i - 1))) != 0);
      parity ^= bit;
    }
    bits[i] = bit;
  }
  //send bit stream
  for (int i = 0; i <= bitcount; i++) {
    if (i != 0) {
      //wait 200 ms = 5 baud
      //delay(200);
      if_wait_5baud();
      if (i == bitcount) break;
    }
    if (bits[i] == 1) {
      digitalWrite(_OBD_TX_PIN, HIGH);
    } else {
      digitalWrite(_OBD_TX_PIN, LOW);
    }
  }
  obd->flush();
}

char KW1281_dv::obdRead() {
  unsigned long timeout = millis() + 1000;
  while (!obd->available()) {
    if (millis() >= timeout) {
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: obdRead timeout"));
#endif
      reset();
      errorTimeout++;
      return RETURN_TIMEOUT_ERROR;
    }
  }
  uint8_t data = obd->read();
  return data; //return the byte we read
}

char KW1281_dv::KWPSendBlock(char *sendMessage, int sendMessageSize) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("KWPSendBlock"));
#endif
  for (int i = 0; i < sendMessageSize; i++) {
    uint8_t data = sendMessage[i]; //the single byte we are currently sending
#ifdef DEBUG_SHOW_SENT
    Serial.println(data, HEX); //show the byte we are sending
#endif
    obd->write(data); //write the current byte

    if (i < (sendMessageSize - 1)) { //last byte of message doesn't receive a complement
      uint8_t complement = obdRead(); //read the complement
#ifdef DEBUG_SHOW_RECEIVED
      Serial.print('\t'); //tab
      Serial.println(complement, HEX); //show what complement the control module sent
#endif
      if (complement != (data ^ 0xFF)) { //if we received something other than the expected complement, it's a comm error
#ifdef DEBUG_SHOW_ERRORS
        Serial.println(F("ERROR: invalid complement"));
#endif
        reset();
        errorData++;
        return RETURN_COMPLEMENT_ERROR;
      }
    }
  }
  blockCounter++; //increment sequence counter, after 0xFF protocol says it should roll over to 0, which we don't need to handle as that's normal behaviour for unsigned 8-bit variables
  return RETURN_SUCCESS;
}

char KW1281_dv::KWPReceiveBlock(char receivedMessage[], int maxMessageSize, int &receivedMessageSize) {
  //always call this function with the last parameter=0
  //except in the connect() function, where we call it with receivedMessageSize=3 because we know we are expecting exactly 3 bytes
  //otherwise, call it with 0 and the received size will be contained in that parameter after the function is done
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("KWPReceiveBlock"));
#endif
  bool ackEachByte = false;
  uint8_t data;
  int receivedMessageCounter = 0;

  if (receivedMessageSize == 0) ackEachByte = true;
  if (receivedMessageSize > maxMessageSize) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println("ERROR: invalid maxMessageSize");
#endif
    return RETURN_MAXMSGSIZE_ERROR;
  }

  unsigned long timeout = millis() + 1000;
  
  while ((receivedMessageCounter == 0) || (receivedMessageCounter != receivedMessageSize)) {
    while (obd->available()) {
      data = obdRead();
#ifdef DEBUG_SHOW_RECEIVED
      Serial.print('\t');
      Serial.println(data, HEX);
#endif
      receivedMessage[receivedMessageCounter] = data;
      receivedMessageCounter++;

      if ((receivedMessageSize == 0) && (receivedMessageCounter == 1)) {
        //the control module sends how long the message it is sending is
        //but that value is not including that byte itself, so the real length is that byte+1
        receivedMessageSize = data + 1;

        if (receivedMessageSize > maxMessageSize) { //if what we read is exceeding the max we gave to the function
#ifdef DEBUG_SHOW_ERRORS
          Serial.println("ERROR: invalid maxMessageSize");
#endif
          return RETURN_MAXMSGSIZE_ERROR;
        }
      }
      if ((ackEachByte) & (receivedMessageCounter == 2)) { //the third byte in the received message indicates the sequence number
        if (data != blockCounter) {
#ifdef DEBUG_SHOW_ERRORS
          Serial.println(F("ERROR: invalid blockCounter"));
#endif
          reset();
          errorData++;
          return RETURN_BLOCKCOUNT_ERROR;
        }
      }
      //if everything is alright, proceed to send our complement to what the control module is sending
      if (((!ackEachByte) && (receivedMessageCounter == receivedMessageSize)) || ((ackEachByte) && (receivedMessageCounter < receivedMessageSize))) {
#ifdef DEBUG_SHOW_SENT
        Serial.println((data ^ 0xFF), HEX);
#endif
        obd->write(data ^ 0xFF); //send complement
      }
      timeout = millis() + 1000;
    }
    if (millis() >= timeout) {
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: timeout"));
#endif
      reset();
      errorTimeout++;
      return RETURN_TIMEOUT_ERROR;
    }
  }
  blockCounter++; //increment sequence counter
  return RETURN_SUCCESS;
}

/* * *
  Format of message block:
  SIZ BLK CMD (PRM) END
    meaning:
      SIZ = message size, not including that byte itself
      BLK = sequence number
      CMD = command byte, defined in the .h
     (PRM)= parameters if the command requires them
      END = always 0x03
* * */

char KW1281_dv::KWPSendAckBlock() {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("KWPSendAckBlock"));
#endif
  //format of message block:
  //SZ BL CMD END
  //meaning:
  //SZ = message size, not including that byte itself
  //BL = sequence number
  //CMD = command byte, 0x09 for ack
  //END = always 0x03
  char buf[4] = {0x03, 0x00, KWP_ACK, 0x03};
  buf[1] = blockCounter;
  return (KWPSendBlock(buf, 4));
}

char KW1281_dv::keepAlive() {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("KWPKeepAlive"));
#endif
  char s[64] = {0x03, 0x00, KWP_ACK, 0x03};
  s[1] = blockCounter;
  KWPSendBlock(s, 4);

  int size = 0;
  if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  if (size == 0) return RETURN_NULLSIZE_ERROR;
  if (s[2] != KWP_ACK) { //if it's not an answer to our faults reading request
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: invalid answer"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
}

//when the connection is successful, the control module sends text regarding to its model number
//and coding + workshop code
char KW1281_dv::KWPReadAscii(char id[], uint16_t &coding, uint32_t &wsc) {
  memset(id, 0, (size_t)(3 * 12 + 1));
  uint8_t charCounter = 0;
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("KWPReadAscii"));
#endif
#ifdef DEBUG_SHOW_INTRO_ID
  Serial.println(F("-----ID:"));
#endif
  bool first = true;
  while (true) {
    int size = 0;
    char recvMessage[65];
    if (KWPReceiveBlock(recvMessage, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
    if (size == 0) return RETURN_NULLSIZE_ERROR;
    if (recvMessage[2] == KWP_ACK) break; //if we got an ack block then it finished sending the ascii data
    if (recvMessage[2] != KWP_R_ASCII_DATA) { //if it's not sending ASCII
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: unexpected answer"));
#endif
      reset();
      errorData++;
      return RETURN_UNEXPECTEDANSWER_ERROR;
    }

    if (size == 9) {
      uint8_t b1 = recvMessage[4] &= 0xFF,
                                     b2 = recvMessage[5] &= 0xFF,
                                         b3 = recvMessage[6] &= 0xFF,
                                             b4 = recvMessage[7] &= 0xFF;

      coding = ((b1 << 8) + ((b2 >> 4) << 4)) >> 1;
      wsc = (((uint32_t)b2 & 0xF) << 16) + (b3 << 8) + b4;
      break;
    } else {
      if (first) {
        first = false;
        recvMessage[3] -= 0x80;
      }
      memcpy(&id[charCounter], &recvMessage[3], (size_t)(size - 4));
      charCounter += size - 4;
#ifdef DEBUG_SHOW_INTRO_ID
      recvMessage[size - 1] = '\0';
      Serial.println(&recvMessage[3]);
#endif
    }
    if (KWPSendAckBlock() != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  }
#ifdef DEBUG_SHOW_INTRO_ID
  Serial.println(F("--------"));
#endif
  return RETURN_SUCCESS;
}

char KW1281_dv::KWPReadAscii(uint16_t &coding, uint32_t &wsc) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("KWPReadAscii"));
#endif
  while (true) {
    int size = 0;
    char recvMessage[65];
    if (KWPReceiveBlock(recvMessage, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
    if (size == 0) return RETURN_NULLSIZE_ERROR;
    if (recvMessage[2] == KWP_ACK) break; //if we got an ack block then it finished sending the ascii data
    if (recvMessage[2] != KWP_R_ASCII_DATA) { //if it's not sending ASCII
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: unexpected answer"));
#endif
      reset();
      errorData++;
      return RETURN_UNEXPECTEDANSWER_ERROR;
    }

    if (size == 9) {
      uint8_t b1 = recvMessage[4] &= 0xFF,
              b2 = recvMessage[5] &= 0xFF,
              b3 = recvMessage[6] &= 0xFF,
              b4 = recvMessage[7] &= 0xFF;

      coding = ((b1 << 8) + ((b2 >> 4) << 4)) >> 1;
      wsc = (((uint32_t)b2 & 0xF) << 16) + (b3 << 8) + b4;
      break;
    }
    if (KWPSendAckBlock() != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  }
  return RETURN_SUCCESS;
}

char KW1281_dv::KWPReadAscii() {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("KWPReadAscii"));
#endif
  bool first = true;
  while (true) {
    int size = 0;
    char recvMessage[65];
    if (KWPReceiveBlock(recvMessage, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
    if (size == 0) return RETURN_NULLSIZE_ERROR;
    if (recvMessage[2] == KWP_ACK) break; //if we got an ack block then it finished sending the ascii data
    if (recvMessage[2] != KWP_R_ASCII_DATA) { //if it's not sending ASCII
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: unexpected answer"));
#endif
      reset();
      errorData++;
      return RETURN_UNEXPECTEDANSWER_ERROR;
    }
    if (size == 9) break;
    if (KWPSendAckBlock() != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  }
  return RETURN_SUCCESS;
}

char KW1281_dv::connect(uint8_t address, int baud, char id[], uint16_t &coding, uint32_t &wsc) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("connect"));
#endif
#ifdef DEBUG
  Serial.print(F("Connecting to ")); Serial.print(address, HEX); Serial.print(F(" at baud ")); Serial.println(baud);
#endif

  //call the function which was defined by the user
  //usually it would contain a 1-second delay and reinitialising variables
  if_has_reset();

  connected = false;
  blockCounter = 0; //reset the sequence counter
  currAddr = 0;

  obd->begin(baud); //init SoftwareSerial

  bitBang5Baud(address); //send the slow init sequence

  //if the connection was successful, we should get the message 55 01 8A
  char s[3];
  int size = 3;
  if (KWPReceiveBlock(s, 3, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR; //receive the block of 3 bytes
  
  if ((((uint8_t)s[0]) != 0x55) || (((uint8_t)s[1]) != 0x01) || (((uint8_t)s[2]) != 0x8A)) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: Unexpected init response"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  currAddr = address;
  connected = true;
  //now read the text data the control module always sends when connected
  if (KWPReadAscii(id, coding, wsc) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  return RETURN_SUCCESS;
}

char KW1281_dv::connect(uint8_t address, int baud, uint16_t &coding, uint32_t &wsc) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("connect"));
#endif
#ifdef DEBUG
  Serial.print(F("Connecting to ")); Serial.print(address, HEX); Serial.print(F(" at baud ")); Serial.println(baud);
#endif

  //call the function which was defined by the user
  //usually it would contain a 1-second delay and reinitialising variables
  if_has_reset();

  connected = false;
  blockCounter = 0; //reset the sequence counter
  currAddr = 0;

  obd->begin(baud); //init SoftwareSerial

  bitBang5Baud(address); //send the slow init sequence

  //if the connection was successful, we should get the message 55 01 8A
  char s[3];
  int size = 3;
  if (KWPReceiveBlock(s, 3, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR; //receive the block of 3 bytes

  if ((((uint8_t)s[0]) != 0x55) || (((uint8_t)s[1]) != 0x01) || (((uint8_t)s[2]) != 0x8A)) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: Unexpected init response"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  currAddr = address;
  connected = true;
  //now read the text data the control module always sends when connected
  if (KWPReadAscii(coding, wsc) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  return RETURN_SUCCESS;
}

char KW1281_dv::connect(uint8_t address, int baud) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("connect"));
#endif
#ifdef DEBUG
  Serial.print(F("Connecting to ")); Serial.print(address, HEX); Serial.print(F(" at baud ")); Serial.println(baud);
#endif

  //call the function which was defined by the user
  //usually it would contain a 1-second delay and reinitialising variables
  if_has_reset();

  connected = false;
  blockCounter = 0; //reset the sequence counter
  currAddr = 0;

  obd->begin(baud); //init SoftwareSerial

  bitBang5Baud(address); //send the slow init sequence

  //if the connection was successful, we should get the message 55 01 8A
  char s[3];
  int size = 3;
  if (KWPReceiveBlock(s, 3, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR; //receive the block of 3 bytes

  if ((((uint8_t)s[0]) != 0x55) || (((uint8_t)s[1]) != 0x01) || (((uint8_t)s[2]) != 0x8A)) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: Unexpected init response"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  currAddr = address;
  connected = true;
  //now read the text data the control module always sends when connected
  if (KWPReadAscii() != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  return RETURN_SUCCESS;
}

void KW1281_dv::reset() {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("reset"));
#endif
  connected = false;
  currAddr = 0;
}

void KW1281_dv::disconnect() {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("disconnect"));
#endif

  char s[4] = {0x03, 0x00, KWP_DISCONNECT, 0x03};
  s[1] = blockCounter;
  KWPSendBlock(s, 4);

  connected = false;
  currAddr = 0;
}

char KW1281_dv::readFaults(uint16_t faults[], uint8_t startFrom, uint8_t amount) {
  dtcCounter = 0;
  memset(faults, 0, amount * sizeof(uint16_t*));
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("readFaults"));
#endif
  char s[64] = {0x03, 0x00, KWP_READ_FAULTS, 0x03};
  s[1] = blockCounter;
  KWPSendBlock(s, 4);
#ifdef DEBUG
  Serial.println(F("---DTCs:"));
#endif
  while (true) {
    int size = 0;
    if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
    if (size == 0) return RETURN_NULLSIZE_ERROR;
    if (s[2] == KWP_ACK) break;
    if (s[2] != KWP_R_FAULTS) { //if it's not an answer to our faults reading request
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: invalid answer"));
#endif
      reset();
      errorData++;
      return RETURN_UNEXPECTEDANSWER_ERROR;
    }
    int count = (size - 4) / 3;
    for (int idx = 0; idx < count; idx++) {
      byte hi = s[3 + idx * 3];
      byte lo = s[3 + idx * 3 + 1];
      byte sts = s[3 + idx * 3 + 2];

      if (hi == 0xFF && lo == 0xFF && sts == 0x88) {
#ifdef DEBUG
        Serial.print(F("No DTCs stored!"));
#endif
      } else {
        uint16_t dtc = (hi << 8) + lo;
        if (dtcCounter >= startFrom && dtcCounter - startFrom < amount) {
          faults[dtcCounter - startFrom] = dtc;
        }
#ifdef DEBUG
        char fault[6];
        sprintf(fault, "%05d", dtc);
        Serial.print(fault);
        Serial.print(' ');
#endif
        dtcCounter++;
      }
    }
    if (KWPSendAckBlock() != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  }
#ifdef DEBUG
  Serial.println();
  Serial.println(F("--------"));
#endif
  return dtcCounter;
}

char KW1281_dv::clearFaults() {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("clearFaults"));
#endif
  char s[64] = {0x03, 0x00, KWP_CLEAR_FAULTS, 0x03};
  s[1] = blockCounter;
  KWPSendBlock(s, 4);

  int size = 0;
  if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  if (size == 0) return RETURN_NULLSIZE_ERROR;
  if (s[2] != KWP_ACK) { //if it's not an answer (ack) to our faults clearing request
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: invalid answer"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
#ifdef DEBUG_SHOW_ERRORS
  Serial.println(F("INFO: Cleared all DTCs"));
#endif
  return RETURN_SUCCESS;
}

char KW1281_dv::Login(int code, uint32_t wsc) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("Login"));
#endif
  char s[64] = {0x08, 0x00, KWP_LOGIN, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = highByte(code);
  s[4] = lowByte(code);
  s[5] = (wsc >> 16);
  s[6] = ((wsc & 0xFFFF) >> 8);
  s[7] = (wsc & 0xFF);
  KWPSendBlock(s, 9);

  int size = 0;
  if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  if (size == 0) return RETURN_NULLSIZE_ERROR;
  if (s[2] == KWP_ACK) {
    return RETURN_SUCCESS;
  }
  if (s[2] == KWP_NAK) {
    return RETURN_LOGIN_ERROR;
  }
}

char KW1281_dv::Coding(uint16_t &coding, uint32_t wsc) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("Coding"));
#endif
#ifdef DEBUG
  Serial.print(F("Recoding: "));
  Serial.print(coding);
  Serial.print(F(", WSC: "));
  Serial.println(wsc);
#endif
  char s[64] = {0x07, 0x00, KWP_RECODING, 0x00, 0x00, 0x00, 0x00, 0x03};
  s[1] = blockCounter;
  coding = coding << 1;
  s[3] = highByte(coding);
  s[4] = lowByte(coding) + (wsc >> 16);
  s[5] = (wsc & 0xFFFF) >> 8;
  s[6] = (wsc & 0xFF);
  KWPSendBlock(s, 8);
  int size = 0;
  if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  if (size == 0) return RETURN_NULLSIZE_ERROR;
  if (s[2] != KWP_R_ASCII_DATA) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: unexpected answer"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  s[0] = 0x03;
  s[1] = blockCounter;
  s[2] = KWP_READ_IDENT;
  s[3] = 0x03;
  KWPSendBlock(s, 4);
  while (true) {
    size = 0;
    if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
    if (size == 0) return RETURN_NULLSIZE_ERROR;
    if (s[2] != KWP_R_ASCII_DATA) {
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: invalid answer"));
#endif
      reset();
      errorData++;
      return RETURN_UNEXPECTEDANSWER_ERROR;
    }
    if (size == 9) {
      uint8_t b1 = s[4] &= 0xFF,
                           b2 = s[5] &= 0xFF,
                                        b3 = s[6] &= 0xFF,
                                            b4 = s[7] &= 0xFF;


      coding = ((b1 << 8) + ((b2 >> 4) << 4)) >> 1;
      wsc = (((uint32_t)b2 & 0xF) << 16) + (b3 << 8) + b4;

#ifdef DEBUG
      Serial.print(F("Recoded: "));
      Serial.print(coding);
      Serial.print(F(", WSC: "));
      Serial.println(wsc);
#endif

      break;
    } else {
      if (KWPSendAckBlock() != RETURN_SUCCESS) return RETURN_SEND_ERROR;
    }
  }

  s[0] = 0x03;
  s[1] = blockCounter;
  s[2] = KWP_READ_IDENT;
  s[3] = 0x03;
  KWPSendBlock(s, 4);

  while (true) {
    size = 0;
    if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
    if (size == 0) return RETURN_NULLSIZE_ERROR;
    if (s[2] == KWP_ACK) break;
    if (s[2] != KWP_R_ASCII_DATA) {
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: invalid answer"));
#endif
      reset();
      errorData++;
      return RETURN_UNEXPECTEDANSWER_ERROR;
    }
    if (KWPSendAckBlock() != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  }
  return RETURN_SUCCESS;
}

uint16_t KW1281_dv::readAdaptation(uint8_t channel) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("readAdaptation"));
#endif
  char s[65] = {0x04, 0x00, KWP_ADAPTATION, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = (char)channel;
  KWPSendBlock(s, 5);

  int size = 0;
  if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  if (size == 0) return RETURN_NULLSIZE_ERROR;
  if (s[2] == KWP_NAK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: invalid channel - "));
    Serial.println(channel);
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] != KWP_R_ADAPTATION) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: unexpected answer: "));
    Serial.println(s[2], HEX);
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  if (s[3] != (char)channel) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: wrong response given: "));
    Serial.println(s[3], HEX);
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  uint16_t contents = ((s[4] & 0xFF) << 8) + (s[5] & 0xFF);
  return contents;
}

uint16_t KW1281_dv::testAdaptation(uint8_t channel, uint16_t value) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("testAdaptation"));
#endif
  char s[65] = {0x06, 0x00, KWP_TEST_ADAPTATION, 0x00, 0x00, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = (char)channel;
  s[4] = (char)highByte(value);
  s[5] = (char)lowByte(value);
  KWPSendBlock(s, 7);

  int size = 0;
  if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  if (size == 0) return RETURN_NULLSIZE_ERROR;
  if (s[2] == KWP_NAK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: invalid channel - "));
    Serial.println(channel);
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] != KWP_R_ADAPTATION) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: unexpected answer: "));
    Serial.println(s[2], HEX);
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  if (s[3] != (char)channel) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: wrong response given: "));
    Serial.println(s[3], HEX);
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }

  uint16_t contents = ((s[4] & 0xFF) << 8) + (s[5] & 0xFF);
  if (contents == value) {
    return RETURN_SUCCESS;
  } else {
    return contents;
  }
}

uint16_t KW1281_dv::Adaptation(uint8_t channel, uint16_t value, uint32_t wsc) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("Adaptation"));
#endif
  char s[65] = {0x09, 0x00, KWP_SAVE_ADAPTATION, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = (char)channel;
  s[4] = (char)highByte(value);
  s[5] = (char)lowByte(value);
  s[6] = (char)(wsc >> 16);
  s[7] = (char)((wsc >> 8) & 0xFF);
  s[8] = (char)(wsc & 0xFF);
  KWPSendBlock(s, 10);

  int size = 0;
  if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
  if (size == 0) return RETURN_NULLSIZE_ERROR;
  if (s[2] == KWP_NAK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: invalid channel or set value!"));
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] != KWP_R_ADAPTATION) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: unexpected answer: "));
    Serial.println(s[2], HEX);
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }
  if (s[3] != (char)channel) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("ERROR: wrong response given: "));
    Serial.println(s[3], HEX);
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }

  uint16_t contents = ((s[4] & 0xFF) << 8) + (s[5] & 0xFF);
  return contents;
}

char KW1281_dv::VIN(char vin[]) {
  memset(vin, 0, (size_t)(3 * 12 + 1));
  uint8_t charCounter = 0;
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("VIN"));
#endif
  char s[65] = {0x03, 0x00, KWP_READ_IDENT, 0x03};
  s[1] = blockCounter;
  KWPSendBlock(s, 4);

#ifdef DEBUG
  Serial.println(F("---VIN:"));
#endif
  while (true) {
    int size = 0;
    if (KWPReceiveBlock(s, 64, size) != RETURN_SUCCESS) return RETURN_RECEIVE_ERROR;
    if (size == 0) return RETURN_NULLSIZE_ERROR;
    if (s[2] == KWP_ACK) break;
    if (s[2] != KWP_R_ASCII_DATA) { //if it's not an answer to our VIN reading request
#ifdef DEBUG_SHOW_ERRORS
      Serial.println(F("ERROR: unexpected answer"));
#endif
      reset();
      errorData++;
      return RETURN_UNEXPECTEDANSWER_ERROR;
    }
    memcpy(&vin[charCounter], &s[3], (size_t)(size - 4));
    charCounter += size - 4;
#ifdef DEBUG
    s[size - 1] = '\0';
    Serial.println(&s[3]);
#endif
    if (KWPSendAckBlock() != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  }
#ifdef DEBUG
  Serial.println(F("-------"));
#endif
  return RETURN_SUCCESS;
}

char KW1281_dv::MeasBlocks(uint8_t group, float &result1, float &result2, float &result3, float &result4) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("MeasBlocks"));
#endif
  char s[64] = {0x04, 0x00, KWP_GROUP_READING, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = group;
  if (KWPSendBlock(s, 5) != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  int size = 0;
  KWPReceiveBlock(s, 64, size);
  if (s[2] == KWP_NAK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("INFO: This group is not valid!"));
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] == KWP_ACK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("INFO: This group is valid, but there are no measurements available!"));
    Serial.println(group);
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] != KWP_R_GROUP_READING) { //if it's not an answer to our group reading request
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: unexpected answer"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }

  int count = (size - 4) / 3; //how many values are in the group

  switch (count) {
    case 1:
      result2 = -1;
      result3 = -1;
      result4 = -1;
      break;
    case 2:
      result3 = -1;
      result4 = -1;
      break;
    case 3:
      result4 = -1;
      break;
  }

  for (int idx = 0; idx < count; idx++) {
    byte type = s[3 + idx * 3];
    byte a = s[3 + idx * 3 + 1];
    byte b = s[3 + idx * 3 + 2];
#ifdef DEBUG_SHOW_GROUP_VALUES_RAW
    Serial.print(F("type="));
    Serial.print(type);
    Serial.print(F("  a="));
    Serial.print(a);
    Serial.print(F("  b="));
    Serial.print(b);
    Serial.print(' ');
#endif

    float v = calculateResult(type, a, b);
    //Serial.println(v);

    if (idx == 0) {
      result1 = v;
    }
    if (idx == 1) {
      result2 = v;
    }
    if (idx == 2) {
      result3 = v;
    }
    if (idx == 3) {
      result4 = v;
    }
  }
  return RETURN_SUCCESS;
}

char KW1281_dv::MeasBlocksWithUnits(uint8_t group, float &result1, float &result2, float &result3, float &result4, uint8_t &t1, uint8_t &t2, uint8_t &t3, uint8_t &t4) {
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("MeasBlocks"));
#endif
#ifdef DEBUG
  Serial.print(F("---Group: "));
  Serial.println(group);
#endif
  char s[64] = {0x04, 0x00, KWP_GROUP_READING, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = group;
  if (KWPSendBlock(s, 5) != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  int size = 0;
  KWPReceiveBlock(s, 64, size);
  if (s[2] == KWP_NAK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("INFO: This group is not valid!"));
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] == KWP_ACK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("INFO: This group is valid, but there are no measurements available!"));
    Serial.println(group);
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] != KWP_R_GROUP_READING) { //if it's not an answer to our group reading request
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: unexpected answer"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }

  int count = (size - 4) / 3; //how many values are in the group

  switch (count) {
    case 1:
      result2 = -1;
      result3 = -1;
      result4 = -1;
      break;
    case 2:
      result3 = -1;
      result4 = -1;
      break;
    case 3:
      result4 = -1;
      break;
  }

  for (int idx = 0; idx < count; idx++) {
    byte type = s[3 + idx * 3];
    byte a = s[3 + idx * 3 + 1];
    byte b = s[3 + idx * 3 + 2];
#ifdef DEBUG_SHOW_GROUP_VALUES_RAW
    Serial.print(F("type="));
    Serial.print(type);
    Serial.print(F("  a="));
    Serial.print(a);
    Serial.print(F("  b="));
    Serial.print(b);
    Serial.print(' ');
#endif

    float v = calculateResult(type, a, b);

    if (idx == 0) {
      result1 = v;
      t1 = type;
    }
    if (idx == 1) {
      result2 = v;
      t2 = type;
    }
    if (idx == 2) {
      result3 = v;
      t3 = type;
    }
    if (idx == 3) {
      result4 = v;
      t4 = type;
    }
  }
  return RETURN_SUCCESS;
}

char KW1281_dv::SingleReading(uint8_t group, uint8_t index, float &result) { //returns the formula type, pass by reference the a and b
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("MeasBlocks"));
#endif
#ifdef DEBUG
  Serial.print(F("---Group: "));
  Serial.println(group);
#endif
  char s[64] = {0x04, 0x00, KWP_GROUP_READING, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = group;
  if (KWPSendBlock(s, 5) != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  int size = 0;
  KWPReceiveBlock(s, 64, size);
  if (s[2] == KWP_NAK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("INFO: This group is not valid!"));
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] == KWP_ACK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("INFO: This group is valid, but there are no measurements available!"));
    Serial.println(group);
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] != KWP_R_GROUP_READING) { //if it's not an answer to our group reading request
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: unexpected answer"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }

  int count = (size - 4) / 3; //how many values are in the group

  byte type = s[3 + index * 3];
  uint8_t a = s[3 + index * 3 + 1];
  uint8_t b = s[3 + index * 3 + 2];
#ifdef DEBUG_SHOW_GROUP_VALUES_RAW
  Serial.print(F("type="));
  Serial.print(type);
  Serial.print(F("  a="));
  Serial.print(a);
  Serial.print(F("  b="));
  Serial.print(b);
  Serial.print(' ');
#endif
  result = calculateResult(type, a, b);
  return RETURN_SUCCESS;
}

char KW1281_dv::SingleReadingWithUnits(uint8_t group, uint8_t index, uint8_t &t, float &result) { //returns the formula type, pass by reference the a and b
#ifdef DEBUG_SHOW_CURRENT_FUNCTION
  Serial.println(F("MeasBlocks"));
#endif
#ifdef DEBUG
  Serial.print(F("---Group: "));
  Serial.println(group);
#endif
  char s[64] = {0x04, 0x00, KWP_GROUP_READING, 0x00, 0x03};
  s[1] = blockCounter;
  s[3] = group;
  if (KWPSendBlock(s, 5) != RETURN_SUCCESS) return RETURN_SEND_ERROR;
  int size = 0;
  KWPReceiveBlock(s, 64, size);
  if (s[2] == KWP_NAK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("INFO: This group is not valid!"));
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] == KWP_ACK) {
#ifdef DEBUG_SHOW_ERRORS
    Serial.print(F("INFO: This group is valid, but there are no measurements available!"));
    Serial.println(group);
#endif
    return RETURN_INVALID_ERROR;
  }
  if (s[2] != KWP_R_GROUP_READING) { //if it's not an answer to our group reading request
#ifdef DEBUG_SHOW_ERRORS
    Serial.println(F("ERROR: unexpected answer"));
#endif
    reset();
    errorData++;
    return RETURN_UNEXPECTEDANSWER_ERROR;
  }

  int count = (size - 4) / 3; //how many values are in the group

  byte type = s[3 + index * 3];
  byte a = s[3 + index * 3 + 1];
  byte b = s[3 + index * 3 + 2];
#ifdef DEBUG_SHOW_GROUP_VALUES_RAW
  Serial.print(F("type="));
  Serial.print(type);
  Serial.print(F("  a="));
  Serial.print(a);
  Serial.print(F("  b="));
  Serial.print(b);
  Serial.print(' ');
#endif
  result = calculateResult(type, a, b);
  t = type;
  return RETURN_SUCCESS;
}

float KW1281_dv::calculateResult(uint8_t formula, uint8_t a, uint8_t b) {
  float v = 0;
  switch (formula) {
    case 1:  v = 0.2 * a * b;                                          break;
    case 2:  v = a * 0.002 * b;                                        break;
    case 3:  v = 0.002 * a * b;                                        break;
    case 4:  v = abs(b - 127) * 0.01 * a;                              break;
    case 5:  v = a * (b - 100) * 0.1;                                  break;
    case 6:  v = 0.001 * a * b;                                        break;
    case 7:  v = 0.01 * a * b;                                         break;
    case 8:  v = 0.1 * a * b;                                          break;
    case 9:  v = (b - 127) * 0.02 * a;                                 break;
    case 10: v = b; /*if (b == 0) t = F("COLD"); else t = F("WARM");*/ break;
    case 11: v = 0.0001 * a * (b - 128) + 1;                           break;
    case 12: v = 0.001 * a * b;                                        break;
    case 13: v = (b - 127) * 0.001 * a;                                break;
    case 14: v = 0.005 * a * b;                                        break;
    case 15: v = 0.01 * a * b;                                         break;
    case 16: v = 0;                                                      break;
    case 17: v = 0;                                                      break;
    case 18: v = 0.04 * a * b;                                         break;
    case 19: v = a * b * 0.01;                                         break;
    case 20: v = a * (b - 128) / 128;                                  break;
    case 21: v = 0.001 * a * b;                                        break;
    case 22: v = 0.001 * a * b;                                        break;
    case 23: v = b / 256 * a;                                          break;
    case 24: v = 0.001 * a * b;                                        break;
    case 25: v = (b * 1.421) + (a / 182);                              break;
    case 26: v = float(b - a);                                         break;
    case 27: v = abs(b - 128) * 0.01 * a;                              break;
    case 28: v = float(b - a);                                         break;
    case 29: v = 0;                                                      break;
    case 30: v = b / 12 * a;                                           break;
    case 31: v = b / 2560 * a;                                         break;
    case 32: if (b > 128 )v = b - 256; else v = b;                             break;
    case 33: v = 100 * b / a;                                          break;
    case 34: v = (b - 128) * 0.01 * a;                                 break;
    case 35: v = 0.01 * a * b;                                         break;
    case 36: v = ((unsigned long)a) * 2560 + ((unsigned long)b) * 10;  break;
    case 37: v = 0;                                                    break; //legend says no one will ever fix the oil pressure
    case 38: v = (b - 128) * 0.001 * a;                                break;
    case 39: v = b / 256 * a;                                          break;
    case 40: v = b * 0.1 + (25.5 * a) - 400;                           break;
    case 41: v = b + a * 255;                                          break;
    case 42: v = b * 0.1 + (25.5 * a) - 400;                           break;
    case 43: v = b * 0.1 + (25.5 * a);                                 break;
    case 44: v = a + ((float)b / 100.00);                                  break; //time
    case 45: v = 0.1 * a * b / 100;                                    break;
    case 46: v = (a * b - 3200) * 0.0027;                              break;
    case 47: v = (b - 128) * a;                                        break;
    case 48: v = b + a * 255;                                          break;
    case 49: v = (b / 4) * a * 0.1;                                    break;
    case 50: v = (b - 128) / (0.01 * a);                               break;
    case 51: v = ((b - 128) / 255) * a;                                break;
    case 52: v = b * 0.02 * a - a;                                     break;
    case 53: v = (b - 128) * 1.4222 + 0.006 * a;                       break;
    case 54: v = a * 256 + b;                                          break;
    case 55: v = a * b / 200;                                          break;
    case 56: v = a * 256 + b;                                          break;
    case 57: v = a * 256 + b + 65536;                                  break;
    case 58: if (b > 128) v = 1.0225 * (256 - b); else v = 1.0225 * b;             break;
    case 59: v = (a * 256 + b) / 32768;                                break;
    case 60: v = (a * 256 + b) * 0.01;                                 break;
    case 61: if (a == 0 )v = b - 128; else v = (float)((b - 128) / a);             break;
    case 62: v = 0.256 * a * b;                                        break;
    case 63: v = 0;                                                    break; //string
    case 64: v = float(a + b);                                         break;
    case 65: v = 0.01 * a * (b - 127);                                 break;
    case 66: v = (a * b) / 511.12;                                     break;
    case 67: v = (640 * a) + b * 2.5;                                  break;
    case 68: v = (256 * a + b) / 7.365;                                break;
    case 69: v = (256 * a + b) * 0.3254;                               break;
    case 70: v = (256 * a + b) * 0.192;                                break;
    default: v = 0;                                                    break;
  }
  return v;
}

void KW1281_dv::getUnits(uint8_t formula, char units[]) {
  //char *units = (char*)malloc(8);

  switch (formula) {
    case 1:  strcpy(units, "rpm");                                 break;
    case 2:  strcpy(units, "%%");                                  break;
    case 3:  strcpy(units, "Deg");                                 break;
    case 4:  strcpy(units, "ATDC");                                break;
    case 5:  strcpy(units, "°C");                                  break;
    case 6:  strcpy(units, "V");                                   break;
    case 7:  strcpy(units, "km/h");                                break;
    case 8:  strcpy(units, " ");                                   break;
    case 9:  strcpy(units, "Deg");                                 break;
    case 10: strcpy(units, " ");                                   break;
    case 11: strcpy(units, " ");                                   break;
    case 12: strcpy(units, "Ohm");                                 break;
    case 13: strcpy(units, "mm");                                  break;
    case 14: strcpy(units, "bar");                                 break;
    case 15: strcpy(units, "ms");                                  break;
    case 16: units[0] = '\0';                                        break;
    case 17: units[0] = '\0';                                        break;
    case 18: strcpy(units, "mbar");                                break;
    case 19: strcpy(units, "L");                                   break;
    case 20: strcpy(units, "%%");                                  break;
    case 21: strcpy(units, "V");                                   break;
    case 22: strcpy(units, "ms");                                  break;
    case 23: strcpy(units, "%%");                                  break;
    case 24: strcpy(units, "A");                                   break;
    case 25: strcpy(units, "g/s");                                 break;
    case 26: strcpy(units, "C");                                   break;
    case 27: strcpy(units, "°");                                   break;
    case 28: strcpy(units, " ");                                   break;
    case 29: units[0] = '\0';                                        break;
    case 30: strcpy(units, "Deg k/w");                             break;
    case 31: strcpy(units, "°C");                                  break;
    case 32: strcpy(units, " ");                                   break;
    case 33: strcpy(units, "%%");                                  break;
    case 34: strcpy(units, "kW");                                  break;
    case 35: strcpy(units, "l/h");                                 break;
    case 36: strcpy(units, "km");                                  break;
    case 37: units[0] = '\0';                                        break;
    case 38: strcpy(units, "Deg k/w");                             break;
    case 39: strcpy(units, "mg/h");                                break;
    case 40: strcpy(units, "A");                                   break;
    case 41: strcpy(units, "Ah");                                  break;
    case 42: strcpy(units, "Kw");                                  break;
    case 43: strcpy(units, "V");                                   break;
    case 44: units[0] = '\0';                                        break; //time
    case 45: strcpy(units, " ");                                   break;
    case 46: strcpy(units, "Deg k/w");                             break;
    case 47: strcpy(units, "ms");                                  break;
    case 48: strcpy(units, " ");                                   break;
    case 49: strcpy(units, "mg/h");                                break;
    case 50: strcpy(units, "mbar");                                break;
    case 51: strcpy(units, "mg/h");                                break;
    case 52: strcpy(units, "Nm");                                  break;
    case 53: strcpy(units, "g/s");                                 break;
    case 54: strcpy(units, "count");                               break;
    case 55: strcpy(units, "s");                                   break;
    case 56: strcpy(units, "WSC");                                 break;
    case 57: strcpy(units, "WSC");                                 break;
    case 58: strcpy(units, " ");                                   break;
    case 59: strcpy(units, "g/s");                                 break;
    case 60: strcpy(units, "sec");                                 break;
    case 61: strcpy(units, " ");                                   break;
    case 62: strcpy(units, "S");                                   break;
    case 63:                                                       break; //string
    case 64: strcpy(units, "Ohm");                                 break;
    case 65: strcpy(units, "mm");                                  break;
    case 66: strcpy(units, "V");                                   break;
    case 67: strcpy(units, "Deg");                                 break;
    case 68: strcpy(units, "deg/s");                               break;
    case 69: strcpy(units, "Bar");                                 break;
    case 70: strcpy(units, "m/s^2");                               break;
    default: /*sprintf(units, "d%02X/%02X", a, b);*/units[0] = '\0'; break;
  }
}

void KW1281_dv::define_reset_function(m_cb functionPointer) {
  usingCustomReset = true;
  res = functionPointer;
}

void KW1281_dv::define_wait_5baud_function(m_cb functionPointer) {
  usingCustomWait = true;
  wt = functionPointer;
}

void KW1281_dv::if_has_reset() {
  if (usingCustomReset) res(); //invoke the callback
  else defaultReset(); //if the user hasn't defined something else, use the default function
}

void KW1281_dv::if_wait_5baud() {
  if (usingCustomWait) wt(); //invoke the callback
  else defaultWait(); //if the user hasn't defined something else, use the default function
}

void KW1281_dv::defaultReset() {
  delay(1000);
#ifdef DEBUG
  Serial.println(F("Default reset"));
#endif
}

void KW1281_dv::defaultWait() {
  delay(200);
#ifdef DEBUG
  Serial.println(F("Default wait"));
#endif
}
