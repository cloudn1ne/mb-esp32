#ifndef settings_h
#define settings_h
//
// settings.h
//
// Variables for all settings that can be modified via the web(sockets)
//
#include<Preferences.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

extern bool setting_SwapColors;
extern bool setting_ShowIdleFrame;
extern String setting_TargetBoardName;
extern String setting_AdvertisedBoardName;  

// funcs
void loadSettings(Preferences *prefs);
void saveSettings(Preferences *prefs);
void sendSettings(AsyncWebSocket *ws);

#endif