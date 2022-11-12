//The most valuable resource for understanding the protocol if you need to debug errors in this program if https://www.blafusel.de/obd/obd2_kw1281.html
//Code based on the work of Alexander Grau (http://grauonline.de/wordpress/?p=74), and certain command bytes found by Mike Naberezny (https://github.com/mnaberez/vwradio/blob/main/kwp1281_tool/firmware/kwp1281.h)

/* * *
 *  This demo presents all of the function made available by the library:
 *  
 *  1.  Connecting to a module at a manually-set baud rate
 *  2.  Reading the module's part number + extra fields, the coding and the workshop
 *  3.  Reading the module's identification field (e.g. vehicle's VIN and IMMO serial number for 17-Instruments)
 *  4.  (disabled by default, remove the block comment on lines #116 and #132 in this sketch to enable) - Logging in with a login code, Changing the coding, Changing the WSC
 *  5.  Reading stored fault codes
 *  6.  (disabled by default, remove the block comment on lines #153 and #158 in this sketch to enable) - Clearing stored fault codes
 *  7.  Reading the adaptation value on a specified channel, Testing a value on a specified channel, Saving a value to a specified channel
 *  8.  Keep-alive function (so the connection is kept while waiting, in this example, for 5 seconds, non-blocking of course)
 *  9.  Reading an entire Measurement Block (Calculating actual value + Providing proper units of measurement)
 *  10. Reading a single value inside a Measurement Block (Calculating actual value + Providing proper units of measurement)
 *  
 *  Features currently missing:
 *  
 *  1. Actuator/Output tests (no use)
 *  2. Reading/Writing firmware (dangerous)
 *  3. Security access mode 1/mode 2 (can't test);
 *  4. Basic settings (can't test)
 *  
 *  All delays are non-blocking in this example at least, but the communication protocol is slow, timings for some function explained in comments.
 *  By default there is a blocking delay when reconnecting and initialising, but they can be changed, explained on lines #91 + #92.
 * * */

#include <KW1281_dv.h>

#define pinKLineRX 2 //RX pin, if repurposing a VAG-KKL cable, this will connect directly to a point on the PCB that's connected to the USB chip's RX
                     //for PCBs with an LM339 (14-pin-package) and CH340G chip, connect to the LM339's pins #2 and #13
                     
#define pinKLineTX 3 //TX pin, if repurposing a VAG-KKL cable, this will connect to the trace you cut, on the USB chip's side
                     //for PCBs with an LM339 (14-pin-package) and CH340G chip, connect to the LM339's pins #7 and #9 but as close to the CH340G as possible

KW1281_dv KWP(pinKLineRX, pinKLineTX); //create the object, here it's named KWP

bool doOnce = true; //to make a function only run once
unsigned long globalTimer = 0; //for a non-blocking delay later

#define workshop_code 12345 //doesn't really matter, just has to be there; change to something else if you want to change (5 digits long, no leading zeroes), but you will need the correct login code
#define login_code 8163 //works for my instrument cluster, often found in repair manuals or extracted with programs such as VAG-K+CAN COMMANDER, only needed if you plan on recoding the workshop code and on some adaptation channels, no leading zeroes please

//I've only tested it with 17-Instruments, but the protocol is the same for any other control module
#define _17_INSTRUMENTS 0x17
//valid baudrates are 10400, 9600, 4800
//if using another controller, start with the highest and go lower if it doesn' work at all after ~5 tries
//if none of the 3 baudrates work, go where this library is installed, open the file KW1281_dv.h, uncomment #define DEBUG, #define DEBUG_SHOW_SENT, #define DEBUG_SHOW_RECEIVED, #define DEBUG_SHOW_ERRORS, save the file, reupload the code and look in the serial monitor for the issue
#define _17_INSTRUMENTS_BAUDRATE 10400

uint16_t DTCs[16]; //array which will hold the DTCs, here declared with max size of 16 DTCs

void while_waiting_reconnection() {
  unsigned long timer = millis();
  while (millis() - timer < 1000) {
    //do stuff while it's waiting
  }
  //reset global variables if the connection restarted
  doOnce = true;
  globalTimer = 0;
}

