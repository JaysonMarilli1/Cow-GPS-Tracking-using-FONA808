/*****************************************************************
   EEPROM specific functions
 *****************************************************************/

// Reading int from EEPROM
int readEepromInt(int addr) {
  int output = 0;
  output = EEPROM.readInt(addr);
  return output;
}

//  writing int to EEPROM
void writeEepromInt(int addr, int val) {
  EEPROM.writeInt(addr, val);
}
