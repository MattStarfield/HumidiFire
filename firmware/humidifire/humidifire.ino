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
  #define PIN_ONBOARD_LED                       13   // Built-in LED pin
  #define PIN_CS                                10  // output - Chip Select for SD Card
  
  #define PIN_FAN                               5   // output - PWM control MOSFET to Blower Fan
  #define PIN_MIST                              6   // output - PWM control MOSFET to Mister / Bubbler

  // Sensor Settings
  //-------------
  //#define DHTTYPE                               DHT11     // Tells Adafruit DHTXX Library which type of DHT sensor to use
  
  // Serial Settings
  //-------------
  #define SERIAL_BAUD_RATE                      115200    // (bps)

  // PWM Settings
  //-------------
  #define PWM_DEFAULT_PERCENT                   0.50      // (%) default PWM duty cycle
  #define PWM_INCREMENT                         0.004      // (%) controller increments
  #define PWM_MAX                               1.00      // (%) Max PWM duty cycle % allowed in software
  #define PWM_MIN                               0.00      // (%) Min PWM duty cycle % allowed in software


/*=========================================================================*/
// == Declare Global Variables == //
/*=========================================================================*/
  extern uint8_t packetbuffer[];          // the packet buffer for Bluetooth communication

  // Controls Verbose "Print" Output to Serial Terminal
  const bool debugSet = false;
  //const bool debugSet = true;
  
  bool ledState = true;           // var to toggle onboard LED state

  enum Scenes 
  {
    OFF = -1,     // Sytem off, standby
    
    CAMPFIRE,     // Orange, Red, & Yellow; Crackling fire
    SEASHORE,     // Blue, Green, & White; Ocean waves
    JUNGLE,       // Green, Yellow, White; Tropical Rainforest
    
    NUM_SCENES    // Keep in LAST place - translates to the number of scenes in the list
  } scene;
 
  //Scenes scene = OFF;  // Use declared enum class to create Global Var to store scene. Initialize to OFF

  float fanPwmPct;     // Percent Duty Cycle of PWM signal, translates to blower fan speed
  float mistPwmPct;    // Percent Duty Cycle of PWM signal, translates to mister intensity

