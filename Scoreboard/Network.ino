
static void connectWifi()
{
    //Serial.println("Connecting as wifi client...");
    int n = WiFi.scanNetworks();
    unsigned long tick = millis();
    storageIndex = -1;
    int j;
    for (int i = 0; i < storedNetworks; i++)
    {
      for (j = 0; j < n; j++)
      {
        Serial.printf("Now checking the SSID %s at storage index %d with Scanned Network: %s\n",
              Networks[i].SSID, i, WiFi.SSID(j).c_str());

        if (WiFi.SSID(j).compareTo(String(Networks[i].SSID)) == 0)
        {
            storageIndex = i;
            break;
        }
      }
      if (j < n)  //storageIndex >= 0
      {
        if (tryConnect())
            break;
      }
    }

    if (bConnect)
    {          
      Serial.println("Connected to Network");
    }
    else
    {
      Serial.println("Not Connected to Network");
    }

    return;
}

//First 
static bool tryConnect()
{
    int i = 0;
    bConnect = false;
    Serial.printf("Trying to connect with SSID: %s, Password: %s\n", Networks[storageIndex].SSID, Networks[storageIndex].Password);

    WiFi.begin(Networks[storageIndex].SSID, Networks[storageIndex].Password);

    int status = WiFi.status();
    int startTime = millis();
    while (status != WL_CONNECTED && status != WL_NO_SSID_AVAIL && status != WL_CONNECT_FAILED && (millis() - startTime) <= WIFI_TIMEOUT * 1000)
    {
        delay(WHILE_LOOP_DELAY);
        status = WiFi.status();
    }

    if (WiFi.status() == WL_CONNECTED)
        bConnect = true;

    Serial.printf("Connection status of SSID: %s is %d\n", Networks[storageIndex].SSID, WiFi.status());
    return bConnect;
}

//Server start when connection fails
static void startServer()
{
    server.begin(); // Web server start
    Serial.println("HTTP server started");
    Serial.printf("BGTask handle: %d\n", bgTask);

    // Setup MDNS responder
    if (!MDNS.begin(HOSTNAME))
        Serial.println("Error setting up MDNS responder!");
    else
    {
        Serial.println("mDNS responder started");
        MDNS.addService("http", "tcp", 80);
    }
    bServer = true;
    if (bgTask != null)
        vTaskDelete(bgTask);

    dma_display->setFont();
    dma_display->fillScreen(myBLACK);
    Serial.println("Cleared Screen");
    int line = 3;
    int col = 5;
    
    dma_display->setTextSize(1);
    dma_display->setCursor(col, line);
    dma_display->setTextColor(myGREEN);
    dma_display->println("Web Server Started");
        
    line += SINGLEY + 2;
    dma_display->setCursor(col, line);
    dma_display->setTextColor(myBLUE + myGREEN);
    dma_display->println("IP: 192.168.8.1");
        
    line += SINGLEY + 4;
    dma_display->setCursor(col, line);
    dma_display->setTextColor(myBLUE + myRED);
    dma_display->printf("SSID = %s", ESPSSID.c_str());

    line += SINGLEY + 4;
    dma_display->setCursor(col, line);
    dma_display->setTextColor(myGREEN + myRED);
    dma_display->printf("Password = %s", WIFIPwd);

    line += SINGLEY + 4;
    dma_display->setCursor(col, line);
    dma_display->setTextColor(myGREEN);
    dma_display->print("http://192.168.8.1");

}

/* This method is not used. Originally tried for AWS server but that did not work.
 * Hence SecurePOSTRequest is used instead. This method is kept here for future
 * referemce */
/*
String SecureWifiRequest(String url, String Header = "", String parse = "")
{
    String payload = "";
    WiFiClientSecure client;
    client.setInsecure();
    if (Header == "")
    {
        String Host = url.substring(url.indexOf(':') + 3); //remove '://'
        String uri = Host.substring(Host.indexOf('/'));
        Host = Host.substring(0, Host.indexOf('/'));
        Header += "GET " + uri + " HTTP/1.1\r\n";
        Header += "Host: " + Host + "\r\n";
    }
    else
    {
        Header += ConnectionClose;
    }
    //Serial.printf("URL to be used: %s\n", url.c_str());
    //Serial.printf("Header to be used: %s\n,Header", Header.c_str());
    //Serial.printf("Parse String to be used: %s\n", parse.c_str());


    if (!client.connect(url.c_str(), 443))
        Serial.println("Connection failed!");
    else
    {
        // Make a HTTP request:

        client.print(Header);

        int Bodyline = 0;
        int Headline = 0;
        // Read and print the response body
        while (client.connected())
        {
            Headline++;
            String line = client.readStringUntil('\n');
            if (line == "\r")
            {
                Headline = -1;
                break;
            }
        }
        // Read and print the response body
        while (client.available())
        {
            String line = client.readStringUntil('\n');
            Bodyline++;
            if (parse != "")
            {
                if (line.startsWith(parse))
                {
                    payload = line;
                    break;
                }

                if (line.startsWith("{\"errorMessage\":\""))
                {
                    payload = "error";
                    break;
                }
            }
            else
            {
                payload += line;
                //break;
            }
        }

        //Serial.printf("Head Line Nos: %d Body Line Nos: %d\r\n", Headline, Bodyline);
        //logInfo += "H: " + String(Headline) + "; L: " + String(Bodyline);
        //if (parse == "")
            //Serial.printf("Results Obtained: for URL: %s----\n %s\n", url.c_str(), payload.c_str());
    }
    return payload;
}
*/



static String GETRequest(String uri)
{
    HTTPClient http;
    WiFiClient wifiClient;
    //Serial.printf("HTTPClient will use URL:\n %s\n", uri.c_str());
    http.begin(wifiClient, uri.c_str());
    int httpResponseCode = http.GET();

    String payload = "{}";

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    http.end();
    //Serial.printf("Payload from HTTP Client: %s\n", payload.c_str());
    return payload;
}


/****************************************************************
 * This will securely POST to AWS server. This is the only method
 * that has been tested to work. GameID in the Data is currently 
 * embedded in the code. Proper integration with WEBUI is yet to be
 * done. PID - 10c4 and VID - ea60  are impotant. These are updated
 * in the backend server: 
 * http://cricket-score-sv.s3-website-us-east-1.amazonaws.com/ 
 * First Field on the website is PID and the second one is VID. 
 * Third field is the Game ID
 * 
 * Modified on 10th Feb 2024 - Vasu Prog Ver 1008
 ***************************************************************/
static String SecurePOSTRequest()
{
    HTTPClient httpclient;
    WiFiClientSecure wificlient;
    wificlient.setInsecure();
    String payload = "";
    Serial.println("Now attempting to connect through HTTPClient");

    if (httpclient.begin(wificlient, server1URL))
    {
        httpclient.addHeader("Host", "zg1emv3wlf.execute-api.us-east-1.amazonaws.com");
        httpclient.addHeader("content-type", "application/json");
        httpclient.addHeader("Content-Length", String(sizeof(httpData)).c_str());
        int response = httpclient.POST(httpData);
        payload += httpclient.getString();
        Serial.printf("Response obtained from server: %d\n", response);
        httpclient.end();
        Serial.println(payload);
    }
    else
    {
        Serial.println("Unable to connect using httpclient");
    }
    return payload;
}