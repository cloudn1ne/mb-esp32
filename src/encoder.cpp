#include <Arduino.h>
#include "encoder.h"
#include "ble_uuid.h"

// #define DEBUG 1

// called when we find BLE devices in search for the real KilterBoard
MyAdvertisedDeviceCallbacks::MyAdvertisedDeviceCallbacks(std::string targetBoardName, bool *readyToConnectFlag)
{    
    ready_to_connect_flag = readyToConnectFlag;
    board_name = targetBoardName;    
}


void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice) 
{
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    if (advertisedDevice.getName() == board_name) 
    { //Check if the name of the advertiser matches
	  	advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
		address = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
		*ready_to_connect_flag = true; //Set indicator, stating that we are ready to connect
		Serial.printf("Device %s found. Connecting!\n", board_name.c_str());
	}
}

BLEAddress* MyAdvertisedDeviceCallbacks::getAddress()
{
    return(address);
}

KilterEncoder::KilterEncoder(std::string boardName, uint8_t max_per_packet=20)
{          
    // char name[] = "Fake Kilter Board@3";

    board_name = boardName;        
    Serial.printf("KilterEncoder::KilterEncoder - Starting to scan for %s ...\n", board_name.c_str());            
    cb = new MyAdvertisedDeviceCallbacks(board_name, &readyToConnectFlag);

    pClient = BLEDevice::createClient();
    isConnectedFlag = false;    
    readyToConnectFlag = false;
    readyToReconnectFlag = false;
    pServerAddress = nullptr;
	pBLEScan = BLEDevice::getScan(); //create new scan
	/// pBLEScan->setAdvertisedDeviceCallbacks(cb);
	pBLEScan->setActiveScan(false); //active scan will prevent WIFI AP from working
    pBLEScan->setInterval(1000);
	pBLEScan->setWindow(100);  // less or equal setInterval value    
    maxHoldsPerPacket = max_per_packet;    
    numHolds = 0;
    state="init";
}

/// @brief Needs to be called regularly to connect to the discovered KilterBoard
///        
void KilterEncoder::process(AsyncWebSocket *ws)
{   
    //Serial.println("process start");
    if (!pClient->isConnected() && readyToReconnectFlag)
    {
        Serial.println("process - reconnecting");
        Serial.println("========= RECONNECTING ==========");
        readyToConnectFlag = true;
        state="reconnecting";
    }
    if (!isConnectedFlag)
	{		
        Serial.printf("process - scanning for %s\n", board_name.c_str());
        state="scanning";
        int scanTime = 3; //In seconds
		BLEScanResults foundDevices = pBLEScan->start(scanTime, true);	
        int deviceCount = foundDevices.getCount();        
        for (uint32_t i = 0; i < deviceCount; i++)
        {
            BLEAdvertisedDevice device = foundDevices.getDevice(i);
            Serial.printf("xDevice %s\n", device.toString().c_str());            
            if (device.getServiceUUID().equals(BLEUUID(ADVERTISING_SERVICE_UUID)))
             { //Check if the name of the advertiser begins with board_name                  
                    pServerAddress = new BLEAddress(device.getAddress()); //Address of advertiser is the one we need
                    readyToConnectFlag = true; //Set indicator, stating that we are ready to connect
                    Serial.printf("KilterBoard Device found. Connecting!\n");
             }            
        }
		pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
		pBLEScan->stop();
        Serial.println("process - scanning completed");
	}
    if (readyToConnectFlag) {    
        Serial.println("process - ready to connect");   
		if (connectToKilterBoard()) 
        {
            Serial.println("process - connected");
            state="connected";
            readyToReconnectFlag = true;
			Serial.printf("KilterEncoder::process - We are now connected to %s\n", board_name.c_str());			
			isConnectedFlag = true;
            if (numHolds > 0)   
            {
                sendHolds();
            }
		} 
		else {
            Serial.println("process - failed");
            state="failed";
			Serial.printf("ERR KilterEncoder::process - FAILED to connect to %s\n", board_name.c_str());
            isConnectedFlag = false;
		}
    	readyToConnectFlag = false;
    }
   // Serial.println("process end");
}

String KilterEncoder::getConnectionState()
{
    return state;
}

void KilterEncoder::resetHolds()
{
    numHolds = 0;
    memset(holds, 0, MAX_HOLDS);    
    memset(colors, 0, MAX_HOLDS);    
}

