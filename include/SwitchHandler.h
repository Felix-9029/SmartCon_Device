//
// Created by felix on 14.03.25.
//

#ifndef SWITCHHANDLER_H
#define SWITCHHANDLER_H

#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "Helper.h"
#include "SwitchOnPin.h"
#include "WebServerManager.h"

class WebServerManager;

class SwitchHandler {
public:
    SwitchHandler(WebServerManager *webServerManager);
    void handlePinListGet(AsyncWebServerRequest *request);
    void handleGet(AsyncWebServer* server);
    void handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject);
    void handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject);

private:
    WebServerManager *webServerManager;
    std::vector<SwitchOnPin *> switchOnPinList = {};
};

#endif //SWITCHHANDLER_H
