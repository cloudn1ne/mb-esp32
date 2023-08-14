#include <Arduino.h>
#include "decoder.h"
#include "ble_uuid.h"


// Callback when connection state changes, we then readvertise our BLE service to makes sure the app finds us
MyServerCallbacks::MyServerCallbacks(bool *restart_flag)
{
    restartAdvertising = restart_flag;
}
// When a device connects or disconnects the server will stop advertising, so we need to restart it to be discoverable again.
void MyServerCallbacks::onConnect(BLEServer* pServer) {
	*restartAdvertising = true;
}
void MyServerCallbacks::onDisconnect(BLEServer* pServer) {
	*restartAdvertising = true;
}


// Callback when we receive a message via BLE (from the Kilterboard App) - we then pump the data into a queue (mutex locked)
// which is evaluated and decoded by the KilterDecoder::process function
ReceiveCharacteristic::ReceiveCharacteristic(SemaphoreHandle_t mutex, QueueHandle_t queue)
{    
    m = mutex;
    q = queue;
}

void ReceiveCharacteristic::onWrite(BLECharacteristic* pCharacteristic) 
{		
	if(pCharacteristic->getUUID().toString() == BLEUUID(DATA_TRANSFER_CHARACTERISTIC).toString()) 
	{		
		for (uint16_t i=0; i<pCharacteristic->getValue().length(); i++)
		{		
            uint8_t val = pCharacteristic->getValue()[i];
			xSemaphoreTake(m, portMAX_DELAY); // enter critical section            
            xQueueSend(q, &val, portMAX_DELAY);
			xSemaphoreGive(m); // exit critical section				
		}			
	}
}

KilterDecoder::KilterDecoder(std::string boardName)
{       
    /* BLE Setup */    
    BLEDevice::init(boardName);	
    bleServer = BLEDevice::createServer();    
	bleServer->setCallbacks(new MyServerCallbacks(&restartAdvertising));
    restartAdvertising = false;

	BLEService* service = bleServer->createService(DATA_TRANSFER_SERVICE_UUID);
	BLECharacteristic* characteristic = service->createCharacteristic(DATA_TRANSFER_CHARACTERISTIC, BLECharacteristic::PROPERTY_WRITE);
	characteristic->setCallbacks(new ReceiveCharacteristic(rx_mutex, rx_queue));
	service->start();
	
	// Advertising service, this is how the app detects an Aurora board
	BLEService* advertisingService = bleServer->createService(ADVERTISING_SERVICE_UUID);
	advertisingService->start();        
	BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(ADVERTISING_SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections issue
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();

    /* Decoder Setup */
    allPacketsReceived = false;
    resetPacketBuffer();
    resetHolds();
}

void KilterDecoder::resetPacketBuffer()
{
    currentPacketLength = -1;
    currentPacketCRC = 0;    
    memset(currentPacket, 0, MAX_PACKET_SIZE);    
    currentPacketIdx = 0;    
}

void KilterDecoder::resetHolds()
{
    numHolds = 0;
    allPacketsReceived = false;
}

/// @brief Process any newly received data from the Kilterboard App
///        needs to be called continously from the main loop()
void KilterDecoder::process()
{
    uint8_t val;

    if (restartAdvertising) {
		delay(500); // Let the bluetooth hardware sort itself out
		restartAdvertising = false;
		bleServer->startAdvertising();
	}
    if( xQueueReceive( rx_queue, &val, 0 ) == pdPASS )
    {
         xSemaphoreTake(rx_mutex, portMAX_DELAY); // enter critical section
   		 newByteIn(val);			  
		 xSemaphoreGive(rx_mutex); // exit critical section		         
    }
}

/// @brief Add new byte to the decode buffer and attempt to decoded a valid packet (or multple)
/// @param b the new byte
void KilterDecoder::newByteIn(uint8_t b)
{        
        currentPacket[currentPacketIdx++] = b;        
        // Serial.printf("\n\nByte(%d/%d): %d\n", currentPacketIdx, currentPacketLength, b);

        
        if (allPacketsReceived)
        {
            allPacketsReceived = false;
            resetHolds();            
        }        
        if (currentPacket[0] != 1)
        {
            Serial.printf("ERR KilterDecoder::newByteIn - Invalid first byte, expected 1 but got %d\n", b);            
            resetPacketBuffer();
            return;
        }
                                      
        if (currentPacketIdx == 2) 
        {      
            currentPacketLength = b + 5;
            Serial.printf("KilterDecoder::newByteIn() - Length: %d\n", currentPacketLength);
        }
        else if (currentPacketIdx == 3) 
        {      
            currentPacketCRC = b;
            Serial.printf("KilterDecoder::newByteIn() - CRC: %d\n", currentPacketCRC);
        }
        else if (currentPacketIdx == currentPacketLength)
        {
            if (!verifyPacket())
            {
                Serial.printf("ERR KilterDecoder::newByteIn - CRC invalid\n");
                resetPacketBuffer();
            }
            else
            {
                allPacketsReceived = isThisTheLastPacket();
                Serial.printf("KilterDecoder::newByteIn() - isLastPacket?: %s\n", allPacketsReceived?"yes":"no");
            }
            // Serial.printf("KilterDecoder::newByteIn() - resetting\n");
            resetPacketBuffer();
        }
}    

bool KilterDecoder::verifyPacket()
{
        if (!checksumValid())
        {
            Serial.println("ERR KilterDecoder::verifyPacket - invalid checksum");
            return false;
        }
                
        for(uint8_t i = 5; i < currentPacketLength - 1; i += 3) {
            uint16_t position = (currentPacket[i + 1] << 8) + currentPacket[i];
            uint8_t color = currentPacket[i + 2];
            uint8_t r = (int)(((color & 0b11100000) >> 5) / 7. * 255.);
            uint8_t g = (int)(((color & 0b00011100) >> 2) / 7. * 255.);
            uint8_t b = (int)(((color & 0b00000011) >> 0) / 3. * 255.);
            Serial.printf("KilterDecoder::verifyPacket() - activating Hold: #%d (%02x%02x%02x)\n", position, r, g, b);
            holds[numHolds] = position;
            colors[numHolds] = color;
            numHolds++;
        }
        return true;        
}

bool KilterDecoder::checksumValid()
{
        uint8_t crc = 0;
        for (uint8_t i = 4;i<currentPacketIdx-1; i++) 
        {
            crc = (crc + currentPacket[i]) & 255;
        }
        crc = (~crc) & 255;        
        return (crc == currentPacketCRC);
}

bool KilterDecoder::isThisTheLastPacket() 
{    
       return (currentPacket[4] == 84 || currentPacket[4] == 83);    
}

bool KilterDecoder::isComplete()
{
    return(allPacketsReceived);
}

void KilterDecoder::reset()
{
    resetHolds();
}

uint16_t KilterDecoder::getNumHolds()
{
    return numHolds;
}

uint16_t KilterDecoder::getHold(uint16_t holdNumber)
{
    if (holdNumber < numHolds)
    {
        return holds[holdNumber];
    }
    else
    {
        Serial.printf("ERR KilterDecoder::getHold - Invalid hold number requested %d\n", holdNumber);            
        return(0);
    }
}

uint8_t KilterDecoder::getColor(uint16_t holdNumber)
{
    if (holdNumber < numHolds)
    {
        return colors[holdNumber];
    }
    else
    {
        Serial.printf("ERR KilterDecoder::getColor - Invalid hold number requested %d\n", holdNumber);           
        return(0); 
    }
}
