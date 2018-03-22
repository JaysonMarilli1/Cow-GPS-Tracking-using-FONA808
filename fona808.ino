/*****************************************************************
   FONA808 specific functions
 *****************************************************************/

//if there is anything is the fonaSS serial Buffer, clear it out and print it in the Serial Monitor.
void flushFONA() {
  char inChar;
  while (fonaSS.available()) {
    inChar = fonaSS.read();
    Serial.write(inChar);
    delay(20);
  }
}

void turnOnFONA() { //turns FONA ON
  if (! digitalRead(FONA_PS)) { //Check if it's On already. LOW is off, HIGH is ON.
#ifdef ECHO_TO_SERIAL
    Serial.print(F("FONA was OFF, Power ON: "));
#endif
    digitalWrite(FONA_KEY, LOW); //pull down power set pin
    unsigned long KeyPress = millis();
    while (KeyPress + keyTime >= millis()) {} //wait two seconds
    digitalWrite(FONA_KEY, HIGH); //pull it back up again
#ifdef ECHO_TO_SERIAL
    Serial.println(F("FONA Powered Up"));
#endif
  } else {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("FONA Already On.."));
#endif
  }
}

void turnOffFONA() { //does the opposite of turning the FONA ON (ie. OFF)
  if (digitalRead(FONA_PS)) { //check if FONA is OFF
#ifdef ECHO_TO_SERIAL
    Serial.print(F("FONA was ON, Powering OFF: "));
#endif
    digitalWrite(FONA_KEY, LOW);
    unsigned long KeyPress = millis();
    while (KeyPress + keyTime >= millis()) {}
    digitalWrite(FONA_KEY, HIGH);
#ifdef ECHO_TO_SERIAL
    Serial.println(F("FONA is Powered Down"));
#endif
  } else {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("FONA is already off, did nothing."));
#endif
  }
}

void checkGPS_Fix() {
#ifdef ECHO_TO_SERIAL
  int8_t stat;
  stat = fona.GPSstatus();
  if (stat < 0)
    Serial.println(F("Failed to query"));
  if (stat == 0) Serial.println(F("GPS off"));
  if (stat == 1) Serial.println(F("No fix"));
  if (stat == 2) Serial.println(F("2D fix"));
  if (stat == 3) Serial.println(F("3D fix"));
#endif
}

void GetBattStats() {
  { // Read the battery percentage and voltage
    if (! fona.getBattPercent(&vbat)) {
#ifdef ECHO_TO_SERIAL
      Serial.println(F("Failed to read the battery."));
#endif
      battPer = 0;
    } else {
      battPer = vbat;
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Battery Percentage: ")); Serial.print(battPer); Serial.println(F("%"));
#endif
    }
    vbat = 0; // reset the variable
    if (! fona.getBattVoltage(&vbat)) {
#ifdef ECHO_TO_SERIAL
      Serial.println(F("Failed to read the battery."));
#endif
      battVol = 0;
    } else {
      battVol = vbat;
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Battery Voltage: ")); Serial.print(battVol); Serial.println(F(" mV"));
      Serial.println(F(""));
#endif
    }
  }
}

String GetRSSI() { // new function - PLEASE CHECK!!
  // read the RSSI
  /*
     Value  RSSI dBm  Condition
    2 -109  Marginal

    9 -95 Marginal
    10  -93 OK

    14  -85 OK
    15  -83 Good

    19  -75 Good
    20  -73 Excellent

    30  -53 Excellent
  */
  uint8_t n = fona.getRSSI();
  int8_t r;

  Serial.print(F("RSSI = ")); Serial.print(n); Serial.print(F(": "));
  if (n == 0) r = -115;
  if (n == 1) r = -111;
  if (n == 31) r = -52;
  if ((n >= 2) && (n <= 30)) {
    r = map(n, 2, 30, -110, -54);
  }
  Serial.print(r); Serial.println(F(" dBm"));
  return String(r);
}

String GetAndSetTime (void) {
  char gpsbuffer[120];
  uint8_t res_len = fona.getGPS(0, gpsbuffer, 120);

  // make sure we have a response
  if (res_len == 0) {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("Failed getGPS!")); //return false;
#endif
  }

  if (type == FONA808_V2) {
    // Parse 808 V2 response.  See table 2-3 from here for format:
    // http://www.adafruit.com/datasheets/SIM800%20Series_GNSS_Application%20Note%20V1.00.pdf

    // skip fields before date/time
    char *tok = strtok(gpsbuffer, ",");  //    if (! tok) return false;
    tok = strtok(NULL, ",");
  }
  if (type == FONA808_V1) {
    // skip fields before date/time
    char *tok = strtok(gpsbuffer, ",");  //    if (! tok) return false;
    tok = strtok(NULL, ",");
    tok = strtok(NULL, ",");
    tok = strtok(NULL, ",");
  }

  String strDateTime = strtok(NULL, ",");
#ifdef ECHO_TO_SERIAL
  if (! strDateTime) Serial.println(F("Failed to get DATE_TIME"));
