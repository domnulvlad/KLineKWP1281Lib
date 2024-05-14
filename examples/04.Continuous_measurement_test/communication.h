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
