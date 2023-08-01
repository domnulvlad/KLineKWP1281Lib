//Initializes the serial port
void beginFunction(unsigned long baud) {
  K_line.begin(baud);
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
