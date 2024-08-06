// Initialize the serial port
void beginFunction(unsigned long baud)
{
#ifdef RX_pin
  // The configuration for the ESP32 has both the RX and TX pins defined (for consistency), since they can be mapped to any pins.
  K_line.begin(baud, SERIAL_8N1, RX_pin, TX_pin);
#else
  // For other boards (if RX_pin is not defined), use the standard function.
  K_line.begin(baud);
#endif
}

// Stop communication on the serial port
void endFunction()
{
  K_line.end();
}

// Send a byte
void sendFunction(uint8_t data)
{
  K_line.write(data);
}

// Receive a byte
bool receiveFunction(uint8_t *data)
{
  if (K_line.available())
  {
    *data = K_line.read();
    return true;
  }
  return false;
}

// Debugging can be enabled in configuration.h in order to print bus traffic on the Serial Monitor.
#if debug_traffic
void KWP1281debugFunction(bool direction, uint8_t message_sequence, uint8_t message_type, uint8_t *data, size_t length)
{
  Serial.print(direction ? "\tRECEIVE" : "\tSEND");
  
  Serial.print(" S:");
  if (message_sequence < 0x10) Serial.print(0);
  Serial.print(message_sequence, HEX);
  
  Serial.print(" T:");
  if (message_type < 0x10) Serial.print(0);
  Serial.print(message_type, HEX);
  
  Serial.print(" L:");
  Serial.print(length);
  
  if (length)
  {
    Serial.print(" D:");
  
    // Iterate through the bytes of the message.
    for (size_t i = 0; i < length; i++)
    {
      if (data[i] < 0x10) Serial.print(0);
      Serial.print(data[i], HEX);
      Serial.print(' ');
    }
  }
  Serial.println();
}
#endif
