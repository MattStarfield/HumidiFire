/* 
 *  HumidiFire Firmware
 *  by Matt Garfield

 * TODO:
 * ----------
 * - add r89m Buttons library
 *    - add "click encoder button" to increment / toggle mode (BLE not required anymore)
 *    
 * - add modeToString() function to print mode names
 * 
 * - add 'released' button variables to BLE App button function 
 * 
 * - make rotary encoder fan speed control more fluid / intuitive
 *    - percieved fan speed is not linaerly proportional to PWM duty cycle
 *    - most variation is in 0 - 127 range
 *    - lookup table approach? maybe just 4 speed settings?
 * 
 * CHANGELOG:
 * ----------
 * 11/10/2019
 * - Rotary Encoder fan speed control
 * - Bluefruit Connect App fan, mist, and mode control
 * 
 * 11/9/2019
 * - Initial setup
*/

/*=========================================================================*/
// == Debug Set == //
/*=========================================================================*/

  // Controls Verbose "Print" Output to Serial Terminal
  const bool debugSet = false;
  //const bool debugSet = true;
  
/*=========================================================================*/
// == Include Libraries == //
/*=========================================================================*/
  
  // Bluefruit Libraries
  // Library: Adafruit_BluefruitLE_nRF51
  #include <Wire.h>
  //#include <SPI.h>
  
  #include <string.h>
  #include <Arduino.h>
  #include "Adafruit_BLE.h"
  #include "Adafruit_BluefruitLE_SPI.h"
  #include "Adafruit_BluefruitLE_UART.h"
  
  // Config Files
  #include "BluefruitConfig.h"
  //#include "BluefruitSetup.ino"
  
  #include <Encoder.h>      // http://www.pjrc.com/teensy/td_libs_Encoder.html     
  
  
  //#include <SD.h>         //included in Arduino IDE core
  
  //#include <DHT.h>        // Updated to using Adafruit DHT Library https://github.com/adafruit/DHT-sensor-library
                          // Requires Adafruit Libraries "DHT_sensor_library" and "Adafruit_Unified_Sensor"
                          // Setup Instructions here: https://learn.adafruit.com/dht/using-a-dhtxx-sensor
  

  
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
  // NOTE: See BluefruitConfig.h for other pin definitions
  
  #define PIN_ONBOARD_LED                       13   // Built-in LED pin
  #define PIN_CS                                10  // output - Chip Select for SD Card
  
  #define PIN_FAN                               5   // output - PWM control MOSFET to Blower Fan
  #define PIN_MIST                              6   // output - PWM control MOSFET to Mister / Bubbler

  #define PIN_ENCODER_A                         A2  // Labeled "CLK" pin on Encoder board (uses interrupt)
  #define PIN_ENCODER_B                         A1  // Labeled "DT" pin on Encoder board  (uses interrupt)
  #define PIN_ENCODER_SW                        A0  // Labeled "SW" pin on Encoder board

  
  // Bluefruit Connect App Buttons
  // - Numerical designators of buttons in the Control Pad
  //-------------
  #define APP_BTN_1                             1   // 
  #define APP_BTN_2                             2   // 
  #define APP_BTN_3                             3   // 
  #define APP_BTN_4                             4   // 
  #define APP_BTN_UP                            5   // 
  #define APP_BTN_DOWN                          6   // 
  #define APP_BTN_LEFT                          7   // 
  #define APP_BTN_RIGHT                         8   // 
  
  
  // Sensor Settings
  //-------------
  //#define DHTTYPE                               DHT11     // Tells Adafruit DHTXX Library which type of DHT sensor to use
  
  // Serial Settings
  //-------------
  #define SERIAL_BAUD_RATE                      115200    // (bps)

  // PWM Settings
  //-------------
  //#define PWM_DEFAULT_PERCENT                   0.50      // (%) default PWM duty cycle
  //#define PWM_INCREMENT                         0.004      // (%) controller increments
  //#define PWM_MAX                               1.00      // (%) Max PWM duty cycle % allowed in software
  //#define PWM_MIN                               0.00      // (%) Min PWM duty cycle % allowed in software

  #define PWM_DEFAULT                           127      // (0 - 255) default PWM duty cycle
  #define PWM_INCREMENT                         1        // (0 - 255) controller increments
  #define PWM_MAX                               255      // (0 - 255) Max PWM duty cycle allowed in software
  #define PWM_MIN                               0        // (0 - 255) Min PWM duty cycle allowed in software


