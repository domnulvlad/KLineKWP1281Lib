## KW1281_dv

**KW1281_dv** is an Arduino library used for communicating with VAG control modules which use the proprietary VAG-KW1281 / KWP1281 protocol.

---

#### Supported features:
- connecting to any control module that supports KWP1281 at a manually-set baud rate
- reading the module's part number + extra fields
- reading coding and WSC code
- reading the module's identification field (for example, the vehicle's VIN)
- "Login"
- "Coding" for changing the coding and/or WSC
- "Fault Codes" for reading stored fault codes
- clearing stored fault codes
- "Adaptation" for reading an adaptation value on a specified channel
- testing an adaptation value on a specified channel
- saving an adaptation value to a specified channel
- ability to keep the connection alive while doing other tasks
- "Meas. Blocks" reading for an entire Meas. Block (calculating actual value + providing proper units of measurement)
- reading a single value inside a Meas. Block (alculating actual value + providing proper units of measurement)
#### Currently unsupported features:
- reading or writing ROM / EEPROM (firmware)
- "Output Tests"
- "Security Access"
- "Basic Settings"
- labels

## Hardware setup
+ The easiest way to obtain an adapter is to modify a VAG-KKL cable, the cheaper the better.
+ The cheapest cables often contain a Serial-to-USB chip (commonly WCH‑CH340G or FTDI‑FT232RL), which we don't need and a 12V-5V level shifter, usually an LM339 comparator, which we are interested in.
+ In essence, the K-Line diagnostic protocol is just a serial interface with 12V logic.
#### Modifying a cable
---
For simple cables, [**Alexander Grau**'s method](http://grauonline.de/wordpress/wp-content/uploads/obd_adapter_arduino.jpg) is the most straightforward.
- The idea is to cut the connection between the level shifter and the USB chip on the TX (transmit line) so it doesn't interfere with our communications.
- The TX (receive line) can be left alone because it has no negative effects on communication.
- The 5V power to the cable is not necessary after the mod, but the 12V from the car is, it will set the voltage level of the serial interface... and don't forget the ground!
####
It's also possible with more complicated cables, as [**mkirbst** shows on GitHub](https://github.com/mkirbst/lupo-gti-tripcomputer-kw1281).
- The core idea is the same, but the author chose to cut both data lines.
- In such a cable where RX / TX lines control (or are controlled with) transistors, the wires for the Arduino need to be connected to the same trace that used to lead directly into the USB chip before cutting them.
####
---
My solution
- As I had one of the cheaper variants, I chose **Alexander**'s method, but I also wanted to keep the USB cable for aesthetics and ease of connection to the Arduino.
- Like him, I cut the TX trace, but also the D+ and D- going from the USB cable to the Serial-USB converter chip.
- Next, I soldered the TX line (point on the cut trace, on the LM393's side) to where USB D- previously was (white wire from USB cable), and then the RX line (point on the circuit board connected as close as possible to the LM393 level shifter) to where D+ previously was (green wire from USB cable). Of course, it's your own choise whether RX and TX correspond to D- and D+ or vice-versa, but don't forget which way you chose.
- *Warning*, as with every modified USB cable that isn't actual USB anymore... please don't plug it into a computer after modifying!
#### Installation location
---
- For my testing I have been using an original VAG Instruments cluster on my workbench, to which I connected the adapter board directly to the K-Line.
- In reality, this project will most likely be useful in logging values, installing custom displays or DIY scan tools.
- As such, my hardware setup solution above is useful in the scenario of a scan tool. A much neater approach would be including an Arduino Nano / Pro Micro + step-down voltage regulator inside the housing of the OBD2 connector, or even an Arduino Pro Mini for use cases where the USB connection to a computer is not necessary (logging to SD cards, custom displays etc).
- ***One useful detail to consider is that all modules that communicate with this protocol are all connected to the same K-Line. Wherever you choose to connect it, be it to the external diganostic port or by tapping into the K-Line on the instruments cluster or the radio, you will be able to connect to any module inside the vehicle.***
+ For the time being, these are installation methods I have thought of:
+ 1. non-intrusive: cable going from OBD2 connector to an external Arduino connected in turn to a computer (for debugging, logging, scanning, coding)
+ 2. non-intrusive: OBD2 connector with a small form-factor Arduino inside, eventually with an USB cable to a computer
+ 3. slightly intrusive: cable tapped into the K-Line of the cluster or the radio and then going somewhere like the glovebox to an Arduino  (connected to a computer or not) - much neater, and with an added advantage: both of those modules have an S-Kontakt connection available, through which you can check if the vehicle's ignition is on; there is no such connection on the standard OBD2 diagnostic port.
#### Powering options
- If your project requires a computer attached to the Arduino, then you don't need a power source for it. All that is needed is the 12V connection to the cable, which is readily available (on the OBD2 connector or wherever else next to a control module).
- If your project is independent from USB cables, the Arduino will need 5V power. The neatest solution is using a step-down converter module, but a hackier approach could be repurposing a car phone charger, let your creativity flow.
- A worry for computer-independent projects connected permanently to 12V is of course parasitic power draw. If your project is installed permanently, consider "stealing" the 12V from an S-Kontakt wire instead of permanent 12V.
- Also, if your project is mostly computer-independent but you plan on connecting it to a computer occasionally (for debugging, for example), consider adding a switch in series to the 5V output of your regulator of choice, as powering an Arduino both from the USB cable and another source at the same time could have bad results.