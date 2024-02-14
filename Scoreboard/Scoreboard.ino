
/**********************************************************************
 * The main display is based on Development code from the website:
 * https://www.waveshare.com/wiki/RGB-Matrix-P2.5-64x64
 * There are 11 files that are to be stored locally apart from .ino files
 * The display resolution and dma buffers are handled by these 11 files. 
 * Leave them in the same directory as the INO files.
 * Libraries and board manager are already documented in the above link
 * Additional libraries that are loaded are as follows:
 * Arduino_JSON by Arduino
 * Adafruit BusIO by Adafruit
 * Adafruit GFX Library by Adafruit
 * EZTime.h by Rop Gonggrijp
 * Virtuino library for all ESP8266 and ESP32 boards by ILias Lamprou<iliaslampr@gmail.com>
 * Libraries are generally found in C:\Users\user\AppData\Local\Arduino15
 * and also in the Preferences/SketchBooklocation\libraries
 * Note the order of include files. Webserver must be declared after wifi.h & wifiMulti.h
 * Also note that OTA (Over The Air) transmission is used for uploading compiled sketches
 * 
 * For Visual Studio use vMicro extension for Arduino IDE
 * Compiled and tested in both Arduino and Visual Studio with vMicro extension
 * Vasudevan Dated:29/01/2024
 * 
 * Some Sample data is provided for testing purpose. Two flag variables are provided upfront
 * bUseScoreTestData & bUsePanchangTestData Use them for testing. For live make these flags false.
 * Vasudevan Dated 4/02/2024
 * 
 * OTA - Over The Air is used for compiling and pushing the bin file to ESP32. OTA.ino is included
 * Detailed documentation can be found at: https://docs.arduino.cc/arduino-cloud/features/ota-getting-started/
 * 
 * Ver 1008 -Dated 15-02-2024 Vasu
 **************************************************************************/


#include <ezTime.h> //Manage Date / Time
#include <ArduinoOTA.h>
#include <WiFi.h>        //ESP8266 WiFi
#include <WiFiMulti.h>
#include <Webserver.h>
#include <ESPmDNS.h>        //Needed for Web server
#include <EEPROM.h>             //WiFi info are stored. Up to 6 networks can be stored
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>       //JSON library
#include <HTTPClient.h>
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

//Include different fonts here Fonts are at "C:\Users\vdeva\AppData\Local\Arduino15\libraries\Adafruit-GFX-Library\Fonts"
//#include "FreeSans9pt7b.h"
#include "FreeSans18pt7b.h"
#include "Panch2024-25.h"


bool bUseScoreTestData = false;      //For live data, use false
bool bUsePropFont = true;

//Global variables for HTTP Secure POST request VID and PID are embedded in the code. To be isolated
const char httpData[] = "{\"gameId\":\"f73ad3bc\",\"pid\":\"10c4\",\"vid\": \"ea60\"}";
const char* server1URL = "https://zg1emv3wlf.execute-api.us-east-1.amazonaws.com/test/livescore";

//Test data
String scoreData = "{\"notOutPlayers\":[{\"name\":\"Addiya Chowdhury\",\"runs\":12,\"ballsFaced\":44},{\"name\":\"Janaki Vasudevan\",\"runs\":2,\"ballsFaced\":18}],\"battingStats\":{\"overs\":28.4,\"total\":62,\"wickets\":10,\"target\":76},\"bowlingStats\":[]}\"wickets\":10,\"target\":76},\"bowlingStats\":[]}";


//URLs and Keys used for ThinkSpeak Server. Currently not used. Left for future usage
//String TSURL = "http://api.thingspeak.com/apps/thinghttp/send_request?api_key=";

//Used for Weather - Global Variables Timezone is also used to set the clock

//parameters for getting time and Weather
String Location; //to be evaluated as : "latitude=-34.47&longitude=150.42&timezone=Australia/Sydney  / is replaced with %2F
String TimeZone; //Used by Clock and also WUri

//Provides GatewayIP, location and Time zone. Time zone to be constructed from "country_name" / "city_name"
String LocationUri = "http://api.ip2location.io/?key="; 
String LocationKey = "3376D7BBA70D6E78BD7B6E235F95254F";  //to be deleted before posting on public domain

//Do not call WUri without setting Location & TimeZone. Call Getlocation first
//Parameters needed are: // + Location  + TimeZone + "&current=temperature_2m,rain,cloud_cover&hourly=temperature_2m,rain,
// cloud_cover&daily=weather_code,temperature_2m_max,temperature_2m_min,rain_sum&forecast_days=1";
//These are filled up in WeatherUri - split is required to ensure if the location changes, it is accommodated

String WUri = "http://api.open-meteo.com/v1/forecast?"; 
String WeatherUri = ""; 

