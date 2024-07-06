//Initializes the serial port
void beginFunction(unsigned long baud) {
#ifdef RX_pin
  // The configuration for the ESP32 has both the RX and TX pins defined (for consistency), since they can be mapped to any pins.
  K_line.begin(baud, SERIAL_8N1, RX_pin, TX_pin);
#else
  // For other boards (if RX_pin is not defined), use the standard function.
  K_line.begin(baud);
#endif
}

//Stops communication on the serial port
void endFunction() {
  K_line.end();
}

//Sends a byte
void sendFunction(uint8_t data) {
  K_line.write(data);
}

//Receives a byte
bool receiveFunction(uint8_t &data) {
  if (K_line.available()) {
    data = K_line.read();
    return true;
  }
  return false;
}

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
