/*****************************************************************
   Sensor specific functions
 *****************************************************************/
 
void initLuxSensor() {
  if (tsl.begin()) {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("TSL2561 (LUX Sensor) found"));
#endif

  } else {
#ifdef ECHO_TO_SERIAL
    Serial.println(F("TSL2561 (LUX Sensor) NOT found???"));
#endif
  }

  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  //tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)
}

void readLux() {
  // Simple data read example. Just read the infrared, fullspecrtrum diode
  // or 'visible' (difference between the two) channels.
  // This can take 13-402 milliseconds! Uncomment whichever of the following you want to read
  uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);
  //uint16_t x = tsl.getLuminosity(TSL2561_FULLSPECTRUM);
  //uint16_t x = tsl.getLuminosity(TSL2561_INFRARED);

  //  Serial.println(x, DEC);

  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  irDATA = ir;
  fullDATA = full;
  visibleDATA = full - ir;
  luxDATA = tsl.calculateLux(full, ir);
#ifdef ECHO_TO_SERIAL
  Serial.print(F("IR: ")); Serial.print(irDATA);   Serial.print("\t");
  Serial.print(F("Full: ")); Serial.print(fullDATA);   Serial.print("\t");
  Serial.print(F("Visible: ")); Serial.print(visibleDATA);   Serial.print("\t");
  Serial.print(F("Lux: ")); Serial.println(luxDATA);
#endif
}

void readTempInternal() {
  float temperature = 0.0;   // stores the calculated temperature
  int sample;                // counts through ADC samples
  float ten_samples = 0.0;   // stores sum of 10 samples

  // take 10 samples from the TMP36
  for (sample = 0; sample < 10; sample++) {
    // convert A0 value to temperature
    temperature = ((float)analogRead(tempInput) * 5.0 / 1024.0) - 0.5;
    temperature = temperature / 0.01;
    // sample every 0.1 seconds
    delay(100);
    // sum of all samples
    ten_samples = ten_samples + temperature;
  }
  // get the average value of 10 temperatures
  internalTempDATA = ten_samples / 10.0;
#ifdef ECHO_TO_SERIAL
  Serial.print(F("Internal temp sensor reading: ")); Serial.print(internalTempDATA); Serial.println(F(" degrees C"));
#endif
  ten_samples = 0.0;
}

void initTempExternal() {
#ifdef ECHO_TO_SERIAL
  Serial.println(F("Dallas Temperature IC Control Library started."));
#endif
  delay(100);
  // Start up the library
  sensors.begin();
  delay(5000);
}

void readTempExternal() {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperatures
  externalTempDATA = sensors.getTempCByIndex(0);
  //  Serial.print(sensors.getTempCByIndex(0)); // Why "byIndex"?
  // You can have more than one IC on the same bus.
  // 0 refers to the first IC on the wire
#ifdef ECHO_TO_SERIAL
  Serial.print(F("External temp sensor reading: "));
  Serial.print(externalTempDATA); Serial.print(F(" degrees C")); Serial.println(F(""));
#endif
}

