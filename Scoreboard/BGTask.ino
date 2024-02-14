#include "Weather8x8px.h" //Change this every year before April 15

/******************************************************************
 * This is a Scoreboard part of the method. Runs in background on 
 * Processor 1. You can switch between score board and Date/Time 
 * Weather calendar by using the monitor switch. You can also use
 * Proportional fonts instead of standard fixed pitch font. Can
 * be modified through Web UI. Provisions for 4 pages are given
 * currently and the score itself is updated every 30 seconds.
 * If the game score is for first games in first Innings, then the 
 * 4th page is not displayed, which essentially is the Target
 * Score. This display is for children's cricket club and perfectly
 * suits the requirement. Feel free to modify according to your need
 * 
 * Ver: 1008 Vasu Dated: 15-02-2024
 * *****************************************************************/

void WebReq(void* param)
{
    WEBPARAM* wp = (WEBPARAM*)param;
    MatrixPanel_I2S_DMA* dma = wp->dma_display;

    //Color Requirements for Cricket Active Score
    int SCROLLCOLOR = dma_display->color565(255, 255, 255);
    int LINE1COLOR = dma_display->color565(0,255,255);
    int LINE2COLOR = dma_display->color565(255, 255, 0);
    int LINE3COLOR = dma_display->color565(255, 255, 255);

    //Co-ordinates for Scoreboard
    int Line1X = 2;
    int Line1Y = 2;  //Originally 2
    int Line2X = Line1X + 8;
    int Line2Y = Line1Y + TRIPLEY + 6; // (bUsePropFont) ? 10 : 6; //Changed from + 6
    int Line3X = Line2X;
    int Line3Y = Line2Y + TRIPLEY + 2;
    
    uint8_t TEXTSIZE = (bUsePropFont) ? 1 : 3;
    
    if (bUsePropFont)
    {
        Line1Y = 26;
        Line2Y = Line1Y + TRIPLEY + 10;
        TEXTSIZE = 1;
        dma_display->setFont(&FreeSans18pt7b);
    }

    unsigned long pageDelay = PAGEDELAY; 


    delay(SECOND * 3);

    //bool bUpdate = !bNetUpdate;
    uint8_t pages = 0;

    while (!bServer)
    {
        pages = 0;
        while (pages < PAGES)
        {
             //Erase screen
            dma_display->fillScreen(myBLACK);
            dma_display->setTextWrap(false);
            switch (pages)
            {
            case 0:  //Page 1 Display 
                dma_display->setTextSize(TEXTSIZE);
                dma_display->setCursor(Line1X, Line1Y);
                dma_display->setTextColor(LINE1COLOR);
                dma_display->println(Player1); //can be split here for showing runs in different color

                //Line 2 Print runs / Balls Faced
                dma_display->setCursor(Line2X, Line2Y);
                dma_display->setTextColor(LINE2COLOR);
                dma_display->print(Player1Runs); //can be split here for showing runs in different color
                dma_display->print('/');
                dma_display->println(Player1BallsFaced);
                        
                delay(pageDelay);
                break;

            case 1:
                dma_display->setTextSize(TEXTSIZE);
                dma_display->setCursor(Line1X, Line1Y);
                dma_display->setTextColor(LINE1COLOR);
                dma_display->println(Player2); //can be split here for showing runs in different color

                //Line 2 Print runs / Balls Faced
                dma_display->setCursor(Line2X, Line2Y);
                dma_display->setTextColor(LINE2COLOR);
                dma_display->print(Player2Runs); //can be split here for showing runs in different color
                dma_display->print('/');
                dma_display->println(Player2BallsFaced);
                        
                delay(pageDelay);
                break;

            case 2:
                dma_display->setTextSize(TEXTSIZE);
                dma_display->setCursor(Line1X, Line1Y);
                dma_display->setTextColor(LINE1COLOR);
                if (bUsePropFont)
                    dma_display->print("T:");
                dma_display->print(TotalRuns); //can be split here for showing runs in different color
                dma_display->print('/');
                dma_display->println(Wickets);

                //Line 2 Print runs / Balls Faced
                dma_display->setCursor(Line2X, Line2Y);
                dma_display->setTextColor(LINE2COLOR);
                dma_display->print("o "); //can be split here for showing runs in different color
                dma_display->println(OversBowled,1); //1 decimal or 2?

                delay(pageDelay);
                break;

            case 3:
                if (TargetRuns == 0)
                    break;
                dma_display->setTextSize(TEXTSIZE);
                dma_display->setCursor(Line1X, Line1Y);
                dma_display->setTextColor(LINE1COLOR);
                dma_display->print("Target"); //can be split here for showing runs in different color
                //dma_display->print('/');
                //dma_display->println(Wickets);

                //Line 2 Print runs / Balls Faced
                dma_display->setCursor(Line2X, Line2Y);
                dma_display->setTextColor(LINE2COLOR);
                if (bUsePropFont)
                    dma_display->print("R: ");
                dma_display->print(TargetRuns); //can be split here for showing runs in different color
                        
                delay(pageDelay);
                break;

            }

            pages++;

        }
                
    }
}

