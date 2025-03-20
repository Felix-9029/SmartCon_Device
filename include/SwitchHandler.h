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

using namespace std;

class WebServerManager;

class SwitchHandler {
public:
    SwitchHandler(WebServerManager *webServerManager);
    void handleGetServerType(AsyncWebServerRequest *request);
    void handlePinListGet(AsyncWebServerRequest *request);
    void handleGet(AsyncWebServer* server);
    void handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject);
    void handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject);

private:
    WebServerManager *_webServerManager;
    vector<SwitchOnPin *> _switchOnPinList;
};

#endif //SWITCHHANDLER_H
