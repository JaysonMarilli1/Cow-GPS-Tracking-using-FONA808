/*
   ADD RSSI
   Send Lux, Isolar + temps to Web: long way, then Phant!!

   2016/02/26
   Having all SMS functions in its on sketch uses too many global variable. this causes the memory to become too full and causes the phant to stop working?!?!?
*/
//{ // Bunch of stuff....
/***************************************************
                Phase II
****************************************************/

// General libraries
#include <Wire.h>
#include <EEPROMex.h>

// FONA808 library
#include "Adafruit_FONA.h"

// Arduino Json library used for sending data to the interwebs
//#include <ArduinoJson.h>

// include the SD library:
#include <SPI.h>
#include <SD.h>


// avr-libc library includes used for the TIMERs
#include <avr/io.h>
#include <avr/interrupt.h>

// Phant include for posting data to the web
#include <Phant.h>

// For DS18B20 external temp sensor From: http://www.hobbytronics.co.uk/ds18b20-arduino
#include <OneWire.h>
#include <DallasTemperature.h>

// Library for the TSL2561 Lux Sensor
#include "TSL2561.h"

// VB160223 Added LIB
#include <LowPower.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 5

// FONA808 stuff
#define FONA_RX 2
#define FONA_TX 10
#define FONA_RST 4
#define FONA_PS 7 //connect to FONA PS !!!!!!!!!!!!!!!!!!!!!
#define FONA_KEY 6 //connection to FONA KEY

// ************************************************************************************************************************************************************
// ************************************************************************************************************************************************************
//                                   Comment the below line out when sending to the field as a possible energy saver
#define ECHO_TO_SERIAL
// ************************************************************************************************************************************************************
// ************************************************************************************************************************************************************

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// This is to handle the absence of software serial on platforms
// like the Arduino Due. Modify this code if you are using different
// hardware serial port, or if you are using a non-avr platform
// that supports software serial.
#ifdef __AVR__
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
String serialUSED = "Serial interface set to: Software Serial";
#else
HardwareSerial *fonaSerial = &Serial1; // Not used....
String serialUSED = "Serial interface set to: Hardware Serial";
#endif

//TMP36 Pin Variables
int tempInput = 0; //the analog pin the TMP36's Vout (sense) pin is connected to
//the resolution is 10 mV / degree centigrade with a
//500 mV offset to allow for negative temperatures

// SMS requirements below
const String AUTHORISED[] = {"+27798523662", "+27824572031", "test2"}; // INSERT YOUR PHONE NUMBER HERE
const String PIN_NUMBER[] = {"8704", "0467", "1234"};
int pinPointer;
int authPointer;


struct settings {
  String pinNumber = "";
  String command = "";
  String newValue = "";
  String airtimeBalance = "";
  String dataBalance = "";
  String smsBalance = "";
  String nul = ""; // null used in the string manipulation
  const char* readSens PROGMEM = "GetSensorInterval";
  const char* readGPS PROGMEM = "GetGpsInterval";
  const char* writeSens PROGMEM = "SetSensorInterval";
  const char* writeGPS PROGMEM = "SetGpsInterval";
  const char* getStats PROGMEM = "GetStats";
  const char* getBalance PROGMEM = "GetBalance";
  const char* getURL PROGMEM = "GetUrl";
  bool flag1 = false;
  bool firstGpsFlag = true; // flag used to send a SMS at the first GPS fix after power
  bool sendFlag = false;
};

settings mySMS;

int8_t smsnum; // Used to store the number of SMS'es on the sim

int addressInt = 300;
char callerIDbuffer[32];  //we'll store the SMS sender number in here
char fonaInBuffer[64];          //for notifications from the FONA
// this is a large buffer for replies
char replybuffer[141];
// this is a large buffer for received messages
char receivebuffer[255];
String receiveString;
String replyString;

// Initialised for Timer1
const int READ_SENSOR_MIN_ADDRESS = 320; // EEEPROM address for sensor read interval
const int READ_SENSOR_MIN_DEFAULT = 15; // Startup default - Sleep time between sensor reads
int READ_SENSOR_MIN; // Sleep time for Sensor read in min, to be read from EEPROM
unsigned int sensor_minutes = 0; // do not change this!!!*!*!*!* used in Timer1 as a counter
bool READ_SENSOR_FLAG = false;