//End of Clock, Weather and Tamil Calendar declaration


//Internal House Keping Declarations
//WiFi Client Connection change if too low
#define WHILE_LOOP_DELAY  200   //Connection Wait time
#define MAXNETWORK 6            //stored network
#define SECOND 1000
#define WIFI_TIMEOUT 8         //Time out in secs for Wifi connection. 
#define SERVERTIGGER 34        // Trigger Server Request 

//Header For EEPROM storage and retrieval
#define version 1008    // Major 1 and minor 004 Added two fields bool for Active Score / Calendar and gameid
                        // minor 005 HTML file modified to incorporate Active clock or Active Game Score 31/01/2024
                        //1006 failed due to erratic save. 1007 Added Proportional font switch
                        //1008 increased GameId size so as to include Thinkspeak as a key

#define progId 0x5342   //Hex for SB - Score Board
#define GAMEID "66a801e3"
#define PID 0x10c4
#define VID 0xEA60

struct HEADER
{
    uint16_t id;
    uint16_t ver;
    uint8_t  bScore;
    char GameId[21];
    uint8_t bPropFont;
} header;

//Currently not used. Will be required in future
String vid = String(VID, HEX);
String pid = String(PID, HEX);

bool bScoreboard = false; //Determine whether Scoreboard or Calendar to be displayed
String logInfo;  //To be used for storing in the web - currently unused


//Network Declarations

const char WIFIPwd[9] = "pass1234";  //Host password - None
const char HOSTNAME[11] = "scoreboard";   //

//LAN NOLANbyte array is used for displaying the signal 
// strength / connection status at the top right corner

struct NETWORK
{
    char SSID[33];
    char Password[33];
};

//WiFi, UDP, Server & DNS. Change constants as per requirement
String ESPSSID = "SB" + String(progId, HEX) + "_" + pid;

WebServer server(80);// Web server
IPAddress ESPIP(192, 168, 8, 1);  //192.168.8.1 Defalt server IP. Safe on class C Network
IPAddress dhcp(192, 168, 8, 5);
IPAddress netMask(255, 255, 255, 0); //by using 0 in 4th octet, (254 - 5) network connectivity possible



NETWORK Networks[MAXNETWORK];
WiFiClientSecure secureClient;

//Internal Network Variables
uint8_t storedNetworks = 0;
int storageIndex;
unsigned long SecDelay;

//Connection status of Server / client and WiFi retry 
bool bConnect = false;
bool bServer = false;

//String WANPortIP;

//End of Network Declarations


//Panel Display Definitions
//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA* dma_display = nullptr;

#define E_PIN_DEFAULT  32  //Required for 64x64 Matrix
#define PANEL_RES_X 64     // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 64     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 2      // Total number of panels chained one to another

//Note Color565 is defined as 5 bit for Red, 6 bit for Green and 5 bit for Blue. Hence R&B can have only values 0~32 while Green 0~64
uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

uint16_t colorWheel(uint8_t pos)
{
    if (pos < 85)
    {
        return dma_display->color565(pos * 3, 255 - pos * 3, 0);
    }
    else if (pos < 170)
    {
        pos -= 85;
        return dma_display->color565(255 - pos * 3, 0, pos * 3);
    }
    else
    {
        pos -= 170;
        return dma_display->color565(0, pos * 3, 255 - pos * 3);
    }
}

String scrollText;
int Xaxis = 2;
int Yaxis = 2;

#define SINGLEX 6
#define DOUBLEX 12
#define TRIPLEX 18

#define SINGLEY 8
#define DOUBLEY 16
#define TRIPLEY 24
#define PAGEDELAY SECOND * 4
#define PAGES 4

//String SCROLLTEXT = "Fetching results from Web *** ";
String SCROLLTEXT = "Connecting *** ";

//End of Display Panel Declarations


//Score card Global variables
String ScoreCard;
String Player1;
String Player2;
int Player1Runs;
int Player2Runs;
int Player1BallsFaced;
int Player2BallsFaced;
int TotalRuns;
int Wickets;
int TargetRuns;
double OversBowled;



String ScoreKey; //Currently Not used

//Global Variables Used by Calendar
struct TIME
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
    uint8_t date;
    uint8_t ampm;
}dtTime;


String curTemp;
uint8_t Rain;
uint8_t Cloud;
String TamilCalendar;

String Weather = "";
bool bDayChange = false;
bool bDay;
Timezone ClkTZ;

//End of Calendar Variables Declaration

//Background Task Variable
struct WEBPARAM
{
    //Web Request Parameters
    MatrixPanel_I2S_DMA* dma_display;
    int Xaxis;
    int Yaxis;
    int scrollSpeed;
    int textSize;
    char win[10];
    //Result Variables

}webParam;

 TaskHandle_t bgTask = null;
 bool bNetUpdate = false;

 //End of Background Task Variables


