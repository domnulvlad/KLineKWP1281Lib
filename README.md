## KLineKWP1281Lib
**KLineKWP1281Lib** is a library used for communicating with VAG control modules which use the proprietary VAG Key-Word 1281 protocol (KWP1281/KW1281), through an Arduino / ESP development board.

## Supported features:
- **Ability to use any method to send and receive data** (best being a hardware serial port, or software serial if an additional port is not available)
- Connecting to any control module that supports KWP1281
- Reading the module's part number + extra fields
- Reading coding and WSC
- Reading the module's identification field (for example, the vehicle's VIN)
- "Login"
- "Coding" for changing the coding and/or WSC
- "Fault Codes" for reading stored fault codes and their status
- Clearing stored fault codes
- "Adaptation" for reading an adaptation value on a specified channel
- Testing an adaptation value on a specified channel
- Saving an adaptation value to a specified channel
- "Meas. Blocks" for reading for an entire measuring block (calculating actual value + providing proper units of measurement for any measurement and providing text strings for those that require it)
- Reading ROM / EEPROM (firmware)
- "Output Tests"
- "Basic Settings"
- Ability to keep the connection alive while doing other tasks

## Currently unsupported features:
- "Security Access"
- Measurement / DTC labels or descriptions

## Getting started
Download [the latest release (.zip)](https://github.com/domnulvlad/KLineKWP1281Lib/releases/latest) and [add it to your Arduino IDE](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#importing-a-zip-library).

**Read the documentation on [the wiki](https://github.com/domnulvlad/KLineKWP1281Lib/wiki)!**

After completing the hardware setup, proceed to upload the demo by navigating to `File -> Examples -> KLineKWP1281Lib -> 01.Connection_test` in the Arduino IDE.

## Credits
Thanks go out to these people for their efforts:
* [Blafusel.de](https://www.blafusel.de/obd/obd2_kw1281.html), for the best documentation available for the protocol
* [Alexander Grau](http://grauonline.de/wordpress/?p=74), for the base code
* [Mike Naberezny](https://github.com/mnaberez/vwradio/blob/main/kwp1281_tool/firmware/kwp1281.h), for more insight about the commands and parameters

## Contact
For any inquiries, you can contact me at [ne555timer@yahoo.com](mailto:ne555timer@yahoo.com).