// Initialised for Timer2
const int READ_GPS_MIN_ADDRESS = 340; // EEEPROM address for GPS read interval
const int READ_GPS_MIN_DEFAULT = 15; // Startup default - Sleep time between GPS reads
int READ_GPS_MIN; // Sleep time for GPS read in min, to be read from EEPROM
unsigned int gps_minutes = 0; // do not change this!!!*!*!*!* used in Timer2 as a counter
unsigned int count = 0; // do not change this!!!*!*!*!* used in Timer2 as a counter
bool READ_GPS_FLAG = true;  // No point NOT reading the GPS on startup

// Sleep time for the FONA. What happens if SIM runs out of data?
int SLEEP_MINUTES = 2; //15;  // 5; //Sleep time

// MIN and MAX time in minutes that the GPS and Sensor intervals can be set to.
const int MIN PROGMEM = 1;
const int MAX PROGMEM = 60;

const String SW_VERSION PROGMEM = "2.160303a"; // Fixing SD card issue. Removed call to wake and timer stuff. Added COW serial number to log files. No longer crashing if no SD card present
                                               // changed the LUX sensor tsl.setGain(TSL2561_GAIN_16X);  ----> tsl.setGain(TSL2561_GAIN_0X); to stop readings on 53 
                                               // Fixed time formatting 1,2,3,etc  ---> 01,02,03, etc
//const String SW_VERSION PROGMEM = "2.160302b"; // Re-added VB SD card code, taking out this code broke the serial / mega
//const String SW_VERSION PROGMEM = "2.160302a"; // Removed VB SD card code, now data is being logged again. Fixed SMS 'where are you' and then still reading SMS for a command
//const String SW_VERSION PROGMEM = "2.160301a"; // Removed missed call wake up.
//const String SW_VERSION PROGMEM = "2.160229a"; // Memory optimization + Miss call wakes me up
//const String SW_VERSION = "2.160226a"; // Intergrated the SMS commands. I seem to have broken phant - getting 403 error messages. memory issue???
//const String SW_VERSION = "2.160225b"; // Moved SD Card, Sensor and Fona808 functions into seperate sketches
//const String SW_VERSION = "2.160224a"; // fixed lux(?), Phant, format to web @ https://data.sparkfun.com/streams/o8yQW9ZD6qt5jvqyLpAX , added RSSI
//const String SW_VERSION = "2.160223a"; // fixed the RSA time constantly setting the hour to 1. Added getStats SMS
//const String SW_VERSION = "2.160202a"; // Intergrating the data logging with the the FONA808

// The address will be different depending on whether you let
// the ADDR pin float (addr 0x39), or tie it to ground or vcc. In those cases
// use TSL2561_ADDR_LOW (0x29) or TSL2561_ADDR_HIGH (0x49) respectively
TSL2561 tsl(TSL2561_ADDR_FLOAT);

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// set up variables using the SD utility library functions:
Sd2Card card;
//SdVolume volume;
//SdFile root;
File dataFile; // log data file

String response; //global variable for pulling AT command responses from inside functions (there has to be a better way to do this)
const unsigned long ATtimeOut = 10000; // How long we will give an AT command to comeplete

//StaticJsonBuffer<200> jsonBuffer;
// Define the chip select pin on the MEGA
const int chipSelect = 53;

// Data file for Telemetry Data stored on the SD card as well as the header for the file. Need to arrange for a dynamic name??
const char telem_logfilename[13] = "TeleData.txt"; // cmax 8 char befor dot!
const String telem_logFileHeader = " MyUTC , Internal Temp , External Temp , IR , Full , Visible , LUX , Interval , ";
// Data file for FONA808 Data stored on the SD card as well as the header for the file. Need to arrange for a dynamic name??
const char fona_logfilename[13] = "FonaData.txt"; // cmax 8 char befor dot!
const String fona_logFileHeader = " MyUTC , Battery Voltage , Battery Percentage , Latitude , Longitude , Speed , Heading , Altitude , ";

// define sensor variables
float externalTempDATA = 0.0;
float internalTempDATA = 0.0;
int irDATA = 0;
int fullDATA = 0;
int visibleDATA = 0;
int luxDATA = 0;

const int keyTime PROGMEM = 2000; // Time needed to turn on the Fona
uint8_t type; // This should not be needed if we are using a standard version of the FONA. VVB said we would only use v2 FONAs

String myRSSI;

// Used to reset the Arduino if FONA not found?
void(* resetFunc) (void) = 0; //declare reset function @ address 0