void while_executing_5baud() {
  unsigned long timer = millis();
  while (millis() - timer < 200) {
    //do stuff while it's waiting
  }
}

void setup() {
  //define what to do while otherwise wasting time waiting, timings explained before connect() below
  KWP.define_reset_function(while_waiting_reconnection);
  KWP.define_wait_5baud_function(while_executing_5baud);
  Serial.begin(115200);
}

void loop() {
  if (KWP.currAddr != _17_INSTRUMENTS) { //if an error occurs, the library sets the current address to 0 so, to connect, check if you aren't connected and connect
    Serial.print("Attempting connection to 0x");
    Serial.print(_17_INSTRUMENTS, HEX);
    Serial.print(" at ");
    Serial.print(_17_INSTRUMENTS_BAUDRATE);
    Serial.println(" baud");

    char id[37];
    uint16_t coding;
    uint32_t wsc;
    //connect() takes around 3000ms if successful or 4000ms if it fails:
    //1000ms for reconnection wait time BUT by defining a custom function with define_reset_function() as is done in this example, you can execute other code in this time and you can delay however much you want, it will influence how much time it waits before reattempting connection
    //2000ms for 5baud sequence BUT defining a custom function with define_wait_5baud_function(), you can execute other code in this time
    // ^ !!! but keep the time your function takes to 200ms, as precise as you can (it's called 10 times per init)
    //(1000ms for timeout if unsuccessful, again determined by define_reset_function())
    //if you don't use define_reset_function() or define_wait_5baud_function(), the default functions will be used, which are blocking delays of 1000ms, respectively 200ms

    //connect() syntax:
    //address to connect to, the correspondents in VCDS are written in hex, 17-Instruments means addres HEX 0x17 which would be decimal 23,
    //baudrate to use,
    //37-character-long character array where the module name and extra fields will be stored (37 = 3blocks * 12characters + 1null_temrinator),
    //uint16_t-type variable where the current coding will be stored,
    //uint32_t-type variable where the current workshop code will be stored
    //
    //also, connect() can be called with only the address and baudrate if checking the id, coding and wsc isn't necessary
    if (KWP.connect(_17_INSTRUMENTS, _17_INSTRUMENTS_BAUDRATE, id, coding, wsc) == RETURN_SUCCESS) { //run the function and check if the connection was successful
      Serial.print("ID: ");
      Serial.println(id); //print the module name (and extra fields)
      Serial.print("Coding: ");
      Serial.print(coding); //print the coding
      Serial.print(", WSC: ");
      Serial.println(wsc); //print the workshop code
    }
  } else {
    if (doOnce) { //all functions are done in the loop, so in order to create a separate part that only runs once, we use this flag which is initialised to true when the program starts or when the communication restarts because of an error
      doOnce = false; //set it false so that all of the code inside this "if" only runs once after the module is connected

      /* //remove block comment if you want to use, be sure to change uint16_t code to the desired coding, run the program once without this part write down the current coding
            //recoding example, DANGEROUS, use with care

            //login with the defined code and wsc if you want to recode the WSC
            //to recode the WSC, you must login with the WSC you want (not the current one) and then recode
            Serial.println((KWP.Login(login_code, workshop_code) == RETURN_SUCCESS) ? "Login success" : "Login attempt error");

            uint16_t code = 360; //the coding you want, without leading zeroes
            //you must save the coding in a separate variable of type uint16_t which you will pass to the function
            //you cannot give the function the coding without storing it in a variable because the coding it reads afterwards is stored in the same variable
            //so after coding, to check if the recode was successful, compare the value stored in this variable to what you expected
            //to recode the WSC, you must login with the WSC you want (not the current one) and then recode
            //if you don't login, the coding will work, but the WSC will remain the same
            KWP.Coding(code, workshop_code); //recoding takes about 2125ms
            Serial.print("Current coding: ");
            Serial.println(code); //display the new coding, as received from the control module
      */

      char vin[37];
      KWP.VIN(vin); //pass to VIN() the character array, 37 bytes long (3blocks * 12characters + 1null_terminator)
      if (KWP.currAddr == _17_INSTRUMENTS) { //the VIN is always 17 characters long, after it there is some blank space before de immo id
        //in order to print the vin separately when it's in the same array as other things, insert a null terminator right after the VIN so the Serial.print() function knows where to stop trying to print
        vin[17] = '\0';
        Serial.print("VIN:  ");
        Serial.println(vin); //even though there are 36 characters in the array, this will only print the VIN as it stops at the '\0' terminator we inserted at position 17
        Serial.print("IMMO: ");
        //we also know the immo id starts at position 22 in the array and goes until the end, where there already is a null terminator
        //so what we need to do is tell Serial.print() to start printing from position 22 in the array and it will stop at the end
        Serial.println(&vin[22]); //we are basically giving Serial.print() a pointer to the 22nd character in the array instead of to the first character and it will stop printing at the end of the array which already contains the null
      } else { //if we're not reading the instrument cluster, just display all fields on the same line as I don't know what the format is for other modules
        Serial.println(vin);
      }

      //for ease of use, I have created a function in this file with the algorithm for reading DTCs, they will be read in the global variable "uint16_t DTCs[16]"
      //for more info, check out the function at the end of this sketch
      showFaults();

      /* // remove block comment if you are sure you want to test clearing all fault codes
        KWP.clearFaults(); //deleting all DTCs takes about 95ms
        Serial.println("Cleared all DTCs!");
        //read faults again after clearing to check which remain or if there are none left
        showDTCs();
      */

      if (KWP.currAddr == _17_INSTRUMENTS) { //only do this if we are talking to the instrument cluster, only there I'm sure that channel 4 is safe to adapt
        //readAdaptation() usage:
        //the function will return RETURN_INVALID_ERROR if an empty channel was given
        //or RETURN_UNEXPECTEDANSWER_ERROR if there was a communication error
        //to check the return value, save it in a separate variable of type uint16_t and check against the defined return error codes
        uint8_t channel = 4; //this is not necessary, just for ease of modification
        //Reading Adaptation:
        uint16_t exitCode = KWP.readAdaptation(channel); //reading an adaptation value takes about 160ms
        if (exitCode == RETURN_INVALID_ERROR) { //if we supplied an empty adaptation channel
          Serial.print("Channel ");
          Serial.print(channel);
          Serial.println(" is invalid!");
        } else if (exitCode == RETURN_UNEXPECTEDANSWER_ERROR) {
          Serial.println("Communication error!");
        } else if (exitCode > RETURN_SUCCESS) { //reading successful
          Serial.print("Read - CH.");
          Serial.print(channel);
          Serial.print(": ");
          Serial.println(exitCode);
        }

        //adapt the value at channel 4 (language setting, safe place to test adaptation)
        //test with different values for uint16_t value, 1-6 are valid and channel 4 will be adapted to them, anything higher will not be accepted and it will not adapt
        channel = 4; //the variable is already declared above, no need for uint16_t
        uint16_t value = 2;
        //Testing Adaptation:
        exitCode = KWP.testAdaptation(channel, value); //the variable is already declared above, no need for uint16_t
        if (exitCode == RETURN_INVALID_ERROR) { //if we supplied an empty adaptation channel
          Serial.print("Channel ");
          Serial.print(channel);
          Serial.print(" or value ");
          Serial.print(value);
          Serial.println(" is invalid!");
        } else if (exitCode == RETURN_UNEXPECTEDANSWER_ERROR) {
          Serial.println("Communication error!");
        } else if (exitCode == RETURN_SUCCESS) { //reading successful
          Serial.print("Test - CH.");
          Serial.print(channel);
          Serial.print(": value ");
          Serial.print(value);
          Serial.println(" would work");

          //if the value tested would be accepted by the controller, proceed to adapt it
          //Saving Adaptation:
          exitCode = KWP.Adaptation(channel, value, workshop_code);
          if (exitCode == RETURN_INVALID_ERROR) { //if we supplied an empty adaptation channel
            Serial.print("Channel ");
            Serial.print(channel);
            Serial.print(" or value ");
            Serial.print(value);
            Serial.println(" is invalid!");
          } else if (exitCode == RETURN_UNEXPECTEDANSWER_ERROR) {
            Serial.println("Communication error!");
          } else if (exitCode > RETURN_SUCCESS) { //reading successful
            Serial.print("Adapted - CH.");
            Serial.print(channel);
            Serial.print(": ");
            Serial.println(exitCode);
          }
        } else if (exitCode > RETURN_SUCCESS) { //if the value we tested wouldn't be accepted by the control module
          Serial.print("Test -  CH.");
          Serial.print(channel);
          Serial.print(" value ");
          Serial.print(value);
          Serial.print(" wouldn't work, prefers value ");
          Serial.println(exitCode);
        }
      }
      Serial.println("-------Waiting 5 seconds");
      globalTimer = millis();
    }

    if (millis() - globalTimer > 5000) { //do this part if 5 seconds have passed since finishing the instructions above; in the meantime keep the connection alive, that function is at the end of this "if"
      float result1, result2, result3, result4; //variables which will store the numerical values after reading measurement blocks, MUST be separate variables
      uint8_t formula1, formula2, formula3, formula4; //these will store the "type" of measurement, which tells getUnits() what units to return
      uint8_t group = 1; //not necessary to save in a separate variable

      //call this function with the parameters in this order
      KWP.MeasBlocksWithUnits(group, result1, result2, result3, result4, formula1, formula2, formula3, formula4);
      //^ this function takes about 250ms

      char units[8]; //character array to hold the units of the measurement
      group = 2;
      uint8_t index = 2;
      //group 2 index 2 on 17-Instruments is fuel sender resistance, in Ohms
      uint8_t formula5;
      float result5;
      KWP.SingleReadingWithUnits(group, index, formula5, result5); //this function takes about the same time as MeasBlocks(WithUnits), 250ms

      KWP.getUnits(formula1, units);
      Serial.print(result1);
      Serial.print(units); Serial.print(", ");

      KWP.getUnits(formula2, units);
      Serial.print(result2);
      Serial.print(units); Serial.print(", ");

      KWP.getUnits(formula3, units);
      Serial.print(result3);
      Serial.print(units); Serial.print(", ");

      KWP.getUnits(formula4, units);
      Serial.print(result4);
      Serial.print(units); Serial.print(", ");

      KWP.getUnits(formula5, units);
      Serial.print(result5);
      Serial.println(units);

    } else {
      KWP.keepAlive(); //keep-alive takes about 100ms
    }
  }
}

