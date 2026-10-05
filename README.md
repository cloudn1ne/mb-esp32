# mb-esp32

MonkeyBoard Gateway v2 - BLE bridge between your browser and a real Kilter Board.

The ESP32 connects to the **real Kilter Board** over BLE (client role) and runs a
WiFi Access Point (`MonkeyBoard`, 192.168.2.1). It serves the **mb-esp32-web**
React app (from LittleFS) plus a **REST API**. Open `http://192.168.2.1` in any
browser, draw a problem, hit PUBLISH - the ESP32 sends the holds to the board.
No Bluetooth needed on the client side.

## REST API

| Method | Path | Description |
|--------|------|-------------|
| GET  | `/api/status`   | connection state, settings, hold count |
| GET  | `/api/grid`     | authoritative hold layout `[x, y, holdnum]` |
| GET  | `/api/holds`    | current hold buffer (`holdnum`, `x`, `y`, `r`, `g`, `b`) |
| POST | `/api/holds`    | replace + send holds: `{"holds":[{"holdnum":n,"r":..,"g":..,"b":..} \| {"x":..,"y":..,"r":..,"g":..,"b":..}]}` |
| DELETE | `/api/holds`  | clear the board |
| GET  | `/api/settings` | `swpcol`, `idle`, `advname`, `tgtname` |
| POST | `/api/settings` | update + persist: `{"swpcol":bool}`, `{"idle":bool}`, `{"advname":string}` |
| POST | `/api/idle`     | start/stop idle demo: `{"enable":bool}` |

Example:

```sh
curl -X POST http://192.168.2.1/api/holds \
  -H "Content-Type: application/json" \
  -d '{"holds":[{"holdnum":164,"r":255,"g":0,"b":0},{"x":3,"y":5,"r":255,"g":255,"b":0}]}'
```

## Building

```sh
# web app (React) -> builds and copies to data/www
cd web && ./deploy.sh

# firmware
pio run

# flash firmware + LittleFS
pio run -t upload && pio run -t uploadfs
```

## Layout

```
src/main.cpp        - WiFi AP, web server, task scheduler, gateway loop
src/encoder.cpp     - BLE client to the real Kilter Board + hold packet encoding
src/restapi.cpp     - REST endpoints (status/grid/holds/settings/idle)
src/grid.cpp        - 27x29 hold <-> x/y mapping (HomeWall 10x10)
src/colors.cpp      - Kilter 8bit color <-> RGB conversion
src/colorswapper.cpp- remap start/hand/foot/top hold colors
src/idleframe.cpp   - idle demo animation
web/                - mb-esp32-web React app source (deploys to data/www)
```

## To Test (BLE write semantics)

`bleCharacteristic->writeValue(payload, payload_idx, true);` (*TESTED OK, but 4 LEDs only in one frame*)

vs

`bleCharacteristic->writeValue(payload, payload_idx, false);`

Real Kilter Board uses: `pClient->connect(*addr, BLE_ADDR_TYPE_RANDOM);`