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

#define VERSION "1.1"

// The name that will come up in the list in the kilter board app.
// Must be alphanumeric.
#define DISPLAY_NAME "Monkey Kilter Board"

// The name of that real Kilter Board that we will connect too.
#define TARGET_DISPLAY_NAME "Kilter Board#751033@3"

// Aurora API level. must be nonzero, positive, single-digit integer.
// API level 3+ uses a different protocol than API levels 1 and 2 and below.
#define API_LEVEL 3

// number of holds per packet sent to KilterBoard
#define MAX_PER_PACKET 4

#define SSID "MonkeyBoard"

void tx_task();

Preferences prefs;
KilterDecoder *rx_decoder;
KilterEncoder *tx_encoder;
KilterGrid *grid = new KilterGrid(HOMEWALL_COLS, HOMEWALL_ROWS);

// Tasks
Scheduler runner;
Task txTask(250, -1, &tx_task);

// plugins
ColorSwapper *color_swapper;
bool swapColorFlag = true;

// IdleFrame
bool showIdleFrameAnimation = true;

/* Start Webserver */
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const IPAddress apIP(192, 168, 2, 1);
const IPAddress gateway(192, 168, 2, 1);
const IPAddress subnet(255, 255, 255, 0);

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
		if (showIdleFrameAnimation)	// show idle frame animation until we received first good data
		{
			txTask.setInterval(300);
			IdleFrame(tx_encoder);		
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

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
	AwsFrameInfo *info = (AwsFrameInfo*)arg;	
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        
        DynamicJsonDocument json(128);
        DeserializationError err = deserializeJson(json, data);
        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }
		Serial.println(json["name"].as<const char*>());
		Serial.println(json["value"].as<const bool>()?"on":"off");
/*
        const char *val = json["val"];
        if (strcmp(val, "true") == 0) {
            Serial.printf("val = %s", val);
        }
  */

    }	
	else if(type == WS_EVT_CONNECT)
	{ 
    	Serial.println("Websocket client connection received");
  	} else if(type == WS_EVT_DISCONNECT)
	{
    	Serial.println("Client disconnected"); 
  	}
}

void setup() {
	Serial.begin(115200);
	printf("MonkeyBoard Gateway v%s", VERSION);

	prefs.begin("mb-esp32", false);
	prefs.putString("adv_boardname", "Monkey "); 
	prefs.putString("tgt_boardname", ""); 

	// generate our boardnames for a level 3 API (@3)
	// target boardnames must always end in "Kilter Board@3" otherwise the App will not discover them
	char advertisedBoardName[128];
	snprintf(advertisedBoardName, sizeof(advertisedBoardName), "%s%s%d", prefs.getString("adv_boardname").c_str(), "Kilter Board@", API_LEVEL);
    printf("\nAdvertised Boardname: %s\n\n", advertisedBoardName);

	//char targetBoardName[128]="Kilter Board#751033@3";
	char targetBoardName[128]="Kilter Board@3";
	//snprintf(targetBoardName, sizeof(targetBoardName), "%s%s%d", prefs.getString("tgt_boardname").c_str(), "Kilter Board@", API_LEVEL);

    printf("\nTarget Boardname: %s\n\n", targetBoardName);
	prefs.end();

	// setBLEPowerMax();
	rx_decoder = new KilterDecoder(advertisedBoardName);	
	tx_encoder = new KilterEncoder(targetBoardName, MAX_PER_PACKET);
	
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
		hold["h"] = rx_decoder->getHold(i);
		hold["c"] = rx_decoder->getColor(i);
	}	
    char data[2048];
    size_t len = serializeJson(doc, data);
    ws.textAll(data, len);
}

void loop() 
{		
	runner.execute();
	rx_decoder->process();
	tx_encoder->process();
	
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