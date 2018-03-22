/*****************************************************************
   SD Card specific functions
 *****************************************************************/

// The below function initialises the SD card then, if the data file does not already exsist, writes the header to the file.
void initSDCard() {
#ifdef ECHO_TO_SERIAL
  Serial.print(F("Initializing SD card... "));
#endif
  // open (create) file and close it, as check
  if (!SD.begin(chipSelect)) {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("Card failed, or not present"));
#endif
    // don't do anything more. But if we cant log data is it worth it to continue? If so send warning SMS>??
    return;
  } else {
    // *************************************************************************************************************
    // Creating the log file for Telemetry data below
    // If the file does not exsist, create it with the header and first time info
    // *************************************************************************************************************
    if (SD.exists(telem_logfilename) == false) {
      dataFile = SD.open(telem_logfilename, FILE_WRITE); // if file does not exist it will be created but it should exsist at this point
      if (dataFile) {
        dataFile.println("Author: @JaysonMarilli & @VernonBreet");
        dataFile.print("Software Version: "); dataFile.println(SW_VERSION);
        dataFile.print("COW: "); dataFile.println(_serial_number);
        dataFile.println(telem_logFileHeader);
        dataFile.close();
#ifdef ECHO_TO_SERIAL
        Serial.print(telem_logfilename); Serial.println(F(" created with headers."));
#endif
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("Error opening " + String(telem_logfilename));
      }
      // *************************************************************************************************************
      // Telemetry data log file END!
      // *************************************************************************************************************

      // *************************************************************************************************************
      // Creating the log file for FONA GPS data below
      // If the file does not exsist, create it with the header and first time info
      // *************************************************************************************************************
      if (SD.exists(fona_logfilename) == false) {
        dataFile = SD.open(fona_logfilename, FILE_WRITE); // if file does not exist it will be created but it should exsist at this point
        if (dataFile) {
          dataFile.println("Author: @JaysonMarilli");
          dataFile.print("Software Version: "); dataFile.println(SW_VERSION);
          dataFile.print("COW: "); dataFile.println(_serial_number);
          dataFile.println(fona_logFileHeader);
          dataFile.close();
#ifdef ECHO_TO_SERIAL
          Serial.print(fona_logfilename); Serial.println(F(" created with headers."));
#endif
        }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("Error opening " + String(fona_logfilename));
        }
      }
      // *************************************************************************************************************
      // FONA GPS data log file END!
      // *************************************************************************************************************
    } else {
#ifdef ECHO_TO_SERIAL
      Serial.println(F("Card initialized."));
#endif
    }
  }
}

// Below function writes the Telemetry data to the SD card
void write_Telem_ToSD() {
  dataFile = SD.open(telem_logfilename, FILE_WRITE); // if file does not exist it will be created but it should exsist at this point
  if (dataFile) {
    // Below is the string that will be written to the SD card. Need to maybe add error checking if sensor data is invalid.
    // Telemetry data will be written under the below HEADINGS. 1 line per write all data seperated by "," for ease of import to excel. To be upgraded!!!*!*!**!
    //  Internal Temp , External Temp , LUX ,
    dataFile.print(" ");
    dataFile.print(DateAndTime); dataFile.print(" ,");
    dataFile.print(internalTempDATA); dataFile.print(" ,");
    dataFile.print(externalTempDATA); dataFile.print(" ,");
    dataFile.print(irDATA); dataFile.print(" ,");
    dataFile.print(fullDATA); dataFile.print(" ,");
    dataFile.print(visibleDATA); dataFile.print(" ,");
    dataFile.print(luxDATA); dataFile.print(" ,");

    dataFile.print(READ_SENSOR_MIN); dataFile.print(" ,");
    dataFile.println(""); // EOL
    dataFile.close(); // close data file until next write
#ifdef ECHO_TO_SERIAL
    Serial.println(F(""));
    Serial.println(F("***** TELEMETRY DATA LOGGING COMPLETE *****"));
    Serial.println(F(""));
#endif
  } else {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("***** ERROR WRITING TO TELEMETRY DATA FILE *****"));
    Serial.println(F(""));
#endif
  }
}

// Below function writes the FONA 808 GPS data to the SD card
void write_GPS_ToSD() {
  dataFile = SD.open(fona_logfilename, FILE_WRITE); // if file does not exist it will be created but it should exsist at this point
  if (dataFile) {
    // Writing all the data to the SD card, because, why not? We have it, dont send to the web BUT log to SD card
    dataFile.print(" ");
    dataFile.print(DateAndTime); dataFile.print(" ,");
    dataFile.print(battVol); dataFile.print(" ,");
    dataFile.print(battPer); dataFile.print(" ,");
    dataFile.print(Latitude, 6); dataFile.print(" ,");
    dataFile.print(Longitude, 6); dataFile.print(" ,");
    dataFile.print(mySpeed); dataFile.print(" ,");
    dataFile.print(myHeading); dataFile.print(" ,");
    dataFile.print(myAltitude); dataFile.print(" ,");
    dataFile.println(""); // EOL
    dataFile.close(); // close data file until next write
#ifdef ECHO_TO_SERIAL
    Serial.println(F("***** GPS DATA LOGGING COMPLETE *****"));
    Serial.println(F(""));
#endif
  } else {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("***** ERROR WRITING TO GPS DATA FILE *****"));
    Serial.println(F(""));
#endif
  }
}
