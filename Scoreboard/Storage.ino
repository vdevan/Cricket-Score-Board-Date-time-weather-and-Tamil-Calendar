/** Load WLAN credentials from EEPROM 
* There are 512 byte storage buffer available. We will use 
* this to store up to 6 WiFi configurations 32 byte for SSID
* and 32 byte for password. That will occupy 33*6*2 = 396 bytes in total
* to ensure we have the right data we will store header info hich will be
* a Prog ID and a version info both as UINT16. We will have two flags on the
* header - one for Scoreboard or clock flag and the other for using Porportional 
* font or fixed-pitch font. There is also 21 byte char array to store 
* Game ID - currently unusedT his will be the first
* header information on the EEPROM. Followed by NETWORK array
* We are looking at 16+16+8+8+21 a Total of 69 bytes for header and 396 bytes 
* Stored networks - well under 512 bytes
* 11/02/2024 - Vasu
* ***/

//If valid data exists in EEPROM read it and return true. else return false
bool loadCredentials()
{
  EEPROM.begin(512);
  int offset = 0;
  EEPROM.get(offset, header);
  offset += sizeof(header);
  
  if ((header.id != progId) || (header.ver != version))
  {
    Serial.println("Valid header not found");
    header.id = progId;
    header.ver = version;
    header.bScore = false;
    header.bPropFont = true;
    Serial.printf("Before version change: Header bScore Value: %d, bPropFont Value: %d\n", header.bScore, header.bPropFont);
    String(GAMEID).toCharArray(header.GameId,sizeof(header.GameId));
    EraseStoredValue();
    return true;
  }     
  else
  {
    bScoreboard = header.bScore;
    bUsePropFont = header.bPropFont;
    Serial.printf("After Version Change Header bScore Value: %d, bPropFont Value: %d\n", header.bScore, header.bPropFont);
    ScoreKey = header.GameId;
    Serial.printf ("Header Values read: ID: %d, Version: %d, Score: %d, Game ID: %s\n", header.id,header.ver,header.bScore,header.GameId);
  }
  
  storedNetworks = 0;
  for (int i = 0; i < MAXNETWORK; i++)
  {
    EEPROM.get(offset, Networks[i]);
    offset += sizeof(NETWORK);
    Serial.printf("READ From Storage[%d]: SSID: %s, Password: %s\n", i + 1, Networks[i].SSID, Networks[i].Password);
    if (strlen(Networks[i].SSID) > 0)
      storedNetworks++;
  }
  EEPROM.end();
  Serial.printf("Program ID: %d\nProgram Version: %d\nStored Network: %d\n",header.id,header.ver,(int)storedNetworks );

  return true;   
}

void EraseStoredValue()
{
    /* Let us not erase network contents. That has to be done from HTML page*/
    for (int i = 0; i < MAXNETWORK; i++)
    {
        memset(Networks[i].SSID, '\0', sizeof(Networks[i].SSID));
        memset(Networks[i].Password, '\0', sizeof(Networks[i].Password));
    }
    
    Serial.println("Erasing the stored contents");
    saveCredentials();
}

/** Store WLAN credentials to EEPROM */
void saveCredentials()
{

    EEPROM.begin(512);
    int offset = 0;
    EEPROM.put(offset, header);
    offset += sizeof(header);

    for (int i = 0; i < MAXNETWORK; i++)
    {
        EEPROM.put(offset, Networks[i]);
        offset += sizeof(NETWORK);
    }
    EEPROM.commit();

    Serial.println("Saving Network Configuration");

    EEPROM.end();
    delay(5000);
    ESP.restart();
}

