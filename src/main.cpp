#include <Arduino.h>
#include <Preferences.h>
// Captive Portal
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <BLEDevice.h>
// Tasks 
#include <TaskScheduler.h>
// KilterBoard Interface
#include "encoder.h"
#include "grid.h"
// plugins
#include "colorswapper.h"
#include "idleframe.h"
// settings 
#include "settings.h"
// REST API
#include "restapi.h"


#define VERSION "2.0"

// number of holds per packet sent to KilterBoard
#define MAX_PER_PACKET 4

#define SSID "MonkeyBoard"

void tx_task();


KilterEncoder *tx_encoder;
KilterGrid *grid = new KilterGrid(HOMEWALL_COLS, HOMEWALL_ROWS);
Preferences prefs;

// Tasks
Scheduler runner;
Task txTask(250, -1, &tx_task);

// plugins
ColorSwapper *color_swapper;
// last tgt connection state
String lastConnectionState;

// IdleFrame
bool showIdleFrameAnimation = true;

/* Start Webserver */
AsyncWebServer server(80);

const IPAddress apIP(192, 168, 2, 1);
const IPAddress gateway(192, 168, 2, 1);
const IPAddress subnet(255, 255, 255, 0);

void tx_task()
{			
	if (tx_encoder->isConnected())	
	{
		if (setting_ShowIdleFrame && showIdleFrameAnimation)	// show idle frame animation until we received first good data
		{
			txTask.setInterval(300);
			IdleFrame(tx_encoder);		
		}
		else
		{
			txTask.setInterval(1000);
		}
		tx_encoder->sendHolds();
	}	
}

void redirectToIndex(AsyncWebServerRequest *request)
{
#ifdef CAPTIVE_DOMAIN
  request->redirect(CAPTIVE_DOMAIN);
#else
  request->redirect("http://" + apIP.toString());
#endif
}

void setup() {
	Serial.begin(115200);
	loadSettings(&prefs);
	
	printf("\nMonkeyBoard Gateway v%s\n", VERSION);
	
    Serial.printf("Target Boardname: %s\n", setting_TargetBoardName.c_str());

    // initialize the BLE stack (used by the KilterEncoder client below)
    BLEDevice::init(setting_TargetBoardName.c_str());

	tx_encoder = new KilterEncoder(setting_TargetBoardName.c_str(), MAX_PER_PACKET);
	
	/* Connect WiFi */	
  	WiFi.mode(WIFI_AP);
#ifndef PASSWORD
  	WiFi.softAP(SSID);
#else
  	WiFi.softAP(SSID, PASSWORD);
#endif
  	WiFi.softAPConfig(apIP, gateway, subnet);
  	// dnsServer.start(DNS_PORT, "*", apIP);
	
  	Serial.println("\nWiFi AP is now running\nIP address: ");
  	Serial.println(WiFi.softAPIP());

	// Mount LittleFS
	if (!LittleFS.begin())
  	{
    	Serial.println("An Error has occurred while mounting LittleFS");
    	return;
  	}


	// REST API - must be registered BEFORE the static / handler so /api/* is not shadowed
	setupRestApi(&server);

	// WebServer - serve the mb-esp32-web app from LittleFS
  	server.serveStatic("/", LittleFS, "/www/")		
  	  .setDefaultFile("index.html");
	
  	// Captive portal to keep the client
  	server.onNotFound(redirectToIndex);

	// REST API
	setupRestApi(&server);
	server.begin();		

 	
	// plugins
	color_swapper = new ColorSwapper();
	color_swapper->toggle(setting_SwapColors);
	color_swapper->setStartHold(0xFF0000);
	color_swapper->setFootHold(0xFFFF00);

	// Scheduled Tasks
	runner.init();	
	runner.addTask(txTask);
	txTask.enable();	
}


void loop() 
{		
	runner.execute();
	tx_encoder->process();
	
	if (!tx_encoder->isConnected())
	{
		txTask.disable();
	}	
	else
	{
		txTask.enable();
	}
}