//Timing Variables
 unsigned long lastTime = 0;
 unsigned long timeDelay = 30000; //30 secs will be used in multiples for next fetch
 unsigned long scrollSpeed = 40; //originally 80

void setup() 
{
    delay(1000);
    Serial.begin(115200);
    Serial.println();
    PanelInit();  //Show Initial Screen

    Serial.println("Configuring access point...");
    pinMode(SERVERTIGGER, INPUT); //Monitor for Server Trigger
    logInfo = "";
    ScoreCard = "";
    Player1Runs = 0;
    Player2Runs = 0;

    WiFi.softAPConfig(ESPIP, ESPIP, netMask, dhcp);
    WiFi.softAP(ESPSSID.c_str(), WIFIPwd);
    delay(500);
    Serial.print("ESP Device IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/wifisave", handleWifiSave);
    server.onNotFound(handleNotFound);

    if (loadCredentials()) // Load WLAN credentials from EPROM if not valid or no network stored then we need to start webserver
        connectWifi();

    
    if (!bConnect)
        startServer();
    else
    {
        BeginOTA();
        /*if (GetScore())
        {
            Serial.printf("Team Won: %s\n", winner.c_str());
            Serial.printf("Home Score: %d\n", homeScore);
            Serial.printf("Opponent Score: %d\n", oppScore);
        }
        else
        {
            Serial.println("Error received");
        }*/

        //Test with various fonts. Both fixed pitch and proportional are available at different sizes
        //dma_display->setFont(&FreeSans9pt7b);

        dma_display->fillScreen(dma_display->color444(0, 0, 0));
        dma_display->setTextSize(1);
        dma_display->setCursor(2, 12);
        dma_display->setTextColor(myGREEN);
        dma_display->setTextWrap(false);
        dma_display->println("System configuring.");
        dma_display->setCursor(2, 24);
        dma_display->println("Please Wait");
        dma_display->setCursor(2, 34);
        dma_display->println("Checking Internet...");

        Serial.printf("VID to be used: %s; PID: %s\n", vid, pid);

        dma_display->fillScreen(dma_display->color444(0, 0, 0));
        dma_display->setTextSize(1);
        dma_display->setCursor(2, 12);
        dma_display->setTextColor(myGREEN + myBLUE);

        if (bScoreboard)
        {
            Serial.printf("Using ScoreKey: %s\n", ScoreKey.c_str());
            dma_display->println("Getting Scores");
            dma_display->setCursor(2, 24);
            dma_display->println("Please Wait...");
            GetScore();
        }
        else
        {
            dma_display->println("Getting Location");
            dma_display->setCursor(2, 24);
            dma_display->println("Please Wait...");
            if (!GetLocation())
            {
                dma_display->setCursor(2, 44);
                dma_display->println("No Internet found");
                dma_display->setCursor(2, 54);
                dma_display->println("Starting Web Server");
                delay(SECOND * 5);
                startServer();
                return;
            }
            else
            {
                setInterval(timeDelay * 20); //Check clock every 10 minutes
                events();
                dma_display->setCursor(2, 44);
                dma_display->println("Getting Weather");
                //dma_display->setCursor(2, 24);
                //dma_display->println("Please Wait...");
                delay(SECOND * 5);

                GetWeather();
                uint16_t dayIndex = (ClkTZ.dayOfYear());
                if (ClkTZ.year() == 2025)
                    dayIndex += 365;
                TamilCalendar = GetTC(dayIndex);

                bDay = bDayChange;
            }
        }

        //Last of display handling 
        dma_display->fillScreen(dma_display->color444(0, 0, 0)); //All display activities will be now handled by processor 1
        scrollText = SCROLLTEXT;
        webParam.dma_display = dma_display;
        webParam.scrollSpeed = scrollSpeed;
        webParam.Xaxis = Xaxis;
        webParam.Yaxis = Yaxis;
        webParam.textSize = 1;
        memset(webParam.win, '\0', sizeof(webParam.win));
        if (bScoreboard)
        {
            xTaskCreatePinnedToCore(WebReq, "bgTask", 10000, (void*)&webParam, 1, &bgTask, 1);
        }
        else
        {
            xTaskCreatePinnedToCore(ClkReq, "bgTask", 15000, (void*)&webParam, 1, &bgTask, 1);
        }
    }

    /*
    //Post your log info here as scroll text. Temprorary arrangement
    if (logInfo != "")
    {
        scrollText = logInfo;
        delay(5000);
        //Wait a minute to scroll the text before moving to next command;
    }
    */
    lastTime = millis();
    SecDelay = millis();
}

