//
// Created by felix on 20.03.25.
//

#ifndef GARDENDOORHANDLER_H
#define GARDENDOORHANDLER_H

#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "Helper.h"
#include "SwitchOnPin.h"
#include "WebServerManager.h"

using namespace std;

class WebServerManager;

class GardenDoorHandler {
public:
    GardenDoorHandler(WebServerManager *webServerManager);
    void handleGetServerType(AsyncWebServerRequest *request);
    void handlePinListGet(AsyncWebServerRequest *request);
    void handleGet(AsyncWebServer* server);
    void handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject);
    void handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject);

private:
    void writeBuffer();
    static void startCountdown(void *pvParameters);
    void stopCountdown();

    WebServerManager *_webServerManager;
    SwitchOnPin *_switchOnPin;
    int _timeInSec = 3;
    TaskHandle_t _countdownTask = nullptr;
};

#endif //GARDENDOORHANDLER_H
