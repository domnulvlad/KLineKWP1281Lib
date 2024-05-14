#ifndef FAULT_CODE_ELABORATION_H
#define FAULT_CODE_ELABORATION_H

const char KWP_ELAB_00[] PROGMEM = "-";
const char KWP_ELAB_01[] PROGMEM = "Signal Shorted to Plus";
const char KWP_ELAB_02[] PROGMEM = "Signal Shorted to Ground";
const char KWP_ELAB_03[] PROGMEM = "No Signal";
const char KWP_ELAB_04[] PROGMEM = "Mechanical Malfunction";
const char KWP_ELAB_05[] PROGMEM = "Input Open";
const char KWP_ELAB_06[] PROGMEM = "Signal too High";
const char KWP_ELAB_07[] PROGMEM = "Signal too Low";
const char KWP_ELAB_08[] PROGMEM = "Control Limit Surpassed";
const char KWP_ELAB_09[] PROGMEM = "Adaptation Limit Surpassed";
const char KWP_ELAB_0A[] PROGMEM = "Adaptation Limit Not Reached";
const char KWP_ELAB_0B[] PROGMEM = "Control Limit Not Reached";
const char KWP_ELAB_0C[] PROGMEM = "Adaptation Limit (Mul) Exceeded";
const char KWP_ELAB_0D[] PROGMEM = "Adaptation Limit (Mul) Not Reached";
const char KWP_ELAB_0E[] PROGMEM = "Adaptation Limit (Add) Exceeded";
const char KWP_ELAB_0F[] PROGMEM = "Adaptation Limit (Add) Not Reached";
const char KWP_ELAB_10[] PROGMEM = "Signal Outside Specifications";
const char KWP_ELAB_11[] PROGMEM = "Control Difference";
const char KWP_ELAB_12[] PROGMEM = "Upper Limit";
const char KWP_ELAB_13[] PROGMEM = "Lower Limit";
const char KWP_ELAB_14[] PROGMEM = "Malfunction in Basic Setting";
const char KWP_ELAB_15[] PROGMEM = "Front Pressure Build-up Time too Long";
const char KWP_ELAB_16[] PROGMEM = "Front Pressure Reducing Time too Long";
const char KWP_ELAB_17[] PROGMEM = "Rear Pressure Build-up Time too Long";
const char KWP_ELAB_18[] PROGMEM = "Rear Pressure Reducing Time too Long";
const char KWP_ELAB_19[] PROGMEM = "Unknown Switch Condition";
const char KWP_ELAB_1A[] PROGMEM = "Output Open";
const char KWP_ELAB_1B[] PROGMEM = "Implausible Signal";
const char KWP_ELAB_1C[] PROGMEM = "Short to Plus";
const char KWP_ELAB_1D[] PROGMEM = "Short to Ground";
const char KWP_ELAB_1E[] PROGMEM = "Open or Short to Plus";
const char KWP_ELAB_1F[] PROGMEM = "Open or Short to Ground";
const char KWP_ELAB_20[] PROGMEM = "Resistance Too High";
const char KWP_ELAB_21[] PROGMEM = "Resistance Too Low";
const char KWP_ELAB_22[] PROGMEM = "No Elaboration Available";
const char KWP_ELAB_23[] PROGMEM = "-";
const char KWP_ELAB_24[] PROGMEM = "Open Circuit";
const char KWP_ELAB_25[] PROGMEM = "Faulty";
const char KWP_ELAB_26[] PROGMEM = "Output won't Switch or Short to Plus";
const char KWP_ELAB_27[] PROGMEM = "Output won't Switch or Short to Ground";
const char KWP_ELAB_28[] PROGMEM = "Short to Another Output";
const char KWP_ELAB_29[] PROGMEM = "Blocked or No Voltage";
const char KWP_ELAB_2A[] PROGMEM = "Speed Deviation too High";
const char KWP_ELAB_2B[] PROGMEM = "Closed";
const char KWP_ELAB_2C[] PROGMEM = "Short Circuit";
const char KWP_ELAB_2D[] PROGMEM = "Connector";
const char KWP_ELAB_2E[] PROGMEM = "Leaking";
const char KWP_ELAB_2F[] PROGMEM = "No Communications or Incorrectly Connected";
const char KWP_ELAB_30[] PROGMEM = "Supply voltage";
const char KWP_ELAB_31[] PROGMEM = "No Communications";
const char KWP_ELAB_32[] PROGMEM = "Setting (Early) Not Reached";
const char KWP_ELAB_33[] PROGMEM = "Setting (Late) Not Reached";
const char KWP_ELAB_34[] PROGMEM = "Supply Voltage Too High";
const char KWP_ELAB_35[] PROGMEM = "Supply Voltage Too Low";
const char KWP_ELAB_36[] PROGMEM = "Incorrectly Equipped";
const char KWP_ELAB_37[] PROGMEM = "Adaptation Not Successful";
const char KWP_ELAB_38[] PROGMEM = "In Limp-Home Mode";
const char KWP_ELAB_39[] PROGMEM = "Electric Circuit Failure";
const char KWP_ELAB_3A[] PROGMEM = "Can't Lock";
const char KWP_ELAB_3B[] PROGMEM = "Can't Unlock";
const char KWP_ELAB_3C[] PROGMEM = "Won't Safe";
const char KWP_ELAB_3D[] PROGMEM = "Won't De-Safe";
const char KWP_ELAB_3E[] PROGMEM = "No or Incorrect Adjustment";
const char KWP_ELAB_3F[] PROGMEM = "Temperature Shut-Down";
const char KWP_ELAB_40[] PROGMEM = "Not Currently Testable";
const char KWP_ELAB_41[] PROGMEM = "Unauthorized";
const char KWP_ELAB_42[] PROGMEM = "Not Matched";
const char KWP_ELAB_43[] PROGMEM = "Set-Point Not Reached";
const char KWP_ELAB_44[] PROGMEM = "Cylinder 1";
const char KWP_ELAB_45[] PROGMEM = "Cylinder 2";
const char KWP_ELAB_46[] PROGMEM = "Cylinder 3";
const char KWP_ELAB_47[] PROGMEM = "Cylinder 4";
const char KWP_ELAB_48[] PROGMEM = "Cylinder 5";
const char KWP_ELAB_49[] PROGMEM = "Cylinder 6";
const char KWP_ELAB_4A[] PROGMEM = "Cylinder 7";
const char KWP_ELAB_4B[] PROGMEM = "Cylinder 8";
const char KWP_ELAB_4C[] PROGMEM = "Terminal 30 missing";
const char KWP_ELAB_4D[] PROGMEM = "Internal Supply Voltage";
const char KWP_ELAB_4E[] PROGMEM = "Missing Messages";
const char KWP_ELAB_4F[] PROGMEM = "Please Check Fault Codes";
const char KWP_ELAB_50[] PROGMEM = "Single-Wire Operation";
const char KWP_ELAB_51[] PROGMEM = "Open";
const char KWP_ELAB_52[] PROGMEM = "Activated";

