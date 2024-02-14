
static bool GetScore()
{
    JSONVar myObj;
    bool bresult;
    String Request = "";

    String jsonArray;

    if (bUseScoreTestData)
        jsonArray = scoreData;
    else
        jsonArray = SecurePOSTRequest();


    //Serial.printf("Data Obtained from web: %s\n", jsonArray.c_str());
    if (jsonArray == "error")
    {
        bresult = false;
        //return false;
    }

    myObj = JSON.parse(jsonArray);

    if (JSON.typeof(myObj) == "undefined")
    {
        bresult = false;
        //return false; //Use the current data
    }

    //Set the scrolling text
    String w = myObj["notOutPlayers"][0]["name"];  //["result"]["winner"]["name"];
    Player1 = w.substring(0, 6);

    Player1Runs = myObj["notOutPlayers"][0]["runs"];
    Player1BallsFaced = myObj["notOutPlayers"][0]["ballsFaced"];

    String x = myObj["notOutPlayers"][1]["name"];  //["result"]["winner"]["name"];
    Player2 = x.substring(0, 6);
    Player2Runs = myObj["notOutPlayers"][1]["runs"];
    Player2BallsFaced = myObj["notOutPlayers"][1]["ballsFaced"];

    OversBowled = myObj["battingStats"]["overs"];
    TotalRuns = myObj["battingStats"]["total"];
    TargetRuns = myObj["battingStats"]["target"];
    Wickets = myObj["battingStats"]["wickets"];

    Serial.printf("Player1: %s Runs: %d Balls Faced: %d\n", Player1.c_str(), Player1Runs, Player1BallsFaced);
    Serial.printf("Player2: %s Runs: %d Balls Faced: %d\n", Player2.c_str(), Player2Runs, Player1BallsFaced);
    Serial.printf("Overs Bowled: %d, TotalRuns: %d, Wickets: %d, TargetRuns: %d\n", OversBowled, TotalRuns, Wickets, TargetRuns );

    /* Random updates. To be commented out. For testing only* /
    OversBowled = random(1, 30);
    Player1BallsFaced = random(0, OversBowled*2);
    Player1Runs = random(0, Player1BallsFaced*3);
    Player2BallsFaced = random(0, OversBowled * 2);
    Player2Runs = random(0, Player2BallsFaced*3);
    TotalRuns = random(0, 50) + Player1Runs + Player2Runs;
    TargetRuns = random(10, 200);
    if (TargetRuns > 110)
        TargetRuns = 0;
    Wickets = random(0, 10);
    /* End of Random Updates */

    return true;
}