/*=========================================================================*/
// == Global Objects == //
/*=========================================================================*/
 
  //File dataFile;                // SD Card File Object 
  //DHT dht(PIN_DHT, DHTTYPE);    // DHT11 Temperature and Humidity Sensor Object

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
    //dht.begin();
    Serial.begin(SERIAL_BAUD_RATE);               // Sets the mode of communication between the CPU and the computer or other device to the serial value     
    delay(500);                                   // Short delay to allow Serial Port to open

    scene = OFF;                                  // initialize scene
    
    // ------------------------------- //
    // -- Setup I/O Pins -- //
    // ------------------------------- //
    pinMode(PIN_CS, OUTPUT);                    // Chip Select pin for the SD card
    pinMode(PIN_ONBOARD_LED, OUTPUT);           // Onboard indicator LED

    pinMode(PIN_FAN, OUTPUT);                   // 
    pinMode(PIN_MIST, OUTPUT);                  // 

    // Configure the reference voltage used for analog input (the value used as the top of the input range) 
    //so the voltage applied to pin AREF (5V) is used as top of analog read range
    //https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
    //analogReference(EXTERNAL);    // Command used for AVR-based boards
    analogReference(AR_EXTERNAL);   // Command used for SAMD-based boards (e.g. Feather M0)    
    
    // ------------------------------- //
    // -- Initialize Devices -- //
    // ------------------------------- //
    fanPwmPct   = 0.0;    // Percent Duty Cycle of PWM signal, translates to blower fan speed
    mistPwmPct  = 0.0;    // Percent Duty Cycle of PWM signal, translates to mister intensity
    
    analogWrite(PIN_FAN, fanPwmPct*255);      // PWM = 0, initialize Fan off
    analogWrite(PIN_MIST, mistPwmPct*255);    // PWM = 0, initialize Mister off


    /*
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
    */
    
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

      // -- SCENE CONTROL 1, 2, 3, 4 -- //
      
        if((buttnum == 1) && (pressed == 1))   //When '1' button is pressed
        {

          //Toggle Blink Indicator LED
          digitalWrite(PIN_ONBOARD_LED, !ledState);
          delay(50);
          digitalWrite(PIN_ONBOARD_LED, ledState);

          if(scene == CAMPFIRE)
          {
            scene = OFF;        // toggle scene on/off
            fanPwmPct = 0.0;
            mistPwmPct = 0.0;
          }
          else
          {
            scene = CAMPFIRE;
            fanPwmPct = 0.50;
            mistPwmPct = 1.00;
          }
  
          Serial.print ("Button ");
          Serial.print(buttnum);
          Serial.print (" pressed, ");
          Serial.print("Scene: ");
          Serial.println(scene);
          
        } // END '1' Button

      // -- END SCENE CONTROL 1, 2, 3, 4 -- //

      
      
      // -- FAN CONTROL ^ v -- //
      
        if((buttnum == 5) && (pressed == 1))   //When 'UP' button is pressed
        {
          fanPwmPct += PWM_INCREMENT;
          if(fanPwmPct > PWM_MAX){fanPwmPct = PWM_MAX;} //duty cycle selection cannot be allowed to increment beyond boundaries
  
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
          fanPwmPct -= PWM_INCREMENT;
          if(fanPwmPct < PWM_MIN){fanPwmPct = PWM_MIN;} //duty cycle selection cannot be allowed to increment beyond boundaries
          
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

      // -- END FAN CONTROL ^ v -- //


      // -- MIST CONTROL -- < > //
      
        if((buttnum == 8) && (pressed == 1))   //When 'RIGHT' button is pressed
        {
          mistPwmPct += PWM_INCREMENT;
          if(mistPwmPct > PWM_MAX){mistPwmPct = PWM_MAX;} // on duration cannot be more than DURATION_MAX
  
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
        
        if((buttnum == 7) && (pressed == 1))   //When 'LEFT' button is pressed
        {
          mistPwmPct -= PWM_INCREMENT;
          if(mistPwmPct < PWM_MIN){mistPwmPct = PWM_MIN;} //on duration cannot be less than DURATION_MIN
          
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
      
      // -- END MIST CONTROL < > -- //
         

      // Update Control Pad Display
      // Whever a BLE Control Pad button is pressed, print updated values to the Control Pad UART display
      // Do this at the END of the button update routine
      if(pressed == 1)
      {
        ble.println();  // Clear the previous 2 lines from the screen
        ble.println();  // Clear the previous 2 lines from the screen
        
        ble.print("Scene : ");
        ble.print(scene);

        ble.println();
        
        ble.print("Fan : ");
        ble.print(int(fanPwmPct*100));
        ble.print(" %\t\t");

        ble.print("Mist: ");
        ble.print(int(mistPwmPct*100));
        ble.print(" %");
        
      } // End Update Control Pad Display UART
    
    } // END Button press 'B' packet handling

    /*
    // ------------------------------- //
    // -- Get DHT11 Temp & Humidity Readings -- //
    // ------------------------------- //
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float dhtTempF = dht.readTemperature(true);                       // Read temperature as Fahrenheit (isFahrenheit = true)
    float dhtHum = dht.readHumidity();                                // Read Humidity %
    float dhtHeatIndexF = dht.computeHeatIndex(dhtTempF, dhtHum);     // Compute heat index in Fahrenheit (the default)
    */

    // ------------------------------- //
    // -- Run Scene Operations -- //
    // ------------------------------- //   

    switch (scene) 
    {
      case OFF:
        // This prevents interface from adjusting fan & mist settings in OFF mode
        fanPwmPct = 0.0;
        mistPwmPct = 0.0;
        
        analogWrite(PIN_FAN, uint8_t(fanPwmPct*255));
        analogWrite(PIN_MIST, uint8_t(mistPwmPct*255));

        break;
        
      case CAMPFIRE:
        analogWrite(PIN_FAN, uint8_t(fanPwmPct*255));
        analogWrite(PIN_MIST, uint8_t(mistPwmPct*255));

        break;
        
      case SEASHORE:

        break;
        
      case JUNGLE:

        break;
             
      default:
        // add default case here (optional)
        break;
    }

 
        
  } // END LOOP

