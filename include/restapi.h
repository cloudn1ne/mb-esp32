#ifndef restapi_h
#define restapi_h
//
// restapi.h
//
// REST API endpoints exposed by the MonkeyBoard Gateway web server
//

#include <ESPAsyncWebServer.h>

void setupRestApi(AsyncWebServer *server);

#endif