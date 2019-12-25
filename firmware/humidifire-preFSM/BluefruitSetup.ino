/* Sequence of commands to setup the Bluefruit Module
 * Moved to seperate file to clean up setup{} section of main code file
 * 
 * Includes:
 * - perform Factory Reset at power up
 * - set device name using (BLE_DEV_NAME_PREFIX + last 4 digits of UID)
 * 
 */

void bluefruitSetup()
{
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
  
}