/*=========================================================================*/
// == Declare Global Variables == //
/*=========================================================================*/
  extern uint8_t packetbuffer[];          // the packet buffer for Bluetooth communication
  
  bool ledState = true;           // var to toggle onboard LED state

  //enum Scenes
  enum Modes
  {
    OFF = -1,     // Sytem off, standby
    
    CAMPFIRE,     // Orange, Red, & Yellow; Crackling fire
    SEASHORE,     // Blue, Green, & White; Ocean waves
    TROPICS,       // Green, Yellow, White; Tropical Rainforest
    
    NUM_MODES    // Keep in LAST place - translates to the number of modes in the list (1-indexed)
    //NUM_SCENES    // Keep in LAST place - translates to the number of scenes in the list
    
  }; // END enum Modes
  //} scene;
 
  //Scenes scene;  // declare enum object
  Modes mode;      // declare enum object

  //float fanPwmPct;     // Percent Duty Cycle of PWM signal, translates to blower fan speed
  //float mistPwmPct;    // Percent Duty Cycle of PWM signal, translates to mister intensity

  byte fanPwm;         // PWM Duty Cycle 0 - 255
  byte mistPwm;        // PWM Duty Cycle 0 - 255

  //long previousEncoderPos;
  //long currentEncoderPos;

/*=========================================================================*/
// == Global Objects == //
/*=========================================================================*/
 
  //File dataFile;                // SD Card File Object 
  //DHT dht(PIN_DHT, DHTTYPE);    // DHT11 Temperature and Humidity Sensor Object

  // Change these two numbers to the pins connected to your encoder.
  //   Best Performance: both pins have interrupt capability
  //   Good Performance: only the first pin has interrupt capability
  //   Low Performance:  neither pin has interrupt capability
  //   avoid using pins with LEDs attached
  Encoder rotaryEncoder(PIN_ENCODER_B, PIN_ENCODER_A);


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
  
  // function prototypes in packetparser.cpp
  uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
  float parsefloat(uint8_t *buffer);
  void printHex(const uint8_t * data, const uint32_t numBytes);

  // function prototypes in BluefruitSetup.h
  //void bluefruitSetup();


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
    
    // ------------------------------- //
    // -- Setup I/O Pins -- //
    // ------------------------------- //
    pinMode(PIN_CS, OUTPUT);                    // Chip Select pin for the SD card
    pinMode(PIN_ONBOARD_LED, OUTPUT);           // Onboard indicator LED

    pinMode(PIN_FAN, OUTPUT);                   // 
    pinMode(PIN_MIST, OUTPUT);                  // 

    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);      //

    // Configure the reference voltage used for analog input (the value used as the top of the input range) 
    //so the voltage applied to pin AREF (5V) is used as top of analog read range
    //https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
    //analogReference(EXTERNAL);    // Command used for AVR-based boards
    analogReference(AR_EXTERNAL);   // Command used for SAMD-based boards (e.g. Feather M0)    
    
    // ------------------------------- //
    // -- Initialize Variables -- //
    // ------------------------------- //
    
    // Initialize devices
    //fanPwmPct   = 0.0;    // Percent Duty Cycle of PWM signal, translates to blower fan speed
    //mistPwmPct  = 0.0;    // Percent Duty Cycle of PWM signal, translates to mister intensity    
    //analogWrite(PIN_FAN, fanPwmPct*255);      // PWM = 0, initialize Fan off
    //analogWrite(PIN_MIST, mistPwmPct*255);    // PWM = 0, initialize Mister off

    fanPwm   = 0;                       // Duty Cycle of PWM signal, translates to blower fan speed
    mistPwm  = 0;                       // Duty Cycle of PWM signal, translates to mister intensity
    analogWrite(PIN_FAN, fanPwm);      // PWM = 0, initialize Fan off
    analogWrite(PIN_MIST, mistPwm);    // PWM = 0, initialize Mister off

    //scene = OFF;                                  // initialize scene
    //mode = CAMPFIRE;                                  // initialize mode to CAMPFIRE
    mode = OFF;                                  // initialize mode to OFF
    
    // Initialize Encoder
    rotaryEncoder.write(0);                      // Initialize Encoder Accumulator to 0
    
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
    bluefruitSetup();   // see BluefruitSetup.ino

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
      //boolean released;

      // -- MODE CONTROL 1, 2, 3, 4 -- //
      
        if((buttnum == APP_BTN_1) && (pressed == 1))   //When '1' button is pressed
        {

          //if(scene == CAMPFIRE)
          if(mode == CAMPFIRE)
          {
            //scene = OFF;        // toggle scene on/off
            //fanPwmPct = 0.0;
            //mistPwmPct = 0.0;

            mode = OFF;        // toggle mode on/off
            fanPwm = 0;
            mistPwm = 0;
          }
          else
          {
            //scene = CAMPFIRE;
            //fanPwmPct = 0.50;
            //mistPwmPct = 1.00;

            mode = CAMPFIRE;
            fanPwm = PWM_DEFAULT;
            mistPwm = PWM_MAX;
          }

          // Toggle Blink Indicator LED
          digitalWrite(PIN_ONBOARD_LED, !ledState);
          delay(50);
          digitalWrite(PIN_ONBOARD_LED, ledState);
  
          // Print
          //Serial.print ("Button ");
          //Serial.print(buttnum);
          //Serial.print (" pressed, ");
          //Serial.print("Scene: ");
          //Serial.println(scene);
          
        } // END '1' Button

      // -- END MODE CONTROL 1, 2, 3, 4 -- //

      
      
      // -- FAN CONTROL ^ v -- //
      
        if((buttnum == APP_BTN_UP) && (pressed == 1))   //When 'UP' button is pressed
        {
          //fanPwmPct += PWM_INCREMENT;
          //if(fanPwmPct > PWM_MAX){fanPwmPct = PWM_MAX;} //duty cycle selection cannot be allowed to increment beyond boundaries
        
          if((fanPwm + PWM_INCREMENT) >= PWM_MAX){fanPwm = PWM_MAX;} //duty cycle selection cannot be allowed to increment beyond boundaries
          else{fanPwm += PWM_INCREMENT;}
  
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
        
        if((buttnum == APP_BTN_DOWN) && (pressed == 1))   //When 'DOWN' button is pressed
        {
          //fanPwmPct -= PWM_INCREMENT;
          //if(fanPwmPct < PWM_MIN){fanPwmPct = PWM_MIN;} //duty cycle selection cannot be allowed to increment beyond boundaries
          
          if((fanPwm - PWM_INCREMENT) <= PWM_MIN){fanPwm = PWM_MIN;} //duty cycle selection cannot be allowed to increment beyond boundaries
          else{fanPwm -= PWM_INCREMENT;}
          
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
      
        if((buttnum == APP_BTN_RIGHT) && (pressed == 1))   //When 'RIGHT' button is pressed
        {
          //mistPwmPct += PWM_INCREMENT;
          //if(mistPwmPct > PWM_MAX){mistPwmPct = PWM_MAX;} //duty cycle selection cannot be allowed to increment beyond boundaries
          
          if((mistPwm + PWM_INCREMENT) >= PWM_MAX){mistPwm = PWM_MAX;} //duty cycle selection cannot be allowed to increment beyond boundaries
          else{mistPwm += PWM_INCREMENT;}
  
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
        
        if((buttnum == APP_BTN_LEFT) && (pressed == 1))   //When 'LEFT' button is pressed
        {
          //mistPwmPct -= PWM_INCREMENT;
          //if(mistPwmPct < PWM_MIN){mistPwmPct = PWM_MIN;} //duty cycle selection cannot be allowed to increment beyond boundaries
         
          if((mistPwm - PWM_INCREMENT) <= PWM_MIN){mistPwm = PWM_MIN;} //duty cycle selection cannot be allowed to increment beyond boundaries
          else{mistPwm -= PWM_INCREMENT;}
          
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
        
        //ble.print("Scene:\t");
        //ble.print(scene);

        ble.print("Mode:\t");
        ble.print(mode);

        ble.println();
        
        ble.print("Fan :\t");
        //ble.print(int(fanPwmPct*100));
        ble.print(fanPwm);
        ble.print(" \t");

        ble.print("Mist:\t");
        //ble.print(int(mistPwmPct*100));
        ble.print(mistPwm);
        ble.print(" ");
        
      } // End Update Control Pad Display UART
    
    } // END Handle Incoming Bluetooth Control Pad Packets 


    // ------------------------------- //
    // -- Handle Rotary Encoder Updates-- //
    // ------------------------------- //

    long encoderDelta = rotaryEncoder.read();

    if(encoderDelta != 0)       // encoder has moved
    {
      rotaryEncoder.write(0);   //reset encoder accumulator

      if(encoderDelta > 0)                                        // encoder moved in POSITIVE direction
      {
        if((fanPwm + encoderDelta) >= PWM_MAX){fanPwm = PWM_MAX;} // do not go above PWM_MAX
        else {fanPwm += encoderDelta;}                            // increase by encoderDelta
        
        if(Serial)
        { 
          Serial.print("encoderDelta: ");
          Serial.println(encoderDelta);
          Serial.print("fanPwm: ");
          Serial.println(fanPwm);
          Serial.println();
        }
      }
      
      if (encoderDelta < 0)                                  // encoder moved in NEGATIVE direction
      {
        if((fanPwm + encoderDelta) <= PWM_MIN){fanPwm = PWM_MIN;} // do not go below PWM_MIN
        else {fanPwm += encoderDelta;}                            // decrease by encoderDelta (add a negative)

        if(Serial)
        { 
          Serial.print("encoderDelta: ");
          Serial.println(encoderDelta);
          Serial.print("fanPwm: ");
          Serial.println(fanPwm);
          Serial.println();
        }
      }

      // If BLE is connected, update Control Pad Display
      if(ble.isConnected())
      {
        ble.println();  // Clear the previous 2 lines from the screen
        ble.println();  // Clear the previous 2 lines from the screen
        
        //ble.print("Scene:\t");
        //ble.print(scene);
    
        ble.print("Mode:\t");
        ble.print(mode);
    
        ble.println();
        
        ble.print("Fan :\t");
        //ble.print(int(fanPwmPct*100));
        ble.print(fanPwm);
        ble.print(" \t");
    
        ble.print("Mist:\t");
        //ble.print(int(mistPwmPct*100));
        ble.print(mistPwm);
        ble.print(" ");
        
      } // END ble.isConnected()
      
    } // END Encoder has moved

    // END Handle Rotary Encoder Updates

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
    // -- Run Mode Operations -- //
    // ------------------------------- //   

    //switch (scene) 
    switch(mode)
    {
      case OFF:
        // This prevents interface from adjusting fan & mist settings in OFF mode
        //fanPwmPct = 0.0;
        //mistPwmPct = 0.0;       
        //analogWrite(PIN_FAN, uint8_t(fanPwmPct*255));
        //analogWrite(PIN_MIST, uint8_t(mistPwmPct*255));

        fanPwm = 0;
        mistPwm = 0;       
        analogWrite(PIN_FAN, fanPwm);
        analogWrite(PIN_MIST, mistPwm);

        break;  // END OFF
        
      case CAMPFIRE:
        //analogWrite(PIN_FAN, uint8_t(fanPwmPct*255));
        //analogWrite(PIN_MIST, uint8_t(mistPwmPct*255));
        
        analogWrite(PIN_FAN, fanPwm);
        analogWrite(PIN_MIST, mistPwm);
        
        break;  // END CAMPFIRE
        
      case SEASHORE:

        break;  // END SEASHORE
        
      case TROPICS:

        break;  // END TROPICS
             
      default:
      
        // add default case here (optional)
        
        break;  // END default
        
    } // END switch(mode)

 
        
  } // END LOOP

