
/*********************************************************************
/* Following function will get the Gateway address, Location and time zone
 * Note time zone must be constructed from "country_name" / "city_name"
 * Location details are "latitude=-34.47&longitude=150.42&" 
 * Timezone = Australia/Sydney which are to 
 * be stored in Location and TimeZone Global variable 
 * This program is called at the start of the day and also once every day 
 * This program is the lifeline for clock. If there is no connection or
 * unable to get details, the program will revert back to server mode
 * Hence 5 attempts are made to make to the site
 * 
 * Site is free for personal usage. URL: https://api.ip2location.io/?key= + key obtained on registering
 * Result Obtained: {"ip":"122.151.141.222","country_code":"AU","country_name":"Australia",
 * "region_name":"New South Wales","city_name":"Sydney","latitude":-33.86778,"longitude":151.207052,
 * "zip_code":"2000","time_zone":"+11:00","asn":"9443","as":"Vocus Retail","is_proxy":false}
 * Changed on 9th Feb 2024 - Vasu
 *****************************************************************/
static bool GetLocation()
{
    Serial.printf("Getting Location URL: %s, Key= %s\n", LocationUri.c_str() , LocationKey.c_str());
    int count = 0;
    String jsonArray;
    JSONVar myObj;


    while (count < 5)
    {
        count++;
        jsonArray = GETRequest(LocationUri + LocationKey);
        myObj = JSON.parse(jsonArray);
        if (JSON.typeof(myObj) == "undefined")
        {
            continue;
        }

        Serial.printf("ip: %s\n", JSON.stringify(myObj["ip"]).c_str());
 
        if (JSON.stringify(myObj["ip"]) == "")
        {
            continue;
        }

        Location = "latitude=" + JSON.stringify(myObj["latitude"]); 
        Location += "&longitude=" + JSON.stringify(myObj["longitude"]) + String("&");
        Location.replace("\"", "");
        TimeZone = JSON.stringify(myObj["country_name"]) + "/" + JSON.stringify(myObj["city_name"]);
        TimeZone.replace("\"", "");
        WeatherUri = "";  //Reset Weather uri in case some things have changed
        break;

        delay(SECOND * 5); //Wait for 5 second and try again
    }
    Serial.printf("Location: %s, TimeZone: %s obtained\n",Location.c_str(), TimeZone.c_str());

    ClkTZ.setLocation(TimeZone);
    delay(SECOND * 2);
    if (count < 5)
        return true;
    else
        return false;

}
/***********************************************************************
 * Weather is called every 25 minutes This can be changed.
 * This function will construct WeatherURI before calling. So ensure Location and TimeZone
 * are properly filled. If not this will automatically call Get Location
 * Requirements for WeatherUri are:
 * WUri  + Location  + TimeZone + "&current=temperature_2m,rain,cloud_cover&hourly=temperature_2m,rain,
 * cloud_cover&daily=weather_code,temperature_2m_max,temperature_2m_min,rain_sum&forecast_days=1";
 * 
 * Modified on 9th February 2024 - Vasu
 **************************************************************************/
static bool GetWeather()
{
    Serial.printf("Weather URI in the system: %s\n",WeatherUri.c_str());
    if (Location == "" || TimeZone == "")
    {
        if (!GetLocation())
        {
            delay(timeDelay * 2);
            ESP.restart();
        }
    }

    if (WeatherUri == "")
    {
        WeatherUri =  WUri + Location + TimeZone;
        WeatherUri += "&current=temperature_2m,rain,cloud_cover&hourly=temperature_2m,rain,";
        WeatherUri += "cloud_cover&daily=weather_code,temperature_2m_max,temperature_2m_min,rain_sum&forecast_days=1"; 
    }

    Serial.printf("Weather URI that will be used is: %s\n", WeatherUri.c_str());
    String jsonArray = GETRequest(WeatherUri);

    JSONVar myObj = JSON.parse(jsonArray);

    if (JSON.typeof(myObj) != "undefined")
    {
        if (myObj["error"])
            return false;

        String units = " C"; //JSON.stringify(myObj["current_units"]["temperature_2m"]); //should give oC

        curTemp = JSON.stringify(myObj["current"]["temperature_2m"]);
        curTemp += units;
        curTemp.replace("\"", "");
        Serial.printf("Current Temperature: %s\n", curTemp.c_str());

        Weather = JSON.stringify(myObj["daily"]["temperature_2m_min"]);
        Weather += "/";
        Weather += JSON.stringify(myObj["daily"]["temperature_2m_max"]);
        Weather.replace("\"", "");
        Serial.printf("Weather now is: %s\n", Weather.c_str());

        
        Rain = myObj["daily"]["rain_sum"];
        Serial.printf("Rain now is: %d\n", Rain);

        Cloud = myObj["current"]["cloud_cover"];
        Serial.printf("Cloud Cover now is: %d\n", Cloud);

        Weather.replace("[", "");
        Weather.replace("]", "");

        bNetUpdate = !bNetUpdate;
        return true;
    }

    return false;
}