//} // ======================== Bunch of stuff ends..... =================================================

// Sparkfun Phant stuff
// https://data.sparkfun.com/streams/o8yQW9ZD6qt5jvqyLpAX
const String myURL PROGMEM = "https://data.sparkfun.com/streams/o8yQW9ZD6qt5jvqyLpAX";
Phant phant("data.sparkfun.com", "o8yQW9ZD6qt5jvqyLpAX", "yz7ZypYbJNtBrKyNpnW6");
// Phase II Phant Variables - 160129 - VVB
String _device_serial, _device_vbat, _latitude, _longitude, _temp_skin, _ver_fw, current1, current2, current3, lux, sd_perc_free, temp_box, volt1, volt2, volt3, rssi, x_utc;

/* My web test info
    Jay_Test_FONA - https://data.sparkfun.com/streams/lz3KV7nWoDF9r1Wo212Z/
*/

const String _serial_number = "COW3-YUN"; //transponder_serial_number -MUST BE unqique for each set of electronics. Could also use FONA808 IMEI?

//const String _serial_number = "COW-FLOR"; //transponder_serial_number -MUST BE unqique for each set of electronics. Could also use FONA808 IMEI?

//const String _serial_number = "COW-5"; //transponder_serial_number -MUST BE unqique for each set of electronics. Could also use FONA808 IMEI?

// Define FONA808 variables
float Latitude;
float Longitude;
float mySpeed;
float myHeading;
float myAltitude;
String DateAndTime;
float battVol;
float battPer;
uint16_t vbat;
int SleepCounter = 120; //    if (SleepCounter < 112) { // 112 x 8 sec = 896 sec ~ 15 min

void setup()
{
  Serial.begin(115200);

#ifdef ECHO_TO_SERIAL
  Serial.println(F("ECHO_TO_SERIAL is enabled. Dont forget to comment this out before sending to the field!!"));
#else
  Serial.println(F("ECHO_TO_SERIAL is disabled for power saving. If you want serial data please enable!!"));
  Serial.println(F("There will be no further serial data printed."));
#endif

  /*

    writeEepromInt(READ_SENSOR_MIN_ADDRESS, READ_SENSOR_MIN_DEFAULT);
    writeEepromInt(READ_GPS_MIN_ADDRESS, READ_GPS_MIN_DEFAULT);
  */

#ifdef ECHO_TO_SERIAL
  Serial.print(F("\n\n====== SW_VERSION = "));
  Serial.println(SW_VERSION);
  Serial.println(serialUSED);
  Serial.print(F("RX pin used: ")); Serial.print(FONA_RX); Serial.print(F("   -   TX pin used: ")); Serial.println(FONA_TX);
#endif
  ;

  // restart with 4800 baud and attempt to enable the FONA
  fonaSerial->begin(4800);
#ifdef ECHO_TO_SERIAL
  Serial.println(F("Init @ 4800 ..."));
#endif
  if (! fona.begin(*fonaSerial)) {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("FONA not found!!!!"));
#endif
    for (int i = 0; i < (2 * SLEEP_MINUTES); i++) {     // We erset every n x 2 sleep if fona 'not ready', e.g. battery unable to support it
#ifdef ECHO_TO_SERIAL
      Serial.print(F("Sleep 1 minute!\n"));
      Serial.print(SLEEP_MINUTES - i);
      Serial.println(F(" minutes to go!\n"));
#endif
      delay(60000);

    }
    resetFunc();
  }

  // This should not be needed if we are using a standard version of the FONA. VVB said we would only use v2 FONAs
  type = fona.type();

  // Attempt to enable the GPS
#ifdef ECHO_TO_SERIAL
  Serial.println(F("Enabling GPS..."));
#endif
  fona.enableGPS(true);
  delay(5000);
#ifdef ECHO_TO_SERIAL
  Serial.println("");
#endif

  // Input timer intervals into the EEPROM and then into interval delay
  //  writeEepromInt(READ_SENSOR_MIN_ADDRESS, READ_SENSOR_MIN_DEFAULT);
  READ_SENSOR_MIN = readEepromInt(READ_SENSOR_MIN_ADDRESS);
  READ_GPS_MIN = readEepromInt(READ_GPS_MIN_ADDRESS);