const char* const fault_elaboration_table[] PROGMEM = {
	KWP_ELAB_00, KWP_ELAB_01, KWP_ELAB_02, KWP_ELAB_03, KWP_ELAB_04, KWP_ELAB_05, 
	KWP_ELAB_06, KWP_ELAB_07, KWP_ELAB_08, KWP_ELAB_09, KWP_ELAB_0A, KWP_ELAB_0B, 
	KWP_ELAB_0C, KWP_ELAB_0D, KWP_ELAB_0E, KWP_ELAB_0F, KWP_ELAB_10, KWP_ELAB_11, 
	KWP_ELAB_12, KWP_ELAB_13, KWP_ELAB_14, KWP_ELAB_15, KWP_ELAB_16, KWP_ELAB_17, 
	KWP_ELAB_18, KWP_ELAB_19, KWP_ELAB_1A, KWP_ELAB_1B, KWP_ELAB_1C, KWP_ELAB_1D, 
	KWP_ELAB_1E, KWP_ELAB_1F, KWP_ELAB_20, KWP_ELAB_21, KWP_ELAB_22, KWP_ELAB_23, 
	KWP_ELAB_24, KWP_ELAB_25, KWP_ELAB_26, KWP_ELAB_27, KWP_ELAB_28, KWP_ELAB_29, 
	KWP_ELAB_2A, KWP_ELAB_2B, KWP_ELAB_2C, KWP_ELAB_2D, KWP_ELAB_2E, KWP_ELAB_2F, 
	KWP_ELAB_30, KWP_ELAB_31, KWP_ELAB_32, KWP_ELAB_33, KWP_ELAB_34, KWP_ELAB_35, 
	KWP_ELAB_36, KWP_ELAB_37, KWP_ELAB_38, KWP_ELAB_39, KWP_ELAB_3A, KWP_ELAB_3B, 
	KWP_ELAB_3C, KWP_ELAB_3D, KWP_ELAB_3E, KWP_ELAB_3F, KWP_ELAB_40, KWP_ELAB_41, 
	KWP_ELAB_42, KWP_ELAB_43, KWP_ELAB_44, KWP_ELAB_45, KWP_ELAB_46, KWP_ELAB_47, 
	KWP_ELAB_48, KWP_ELAB_49, KWP_ELAB_4A, KWP_ELAB_4B, KWP_ELAB_4C, KWP_ELAB_4D, 
	KWP_ELAB_4E, KWP_ELAB_4F, KWP_ELAB_50, KWP_ELAB_51, KWP_ELAB_52, 
};

#endif
