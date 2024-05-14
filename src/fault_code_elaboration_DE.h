#ifndef FAULT_CODE_ELABORATION_H
#define FAULT_CODE_ELABORATION_H

const char KWP_ELAB_00[] PROGMEM = "-";
const char KWP_ELAB_01[] PROGMEM = "Signal an Plus";
const char KWP_ELAB_02[] PROGMEM = "Signal an Masse";
const char KWP_ELAB_03[] PROGMEM = "kein Signal";
const char KWP_ELAB_04[] PROGMEM = "Mechanischer Fehler";
const char KWP_ELAB_05[] PROGMEM = "Eingang Offen";
const char KWP_ELAB_06[] PROGMEM = "Signal zu gro" "\xDF" "";
const char KWP_ELAB_07[] PROGMEM = "Signal zu klein";
const char KWP_ELAB_08[] PROGMEM = "Regelgrenze " "\xFC" "berschritten";
const char KWP_ELAB_09[] PROGMEM = "Adaptionsgrenze " "\xFC" "berschritten";
const char KWP_ELAB_0A[] PROGMEM = "Adaptionsgrenze unterschritten";
const char KWP_ELAB_0B[] PROGMEM = "Regelgrenze unterschritten";
const char KWP_ELAB_0C[] PROGMEM = "Adaptionsgrenze ( mul ) " "\xFC" "berschritten";
const char KWP_ELAB_0D[] PROGMEM = "Adaptionsgrenze ( mul ) unterschritten";
const char KWP_ELAB_0E[] PROGMEM = "Adaptionsgrenze ( add ) " "\xFC" "berschritten";
const char KWP_ELAB_0F[] PROGMEM = "Adaptionsgrenze ( add ) unterschritten";
const char KWP_ELAB_10[] PROGMEM = "Signal au" "\xDF" "erhalb der Toleranz";
const char KWP_ELAB_11[] PROGMEM = "Regeldifferenz";
const char KWP_ELAB_12[] PROGMEM = "oberer Anschlagwert";
const char KWP_ELAB_13[] PROGMEM = "unterer Anschlagwert";
const char KWP_ELAB_14[] PROGMEM = "Fehler in Grundeinstellung";
const char KWP_ELAB_15[] PROGMEM = "Zeit f" "\xFC" "r Druckaufbau vorn zu lang";
const char KWP_ELAB_16[] PROGMEM = "Zeit f" "\xFC" "r Druckabbau vorn zu lang";
const char KWP_ELAB_17[] PROGMEM = "Zeit f" "\xFC" "r Druckaufbau hinten zu lang";
const char KWP_ELAB_18[] PROGMEM = "Zeit f" "\xFC" "r Druckabbau hinten zu lang";
const char KWP_ELAB_19[] PROGMEM = "undefinierter Schalterzustand";
const char KWP_ELAB_1A[] PROGMEM = "Ausgang Offen";
const char KWP_ELAB_1B[] PROGMEM = "unplausibles Signal";
const char KWP_ELAB_1C[] PROGMEM = "Kurzschluss nach Plus";
const char KWP_ELAB_1D[] PROGMEM = "Kurzschluss nach Masse";
const char KWP_ELAB_1E[] PROGMEM = "Unterbrechung / Kurzschlu" "\xDF" " nach Plus";
const char KWP_ELAB_1F[] PROGMEM = "Unterbrechung / Kurzschlu" "\xDF" " nach Masse";
const char KWP_ELAB_20[] PROGMEM = "Widerstandswert zu gro" "\xDF" "";
const char KWP_ELAB_21[] PROGMEM = "Widerstandswert zu klein";
const char KWP_ELAB_22[] PROGMEM = "keine Fehlerart erkannt";
const char KWP_ELAB_23[] PROGMEM = "-";
const char KWP_ELAB_24[] PROGMEM = "Unterbrechung";
const char KWP_ELAB_25[] PROGMEM = "defekt";
const char KWP_ELAB_26[] PROGMEM = "Ausgang schaltet nicht / KS nach Plus";
const char KWP_ELAB_27[] PROGMEM = "Ausgang schaltet nicht / KS nach Masse";
const char KWP_ELAB_28[] PROGMEM = "Kurzschlu" "\xDF" " zu anderem Ventil";
const char KWP_ELAB_29[] PROGMEM = "Blockiert oder Spannungslos";
const char KWP_ELAB_2A[] PROGMEM = "Drehzahlabweichung zu gro" "\xDF" "";
const char KWP_ELAB_2B[] PROGMEM = "geschlossen";
const char KWP_ELAB_2C[] PROGMEM = "Kurzschluss";
const char KWP_ELAB_2D[] PROGMEM = "Steckverbindung";
const char KWP_ELAB_2E[] PROGMEM = "undicht";
const char KWP_ELAB_2F[] PROGMEM = "falsch Angeschlossen";
const char KWP_ELAB_30[] PROGMEM = "Spannungsversorgung";
const char KWP_ELAB_31[] PROGMEM = "keine Kommunikation";
const char KWP_ELAB_32[] PROGMEM = "Stellung fr" "\xFC" "h nicht erreicht";
const char KWP_ELAB_33[] PROGMEM = "Stellung sp" "\xE4" "t nicht erreicht";
const char KWP_ELAB_34[] PROGMEM = "Spannungsversorgung zu gro" "\xDF" "";
const char KWP_ELAB_35[] PROGMEM = "Spannungsversorgung zu klein";
const char KWP_ELAB_36[] PROGMEM = "falsche Ausstattung";
const char KWP_ELAB_37[] PROGMEM = "Adaption nicht erfolgt";
const char KWP_ELAB_38[] PROGMEM = "im Notlauf";
const char KWP_ELAB_39[] PROGMEM = "elektr. Fehler im Stromkreis";
const char KWP_ELAB_3A[] PROGMEM = "verriegelt nicht";
const char KWP_ELAB_3B[] PROGMEM = "entriegelt nicht";
const char KWP_ELAB_3C[] PROGMEM = "safed nicht";
const char KWP_ELAB_3D[] PROGMEM = "entsafed nicht";
const char KWP_ELAB_3E[] PROGMEM = "keine oder falsche Einstellung";
const char KWP_ELAB_3F[] PROGMEM = "Temperaturabschaltung";
const char KWP_ELAB_40[] PROGMEM = "zur Zeit nicht pr" "\xFC" "fbar";
const char KWP_ELAB_41[] PROGMEM = "nicht berechtigt";
const char KWP_ELAB_42[] PROGMEM = "Abgleich nicht durchgef" "\xFC" "hrt";
const char KWP_ELAB_43[] PROGMEM = "Sollwert nicht erreicht";
const char KWP_ELAB_44[] PROGMEM = "Zylinder 1";
const char KWP_ELAB_45[] PROGMEM = "Zylinder 2";
const char KWP_ELAB_46[] PROGMEM = "Zylinder 3";
const char KWP_ELAB_47[] PROGMEM = "Zylinder 4";
const char KWP_ELAB_48[] PROGMEM = "Zylinder 5";
const char KWP_ELAB_49[] PROGMEM = "Zylinder 6";
const char KWP_ELAB_4A[] PROGMEM = "Zylinder 7";
const char KWP_ELAB_4B[] PROGMEM = "Zylinder 8";
const char KWP_ELAB_4C[] PROGMEM = "Klemme 30 fehlt";
const char KWP_ELAB_4D[] PROGMEM = "interne Spannungsversorgung";
const char KWP_ELAB_4E[] PROGMEM = "fehlende Botschaften";
const char KWP_ELAB_4F[] PROGMEM = "bitte Fehlerspeicher auslesen";
const char KWP_ELAB_50[] PROGMEM = "im Eindrahtbetrieb";
const char KWP_ELAB_51[] PROGMEM = "offen";
const char KWP_ELAB_52[] PROGMEM = "ausgel" "\xF6" "st";

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