#ifdef ECHO_TO_SERIAL
  Serial.print(F("Sensor read interval set to ")); Serial.print(READ_SENSOR_MIN); Serial.println(F(" minutes."));
  Serial.print(F("GPS read interval set to ")); Serial.print(READ_GPS_MIN); Serial.println(F(" minutes."));
#endif

  // Initialise the sensors. (internal temp sensor doesnt need to be initialised).
  initLuxSensor();
  initTempExternal();

  // Initialise the SD card then write the header to the log file
  initSDCard();
  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
//  if (!volume.init(card)) {
//    Serial.println(F("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card"));
//    return;
//  }
//
//
//  // print the type and size of the first FAT-type volume
//  uint32_t volumesize;
//  Serial.print(F("\nVolume type is FAT"));
//  Serial.println(volume.fatType(), DEC);
//  Serial.println();
//
//  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
//  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
//  volumesize *= 512;                            // SD card blocks are always 512 bytes
//  Serial.print(F("Volume size (bytes): "));
//  Serial.println(volumesize);
//  Serial.print(F("Volume size (Kbytes): "));
//  volumesize /= 1024;
//  Serial.println(volumesize);
//  Serial.print(F("Volume size (Mbytes): "));
//  volumesize /= 1024;
//  Serial.println(volumesize);
//
//
//  Serial.println(F("\nFiles found on the card (name, date and size in bytes): "));
//  root.openRoot(volume);
//
//  // list all files in the card with date and size
//  root.ls(LS_R | LS_DATE | LS_SIZE);

#ifdef ECHO_TO_SERIAL
  Serial.println(F(""));
  //  Serial.println("Leaving setup now. Proceeding to MAIN loop and waiting for TIMERs to trigger.");
  Serial.println(F("Leaving setup now. Proceeding to MAIN loop and waiting for 'powerDown' to trigger."));
  Serial.println(F(""));
#endif
  turnOffFONA();
  delay(10000); // 10 sec - allow all messages relating to this, to go out
}

void loop()
{
  // Create a small string buffer to hold incoming call number.
//  char phone[32] = {0};

  if (SleepCounter < 112) { // 112 x 8 sec = 896 sec ~ 15 min
  //if (SleepCounter < 32) {   // during debug, each 32 sec=4, 24=3min
    SleepCounter++; // sleep out the 15 min....
  } else { //  DO ACTIVE STUFF
    SleepCounter = 0;
    //pushover("OMG, Yes it works!!!");
    DoRealStuff();
  }

  //    // Enter power down state for 8 s with ADC and BOD module disabled
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

}


//================================= FOCUS !!! ===========================================

