#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include "restapi.h"
#include "encoder.h"
#include "grid.h"
#include "colors.h"
#include "colorswapper.h"
#include "settings.h"

#define MB_VERSION "2.0"

extern KilterEncoder *tx_encoder;
extern KilterGrid *grid;
extern ColorSwapper *color_swapper;
extern Preferences prefs;
extern bool showIdleFrameAnimation;

// ---------------------------------------------------------------------------
// GET /api/status
// ---------------------------------------------------------------------------
static void handleStatus(AsyncWebServerRequest *request)
{
    AsyncJsonResponse *response = new AsyncJsonResponse(false, 2048);
    JsonObject root = response->getRoot();
    JsonObject status = root.createNestedObject("status");

    String state = tx_encoder->getConnectionState();
    status["state"] = state;
    status["connected"] = (state == "connected");
    status["scanning"] = (state == "scanning");
    status["failed"] = (state == "failed");
    status["reconnecting"] = (state == "reconnecting");
    status["target"] = setting_TargetBoardName;
    status["advertised"] = setting_AdvertisedBoardName;
    status["idle"] = setting_ShowIdleFrame && showIdleFrameAnimation;
    status["swpcol"] = setting_SwapColors;
    status["numHolds"] = tx_encoder->getNumHolds();
    status["version"] = MB_VERSION;

    response->setLength();
    request->send(response);
}

// ---------------------------------------------------------------------------
// GET /api/grid - authoritative hold layout (x, y, holdnum)
// streamed manually - no large JSON document on the heap (no PSRAM on this board)
// ---------------------------------------------------------------------------
static void handleGrid(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{\"cols\":");
    response->print(HOMEWALL_COLS);
    response->print(",\"rows\":");
    response->print(HOMEWALL_ROWS);
    response->print(",\"mapping\":[");

    bool first = true;
    for (uint8_t y = 0; y < HOMEWALL_ROWS; y++)
    {
        for (uint8_t x = 0; x < HOMEWALL_COLS; x++)
        {
            int16_t v = grid->getHoldNumber(x, y);
            if (v != -1)
            {
                if (!first)
                {
                    response->print(",");
                }
                response->printf("[%d,%d,%d]", x, y, v);
                first = false;
            }
        }
    }
    response->print("]}");
    request->send(response);
}

// ---------------------------------------------------------------------------
// GET /api/holds - current hold buffer (streamed, no big JSON doc)
// ---------------------------------------------------------------------------
static void handleGetHolds(AsyncWebServerRequest *request)
{
    uint16_t numHolds = tx_encoder->getNumHolds();
    uint16_t *holdNums = (uint16_t *)malloc((numHolds ? numHolds : 1) * sizeof(uint16_t));
    uint8_t *holdColors = (uint8_t *)malloc((numHolds ? numHolds : 1) * sizeof(uint8_t));

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{\"holds\":[");

    if (holdNums && holdColors)
    {
        tx_encoder->copyHolds(holdNums, holdColors);
        for (uint16_t i = 0; i < numHolds; i++)
        {
            if (i)
            {
                response->print(",");
            }
            uint32_t rgb = KilterColorToRGB(holdColors[i]);
            response->printf("{\"holdnum\":%d,\"x\":%d,\"y\":%d,\"r\":%d,\"g\":%d,\"b\":%d}",
                             holdNums[i],
                             grid->getX(holdNums[i]),
                             grid->getY(holdNums[i]),
                             (rgb >> 16) & 0xFF,
                             (rgb >> 8) & 0xFF,
                             rgb & 0xFF);
        }
    }
    free(holdNums);
    free(holdColors);

    response->print("]}");
    request->send(response);
}

// ---------------------------------------------------------------------------
// POST /api/holds - replace the hold buffer and send it to the Kilter Board
// body: {"holds": [ {"holdnum": n, "r":..,"g":..,"b":..} | {"x":..,"y":..,"r":..,"g":..,"b":..} ]}
// ---------------------------------------------------------------------------
static void handlePostHolds(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonArray holds = json.as<JsonObject>()["holds"];
    if (holds.isNull())
    {
        request->send(400, JSON_MIMETYPE, "{\"error\":\"missing holds array\"}");
        return;
    }

    tx_encoder->resetHolds();
    bool connected = tx_encoder->isConnected();

    for (JsonVariant v : holds)
    {
        JsonObject h = v.as<JsonObject>();
        if (h.isNull())
        {
            continue;
        }

        // resolve hold number: explicit holdnum, or x/y via the grid mapping
        uint16_t holdnum;
        if (h.containsKey("holdnum"))
        {
            holdnum = h["holdnum"].as<uint16_t>();
        }
        else if (h.containsKey("x") && h.containsKey("y"))
        {
            int16_t vh = grid->getHoldNumber(h["x"].as<uint8_t>(), h["y"].as<uint8_t>());
            if (vh < 0)
            {
                continue; // no hold at this position
            }
            holdnum = (uint16_t)vh;
        }
        else
        {
            continue;
        }

        // color: explicit kilter code wins, otherwise convert RGB
        uint8_t color;
        if (h.containsKey("color"))
        {
            color = h["color"].as<uint8_t>();
        }
        else
        {
            uint32_t rgb = ((uint32_t)h["r"].as<uint8_t>() << 16) |
                           ((uint32_t)h["g"].as<uint8_t>() << 8) |
                           ((uint32_t)h["b"].as<uint8_t>());
            color = RGBToKilterColor(rgb);
        }

        tx_encoder->setHold(holdnum, color);
    }

    if (connected)
    {
        tx_encoder->sendHolds();
    }
    // a real problem is on the board - stop the idle demo until asked again
    showIdleFrameAnimation = false;

    AsyncJsonResponse *response = new AsyncJsonResponse(false, 1024);
    response->getRoot()["sent"] = tx_encoder->getNumHolds();
    response->getRoot()["connected"] = connected;
    response->getRoot()["state"] = tx_encoder->getConnectionState();
    response->setLength();
    request->send(response);
}

