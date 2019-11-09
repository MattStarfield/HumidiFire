/* 
 *  HumidiFire Firmware
 *  by Matt Garfield

 * TODO:
 * ----------
 * 
 * CHANGELOG:
 * ----------
 * 11/9/2019
 * - Initial setup
*/

/*=========================================================================*/
// == Include Libraries == //
/*=========================================================================*/

  #include <Wire.h>       //included in Adafruit SAMD Board Manager for Tools> Board > "Adafruit Feather M0"
  //#include <SPI.h>        //included in Adafruit SAMD Board Manager for Tools> Board > "Adafruit Feather M0"
  
  //#include <SD.h>         //included in Arduino IDE core
  
  //#include <RTClib.h>     //https://github.com/adafruit/RTClib
  //#include <DHT.h>        // Updated to using Adafruit DHT Library https://github.com/adafruit/DHT-sensor-library
                          // Requires Adafruit Libraries "DHT_sensor_library" and "Adafruit_Unified_Sensor"
                          // Setup Instructions here: https://learn.adafruit.com/dht/using-a-dhtxx-sensor
  
  // Bluefruit Libraries
  #include <string.h>
  #include <Arduino.h>
  #include "Adafruit_BLE.h"
  #include "Adafruit_BluefruitLE_SPI.h"
  #include "Adafruit_BluefruitLE_UART.h"
  
  #include "BluefruitConfig.h"
  
  #if SOFTWARE_SERIAL_AVAILABLE
    #include <SoftwareSerial.h>
  #endif

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

/*=========================================================================*/
// == Global DEFINES == //
/*=========================================================================*/
  
  // App Details
  //------------
  #define FIRMWARE_VERSION                        "v0.1"
  #define FIRMWARE_NAME                           "HumidiFire"
  #define BLE_DEV_NAME_PREFIX                     "HumidiFire-" // 16 char max

  // Pin Defines
  //-------------
  #define PIN_CS                                10  // output - Chip Select for SD Card
  
  #define PIN_BLOWER                            5   // output - PWM control MOSFET to Heating Element A
  #define PIN_MISTER                            6   // output - PWM control MOSFET to Heating Element B
  #define PIN_CHE                               9   // output - PWM control MOSFET to Heating Element C

  // use IDE pin number for Analog Pins for Feather M0 to use pin name as argument in function
  #define PIN_ATS                               A1   // A1 input - AnalogRead Thermistor A
  #define PIN_BTS                               A2   // A2 input - AnalogRead Thermistor B
  #define PIN_CTS                               A3   // A3 input - AnalogRead Thermistor C
  #define PIN_DHT                               A4   // A4 input - DigitalRead DHT11

  #define PIN_ONBOARD_LED                       13   // Built-in LED pin

  // Sensor Settings
  //-------------
  #define DHTTYPE                               DHT11     // Tells Adafruit DHTXX Library which type of DHT sensor to use
  
  #define TS_SERIES_RESISTOR                    10000         //(Ohm) value of 1% series resistors used for NTC Thermistors
  #define TS_NOMINAL_RESISTANCE                 (10000+1200)     // (Ohm) nominal resistance of NTC Thermistor at 25 degrees C + resitance of wire
  #define TS_NOMINAL_TEMP                       25        // (deg C) temperature for nominal resistance value (almost always 25 C)
  #define TS_NUM_SAMPLES                        5         // (samples) number of samples to average thermistor readings over
  #define TS_BETA_COEFFICIENT                   3950      // (beta) The beta coefficient of the thermistor (usually 3000-4000)

  // Serial Settings
  //-------------
  #define SERIAL_BAUD_RATE                      115200    // (bps)

  // Heater Settings
  //-------------
  #define PWM_DEFAULT_PERCENT                   0.50      // (%) default PWM duty cycle
  #define PWM_INCREMENT                         0.05      // (%) controller increments
  #define PWM_MAX                               1.00      // (%) Max PWM duty cycle % allowed in software
  #define PWM_MIN                               0.00      // (%) Min PWM duty cycle % allowed in software

  #define DURATION_DEFAULT_SEC                  15        // (sec) initial heating cycle duration
  #define DURATION_INCREMENT                    1         // (sec) increment/decrement interval
  #define DURATION_MAX                          30        // (sec) Max heater "on time" duration
  #define DURATION_MIN                          0         // (sec) Min heater "on time" duration

  #define OFF_TIME_DEFAULT_SEC                  3         // (sec) Default off time between heater cycles