void DoRealStuff() {

  delay(10000); // 10 sec ???????????????
  // Turn the FONA808 on to start reading data
  turnOnFONA(); //turn on FONA, waits inside function
#ifdef ECHO_TO_SERIAL
  Serial.println(F("Initialising FONA808: please wait a few sec..."));
#endif
  //delay(10000); // 10 sec
  //  float latitude, longitude, speed_kph, heading, speed_mph, altitude;
  float speed_kph, heading, speed_mph, altitude;

  // check GPS fix - purely informational, does not affect program flow
  checkGPS_Fix();
  delay(2000); // 2 sec ?????????????

  GetBattStats();
  myRSSI = GetRSSI(); /// =============================================================================
  // Assign reading from the FONA to the variables
  DateAndTime = GetAndSetTime();

  readLux();
  readTempInternal();
  readTempExternal();
  write_Telem_ToSD();

  // if you ask for an altitude reading, getGPS will return false if there isn't a 3D fix
  boolean gps_success = fona.getGPS(&Latitude, &Longitude, &speed_kph, &heading, &altitude);
  if (gps_success) {

    { //print out the GPS readings
#ifdef ECHO_TO_SERIAL
      Serial.print(F("\n\n=== GPS SUCCESS ===\n"));

      Serial.print(F("GPS lat: "));
      Serial.println(Latitude, 6);
      Serial.print(F("GPS long: "));
      Serial.println(Longitude, 6);
      Serial.print(F("GPS speed KPH: "));
      Serial.println(speed_kph);
      Serial.print(F("GPS heading: "));
      Serial.println(heading);
      Serial.print(F("GPS altitude: "));
      Serial.println(altitude);
      Serial.println("");
#endif
    }

    // Send an SMS to Jayson the first time we get a GPS fix after power. Using this for debugging, will possibly disable to avoid SMS spam
    if (mySMS.firstGpsFlag) {
      //        replyString = "This Cow has now received a GPS fix: https://maps.google.com/?q=" + String(Latitude, 6) + "," + String(Longitude, 6) + " - at " + String(DateAndTime);
      if (!fona.sendSMS("+27798523662", "This Cow has now received a GPS fix")) {
#ifdef ECHO_TO_SERIAL
        Serial.println(F("Failed"));
#endif
      } else {
#ifdef ECHO_TO_SERIAL
        Serial.println(F("Sent!"));
#endif
      }
      mySMS.firstGpsFlag = false; // Flag condition only ever set to true on starup
    }


    // Set GPS data to variables
    mySpeed = speed_kph;
    myHeading = heading;
    myAltitude = altitude;
  }

  write_GPS_ToSD();

  Serial.println(F("Checking for SMS's..."));
  while (checkForSMS()) { // If there is an sms on the SIM card
    readSMS();
    if (isAuthorised()) {
      checkSMSCommand();
      if (mySMS.sendFlag) {
        sendSMS(replyString);
        mySMS.sendFlag = false;
        replyString = "";
      }
    }
    Serial.println(F("Deleting the SMS just read..."));
    // this could cause a problem if there are already SMS on the sim card in positions, say, 7 to 15, it will try and delete SMS #1 and fail...
    deleteSMS(smsnum); // last thing we do is delete the sms just read
  }


  setupGPRS(); //Setup a GPRS context and get ready for HTTP command
  delay(5000);
  //******************************************************************************************************

  Serial.println(F("\n\n         ============ makeRequestPHANT(); PHANT, to VB SITE =============================\n"));
  makeRequestPHANT(); // Send PHANT data
  Serial.println(F("\n\n         ============ DONE !! makeRequestPHANT(); PHANT, to VB SITE =============================\n"));

  delay(10000);
  // write_GPS_ToSD(); // SHOULD THIS BE HERE???????
  // flushFONA();
  turnOffFONA(); // Turn the FONA off to save battery
}

//================================= FOCUS !!! ===========================================

/*****************************************************************
   String manipulation function below
 *****************************************************************/
String getStringPartByNr(String data, char separator, int index) {
  // spliting a string and return the part nr index
  // split by separator
  int stringData = 0;        //variable to count data part nr
  String dataPart = "";      //variable to hole the return text
  for (int i = 0; i < data.length(); i++) { //Walk through the text one letter at a time
    if (data[i] == separator) {
      //Count the number of times separator character appears in the text
      stringData++;
    } else if (stringData == index) {
      //get the text when separator is the rignt one
      dataPart.concat(data[i]);
    } else if (stringData > index) {
      //return text and stop if the next separator appears - to save CPU-time
      return dataPart;
      break;
    }
  }
  //return text if this is the last part
  return dataPart;
}

/*****************************************************************
   SMS & EEPROM stuff below
 *****************************************************************/
// Moved to SMS.ino
// Moved to EEPROM.ino

/*****************************************************************
   TIMER stuff below
 *****************************************************************/
// No longer needed because we are using SLEEP_8S but moved to oldstuff.ino

/*****************************************************************
   FONA808 stuff below
 *****************************************************************/
// All functions moved to fona808.ino

/*****************************************************************
   Telemetry Data / SD card stuff below
 *****************************************************************/
// Moved to Sensors.ino
// Moved to SDCard.ino

/*****************************************************************
   Phant stuff below
 *****************************************************************/
 ///////////////////// 29 JAn 2016 - Phant