#endif

  // Below is used to formatt the raw date&time data into a readable UTC time, which is adjusted to RSA time.
  String tmp = strDateTime.substring(8, 10);
  char tmp2[3];
  int real_hour = tmp.toInt();
  real_hour += 2; // for RSA time zone BUT time shows as 24h00 to 25h00 ....
  
  if (real_hour == 24) { // Added for correct time formatting
    real_hour = '00';
  } else if (real_hour == 25) {
    real_hour = '01';
  } else if (real_hour == 2) {
    real_hour = '02';
  } else if (real_hour == 3) {
    real_hour = '03';
  } else if (real_hour == 4) {
    real_hour = '04';
  } else if (real_hour == 5) {
    real_hour = '05';
  } else if (real_hour == 6) {
    real_hour = '06';
  } else if (real_hour == 7) {
    real_hour = '07';
  } else if (real_hour == 8) {
    real_hour = '08';
  } else if (real_hour == 9) {
    real_hour = '09';
  }
  String retStr = strDateTime.substring(0, 4) + "-" + strDateTime.substring(4, 6) + "-" + strDateTime.substring(6, 8) + "T" + itoa(real_hour, tmp2, 10) + ":" + strDateTime.substring(10, 12); // + "+02"; // 2015-10-10T09:24+02

  return (retStr);
}

// All the commands to setup a GPRS context and get ready for HTTP command
void setupGPRS() {
  //the sendATCommand sends the command to the FONA and waits until the recieves a response before continueing on.
  Serial.print(F("\n\n *** in 'setupGPRS()'\n"));

  if (sendATCommand("ATE0")) { //disable local ech)o
    Serial.println(response);
  } else {
    Serial.print(F("ERROR: ATE0"));
    //          Serial.println(response);
  }

  Serial.print(F("Set to TEXT Mode: "));
  if (sendATCommand("AT+CMGF=1")) { //sets SMS mode to TEXT mode....This MIGHT not be needed. But it doesn't break anything with it there.
    Serial.println(response);
  }
  //    else {
  //            Serial.print("ERROR: ");
  //            //Serial.println(response);
  //        }

  Serial.print(F("Attach GPRS: "));
  if (sendATCommand("AT+CGATT=1")) { //Attach to GPRS service (1 - attach, 0 - disengage)
    Serial.println(response);
  }

  Serial.print(F("Set Conn To GPRS: ")); //AT+SAPBR - Bearer settings for applications based on IP
  if (sendATCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"")) { //3 - Set bearer perameters
    Serial.println(response);
  }
  else {
    Serial.print(F("ERROR: "));
    Serial.println(response);
  }

  Serial.print(F("Set APN: "));
  if (setAPN()) {
    Serial.println(response);
  }
  else {
    Serial.print(F("ERROR: "));
    Serial.println(response);
  }

  if (sendATCommand("AT+SAPBR=1,1")) { //Open Bearer
    if (response == "OK") {
      Serial.println(F("Engaged GPRS"));
    } else {
      Serial.println(F("GPRS Already on"));
    }
  }
  Serial.println(F("\nExit SetupGPRS()\n"));

}

//Set the APN. See sendATCommand for full comments on flow
boolean setAPN() {
  int complete = 0;
  char c;
  String content;
  unsigned long commandClock = millis();                      // Start the timeout clock
  fonaSS.print(F("AT+SAPBR=3,1,\"APN\",\""));
  //    fonaSS.print(APN);
  fonaSS.print(F("internet"));
  fonaSS.print("\"");
  fonaSS.println();
  while (!complete && commandClock <= millis() + ATtimeOut) {
    while (!fonaSS.available() && commandClock <= millis() + ATtimeOut);
    while (fonaSS.available()) {
      c = fonaSS.read();
      if (c == 0x0A || c == 0x0D);
      else content.concat(c);
    }
    response = content;
    complete = 1;
  }
  if (complete == 1) return 1;
  else return 0;
}

////Send an AT command and wait for a response
boolean sendATCommand(char Command[]) {
  int complete = 0; // have we collected the whole response?
  char c; //capture serial stream
  String content; //place to save serial stream
  unsigned long commandClock = millis(); //timeout Clock
  fonaSS.println(Command); //Print Command
  while (!complete && commandClock <= millis() + ATtimeOut) { //wait until the command is complete
    while (!fonaSS.available() && commandClock <= millis() + ATtimeOut); //wait until the Serial Port is opened
    while (fonaSS.available()) { //Collect the response
      c = fonaSS.read(); //capture it
      if (c == 0x0A || c == 0x0D); //disregard all new lines and carrige returns (makes the String matching eaiser to do)
      else content.concat(c); //concatonate the stream into a String
    }
    //Serial.println(content); //Debug
    response = content; //Save it out to a global Variable (How do you return a String from a Function?)
    complete = 1;  //Lable as Done.
  }
  if (complete == 1) return 1; //Is it done? return a 1
  else return 0; //otherwise don't (this will trigger if the command times out)
  /*
      Note: This function may not work perfectly...but it works pretty well. I'm not totally sure how well the timeout function works. It'll be worth testing.
      Another bug is that if you send a command that returns with two responses, an OK, and then something else, it will ignore the something else and just say DONE as soon as the first response happens.
      For example, HTTPACTION=0, returns with an OK when it's intiialized, then a second response when the action is complete. OR HTTPREAD does the same. That is poorly handled here, hence all the delays up above.
  */
}


