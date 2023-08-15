#include <Arduino.h>
#include <Preferences.h>
// Captive Portal
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
// Tasks 
#include <TaskScheduler.h>
// KilterBoard Interface
#include "decoder.h"
#include "encoder.h"
#include "grid.h"
// plugins
#include "colorswapper.h"
#include "idleframe.h"
// settings 
#include "settings.h"


#define VERSION "1.1"

// Aurora API level. must be nonzero, positive, single-digit integer.
// API level 3+ uses a different protocol than API levels 1 and 2 and below.
#define API_LEVEL 3

// number of holds per packet sent to KilterBoard
#define MAX_PER_PACKET 4

#define SSID "MonkeyBoard"

void tx_task();


KilterDecoder *rx_decoder;
KilterEncoder *tx_encoder;
KilterGrid *grid = new KilterGrid(HOMEWALL_COLS, HOMEWALL_ROWS);
Preferences prefs;

// Tasks
Scheduler runner;
Task txTask(250, -1, &tx_task);

// plugins
ColorSwapper *color_swapper;
// last tgt connection state (used to update websocket)
String lastConnectionState;

// IdleFrame
bool showIdleFrameAnimation = true;

/* Start Webserver */
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const IPAddress apIP(192, 168, 2, 1);
const IPAddress gateway(192, 168, 2, 1);
const IPAddress subnet(255, 255, 255, 0);

// funcs

void sendConnectionState(AsyncWebSocket *ws);

/*
void setBLEPowerMax()
{
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_DEFAULT);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL0);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL1);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL2);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL3);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL4);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL5);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL6);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL7);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL8);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_ADV);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_SCAN);
}
*/

void tx_task()
{			
	if (tx_encoder->isConnected())	
	{
		if (setting_ShowIdleFrame && showIdleFrameAnimation)	// show idle frame animation until we received first good data
		{
			txTask.setInterval(300);
			IdleFrame(tx_encoder);		
		}				
		tx_encoder->sendHolds();
	}	
	sendConnectionState(&ws);
}

void redirectToIndex(AsyncWebServerRequest *request)
{
#ifdef CAPTIVE_DOMAIN
  request->redirect(CAPTIVE_DOMAIN);
#else
  request->redirect("http://" + apIP.toString());
#endif
}

/// @brief WebSocket Eventhandler
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
	
	if(type == WS_EVT_DATA)
	{
		AwsFrameInfo *info = (AwsFrameInfo*)arg;	
		if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
		{        
			// DynamicJsonDocument json(128);
			StaticJsonDocument<256> json;
			DeserializationError err = deserializeJson(json, data);
			if (err) {
				Serial.print(F("deserializeJson() failed with code "));
				Serial.println(err.c_str());
				return;
			}
			if (json["name"] == "idle")
			{
				setting_ShowIdleFrame = json["value"].as<const bool>();
			}
			if (json["name"] == "swpcol")
			{
				setting_SwapColors = json["value"].as<const bool>();
				color_swapper->toggle(setting_SwapColors);
			}
			if (json["name"] == "advname")
			{				
				setting_AdvertisedBoardName = json["value"].as<const char*>();	
				Serial.printf("Setting new advname=%s, restarting\n", setting_AdvertisedBoardName);
				saveSettings(&prefs);				
				ESP.restart();
			}
			Serial.println(json["name"].as<const char*>());
			Serial.println(json["value"].as<const bool>()?"on":"off");			
			saveSettings(&prefs);
		}

    }	
	else if(type == WS_EVT_CONNECT)
	{ 
    	Serial.println("Websocket client connection received, sending settings");
		sendSettings(&ws);
		lastConnectionState="";
		sendConnectionState(&ws);
  	} else if(type == WS_EVT_DISCONNECT)
	{
    	// Serial.println("Client disconnected"); 
  	}
}

void setup() {
	Serial.begin(115200);
	loadSettings(&prefs);
	//saveSettings(&prefs);

	printf("\nMonkeyBoard Gateway v%s\n", VERSION);
	
	// generate our boardnames for a level 3 API (@3)
	// target boardnames must always end in "Kilter Board@3" otherwise the App will not discover them
	char advertisedBoardName[128];
	snprintf(advertisedBoardName, sizeof(advertisedBoardName), "%s%s%d", setting_AdvertisedBoardName.c_str(), "@", API_LEVEL);	
    Serial.printf("Advertised Boardname: %s\n", advertisedBoardName);

    Serial.printf("Target Boardname: %s\n", setting_TargetBoardName.c_str());

	// setBLEPowerMax();
	rx_decoder = new KilterDecoder(advertisedBoardName);	
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


	// WebServer
  	server.serveStatic("/", LittleFS, "/www/")		
    	  .setDefaultFile("index.html");
	
	
  	// Captive portal to keep the client
  	server.onNotFound(redirectToIndex);

	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
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


void notifyClients() {
	StaticJsonDocument<2048> doc;
    
	JsonArray leds = doc.createNestedArray("leds");
	for (uint16_t i=0; i<rx_decoder->getNumHolds(); i++)
	{
		JsonObject hold = leds.createNestedObject();
		uint16_t holdnum = rx_decoder->getHold(i);
		hold["x"] = grid->getX(holdnum);
		hold["y"] = grid->getY(holdnum);
		hold["c"] = KilterColorToRGB(color_swapper->swap(rx_decoder->getColor(i)));	
	}	
    char data[2048];
    size_t len = serializeJson(doc, data);
    ws.textAll(data, len);
}

/// @brief Publish Connection state to WebClient
/// @param ws 
void sendConnectionState(AsyncWebSocket *ws)
{	
	String connectionState = tx_encoder->getConnectionState();

	if (connectionState != lastConnectionState)
	{
		StaticJsonDocument<128> doc;
			
		JsonObject cfg = doc.createNestedObject("conn");
		cfg["connected"] = (connectionState=="connected");
		cfg["scanning"] = (connectionState=="scanning");
		char data[128];
		size_t len = serializeJson(doc, data);
		ws->textAll(data, len);		
		lastConnectionState	= connectionState;
	}
}

void loop() 
{		
	runner.execute();
	rx_decoder->process();
	tx_encoder->process(&ws);
	
	if (tx_encoder->isConnected())
	{				
		if (rx_decoder->isComplete())
		{			
			showIdleFrameAnimation = false;
			txTask.setInterval(1000);

			Serial.printf("New HoldSetup received - sending to KilterBoard\n\n");
			tx_encoder->resetHolds();
			for (uint16_t i=0; i<rx_decoder->getNumHolds(); i++)
			{
				Serial.printf(" * Tx Hold: #%d (%06x)\n", rx_decoder->getHold(i), KilterColorToRGB(rx_decoder->getColor(i)));				
				tx_encoder->setHold(rx_decoder->getHold(i), color_swapper->swap(rx_decoder->getColor(i)));								
			}
			notifyClients();
			rx_decoder->reset(); // ready to receive next message	
			txTask.enable(); // triggers a sendHolds();
			// tx_encoder->sendHolds();								
			// grid->show();			
		}		
	}
	else
	{
		txTask.disable();
	}	
}
