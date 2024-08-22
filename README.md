## KLineKWP1281Lib
**KLineKWP1281Lib**  is a library designed for communicating with VAG control modules which use the proprietary VAG Key-Word 1281 protocol (KWP1281/KW1281), through an Arduino / ESP / RPi Pico development board.

For a library targeted strictly at the ESP32, optimized for the FreeRTOS operating system, check out **[KLineKWP1281Lib_ESP32](https://github.com/domnulvlad/KLineKWP1281Lib_ESP32)**.

## Supported features:
- **Ability to use any method to send and receive data** (best being a hardware serial port, or software serial if an additional port is not available)
- Ability to keep the connection alive while doing other tasks
- Connecting to any control module that supports KWP1281
- Reading the module's part number + extra fields, coding, WSC
- Recoding
- Performing a login operation
- **Reading stored fault codes and their elaboration codes**
- **Converting fault/elaboration codes to strings** (available in a few different languages)
- Clearing stored fault codes
- Reading, testing and saving an adaptation value
- **Reading measuring blocks, calculating a measurement's value and providing its units, or displaying text from a table where applicable** (available in a few different languages)
- Reading ROM / EEPROM (firmware)
- Performing actuator tests
- Performing basic settings

## Getting started
Download [the latest release (.zip)](https://github.com/domnulvlad/KLineKWP1281Lib/releases/latest) and [add it to your Arduino IDE](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#importing-a-zip-library).

**Read the documentation on [the wiki](https://github.com/domnulvlad/KLineKWP1281Lib/wiki)!** Chapter 3 describes using the demo sketches.

## Credits
Thanks go out to these people for their efforts:
* [Blafusel.de](https://www.blafusel.de/obd/obd2_kw1281.html), for the best documentation available for the protocol
* [Alexander Grau](http://grauonline.de/wordpress/?p=74), for the base code
* [Mike Naberezny](https://github.com/mnaberez/vwradio/blob/main/kwp1281_tool/firmware/kwp1281.h), for more insight about the commands and parameters

## Contact
For any inquiries, you can open an issue or contact me at [ne555timer@yahoo.com](mailto:ne555timer@yahoo.com) (but don't include any links in the email, as it will be marked as spam).
