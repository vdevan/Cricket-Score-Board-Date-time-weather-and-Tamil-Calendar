//Root - Portal
void handleRoot()
{
    Serial.println("Server Root Handler");
    String Page = GetHeadPage();
    Page += ("<h4>Available Networks</h4><div id=\"scnnet\">");
    
    int n = WiFi.scanNetworks(); //WIFI_AUTH_OPEN

    if (n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            Page += String(F("<div class=\"inf\"><a href=\"#p\" class=\"n\">")) + WiFi.SSID(i) + F("</a>&nbsp;<span class=\"q") + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? F("\">") : F(" l\">")) + abs(WiFi.RSSI(i)) + " %";
            Page += F("</span></div>");
        }
    }
    else
        Page += F("<div class=\"inf\"><p>No WLAN found</p>");

    Page += F("</div>"); //Closing Div for wifi Networks info

    //Start of Form and SSID, Password fields
    Page += F("<form method=\"POST\" action=\"wifisave\"><table>");
    if (storedNetworks >= MAXNETWORK)
    {
      Page += F("<tr><td colspan=\"2\"><h4 style=\"color:red;\">Please DELETE a Network to store new one</h4><td></tr>");
    }
    else
    {
        Page += F("<tr><td><span><label>SSID: </label><br /><input id=\"s\" class=\"txt\" name=\"s\" length=32 placeholder=\"SSID\"></span></td>"
                  "<td><span><label>Password: </label><br /><input id=\"p\" class=\"txt\" name=\"p\" length=32 placeholder=\"password\"></span></td></tr>");
    }
    Page += F("<tr><td><p>&nbsp;</p></td></tr>");
    Page += F("<tr>");

    Page += F("<td><span><input type=\"checkbox\" name=\"as\" class=\"am\" ");
    Page += header.bScore ==1  ? "checked=\"checked\"" : "";
    Page += F("><label>Scoreboard</label></span></td>");

    Page += F("<td><span><label>Game Id: </label><input id=\"stz\" class=\"dst\" name=\"stz\" length=20 value=\"");
    Page += String(header.GameId);
    Page += F("\"></span></td></tr>");
    
    Page += F("<tr>");
    Page += F("<td><span><input type=\"checkbox\" name=\"afp\" class=\"am\" ");
    Page += header.bPropFont ==1  ? "checked=\"checked\"" : "";
    Page += F("><label>Use Proportional Font</label></span></td>");


    Page += F("</tr></table>");
    Page += GetSubmitPage();
    

    Page += F("<h4>Stored Networks</h4><div id=\"strnet\"><table>");

    //Stored Network contents 
    for (int i = 0; i < storedNetworks; i++)
    {
        Page += F("<tr><td class=\"stbtn\"><label>");
        Page += Networks[i].SSID;
        Page += F("</label></td><td class=\"stbtn\"><button id=\"btn\" class=\"del\" name=\"del\" value=\"");
        Page += Networks[i].SSID;
        Page += F("\" type=\"Submit\">Delete</button></td></tr>");
    }
    Page += F("</table></div>");

    Page += F("<p>&nbsp;</p></form></div></body></html>");
    server.send(200, "text/html", Page); //Send the page to client
}

//WiFi Save
void handleWifiSave()
{
    Serial.printf("Server Argument 2: %s\n",server.argName(2));

    String st;
    int offset = 0;
    if (server.hasArg("reset"))
    {
      sendResponse();
      Serial.println("Reset Request Initiated");
      ESP.restart();
      return;
    }

    if (server.hasArg("del")) //handle delete
    {
        st = server.arg("del");
        st.trim();
        Serial.printf("Delete Arg: %s\n",st);
        int j;
        for (j=0; j < storedNetworks; j++)
        {
            if (st == Networks[j].SSID)
              break;
        }
        Serial.printf("Index to delete found: %d\n",j);
        // J has the index for deleting. Let us zero it first
        memset(Networks[j].SSID, '\0', sizeof(Networks[j].SSID));
        memset(Networks[j].Password, '\0', sizeof(Networks[j].Password));
        
        //Rearrange storage
        for (int i = j; i < MAXNETWORK -1; i++)
        {
            memcpy(Networks[i].SSID, Networks[i+1].SSID, sizeof(Networks[i+1].SSID));
            memcpy(Networks[i].Password, Networks[i+1].Password, sizeof(Networks[i+1].Password));
        }
        //Erase the last storage
        memset(Networks[MAXNETWORK -1].SSID, '\0', sizeof(Networks[MAXNETWORK -1].SSID));
        memset(Networks[MAXNETWORK -1].Password, '\0', sizeof(Networks[MAXNETWORK -1].Password));
       
      }
      else
      {
        Serial.println("Save & Restart Request Initiated");
        header.bScore = (server.arg("as") == "on");
        header.bPropFont = (server.arg("afp") == "on");
        server.arg("stz").toCharArray(header.GameId, sizeof(header.GameId));
        Serial.printf("GameID: from HTML: %s\n", server.arg("stz").c_str());
        Serial.printf("GameID: from Header: %s\n", header.GameId);
        Serial.printf("Score: from HTML: %s\n", server.arg("as").c_str());
        Serial.printf("Score: from Header: %d\n", header.bScore);

        if (header.GameId == "")
            String(GAMEID).toCharArray(header.GameId, sizeof(header.GameId));
        st = server.arg("s");
        st.trim();
        if (st != "")
        {
            //handle network storage
            //Pick the first empty storage
            for (int i = 0; i < MAXNETWORK; i++)
            {
                if (Networks[i].SSID[0] == 0)
                {
                    offset = i;
                    break;
                }
            }
            server.arg("s").toCharArray(Networks[offset].SSID, sizeof(Networks[offset].SSID));
            server.arg("p").toCharArray(Networks[offset].Password, sizeof(Networks[offset].Password));
            Serial.printf("SSID: %s  Password: %s\n",server.arg("s").c_str(),server.arg("p").c_str());
            Serial.printf("Stored at Location %d\n",offset);

            
        }
      }
      sendResponse();

      //Serial.printf("storage offset to be stored: %d\n Original stored SSID length: %d\n", offset, strlen(Networks[offset].SSID));
      //storeEpoch(server.arg("save"));

      saveCredentials();    
}

