// encoder.h
#ifndef encoder_h
#define encoder_h

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define MAX_PACKET_SIZE 512
#define MAX_HOLDS 1024

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    private:
        BLEAddress* address;    
        bool *ready_to_connect_flag;    
        std::string board_name;

    public:
        MyAdvertisedDeviceCallbacks(std::string targetBoardName, bool *readyToConnectFlag);
        void onResult(BLEAdvertisedDevice advertisedDevice);            
        BLEAddress* getAddress();
};


class KilterEncoder {
    private:            
        uint16_t holds[MAX_HOLDS];
        uint8_t colors[MAX_HOLDS];
        uint16_t numHolds;        
        uint8_t maxHoldsPerPacket;        

        // BLE
        BLEScan* pBLEScan;
        BLERemoteCharacteristic *bleCharacteristic;                
        BLEAddress* pServerAddress;
        BLEClient* pClient;
        MyAdvertisedDeviceCallbacks *cb;
        bool readyToConnectFlag;
        bool readyToReconnectFlag;
        bool isConnectedFlag;
        std::string board_name;
        bool connectToKilterBoard();
        void sendPacket(uint8_t type, uint16_t start_hold_idx, uint16_t end_hold_idx);

    public:        
        KilterEncoder(std::string boardName, uint8_t max_per_packet);               
        void process(); 
        void resetHolds();
        void setHold(uint16_t holdNumber, uint8_t holdColor);                
        void setBLECharacteristic(BLERemoteCharacteristic *KilterBoardCharacteristic);
        void sendHolds();
        bool isConnected();
};

#endif