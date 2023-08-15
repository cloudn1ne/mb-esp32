#include<Arduino.h>
#include<Preferences.h>
#include "settings.h"

bool setting_SwapColors;
bool setting_ShowIdleFrame;
String setting_TargetBoardName;
String setting_AdvertisedBoardName;  

void loadSettings(Preferences *prefs)
{
    Serial.println("Loading settings");
    prefs->begin("mb-esp32", true);
	setting_SwapColors = prefs->getBool("swpcol", false);
    setting_ShowIdleFrame = prefs->getBool("idle", true);
    //setting_TargetBoardName = prefs->getString("tgtname", String("Kilter Board#751033@3"));
    setting_TargetBoardName = prefs->getString("tgtname", String("Kilter Board"));
    setting_AdvertisedBoardName = prefs->getString("advname", String("Monkey Kilter Board"));    
    prefs->end();

    Serial.printf("\tSwapColors: %s\n", setting_SwapColors?"yes":"no");
    Serial.printf("\tShowIdleFrame: %s\n", setting_ShowIdleFrame?"yes":"no");
    Serial.printf("\tTargetBoardName: %s\n", setting_TargetBoardName.c_str());
    Serial.printf("\tAdvertisedBoardName: %s\n", setting_AdvertisedBoardName.c_str());
}

void saveSettings(Preferences *prefs)
{
    Serial.println("Saving settings");
    prefs->begin("mb-esp32", false);
    // prefs->clear();
	prefs->putBool("swpcol", setting_SwapColors);
    prefs->putBool("idle", setting_ShowIdleFrame);
    prefs->putString("tgtname", setting_TargetBoardName);
    prefs->putString("advname", setting_AdvertisedBoardName);
    prefs->end();
}

void sendSettings(AsyncWebSocket *ws)
{
    StaticJsonDocument<2048> doc;
    
	JsonObject cfg = doc.createNestedObject("settings");
    cfg["swpcol"] = setting_SwapColors;
    cfg["idle"] = setting_ShowIdleFrame;
    cfg["tgtname"] = setting_TargetBoardName;
    cfg["advname"] = setting_AdvertisedBoardName;
    char data[2048];
    size_t len = serializeJson(doc, data);
    ws->textAll(data, len);
}