/*************************************************************************
 * Background application for displaying Clock amd Tamil Calendar. To ensure
 * maximum utilisation of CPU cycles, only Seconds are updated every second. 
 * Minute and hour are updated as they change. AM / PM also updated when needed
 * This method avoids flickering and ensures smooth display. Runs on Processor 1.
 * Time used by eztinme.h gets updated every 10 minutes on processor 0. 
 * Global variable ClkTZ keeps track of date & time and this is copied locally
 * Weather is updated every 25 minutes. Global variable Weather, Rain and Cloud
 * are used for weather display. Weather variable is copied locally
 * All environmental requirements like WAN IP address, Latitude, Longitude and
 * Timezones are obtained from Web thus providing flexibility
 * 
 * Ver 1008 - Vasu deated 15/02/2024
 * *******************************************************************/

void ClkReq(void* param)
{   
    WEBPARAM* wp = (WEBPARAM*)param;
    
    MatrixPanel_I2S_DMA* dma = wp->dma_display;
    dma_display->fillScreen(dma_display->color444(0, 0, 0)); //Clear Screen
    String wr = Weather;
    bool bUpdate = !bNetUpdate; //Force displaying weather
    String TC = TamilCalendar;
    String temp = curTemp;

    //Used for first time display at start of the program and start of the day
    String dispTime;
    String dispDate;

    //Used for keeping track of change in date and time
    uint8_t min;
    uint8_t hr;
    TIME dt;

    //Various colors used
    int TIMECOLOR = myGREEN + myBLUE;
    int AMCOLOR = dma->color565(120, 150, 64);
    int PMCOLOR = dma->color565(72, 250, 160);
    int DATECOLOR = dma->color565(172, 250, 60);
    int WEATHERCOLOR = dma->color565(70, 201, 66);
    int TEMPCOLOR = dma->color565(220, 75, 75);
    int CALENDARCOLOR = dma->color565(255, 35, 225);

    //Co-ordinates for Time Display. Make any changes here
    int timeX = 6;
    int timeY = 2;
    int TwocharWidth = 2 * DOUBLEX;
    int TwocharHeight = DOUBLEY;
    int hourStart = timeX;
    int minuteStart = timeX + (DOUBLEX * 3);
    int secondStart = timeX + (DOUBLEX * 6);

    //Co-ordinates for AM / PM display
    int amX = 106;
    int amY = 9;
    int pmY = timeY;

    //Co-ordinates for Date
    int dateX = timeX - 4;
    int dateY = timeY + DOUBLEY + 1;

    //Co-ordinates for current Temperature
    int tempX = dateX + SINGLEX * 14;
    int tempY = dateY;

    //Co-ordinates for Weather
    int weatherX = dateX;
    int weatherY = dateY + SINGLEY + 2;

    //Bitmap Co-ordinates for Rain
    int rainX = dateX + (SINGLEX * 10);
    int rainY = weatherY;    

    //Bitmap Size - Fixed 8px x 8px
    uint16_t bmpW = 8;
    uint16_t bmpH = 8;

    //Bitmap Co-ordinates for Cloud
    int cloudX = rainX + (SINGLEX * 3) + 15; //Rain bitmap width is 8  + 2 & 5 for one space.
    int cloudY = weatherY;

    //Co-ordinates for Tamil Calendar
    int tcX = dateX;
    int tcY = weatherY + SINGLEY + 2;

    //Bitmap Co-ordinates for Tithi
    int tithiX = 118; //Last line, Last column
    int tithiY = tcY + SINGLEY + 2;



    //Set date / time for tracking
    String cd = ClkTZ.dateTime(RFC1036);
    Serial.println(cd);
    dt.minutes = ClkTZ.minute();
    dt.hour = ClkTZ.hour();
    dt.ampm = ClkTZ.isAM();
    dt.date = ClkTZ.day();
    dispTime = cd.substring(15, 23);
    dispDate = cd.substring(0, 12);

    //Set Time to be done once - Updates every second
    dma->setCursor(timeX, timeY);
    dma->setTextSize(2);
    dma->setTextColor(TIMECOLOR);
    dma->print(dispTime);
    dma->setTextSize(1);

    bDayChange = !bDayChange; //Notify to fetch Panchangam 
    delay(SECOND * 2);

    //To be done once. - Updates every 12 hours - Display AM / PM
    if (dt.ampm == 1)
    {
        dma->fillRect(amX, timeY, SINGLEX * 2, DOUBLEY, myBLACK);
        dma->setCursor(amX, amY);
        dma->setTextColor(AMCOLOR);
        dma->print("AM");
    }
    else
    {
        dma->fillRect(amX, timeY, SINGLEX * 2, DOUBLEY, myBLACK);
        dma->setCursor(amX, timeY);
        dma->setTextColor(PMCOLOR);
        dma->print("PM");
    }

    //To be done once - updates once every day - Display Date
    dma->fillRect(0, dateY, tempX, SINGLEY, myBLACK);
    dma->setCursor(dateX, dateY);
    dma->setTextSize(1);
    dma->setTextColor(DATECOLOR);
    dma->print(dispDate);

    //Tamil Calendar. To be done once every day 

    dma->fillRect(0, tcY, PANEL_RES_X * PANEL_CHAIN, SINGLEY * 3, myBLACK);
    dma->setCursor(tcX, tcY);
    dma->setTextSize(1);
    dma->setTextWrap(true);
    dma->setTextColor(CALENDARCOLOR);
    dma->print(TC);

    dma->setTextWrap(false);
    //Reset to Clock display
    dma->setTextSize(2);
    dma->setCursor(timeX, timeY);
    dma->setTextColor(TIMECOLOR);


    while (!bServer)
    {
        dt.minutes = ClkTZ.minute();
            
        //Print Seconds first
        //Clear Seconds display
        dma->fillRect(secondStart, timeY, TwocharWidth, TwocharHeight , myBLACK);
        dma->setCursor(secondStart, timeY);
        dma->setTextColor(TIMECOLOR);
        dma->printf("%02d",ClkTZ.second());
            

        //Next Minute
        if (min != dt.minutes)
        {
            min = dt.minutes;
            dma->fillRect(minuteStart, timeY, TwocharWidth, TwocharHeight, myBLACK);
            dma->setCursor(minuteStart, timeY);
            dma->setTextColor(TIMECOLOR);
            dma->printf("%02d",dt.minutes);
            dt.hour = ClkTZ.hour();
        }

        //Next Hour Also check for AM / PM and date 
        if (hr != dt.hour)
        {
            hr = dt.hour;
            dma->fillRect(hourStart, timeY, TwocharWidth, TwocharHeight, myBLACK);
            dma->setCursor(hourStart, timeY);
            dma->setTextColor(TIMECOLOR);
            hr = hr > 12 ? hr - 12 : hr;
            hr = hr == 0 ? 12 : hr;
            dma->printf("%02d",hr);
            dma->setTextSize(1);

            //We will check for AM/PM
            if (dt.ampm != ClkTZ.isAM())
            {
                bDayChange = !bDayChange; //Notify to fetch Panchangam 
                dt.ampm = ClkTZ.isAM();
                if (dt.ampm == 1)
                {
                    dt.ampm = ClkTZ.isAM();
                    dma->fillRect(amX, timeY, SINGLEX * 2, DOUBLEY, myBLACK);
                    dma->setCursor(amX, amY);
                    dma->setTextColor(AMCOLOR);
                    dma->print("AM");
                }
                else
                {
                    dma->fillRect(amX, timeY, SINGLEX * 2, DOUBLEY, myBLACK);
                    dma->setCursor(amX, timeY);
                    dma->setTextColor(PMCOLOR);
                    dma->print("PM");
                }
            }
            //Change Tamil Calendar every hour.
            TC = TamilCalendar;

            //Check for date needed to check every hour
            if (dt.date != ClkTZ.day())
            {
                dt.date = ClkTZ.day();
                String cd = ClkTZ.dateTime(RFC1036);
                dispDate = cd.substring(0, 12);

                //Change date and day
                dma->fillRect(0, dateY, tempX, SINGLEY, myBLACK);
                dma->setCursor(dateX, dateY);
                dma->setTextSize(1);
                dma->setTextColor(DATECOLOR);
                dma->print(dispDate);

                dma->fillRect(0, tcY, PANEL_RES_X* PANEL_CHAIN, SINGLEY * 3, myBLACK);
                dma->setCursor(tcX, tcY);
                dma->setTextSize(1);
                dma->setTextWrap(true);
                dma->setTextColor(CALENDARCOLOR);
                dma->print(TC);
                dma->setTextWrap(false);            

            }

            
            //Reset to Time display
            dma->setTextSize(2); //Needed for keeping time
            dma->setTextColor(TIMECOLOR);


        }

        if (bUpdate != bNetUpdate)
        {
            bUpdate = bNetUpdate;

            //Make a copy of the variable
            wr = Weather;
            temp = curTemp;

            //Put the current temperature next to date
            dma->fillRect(tempX, tempY, SINGLEX * 7, SINGLEY, myBLACK);
            dma->setCursor(tempX, tempY);
            dma->setTextSize(1);
            dma->setTextColor(TEMPCOLOR);
            dma->print(temp);

            dma->fillRect(0, weatherY, PANEL_RES_X * PANEL_CHAIN, SINGLEY, myBLACK);

            //Draw Cloud / Rain bitmap
            dma->drawBitmap(rainX, rainY, bitmapArray[1], bmpW, bmpH,myGREEN);
            dma->drawBitmap(cloudX, cloudY, bitmapArray[2], bmpW, bmpH, myGREEN);

            dma->setCursor(weatherX, weatherY);
            dma->setTextSize(1);
            dma->setTextColor(WEATHERCOLOR);
            dma->print(wr);
            dma->setCursor(rainX + 10, rainY);
            dma->print(Rain);
            dma->setCursor(cloudX + 10, cloudY);
            dma->print(Cloud);

            //Change Tamil Calendar
            TC = TamilCalendar;
            dma->fillRect(0, tcY, PANEL_RES_X * PANEL_CHAIN, SINGLEY * 3, myBLACK);
            dma->setCursor(tcX, tcY);
            dma->setTextSize(1);
            dma->setTextWrap(true);
            dma->setTextColor(CALENDARCOLOR);
            dma->print(TC);

            //Draw Tithi bitmap - Waxing / Waning
            if (bWaxing)
                dma->drawBitmap(tithiX, tithiY, bitmapArray[3], bmpW, bmpH, myWHITE);
            else
                dma->drawBitmap(tithiX, tithiY, bitmapArray[4], bmpW, bmpH, myGREEN + myRED);

            //Reset to Clock display
            dma->setTextWrap(false);
            dma->setTextSize(2);
            dma->setCursor(timeX, timeY);
            dma->setTextColor(TIMECOLOR);

        }

        vTaskDelay(SECOND);
    }

}