/*=========================================================================*/
// == Declare Global Variables == //
/*=========================================================================*/
  
  // Define constants and variables for time-related actions and assign them values
  int onDurationSec = DURATION_DEFAULT_SEC;                     // Variable value for time period, in seconds, between the HE on state and the HE off state, set to a value of 10 for this version of the personal tuning strategy
  int offDurationSec = OFF_TIME_DEFAULT_SEC;                     // Variable value for time period, in seconds, which suspends the program after the HE has been turned off for increased energy efficiency, set to a value of 1 for this version of the personal tuning strategy
    
  // Define constants and variables for voltage management and assign them values
  float pwmDutyCyclePercent = PWM_DEFAULT_PERCENT; // Variable value for pulse width modulation (PWM) duty cycle for root mean square (RMS) voltage value when the HE is on, set to zero as its initial value

  int heaterCycle = 1;                        //variable to keep remember which of the heaters is running in the cycle  
  uint32_t timestamp = 0;                     //keeps the start time of heating cycles
  
  //char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

  extern uint8_t packetbuffer[];          // the packet buffer for Bluetooth communication

  // Controls Verbose "Print" Output to Serial Terminal
  const bool debugSet = false;
  //const bool debugSet = true;
  
  bool ledState = true;           // var to toggle onboard LED state

/*=========================================================================*/
// == Global Objects == //
/*=========================================================================*/
 
  RTC_PCF8523 rtc;              // real time clock object
  File dataFile;                // SD Card File Object 
  DHT dht(PIN_DHT, DHTTYPE);    // DHT11 Temperature and Humidity Sensor Object

  // Create the bluefruit object, either software serial...uncomment these lines
  /*
  SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);
  
  Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                        BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
  */
  
  /* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
  // Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);
  
  /* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
  Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
  
  /* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
  //Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
  //                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
  //                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

  
/*=========================================================================*/
// == Functions == //
/*=========================================================================*/

  double Fahrenheit(double celsius)       // Not used
  //Celsius to Fahrenheit conversion
  {
      return 1.8 * celsius + 32;
  }

  float getThermistorTemperatureF(int pin, int numSamples)
  // Take a series of analog Thermistor readings from pin
  // Average the samples and convert the value to Temperature in F
  // Based on code from https://learn.adafruit.com/thermistor?view=all#converting-to-temperature-3-13
  {
      int samples[numSamples];                    //int array for holding TS samples
      float avg = 0.00;
  
      for (int i=0; i < numSamples; i++)          // take N samples in a row
      {
          samples[i] = analogRead(pin);           // read pin value
          avg += samples[i];                      // add to avg summing       
          delay(10);                              // small delay between sensor readings
      }
      avg /= numSamples;                      // average out sum of sample readings
      
      // convert the value to resistance
      avg = 1023 / avg - 1;
      avg = TS_SERIES_RESISTOR / avg;
  
      // convert to temperature
      float steinhart;
      steinhart = avg / TS_NOMINAL_RESISTANCE;        // (R/Ro)
      steinhart = log(steinhart);                     // ln(R/Ro)
      steinhart /= TS_BETA_COEFFICIENT;               // 1/B * ln(R/Ro)
      steinhart += 1.0 / (TS_NOMINAL_TEMP + 273.15);  // + (1/To)
      steinhart = 1.0 / steinhart;                    // Invert
      steinhart -= 273.15;                            // convert to C from Kelvin
      steinhart = 1.8 * steinhart + 32;               // convert to F
  
      return steinhart;
      
  } // END getThermistorTemperatureF()

  // A small helper function for Bluetooth
  void error(const __FlashStringHelper*err) {
    Serial.println(err);
    while (1);
  }
  
  // function prototypes over in packetparser.cpp
  uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
  float parsefloat(uint8_t *buffer);
  void printHex(const uint8_t * data, const uint32_t numBytes);