////
//// Phase II Phant Variables - 160129
////String ;
//// http://data.sparkfun.com/input/o8yQW9ZD6qt5jvqyLpAX?private_key=yz7ZypYbJNtBrKyNpnW6&_device_serial=SERIAL&_device_vbat=4321&_latitude=-26.1235&_longitude=28.6543&_temp_skin=34.6000&_ver_fw=2.160129a&current1=1.1000&current2=2.2200&current3=3.3330&lux=8765&sd_perc_free=0&temp_box=0&volt1=0&volt2=0&volt3=0&x_utc=0
////
//////builds url for Sparkfun GET Request, sends request and waits for reponse. See sendATCommand() for full comments on the flow
boolean sendPhant() {
  int complete = 0;
  char c;
  String content;
  unsigned long commandClock = millis();                      // Start the timeout clock
  { // Add params here
    phant.add("_device_serial", _serial_number);  // "SERIAL");
    phant.add("_device_vbat", vbat);  // 4321);
    phant.add("_latitude", (Latitude));  // -26.123456);
    phant.add("_longitude", (Longitude)); // 28.654321);

    phant.add("_temp_skin", externalTempDATA);
    phant.add("_ver_fw", SW_VERSION);
    phant.add("current1", 1.1);
    phant.add("current2", 2.22);

    phant.add("current3",  3.333);
    phant.add("lux", luxDATA);  // fullDATA);   // could also use irDATA, visibleDATA, luxDATA
    phant.add("sd_perc_free", 0);
    phant.add("temp_box", internalTempDATA);

    phant.add("volt1", 0);
    phant.add("volt2", 0);
    phant.add("volt3", 0);
    phant.add("rssi", myRSSI);   // vb 160224

    phant.add("x_utc", String(DateAndTime)); //retStr); // 0);

  }

  String LongerString;

  Serial.println(F("\n\n----TEST URL-----"));
  //  Serial.println(phant.url());
  //  //  Serial.println("----HTTP GET----");
  //  //  Serial.println(phant.get());    // This returns a CSV!
  //
  //  //Print all of the URL components out into the Serial Port
  Serial.println(F("----phant.url via HHTP GET -----"));
  fonaSS.print(F("AT+HTTPPARA=\"URL\",\""));
  Serial.print(F("AT+HTTPPARA=\"URL\",\""));


  //  fonaSS.print(phant.post());
  fonaSS.print(phant.url());
  Serial.print(phant.url());

  fonaSS.print("\"");
  Serial.print(F("\""));
  fonaSS.println();
  Serial.println();

  LongerString = "AT+HTTPPARA=\"URL\",\"" + phant.url() + "\"\n";
  Serial.print(F("\n\nLongerString =>"));
  Serial.print(LongerString);
  Serial.print(F("<=====\n\n"));

  Serial.println(F("\n\n\n\n//>>>>>>>> PHANT URL DOne!"));
}

////Make HTTP GET request and then close out GPRS connection
void makeRequestPHANT() {
  /* Lots of other options in the HTTP setup, see the datasheet: google -sim800_series_at_command_manual */
  Serial.print(F("makeRequestPHANT(); // Send PHANT data\n\nHTTP Initialized: "));
  //this checks if it is on. If it is, it's turns it off then back on again. (This Is probably not needed. )

  if (sendATCommand("AT+HTTPINIT")) { //initialize HTTP service. If it's already on, this will throw an Error.
    if (response != "OK") { //if you DO NOT respond OK (ie, you're already on)
      Serial.print(F("Failed, Restarting: "));
      if (sendATCommand("AT+HTTPTERM")) { //TURN OFF
        Serial.print(F("Trying Again: "));
        if (sendATCommand("AT+HTTPINIT")) { //TURN ON
          Serial.println(response);
        }
      }
    } else {
      Serial.println(response);
    }
    Serial.println(response);
  }
  Serial.print(F("Set Bearer Profile ID: "));
  if (sendATCommand("AT+HTTPPARA=\"CID\",1")) { //Mandatory, Bearer profile identifier
    Serial.println(response);
  }


  //============ Send to SparkFun ===============
  Serial.print(F("\n=========================================================== Send PHANT URL: "));

  if (sendPhant()) { //sets the URL for Sparkfun.
    Serial.print("\n\nresponse FROM sendPhant() =");
    Serial.println(response);
  }
  // DO I NEED THIS DELAY??
  delay(1000);
  Serial.print(F("\n===========================================================\n"));

  Serial.print(F("Make GET Request: "));
  if (sendATCommand("AT+HTTPACTION=0")) { //make get request =0 - GET, =1 - POST, =2 - HEAD
    Serial.println(response);
  }
  Serial.print(F("Delay 2sec.."));
  delay(2000); //wait for a bit for stuff to complete
  Serial.println(F("OK"));
  Serial.println(F("Flush Ser Port...."));
  flushFONA(); //Flush out the Serial Port
  Serial.print(F("Read HTTP Response: "));
  if (sendATCommand("AT+HTTPREAD")) { //Read the HTTP response and print it out
    Serial.println(response);
  }
  Serial.print(F("Delay for 2sec..."));
  delay(2000);//wait some more
  Serial.println(F("OK"));
  Serial.println(F("Flush Serial Port....."));
  flushFONA(); //Flush out the Serial Port
}


