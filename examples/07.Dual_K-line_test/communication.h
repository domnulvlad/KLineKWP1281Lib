//////////////////////////////// K-line 1

//Initializes the serial port
void beginFunction1(unsigned long baud) {
#ifdef RX1_pin
  // The configuration for the ESP32 has both the RX and TX pins defined (for consistency), since they can be mapped to any pins.
  K_line1.begin(baud, SERIAL_8N1, RX1_pin, TX1_pin);
#else
  // For other boards (if RX_pin is not defined), use the standard function.
  K_line1.begin(baud);
#endif
}

//Stops communication on the serial port
void endFunction1() {
  K_line1.end();
}

//Sends a byte
void sendFunction1(uint8_t data) {
  K_line1.write(data);
}

//Receives a byte
bool receiveFunction1(uint8_t &data) {
  if (K_line1.available()) {
    data = K_line1.read();
    return true;
  }
  return false;
}

//Debugging can be enabled in configuration.h in order to print bus traffic on the Serial Monitor.
#if debug_traffic
void KWP1281debugFunction1(bool type, uint8_t sequence, uint8_t command, uint8_t* data, uint8_t length) {
  Serial.println();

  Serial.println(type ? "1. RECEIVE:" : "1. SEND:");

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

//////////////////////////////// K-line 2

//Initializes the serial port
void beginFunction2(unsigned long baud) {
#ifdef RX2_pin
  // The configuration for the ESP32 has both the RX and TX pins defined (for consistency), since they can be mapped to any pins.
  K_line2.begin(baud, SERIAL_8N1, RX2_pin, TX2_pin);
#else
  // For other boards (if RX_pin is not defined), use the standard function.
  K_line2.begin(baud);
#endif
}

//Stops communication on the serial port
void endFunction2() {
  K_line2.end();
}

//Sends a byte
void sendFunction2(uint8_t data) {
  K_line2.write(data);
}

//Receives a byte
bool receiveFunction2(uint8_t &data) {
  if (K_line2.available()) {
    data = K_line2.read();
    return true;
  }
  return false;
}

//Debugging can be enabled in configuration.h in order to print bus traffic on the Serial Monitor.
#if debug_traffic
void KWP1281debugFunction2(bool type, uint8_t sequence, uint8_t command, uint8_t* data, uint8_t length) {
  Serial.println();

  Serial.println(type ? "1. RECEIVE:" : "1. SEND:");

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
