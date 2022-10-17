## KW1281_dv
**KW1281_dv** is a library used for communicating with VAG control modules which use the proprietary VAG KW1281 / KWP1281 protocol, through an Arduino development board.

## Supported features:
- Connecting to any control module that supports KWP1281
- Reading the module's part number + extra fields
- Reading coding and WSC code
- Reading the module's identification field (for example, the vehicle's VIN)
- "Login"
- "Coding" for changing the coding and/or WSC
- "Fault Codes" for reading stored fault codes
- Clearing stored fault codes
- "Adaptation" for reading an adaptation value on a specified channel
- Testing an adaptation value on a specified channel
- Saving an adaptation value to a specified channel
- Ability to keep the connection alive while doing other tasks
- "Meas. Blocks" reading for an entire Meas. Block (calculating actual value + providing proper units of measurement)
- Reading a single value inside a Meas. Block (calculating actual value + providing proper units of measurement)

## Currently unsupported features:
- Reading or writing ROM / EEPROM (firmware)
- "Output Tests"
- "Security Access"
- "Basic Settings"
- Labels

## Getting started
Download [the latest release (.zip)](https://github.com/domnulvlad/KW1281_dv/releases/latest) and [add it to your Arduino IDE](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#importing-a-zip-library).

**Read [the documentation](../../wiki)** (w.i.p.).

After completing the hardware setup, proceed to upload the demo by navigating to `File -> Examples -> KW1281_dv -> KW1281_dv_Demo`.

## Credits
Thanks go out to these people for their efforts:
* [Blafusel.de](https://www.blafusel.de/obd/obd2_kw1281.html), for the best documentation available for the protocol
* [Alexander Grau](http://grauonline.de/wordpress/?p=74), for the base code
* [Mike Naberezny](https://github.com/mnaberez/vwradio/blob/main/kwp1281_tool/firmware/kwp1281.h), for more insight about the commands and parameters

## Contact
For any inquiries, you can contact me at [ne555timer@yahoo.com](mailto:ne555timer@yahoo.com).
