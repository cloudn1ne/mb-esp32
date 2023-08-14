// decoder.h
#ifndef decoder_h
#define decoder_h

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <CircularBuffer.h>

#define MAX_PACKET_SIZE 512
#define MAX_HOLDS 1024



class MyServerCallbacks: public BLEServerCallbacks {
    bool *restartAdvertising;

    public:
        MyServerCallbacks(bool *restart_flag);
        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);
};

class ReceiveCharacteristic : public BLECharacteristicCallbacks {	    
    SemaphoreHandle_t m;
    QueueHandle_t q;

    public:
        ReceiveCharacteristic(SemaphoreHandle_t mutex, QueueHandle_t queue);
	    void onWrite(BLECharacteristic* pCharacteristic);	
};


class KilterDecoder {
    private:    
        bool allPacketsReceived;
        int16_t currentPacketLength;        
        uint8_t currentPacketCRC;
        uint8_t currentPacket[MAX_PACKET_SIZE];
        uint16_t currentPacketIdx;
        uint16_t holds[MAX_HOLDS];
        uint8_t colors[MAX_HOLDS];
        uint16_t numHolds;

        // BLE
        BLEServer *bleServer = nullptr;
        bool restartAdvertising;     
        // Receive buffer and mutex            
        QueueHandle_t rx_queue = xQueueCreate(MAX_PACKET_SIZE,sizeof(uint8_t));
        SemaphoreHandle_t rx_mutex = xSemaphoreCreateMutex();

        void resetPacketBuffer();
        void resetHolds();
        bool verifyPacket();
        bool checksumValid();
        bool isThisTheLastPacket();
        void newByteIn(uint8_t b);        
        
    public:        
        KilterDecoder(std::string boardName);        
        void process();        
        bool isComplete();
        void reset();
        uint16_t getNumHolds();
        uint16_t getHold(uint16_t holdNumber);
        uint8_t getColor(uint16_t holdNumber);        
};


#endif