/// @brief Tag hold
/// @param holdNumber hold number to tag
/// @param holdColor  hold color to set (kilter format RRRGGGBB encoding)
void KilterEncoder::setHold(uint16_t holdNumber, uint8_t holdColor)
{
    if (holdNumber < MAX_HOLDS)
    {
        holds[numHolds] = holdNumber;
        colors[numHolds] = holdColor;
        numHolds++;
    }
    else
    {
        Serial.printf("ERR KilterEncoder::setHold - invalid holdNumber #%d\n", holdNumber);
    }
}

void KilterEncoder::setBLECharacteristic(BLERemoteCharacteristic *c)
{
    bleCharacteristic = c;
}

/// @brief Publish current hold/color data to Kilter Board
void KilterEncoder::sendHolds()
{
#ifdef DEBUG    
    Serial.printf("KilterEncoder::sendHolds - sending %d holds\n", numHolds);
#endif    
    if (numHolds > maxHoldsPerPacket)
    {
        sendPacket(82, 0, maxHoldsPerPacket);   // first packet
        uint16_t i;
        for (i=maxHoldsPerPacket;i<numHolds-maxHoldsPerPacket;i=i+maxHoldsPerPacket)
        {
            sendPacket(81, i, i+maxHoldsPerPacket);   // middle packet
        }
        sendPacket(83, i, numHolds);   // last packet
    }
    else
    {
        sendPacket(84, 0, numHolds);   // single packet             
    }
}

/// @brief Send a single BLE packet to the Kilter Board
/// @param type packet type, 84=single, 82=first, 83=last, 81=middle
/// @param start_hold_idx first hold to include in packet
/// @param end_hold_idx  last hold to include in packet
void KilterEncoder::sendPacket(uint8_t type, uint16_t start_hold_idx, uint16_t end_hold_idx)
{  
    uint8_t payload[MAX_PACKET_SIZE];
    uint8_t payload_idx;
    int8_t crc = 0;

    
    // header    
    payload[0] = 1;                                     // always 1
    payload[1] = 1+3*(end_hold_idx-start_hold_idx);     // length
    //payload[2]                                        // CRC set at the end
    payload[3] = 2;                                     // always 2


    // assemble payload
    payload[4] = type;    
    payload_idx = 5;
    for (uint16_t i=start_hold_idx;i<end_hold_idx;i++)
    {                
        payload[payload_idx++] = holds[i]&0xFF;
        payload[payload_idx++] = holds[i]>>8;
        payload[payload_idx++] = colors[i];                
    }

    // calc payload crc
    for (uint16_t i=4;i<payload_idx;i++)
    {
        crc = (crc+payload[i]) & 255;
    }
    crc = ~crc & 0xFF;
    payload[2] = crc;

    // add tail
    payload[payload_idx++] = 3;

    // debug
#ifdef DEBUG    
    Serial.printf("KilterEncoder::sendPacket - [#%d to #%d] ", start_hold_idx, end_hold_idx-1);
    for (uint8_t i=0;i<payload_idx;i++)
    {
        Serial.printf("%d ", payload[i]);
    }
    Serial.println("");
#endif

    // send
    bleCharacteristic->writeValue(payload, payload_idx, false);
}

/// @brief Test if Encoder is connected to a KilterBoard
/// @return true if connected
bool KilterEncoder::isConnected()
{
    return(isConnectedFlag);
}

bool KilterEncoder::connectToKilterBoard() 
{                    
    // Connect to the remote BLE Server.
    // BLEAddress* addr = cb->getAddress();    
    // pClient->connect(*addr, BLE_ADDR_TYPE_RANDOM);

    pClient->connect(*pServerAddress);
    
    Serial.print("KilterEncoder::connectToKilterBoard - Number of services:  "); 
    Serial.println(String(pClient->getServices()->size()));

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(BLEUUID(DATA_TRANSFER_SERVICE_UUID));
    if (pRemoteService == nullptr) 
    {
        Serial.printf("ERR KilterEncoder::connectToKilterBoard - Failed to find our service UUID: %s\n",DATA_TRANSFER_SERVICE_UUID);        
        return(false);
    }
    else
    {
        Serial.printf("KilterEncoder::connectToKilterBoard - Found %s Service data transfer service UUID %s\n", board_name.c_str(), DATA_TRANSFER_SERVICE_UUID);
    }
    
    // Obtain a reference to the characteristics in the service of the remote BLE server.
    bleCharacteristic = pRemoteService->getCharacteristic(BLEUUID(DATA_TRANSFER_CHARACTERISTIC));
    if (bleCharacteristic == nullptr)
    {  
        Serial.println("ERROR KilterEncoder::connectToKilterBoard - Error attaching to Data Transfer Characteristic");
        return(false);
    }
    else
    {	
        Serial.println("KilterEncoder::connectToKilterBoard - Successfully attached to Data Transfer Characteristic, ready to communicate");
    }  
    return true;
}