void handleNotFound()
{
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void sendResponse()
{
        server.sendHeader("Location", "/", true);
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.send(302, "text/plain", "");    
        server.client().stop(); // Stop is needed because we sent no content length
}

//Common Header page for Network and Preference
String GetHeadPage()
{
    String HTTP_HEAD;

    HTTP_HEAD = F("<!DOCTYPE html5><html lang=\"en\"><head>"
                    "<meta charset=\"utf-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\" />"
                    "<script src=\"https://code.jquery.com/jquery-2.1.1.min.js\" type=\"text/javascript\"></script>"
                    "<script>$(document).ready(function(){var d = new Date();var tz = d.getTimezoneOffset();var dt=(Date.now()).toString()+\"t\" + tz.toString();$(\".n\").click(function(){document.getElementById('s').value = this.innerText;$(\"#s\").focus();});"
                    "$(\".dt\").click(function(){$(\".dst\").prop(\"disabled\", !this.checked);});"
                    "$(\".sav\").click(function () {var d = new Date();var tz = d.getTimezoneOffset();$(\".sav\").val((Date.now()).toString() + \"t\" + tz.toString());});"
                    "$.get(\"time\",{[\"time\"]: dt}, function (result) {$(\"#retVal\").text(result);}); });</script>"
                    "<script>$(window).load(function () {var d = new Date();var tz = d.getTimezoneOffset(); $(\".pl\").val(tz < 0 ? \"+\" : \"-\"); $(\".tz\").val(tz < 0 ? 0 - tz / 60 : tz / 60);var time=d.getHours()+\":\"+d.getMinutes()+\":\"+d.getSeconds(); }); </script>"
                    "<title>Scoreboard</title>"); 

    HTTP_HEAD += F("<style>body {background-color: #b4e0b4;}"
                    "h4 {margin:2px;} .container {margin: auto;width: 90%;min-width: 340px;background-color: #b5eafe;padding: 0px 10px;}"
                    "@media(min-width:1200px){.container {margin: auto;width: 30%;}}@media(min-width:768px) and (max-width:1200px) {.container {margin: auto;width: 40%;}}"
                    ".q {float: right;width: 64px;text-align: right;} #t{margin:2px;width:50px;padding:5px 0px;text-align:center;}"
                    ".l {background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==') no-repeat left center; background-size: 1em;}"
                    "#scnnet{background-color: beige;padding: 5px;margin-bottom:5px;}"
                    "#strnet{background-color: bisque;padding: 5px;}"
                    ".inf {padding: 2px;}"
                    "table {width:95%;margin: auto; border - collapse: collapse;}"
                    "td.stbtn{border: 1px solid #808080;padding: 2px 10px;width:90%;}"
                    "#btn{border-radius: 4px;border: 0;color: red;cursor: pointer;display: inline-block;margin: 2px;padding: 5px;position: relative;background-color: beige;box-shadow: 0px 6px 5px -4px rgba(0,0,0,0.52);border: 0;}"
                    "#btn:hover {background: #b4e0b4;}"
                    "#btn:active, #btn:focus{background: #b5eafe;}"
                    "#btn:disabled {background-color:rgba(100,100,100,0.1);cursor:not-allowed;}"
                    ".txt{padding: 5px;width: 140px;}#xt {padding: 5px;width:90px;}</style></head>");

    HTTP_HEAD += F("<body><div class=\"container\"><h2>WiFi Settings</h2><p>You are connected to the device: <b>");
    HTTP_HEAD += ESPSSID;
    HTTP_HEAD += F("</b><br /><span id=\"retVal\"></span></p>");  //<br />Current Time: <span id=\"retVal\"></span> - Not used. Useless script still in though
    return HTTP_HEAD;
}

String GetSubmitPage()
{
    String HTTP_SUBMIT;
    HTTP_SUBMIT = F("<div style=\"text-align:center\"><p><span>"
        "<button id = \"btn\" class = \"sav\" name = \"save\" type = \"Submit\">Save and Restart</button>&nbsp;"
        "<button id = \"btn\" class = \"sav\" name = \"reset\" type = \"submit\">Restart without Save</button>"
        "</span></p></div>");
  
    return HTTP_SUBMIT;

}
