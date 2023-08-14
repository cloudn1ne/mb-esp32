# mb-esp32

## To Test

bleCharacteristic->writeValue(payload, payload_idx, true);  (*TESTED OK, but 4 LEDs only in one frame*)

vs

bleCharacteristic->writeValue(payload, payload_idx, false;


Real Kilter Board uses: pClient->connect(*addr, BLE_ADDR_TYPE_RANDOM); 