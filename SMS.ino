/*****************************************************************
   SMS specific functions
 *****************************************************************/

bool checkForSMS() {
  smsnum = fona.getNumSMS();
  if (smsnum < 1) {  // Check to see if there is an sms on the sim card
#ifdef ECHO_TO_SERIAL
    Serial.println(F("There are no SMSes on the SIM card"));
#endif
    return false;
  } else {
#ifdef ECHO_TO_SERIAL
    Serial.print(smsnum);
    Serial.println(F(" SMS's on SIM card!"));
#endif
    return true;
  }
}

void readSMS() {
  uint16_t smslen;
  uint8_t smsn = smsnum;
#ifdef ECHO_TO_SERIAL
  Serial.print(F("Read #"));
  Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);
#endif

  // Retrieve SMS sender address/phone number.
  if (! fona.getSMSSender(smsn, callerIDbuffer, 250)) {
#ifdef ECHO_TO_SERIAL
    Serial.println("Failed!");
#endif
  }
#ifdef ECHO_TO_SERIAL
  Serial.print(F("FROM: ")); Serial.println(callerIDbuffer); // Serial.println(replybuffer);
  Serial.println(F("Reading SMS now... ")); Serial.println(F(""));
#endif
  // Retrieve SMS value.
  if (! fona.readSMS(smsn, receivebuffer, 250, &smslen)) { // pass in buffer and max len!
#ifdef ECHO_TO_SERIAL
    Serial.println(F("Failed!"));
#endif
  }
  receiveString = String(receivebuffer);

  // After reading the SMS data, we check to see if its 'Where are you' as this will reply to any number it is sent from
  if (isWhereAreYou()) { // if true then we send a reply with the string built in the function
    sendSMS(replyString);
    mySMS.flag1 = false; // Set the flag to false so that we do not try read the SMS as a command sms
    return; // We want to return out of this function here. Does this work?
  } else { // if the SMS is not a where are you then set the flag to true so that we can check that command
    mySMS.flag1 = true;
  }

  // String manipulation on the SMS data
  mySMS.pinNumber = getStringPartByNr(receiveString, ' ', 0);
  mySMS.command = getStringPartByNr(receiveString, ' ', 1);
  mySMS.newValue = getStringPartByNr(receiveString, ' ', 2);
  mySMS.nul = getStringPartByNr(receiveString, ' ', 3);

}

void checkSMSCommand() {
  int input = mySMS.newValue.toInt();
  if (mySMS.flag1) {
    if (String(mySMS.command) == mySMS.readSens) {
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Command received: ")); Serial.println(mySMS.command);
#endif
      int val = readEepromInt(READ_SENSOR_MIN_ADDRESS);
      //              writeEepromInt(READ_SENSOR_MIN_ADDRESS, input);
      replyString = "The current timer interval for SENSOR reads is set to " + String(val) + " minutes";
      mySMS.sendFlag = true;

    } else if (String(mySMS.command) == mySMS.readGPS) {
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Command received: ")); Serial.println(mySMS.command);
#endif
      int val = readEepromInt(READ_GPS_MIN_ADDRESS);
      replyString = "The current timer interval for GPS reads is set to " + String(val) + " minutes";
      mySMS.sendFlag = true;
    } else if (String(mySMS.command) == mySMS.writeSens) {
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Command received: ")); Serial.println(mySMS.command);
#endif
      if ((input > (MAX + 1)) || (input < MIN)) {
        replyString = "Your value of " + String(input) + " falls out of the set bounds of MAX: 60 min MIN: 1 min. Try again.";
      } else {
        writeEepromInt(READ_SENSOR_MIN_ADDRESS, input);
        replyString = "The timer interval for SENSOR reads has been set to " + String(mySMS.newValue) + " minutes";
      }
      mySMS.sendFlag = true;
    } else if (String(mySMS.command) == mySMS.writeGPS) {
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Command received: ")); Serial.println(mySMS.command);
#endif
      if ((input > (MAX + 1)) || (input < MIN)) {
        replyString = "Your value of " + String(input) + " falls out of the set bounds of MAX: 60 min MIN: 1 min. Try again.";
      } else {
        writeEepromInt(READ_GPS_MIN_ADDRESS, input);
        replyString = "The timer interval for GPS reads has been set to " + String(mySMS.newValue) + " minutes";
      }
      mySMS.sendFlag = true;
    } else if (String(mySMS.command) == mySMS.getStats) {
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Command received: ")); Serial.println(mySMS.command);
#endif
      replyString = "Date and Time: " + String(DateAndTime) + " | " + "Batt Percent: " + String(battPer) + " | " + "Skin Temp: " + String(externalTempDATA) + " | ""LUX: " + String(luxDATA);
      mySMS.sendFlag = true;
    } else if (String(mySMS.command) == mySMS.getBalance) {
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Command received: ")); Serial.println(mySMS.command);
#endif
      checkBalance();
      deleteSMS(1);
      replyString = "Balances: " + String(mySMS.airtimeBalance) + ", Available" + String(mySMS.dataBalance) + ", and" + String(mySMS.smsBalance) + " remaining";
      mySMS.sendFlag = true;
    } else if (String(mySMS.command) == mySMS.getURL) {
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Command received: ")); Serial.println(mySMS.command);
#endif
      replyString = "The Sparkfun URL is - " + String(myURL);
      mySMS.sendFlag = true;
    } else {
      replyString = "Your command was not recognised. Please try again.";
      mySMS.sendFlag = true;
    }
    mySMS.pinNumber = "";
    mySMS.command = "";
    mySMS.newValue = "";
    mySMS.flag1 = false;
  }
}