// ---------------------------------------------------------------------------
// DELETE /api/holds - clear the board
// ---------------------------------------------------------------------------
static void handleDeleteHolds(AsyncWebServerRequest *request)
{
    tx_encoder->resetHolds();
    bool connected = tx_encoder->isConnected();
    if (connected)
    {
        tx_encoder->sendHolds(); // empty packet (type 84, 0 holds)
    }

    AsyncJsonResponse *response = new AsyncJsonResponse(false, 1024);
    response->getRoot()["sent"] = 0;
    response->getRoot()["connected"] = connected;
    response->setLength();
    request->send(response);
}

// ---------------------------------------------------------------------------
// GET /api/settings
// ---------------------------------------------------------------------------
static void handleGetSettings(AsyncWebServerRequest *request)
{
    AsyncJsonResponse *response = new AsyncJsonResponse(false, 1024);
    JsonObject root = response->getRoot();
    root["swpcol"] = setting_SwapColors;
    root["idle"] = setting_ShowIdleFrame;
    root["advname"] = setting_AdvertisedBoardName;
    root["tgtname"] = setting_TargetBoardName;
    response->setLength();
    request->send(response);
}

// ---------------------------------------------------------------------------
// POST /api/settings - update + persist settings
// body: {"swpcol"?:bool, "idle"?:bool, "advname"?:string}
// ---------------------------------------------------------------------------
static void handlePostSettings(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject root = json.as<JsonObject>();
    if (root.isNull())
    {
        request->send(400, JSON_MIMETYPE, "{\"error\":\"invalid JSON body\"}");
        return;
    }

    if (root.containsKey("swpcol"))
    {
        setting_SwapColors = root["swpcol"].as<bool>();
        color_swapper->toggle(setting_SwapColors);
    }
    if (root.containsKey("idle"))
    {
        setting_ShowIdleFrame = root["idle"].as<bool>();
        showIdleFrameAnimation = setting_ShowIdleFrame;
    }
    if (root.containsKey("advname"))
    {
        setting_AdvertisedBoardName = root["advname"].as<const char *>();
    }
    saveSettings(&prefs);

    AsyncJsonResponse *response = new AsyncJsonResponse(false, 1024);
    response->getRoot()["ok"] = true;
    response->setLength();
    request->send(response);
}

// ---------------------------------------------------------------------------
// POST /api/idle - start/stop the idle demo animation
// body: {"enable": bool}
// ---------------------------------------------------------------------------
static void handlePostIdle(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject root = json.as<JsonObject>();
    if (root.isNull() || !root.containsKey("enable"))
    {
        request->send(400, JSON_MIMETYPE, "{\"error\":\"missing enable\"}");
        return;
    }

    bool enable = root["enable"].as<bool>();
    setting_ShowIdleFrame = enable;
    showIdleFrameAnimation = enable;
    saveSettings(&prefs);

    AsyncJsonResponse *response = new AsyncJsonResponse(false, 1024);
    response->getRoot()["idle"] = enable;
    response->setLength();
    request->send(response);
}

// ---------------------------------------------------------------------------
// route registration
// ---------------------------------------------------------------------------
void setupRestApi(AsyncWebServer *server)
{
    server->on("/api/status", HTTP_GET, handleStatus);
    server->on("/api/grid", HTTP_GET, handleGrid);
    server->on("/api/holds", HTTP_GET, handleGetHolds);
    server->on("/api/holds", HTTP_DELETE, handleDeleteHolds);
    server->on("/api/settings", HTTP_GET, handleGetSettings);

    // JSON body handlers (modest buffers - no PSRAM on this board, big docs OOM)
    AsyncCallbackJsonWebHandler *postHolds =
        new AsyncCallbackJsonWebHandler("/api/holds", handlePostHolds, 16384);
    postHolds->setMethod(HTTP_POST);
    postHolds->setMaxContentLength(16384);
    server->addHandler(postHolds);

    AsyncCallbackJsonWebHandler *postSettings =
        new AsyncCallbackJsonWebHandler("/api/settings", handlePostSettings, 2048);
    postSettings->setMethod(HTTP_POST);
    server->addHandler(postSettings);

    AsyncCallbackJsonWebHandler *postIdle =
        new AsyncCallbackJsonWebHandler("/api/idle", handlePostIdle, 2048);
    postIdle->setMethod(HTTP_POST);
    server->addHandler(postIdle);
}