/* Copied from original example. Used as a test for all the LEDs */
static bool PanelInit()
{
    // Module configuration
    HUB75_I2S_CFG mxconfig(
        PANEL_RES_X,   // module width
        PANEL_RES_Y,   // module height
        PANEL_CHAIN    // Chain length
    );

    mxconfig.gpio.e = 32;
    mxconfig.clkphase = false;
    mxconfig.driver = HUB75_I2S_CFG::FM6124;

    // Display Setup
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(90); //0-255
    dma_display->clearScreen();
    dma_display->fillScreen(myWHITE);

    // fix the screen with green
    dma_display->fillRect(0, 0, dma_display->width(), dma_display->height(), dma_display->color444(0, 15, 0));
    delay(500);

    // draw a box in yellow
    dma_display->drawRect(0, 0, dma_display->width(), dma_display->height(), dma_display->color444(15, 15, 0));
    delay(500);

    // draw an 'X' in red
    dma_display->drawLine(0, 0, dma_display->width() - 1, dma_display->height() - 1, dma_display->color444(15, 0, 0));
    dma_display->drawLine(dma_display->width() - 1, 0, 0, dma_display->height() - 1, dma_display->color444(15, 0, 0));
    delay(500);

    // draw a blue circle
    dma_display->drawCircle(10, 10, 10, dma_display->color444(0, 0, 15));
    delay(500);

    // fill a violet circle
    dma_display->fillCircle(40, 21, 10, dma_display->color444(15, 0, 15));
    delay(500);

    // fill a violet circle
    dma_display->fillCircle(70, 31, 10, dma_display->color444(8, 12, 12));
    delay(500);

    // fill a violet circle
    dma_display->fillCircle(100, 41, 10, dma_display->color444(12, 15, 0));
    delay(500);


    return true;
}

//Monitor the config pin and call Config function if required
static void MonitorConfigPin()
{
    if (digitalRead(SERVERTIGGER) == LOW)
        return;
    int elapse = 0;
    Serial.printf("Detected Config Request\n");
    while (digitalRead(SERVERTIGGER) == HIGH)
    {
        elapse++;
        delay(1000);
    }
    if (elapse < 3) //discard spikes
        return;

    //Over 8 seconds then start Web server
    if (elapse > 8)
    {
        bConnect = false;
        dma_display->setFont();
        startServer();
        return;
    }

    //Over 2 seconds and less than 8 seconds? then Toggle between Clock & Scoreboard

    if (elapse > 2)
    {    
        bServer = true; //This will exit the loop

        //Now you can kill it
        if (bgTask != null)
            vTaskDelete(bgTask);

        dma_display->setFont();

        dma_display->fillScreen(dma_display->color444(0, 0, 0));
        dma_display->setTextSize(1);
        dma_display->setCursor(2, 12);
        dma_display->setTextColor(myGREEN);
        dma_display->setTextWrap(true);
        dma_display->println("System configuring.");
        dma_display->setCursor(2, 24);
        dma_display->println("Please Wait");
        dma_display->setTextWrap(false);

        bScoreboard = !bScoreboard; //Toggle Scoreboard switch
        header.bScore = bScoreboard;
        Serial.println("Getting into Save Credentials");

        saveCredentials(); //Save it to EEPROM
        Serial.println("Back from Save Credentials");
        ESP.restart();
    }


}

void loop() 
{
    
    MonitorConfigPin();  //need to work this out
   if (bServer)
    {
        server.handleClient();
    }
    else
    {
        ArduinoOTA.handle();
        if (bScoreboard)
        {
            while ((millis() - lastTime) > timeDelay) //Get Scorecard every 30 seconds
            {
                if (bConnect)
                {
                    if (!GetScore())
                    {
                        Serial.println("Error received");
                    }
                    else
                    {
                        //Serial.println("30secs complete");
                    }
                }
                lastTime = millis();
            }
        }
        else
        {
            events();
            while ((millis() - lastTime) > timeDelay * 50) //Get Weather every 25 minutes 30 * 50 timedelay is defined as 30seconds
            {
                GetWeather();
                lastTime = millis();
                Serial.println("Calling Weather");

            }
            if (bDay != bDayChange) //One day passed. Update Panchangam Actually called twice a day when Am / PM changes
            {
                if (!GetLocation())
                {
                    delay(timeDelay * 2);
                    ESP.restart();
                }
                bDay = bDayChange;
                delay(SECOND * 5);
                uint16_t dayIndex = (ClkTZ.dayOfYear());
                if (ClkTZ.year() == 2025)
                    dayIndex += 365;
                TamilCalendar = GetTC(dayIndex);
            }

        }
    }
}
