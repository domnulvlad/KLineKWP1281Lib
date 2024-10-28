#ifndef UNITS_H
#define UNITS_H

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

const char KWP_units_RPM[]                  PROGMEM = "/min";
const char KWP_units_Percentage[]           PROGMEM = "%";
const char KWP_units_Angle[]                PROGMEM = "deg";
const char KWP_units_Temperature[]          PROGMEM = "degC";
const char KWP_units_Voltage[]              PROGMEM = "V";
const char KWP_units_Voltage_m[]            PROGMEM = "mV";
const char KWP_units_Speed[]                PROGMEM = "km/h";
const char KWP_units_Resistance[]           PROGMEM = "Ohm";
const char KWP_units_Resistance_k[]         PROGMEM = "kOhm";
const char KWP_units_Resistance_m[]         PROGMEM = "mOhm";
const char KWP_units_Distance[]             PROGMEM = "m";
const char KWP_units_Distance_k[]           PROGMEM = "km";
const char KWP_units_Distance_m[]           PROGMEM = "mm";
const char KWP_units_Distance_c[]           PROGMEM = "cm";
const char KWP_units_Pressure[]             PROGMEM = "bar";
const char KWP_units_Pressure_m[]           PROGMEM = "mbar";
const char KWP_units_Time[]                 PROGMEM = "s";
const char KWP_units_Time_m[]               PROGMEM = "ms";
const char KWP_units_Time_h[]               PROGMEM = "h";
const char KWP_units_Time_mo[]              PROGMEM = "months";
const char KWP_units_Volume[]               PROGMEM = "l";
const char KWP_units_Volume_m[]             PROGMEM = "ml";
const char KWP_units_Current[]              PROGMEM = "A";
const char KWP_units_Current_m[]            PROGMEM = "mA";
const char KWP_units_Capacity[]             PROGMEM = "Ah";
const char KWP_units_Power[]                PROGMEM = "W";
const char KWP_units_Power_k[]              PROGMEM = "kW";
const char KWP_units_Mass_Flow[]            PROGMEM = "g/s";
const char KWP_units_Mass_Flow_m[]          PROGMEM = "mg/s";
const char KWP_units_Mass_Flow_km[]         PROGMEM = "mg/km";
const char KWP_units_Correction[]           PROGMEM = "KW";
const char KWP_units_Segment_Correction[]   PROGMEM = "degKW";
const char KWP_units_Consumption_h[]        PROGMEM = "l/h";
const char KWP_units_Consumption_100km[]    PROGMEM = "l/100km";
const char KWP_units_Consumption_1000km[]   PROGMEM = "l/1000km";
const char KWP_units_Mass_Per_Stroke_m[]    PROGMEM = "mg/h";
const char KWP_units_Mass_Per_Stroke_k[]    PROGMEM = "kg/h";
const char KWP_units_Torque[]               PROGMEM = "Nm";
const char KWP_units_Misfires[]             PROGMEM = "/s";
const char KWP_units_Turn_Rate[]            PROGMEM = "deg/s";
const char KWP_units_Acceleration[]         PROGMEM = "m/s^2";
const char KWP_units_Mass[]                 PROGMEM = "g";
const char KWP_units_Mass_k[]               PROGMEM = "kg";
const char KWP_units_Impulses[]             PROGMEM = "/km";
const char KWP_units_Fuel_Level_Factor[]    PROGMEM = "l/mm";
const char KWP_units_Attenuation[]          PROGMEM = "dB";
const char KWP_units_Parts_Per_Million[]    PROGMEM = "ppm";

const char KWP_units_Warm[] PROGMEM = "Warm";
const char KWP_units_Cold[] PROGMEM = "Cold";

const char KWP_units_Ignition_ATDC[] PROGMEM = "degATDC";
const char KWP_units_Ignition_BTDC[] PROGMEM = "degBTDC";

const char KWP_units_Map1[] PROGMEM = "Map1";
const char KWP_units_Map2[] PROGMEM = "Map2";

const char KWP_units_Vss[] PROGMEM = "Vss";
const char KWP_units_Wm2[] PROGMEM = "W/m^2";
const char KWP_units_Wcm2[] PROGMEM = "W/cm^2";
const char KWP_units_Wh[] PROGMEM = "Wh";
const char KWP_units_Ws[] PROGMEM = "Ws";
const char KWP_units_ms[] PROGMEM = "m/s";
const char KWP_units_lkm[] PROGMEM = "l/km";
const char KWP_units_N[] PROGMEM = "N";
const char KWP_units_angdeg[] PROGMEM = "<deg";
const char KWP_units_degF[] PROGMEM = "degF";
const char KWP_units_Imp[] PROGMEM = "Imp";

#endif
