//
// Created by felix on 14.03.25.
//

#ifndef SWITCHHANDLER_H
#define SWITCHHANDLER_H

#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"

class SwitchHandler {
public:
    void handlePinListGet(AsyncWebServerRequest *request);
    void handleGet(AsyncWebServer* server);
    void handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject);
    void handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject);
};

#endif //SWITCHHANDLER_H