void showFaults() {
  uint8_t totalDTCs;
  char formattedMessage[6]; //will hold a DTC with leading zeroes
  //storeReadFaults() usage:
  //give it the array of type uint16_t where it will store the fault codes, then the index of the code to save from and the amount of codes to save
  //if there are 5 DTCs in the control module and you choose to start saving from index 1, only the last 4 codes will be saved
  //to start saving from the first code, call it with index 0
  //and for the total amount, do not exceed the total size of the array, as you can see below this size is automatically calculated
  //also, it will return the total amount of DTCs read, no matter how many were requested
  totalDTCs = KWP.readFaults(DTCs, 0, sizeof(DTCs) / sizeof(DTCs[0])); //reading DTCs takes about 260ms per group of 4 DTCs; if there are 5 codes there will be 2 groups so about 520ms; 9 codes = 3 groups = 780ms; etc.
  Serial.print("Read DTCs: ");
  if (totalDTCs == 0) Serial.println("None stored!");
  else if (totalDTCs > sizeof(DTCs) / sizeof(DTCs[0])) { //if there were more codes stored in the control module than requested, show all that were saved in the array
    for (uint8_t i = 0; i < sizeof(DTCs) / sizeof(DTCs[0]); i++) {
      sprintf(formattedMessage, "%05d", DTCs[i]); //add leading zeroes
      Serial.print(formattedMessage);
      Serial.print(' ');
    } Serial.print(' ');
    Serial.print("There are more than fit in the given array, total: ");
    Serial.println(totalDTCs);
  } else {
    for (uint8_t i = 0; i < totalDTCs; i++) { //otherwise, if there were less codes stored than we requested, only show that many, otherwise the rest are 0
      sprintf(formattedMessage, "%05d", DTCs[i]); //add leading zeroes
      Serial.print(formattedMessage);
      Serial.print(' ');
    } Serial.print(' ');
    Serial.print("Total: ");
    Serial.println(totalDTCs);
  }
}