//=========================================================================//
// == Setup == //
//=========================================================================//

  void setup()
  {
    // Only used to ensure app does not start printing to Serial Mon until a connection to the IDE is present
    // This will "hang" code when not connected to a PC
    //while(!Serial);     // used for testing only, remove for production code
    
    Wire.begin();
    dht.begin();
    Serial.begin(SERIAL_BAUD_RATE);               // Sets the mode of communication between the CPU and the computer or other device to the serial value     
    delay(500);                                   // Short delay to allow Serial Port to open
    
    // ------------------------------- //
    // -- Setup I/O Pins -- //
    // ------------------------------- //
    pinMode(PIN_AHE, OUTPUT);                    // Sets the PIN_AHE CPU pin to handle outgoing communications
    pinMode(PIN_BHE, OUTPUT);                    // Sets the PIN_BHE CPU pin to handle outgoing communications
    pinMode(PIN_CHE, OUTPUT);                    // Sets the PIN_CHE CPU pin to handle outgoing communications

    pinMode(PIN_ATS, INPUT);
    pinMode(PIN_BTS, INPUT);
    pinMode(PIN_CTS, INPUT);
    pinMode(PIN_DHT, INPUT);

    pinMode(PIN_CS, OUTPUT);                    // Chip Select pin for the SD card
    pinMode(PIN_ONBOARD_LED, OUTPUT);           // Onboard indicator LED

    // Configure the reference voltage used for analog input (the value used as the top of the input range) 
    //so the voltage applied to pin AREF (5V) is used as top of analog read range
    //https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
    //analogReference(EXTERNAL);    // Command used for AVR-based boards
    analogReference(AR_EXTERNAL);   // Command used for SAMD-based boards (e.g. Feather M0)    
    
    // ------------------------------- //
    // -- Initialize Heating Elements to OFF -- //
    // ------------------------------- //   
    analogWrite(PIN_AHE, 0);  // PWM = 0, off
    analogWrite(PIN_BHE, 0);  // PWM = 0, off        
    analogWrite(PIN_CHE, 0);  // PWM = 0, off
     
    // ------------------------------- //
    // -- Setup RTC -- //
    // ------------------------------- //
    if (!rtc.begin()) 
    {
      Serial.println("Couldn't find RTC");
      //while (1);
    }
  
    if (!rtc.initialized()) 
    {
      Serial.println("RTC is NOT running!");
      // the following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }    
    // ------- END RTC Setup


    // ------------------------------- //
    // -- Setup SD Card -- //
    // ------------------------------- //
    if(debugSet)
    {
      Serial.println("Initializing SD card...");
    }
     
    // See if the card is present and can be initialized:
    if (!SD.begin(PIN_CS)) 
    {
      Serial.println("Card failed, or not present");
      //while (1); // don't do anything more
    }
    else // SD Card is present...
    {      
      if(debugSet)
      {
        Serial.println("SD card initialized.");
        Serial.println();
      }

      // Open up the file we're going to log to!
      dataFile = SD.open("datalog.txt", FILE_WRITE);
      
      if (!dataFile)  // dataFile failed
      {
        Serial.println("error opening datalog.txt");    
        //while (1) ; // Wait forever since we can't write data
      }

    }
    // ------- END SD Card Setup
    
    // ------------------------------- //
    // -- Setup BLE module -- //
    // ------------------------------- //
    if(debugSet)
    {
      Serial.print(F("Initializing BLE module... "));
    }
  
    if ( !ble.begin(VERBOSE_MODE) )
    {
      error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
    }
    if(debugSet)
    {
      Serial.println( F("OK!") );
    }
  
    //Turn off Verbose mode
    ble.verbose(false);
    //ble.verbose(true);
    
    // Disable command echo from Bluefruit to Serial Monitor
    ble.echo(false);
    //ble.echo(true);
  
    // Perform a factory reset to make sure everything is in a known state at start up
    if ( FACTORYRESET_ENABLE )
    {
      if(debugSet)
      {
        Serial.print(F("Performing factory reset on BLE module... "));
      }
      
      if ( ! ble.factoryReset() )
      {
        error(F("ERROR: Factory reset failed"));
      }
      
      if(debugSet)
      {
        Serial.println( F("OK!") );
      }
    } // END BLE Factory Reset Routine
  
    // Print Bluefruit device information
    //Serial.println("\n\rRequesting BLE Module device info... "); 
    //ble.info();
  
    // Set BLE Device Unique Name 
    // NOTE: Device must be power cycled and BLE App must make initial connection before name updates in App
    
    const int uidLength = 4;         // number of serial number characters to use
    char uidString[uidLength+1];  // add 1 char for string terminal
    uidString[uidLength] = '\n';  // terminal Serial String
    
    ble.println(F("ATI"));  // print BLE device info 
  
    // read back BLE device info line by line
    // the 3rd line is the serial number, store that to local string
    char serialString[128];
    for(int i=0; i < sizeof(serialString); i++){serialString[i]='\n';}
    
    int count = 0;
    while ( ble.readline() ) 
    {
      if ( !strcmp(ble.buffer, "OK") || !strcmp(ble.buffer, "ERROR")  ) break;
      count++;
      if(count == 3)
      {
        strcpy(serialString, ble.buffer);   // copy BLE buffer to serial number string
      }
    }
  
    //Serial.print("serialString = ");
    //Serial.println(serialString);
  
    // Use the end of the serial number as the unique ID
    int index = 0;
    for( int i=0; i < sizeof(serialString); i++)
    {
      if(serialString[i] == '\n')   // find the end of the serial number 
      {
        index = i - uidLength - 1;
        break;
      }
    }
  
    // Set the Unique ID string reported to phone via BLE
    for(int i=0; i <= uidLength; i++)
    {
      uidString[i] = serialString[index+i];
    }
  
    //Serial.print("uidString = ");
    //Serial.println(uidString);
      
    char devNameCmd[40];
    for(int i=0; i < sizeof(devNameCmd);i++){devNameCmd[i]='\n';}
    
    strcpy(devNameCmd, "AT+GAPDEVNAME=");
    strcat(devNameCmd, BLE_DEV_NAME_PREFIX);
    strcat(devNameCmd, uidString);
    //Serial.println(devNameCmd);
    if (!ble.sendCommandCheckOK(devNameCmd) ) 
    {
      error(F("ERROR: Set device name failed"));
    }
  
    if(debugSet)
    {
      Serial.print(F("\n\rBLE Device Name: "));
      Serial.print(BLE_DEV_NAME_PREFIX);
      Serial.println(uidString);
      Serial.print("\n\r");
    }
    
    // LED Activity command is only supported from 0.6.6
    if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
    {
      // Change Mode LED Activity
      //gSerial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
      ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    }
  
    // Switch to DATA mode to have a better throughput
    // "Verbose Mode" debug info is a little annoying after this point!
    //Serial.println("Switch to DATA mode to have a better throughput...");
    ble.setMode(BLUEFRUIT_MODE_DATA);
    //ble.setMode(BLUEFRUIT_MODE_COMMAND); 

    // ------- END BLE MODULE Setup

    // ------------------------------- //
    // -- Build Data Log Header String -- //
    // ------------------------------- //

    String headerString = "";
    
    headerString += "Firmware:\t";
    headerString += FIRMWARE_NAME;
    headerString += " ";
    headerString += FIRMWARE_VERSION;
    headerString += "\n\r";
    
    headerString += "BLE Dev ID:\t";
    headerString += BLE_DEV_NAME_PREFIX;
    headerString += uidString;
    headerString += "\n\r\n\r";

    headerString += "Timestamp, ";
    headerString += "Ambient Temp (degF), ";
    headerString += "Ambient Humid (%), ";
    headerString += "HE-A Temp (degF), ";
    headerString += "HE-B Temp (degF), ";
    headerString += "HE-C Temp (degF), ";
    headerString += "HE-A Temp (degF), ";
    headerString += "Power (0.00-1.00), ";
    headerString += "Duration (sec)";
    headerString += "\n\r\n\r";

    Serial.print(headerString);     // print header to Serial Mon
    
    dataFile.print(headerString);   // print header to data log file
    dataFile.flush();               // save data log file

    // ------- END Build Data Log Header String
    
  } // END SETUP

/*=========================================================================*/
// == Loop == //
/*=========================================================================*/

  void loop()
  {
    digitalWrite(PIN_ONBOARD_LED, ledState);        // shows that code has gotten this far by lighting LED

    // ------------------------------- //
    // -- Handle Incoming Bluetooth Control Pad Packets -- //
    // ------------------------------- //
    uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);

    // Bluefruit Control Pad Module Buttons
    // see https://learn.adafruit.com/bluefruit-le-connect?view=all#control-pad-8-11
    
    if (packetbuffer[1] == 'B') 
    {
      uint8_t buttnum = packetbuffer[2] - '0';
      boolean pressed = packetbuffer[3] - '0';

      if((buttnum == 5) && (pressed == 1))   //When 'UP' button is pressed
      {
        pwmDutyCyclePercent += PWM_INCREMENT;
        if(pwmDutyCyclePercent > PWM_MAX){pwmDutyCyclePercent = PWM_MAX;} //duty cycle cannot be more than PWM_MAX

        //Toggle Blink Indicator LED
        digitalWrite(PIN_ONBOARD_LED, !ledState);
        delay(50);
        digitalWrite(PIN_ONBOARD_LED, ledState);

        //Serial.print ("Button ");
        //Serial.print(buttnum);
        //Serial.print (" pressed, ");
        //Serial.print("Duty Cyle: ");
        //Serial.println(pwmDutyCyclePercent);
        
      } // END "Up" Button
      
      if((buttnum == 6) && (pressed == 1))   //When 'DOWN' button is pressed
      {
        pwmDutyCyclePercent -= PWM_INCREMENT;
        if(pwmDutyCyclePercent < PWM_MIN){pwmDutyCyclePercent = PWM_MIN;} //duty cycle cannot be less than PWM_MIN
        
        //Toggle Blink Indicator LED
        digitalWrite(PIN_ONBOARD_LED, !ledState);
        delay(50);
        digitalWrite(PIN_ONBOARD_LED, ledState);

        //Serial.print ("Button ");
        //Serial.print(buttnum);
        //Serial.print (" pressed, ");
        //Serial.print("Duty Cyle: ");
        //Serial.println(pwmDutyCyclePercent);
        
      } // END "Down" Button

      if((buttnum == 7) && (pressed == 1))   //When 'LEFT' button is pressed
      {
        onDurationSec -= DURATION_INCREMENT;
        if(onDurationSec < DURATION_MIN){onDurationSec = DURATION_MIN;} //on duration cannot be less than DURATION_MIN
        
        //Toggle Blink Indicator LED
        digitalWrite(PIN_ONBOARD_LED, !ledState);
        delay(50);
        digitalWrite(PIN_ONBOARD_LED, ledState);
        
        //Serial.print ("Button ");
        //Serial.print(buttnum);
        //Serial.print (" pressed, ");
        //Serial.print("Duration: ");
        //Serial.println(onDurationSec);
        
      }  // END "Left" Button
      
      if((buttnum == 8) && (pressed == 1))   //When 'RIGHT' button is pressed
      {
        onDurationSec += DURATION_INCREMENT;
        if(onDurationSec > DURATION_MAX){onDurationSec = DURATION_MAX;} // on duration cannot be more than DURATION_MAX

        //Toggle Blink Indicator LED
        digitalWrite(PIN_ONBOARD_LED, !ledState);
        delay(50);
        digitalWrite(PIN_ONBOARD_LED, ledState);
        
        //Serial.print ("Button ");
        //Serial.print(buttnum);
        //Serial.print (" pressed, ");
        //Serial.print("Duration: ");
        //Serial.println(onDurationSec);
        
      } // END "Right" Button
      

      // Update Control Pad Display
      // Whever a BLE Control Pad button is pressed, print updated values to the Control Pad UART display
      // Do this at the END of the button update routine
      if(pressed == 1)
      {
        ble.println();  // Clear the previous 2 lines from the screen
        ble.println();  // Clear the previous 2 lines from the screen
        
        ble.print("Power: \t\t");
        ble.print(int(pwmDutyCyclePercent*100));
        ble.print(" %");

        ble.println();

        ble.print("Duration: \t");
        ble.print(onDurationSec);
        ble.print(" s");
        
      } // End Update Control Pad Display UART
    
    } // END Button press 'B' packet handling
    
    // ------------------------------- //
    // -- Get RTC Time -- //
    // ------------------------------- //  
    DateTime now = rtc.now();   // save current timestamp to var "now"


    // ------------------------------- //
    // -- Get DHT11 Temp & Humidity Readings -- //
    // ------------------------------- //
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float dhtTempF = dht.readTemperature(true);                       // Read temperature as Fahrenheit (isFahrenheit = true)
    float dhtHum = dht.readHumidity();                                // Read Humidity %
    float dhtHeatIndexF = dht.computeHeatIndex(dhtTempF, dhtHum);     // Compute heat index in Fahrenheit (the default)


    // ------------------------------- //
    // -- Get Thermistor Temperature Readings -- //
    // ------------------------------- //    
    float atsTempF = getThermistorTemperatureF(PIN_ATS, TS_NUM_SAMPLES);      
    float btsTempF = getThermistorTemperatureF(PIN_BTS, TS_NUM_SAMPLES);
    float ctsTempF = getThermistorTemperatureF(PIN_CTS, TS_NUM_SAMPLES);


    // ------------------------------- //
    // -- Power Heating Elements -- //
    // ------------------------------- //

    // No longer needed. Duty Cycle controlled by Control Pad
    //pwmDutyCyclePercent = pow((voltsRmsTarget/vcc), 2);      // Creates percentage value for duty cycle 
  
    //replaced delay()s with timers to allow other parts of the code to run outside of the heater cycle
    switch (heaterCycle) 
    {
      case 1: //AHE ON
        analogWrite(PIN_AHE, uint8_t(pwmDutyCyclePercent*255));  // Turn on heater at PWM duty cycle rate
        if( ((millis() - timestamp) >= (onDurationSec * 1000) )) // when duration is expired...
        {
          heaterCycle = 2;                                       // go to next cycle state
          timestamp = millis();
        }
        break;
        
      case 2: //AHE OFF
        analogWrite(PIN_AHE, 0);                                  // Turn Heater Off
        if( ((millis() - timestamp) >= (offDurationSec * 1000) )) // when duration is expired...
        {
          //heaterCycle = 1;                                      // test code to stay on heater A
          heaterCycle = 3;                                       // go to next cycle state                                        
          timestamp = millis();
        }
        break;
        
      case 3: //BHE ON
        analogWrite(PIN_BHE, uint8_t(pwmDutyCyclePercent*255));  // Turn on heater at PWM duty cycle rate
        if( ((millis() - timestamp) >= (onDurationSec * 1000) )) // when duration is expired...
        {
          heaterCycle = 4;                                       // go to next cycle state
          timestamp = millis();
        }
        break;
        
      case 4: //BHE OFF
        analogWrite(PIN_BHE, 0);                                  // Turn Heater Off
        if( ((millis() - timestamp) >= (offDurationSec * 1000) )) // when duration is expired...
        {
          heaterCycle = 5;                                       // go to next cycle state
          timestamp = millis();
        }
        break;
        
      case 5: //CHE ON
        analogWrite(PIN_CHE, uint8_t(pwmDutyCyclePercent*255));  // Turn on heater at PWM duty cycle rate
        if( ((millis() - timestamp) >= (onDurationSec * 1000) )) // when duration is expired...
        {
          heaterCycle = 6;                                       // go to next cycle state
          timestamp = millis();
        }
        break;
        
      case 6: //CHE OFF
        analogWrite(PIN_CHE, 0);                                  // Turn Heater Off
        if( ((millis() - timestamp) >= (offDurationSec * 1000) )) // when duration is expired...
        {
          heaterCycle = 1;                                       // go to begining cycle state
          timestamp = millis();
        }
        break;
        
      default:
        // add default case here (optional)
        break;
    }

   /* //OLD delay-based Heater Code
    // Operate Heating Element A                    
    analogWrite(PIN_AHE, uint8_t(pwmDutyCyclePercent*255));         // Set PWM rate based on PWM Duty Cycle Percent
    delay (onDurationSec * 1000);                            // Leave Heater ON for duration
    analogWrite(PIN_AHE, 0);                                 // Turn Heater Off
    delay(offDurationSec * 1000);                            // Leave Heater OFF for duration
    
    // Operate Heating Element B                    
    analogWrite(PIN_BHE, uint8_t(pwmDutyCyclePercent*255));         // Set PWM rate based on PWM Duty Cycle Percent
    delay (onDurationSec * 1000);                            // Leave Heater ON for duration
    analogWrite(PIN_BHE, 0);                                 // Turn Heater Off
    delay(offDurationSec * 1000);                            // Leave Heater OFF for duration

    // Operate Heating Element C                      
    analogWrite(PIN_CHE, uint8_t(pwmDutyCyclePercent*255));         // Set PWM rate based on PWM Duty Cycle Percent
    delay (onDurationSec * 1000);                            // Leave Heater ON for duration
    analogWrite(PIN_CHE, 0);                                 // Turn Heater Off
    delay(offDurationSec * 1000);                            // Leave Heater OFF for duration
    */
           

    // ------------------------------- //
    // -- Build Output String -- //
    // ------------------------------- //
    // Output string will be used as row in data log
    
    String outputString = "";      // Clear / Initialize outputString
   
    // Timestamp String
    outputString +=  (now.year());
    outputString += ('/');
     
    if (now.month() < 10) {outputString += (0);}    // Add a zero pad if less than 10
    outputString += (now.month());
    outputString += ('/');
    
    if (now.day() < 10) {outputString +=  (0);}    // Add a zero pad if less than 10
    outputString += (now.day());
    outputString += (' ');

    if (now.hour() < 10) {outputString +=  (0);}    // Add a zero pad if less than 10
    outputString +=  (now.hour());
    outputString += (":");
    
    if (now.minute() < 10) {outputString += (0);}    // Add a zero pad if less than 10
    outputString += (now.minute());
    outputString += (":");
    
    if (now.second() < 10) {outputString += (0);}    // Add a zero pad if less than 10
    outputString += (now.second());

    outputString +=  ", ";

    // DHT String
    outputString += String(dhtTempF,2);  //use 2 decimal places
    outputString += ", ";
    outputString += String(dhtHum,2);    //use 2 decimal places
    outputString += ", ";

    // Thermistor String
    outputString += String(atsTempF,2);  //use 2 decimal places
    outputString += ", ";
    outputString += String(btsTempF,2);  //use 2 decimal places
    outputString += ", ";
    outputString += String(ctsTempF,2);  //use 2 decimal places
    outputString += ", ";

    // Heating Element String
    outputString += String(pwmDutyCyclePercent,2);    //use 2 decimal places
    outputString += ", ";
    outputString += String(onDurationSec);    //use 2 decimal places

    // End with New Line
    outputString += "\n\r";
 
    Serial.print(outputString);
    dataFile.print(outputString);

    // The following line will 'save' the file to the SD card after every
    // line of data - this will use more power and slow down how much data
    // you can read but it's safer! 
    // If you want to speed up the system, remove the call to flush() and it
    // will save the file only every 512 bytes - every time a sector on the 
    // SD card is filled with data.  
    dataFile.flush();
        
  } // END LOOP