bool isWhereAreYou() {
  //  receiveString = String(receivebuffer);
  if ((receiveString == "Where are you?") || (receiveString == "where are you?") || (receiveString == "Where are you") || (receiveString == "where are you")) {
    replyString = "I was here - https://maps.google.com/?q=" + String(Latitude, 6) + "," + String(Longitude, 6) + " - at " + String(DateAndTime);
#ifdef ECHO_TO_SERIAL
    Serial.println(replyString);
#endif
    return true;
  } else return false;
}

bool isAuthorised() {
  pinPointer = 0;
  authPointer = 0;
  uint16_t smslen;
  uint8_t smsn = smsnum;
  for (int i = 0; i < 3; i++) {
    if (String(callerIDbuffer) == (AUTHORISED[i])) { // Checking that the SMS has come from an authorised number
#ifdef ECHO_TO_SERIAL
      Serial.println(F("SMS received from authorized sender"));
#endif
      pinPointer = i;
      authPointer = i;
      if (PIN_NUMBER[pinPointer] == mySMS.pinNumber) { // Check to see if the pin number sent by the auth'ed sender matches our records
#ifdef ECHO_TO_SERIAL
        Serial.print(F("***** SMS #")); Serial.print(smsn);
        Serial.print(F(" (")); Serial.print(smslen); Serial.println(F(") bytes *****"));
        Serial.println(receivebuffer);
        Serial.println(F("*****"));
#endif
        return true;
      } else {  // If the pin number is incorrect, let the auth'ed sender know that the pin is wrong
        replyString = "Your PIN number does not match our records. Please try again.";
        sendSMS(replyString);
        return false;
      }
    } else return false; // If SMS received from an unknown number, dont even waste money replying to it
  }
}

void sendSMS(String reply) {
  int str_len = reply.length() + 1;
  reply.toCharArray(replybuffer, str_len);
  //Send back an automatic response
#ifdef ECHO_TO_SERIAL
  Serial.println(F("Sending reponse..."));
#endif
  if (!fona.sendSMS(callerIDbuffer, replybuffer)) {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("Failed"));
#endif
  } else {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("Sent!"));
#endif
  }
  replyString = "";
}

void deleteSMS(uint8_t number) {
  uint8_t smsn = number;
#ifdef ECHO_TO_SERIAL
  Serial.print(F("\n\rDeleting SMS #")); Serial.println(smsn);
#endif
  if (fona.deleteSMS(smsn)) {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("OK!"));
#endif
  } else {
#ifdef ECHO_TO_SERIAL
#endif
    Serial.println(F("Couldn't delete"));
  }
}

void checkBalance() { // @@@
  char* ussdCommand = "*111*502#";
  uint16_t ussdlen;

  Serial.println(F("Checking SIM balances via USSD..."));
  if (!fona.sendUSSD(ussdCommand, replybuffer, 250, &ussdlen)) { // pass in buffer and max len!
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
    Serial.print(F("***** USSD Reply > "));
    Serial.println(replybuffer);
    Serial.println(F("*****"));
    mySMS.airtimeBalance = getStringPartByNr(replybuffer, ';', 0);
    mySMS.dataBalance = getStringPartByNr(replybuffer, ';', 1);
    mySMS.smsBalance = getStringPartByNr(replybuffer, ';', 2);
    mySMS.nul = getStringPartByNr(replybuffer, ';', 3);
  }

}



