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
