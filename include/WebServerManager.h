//
// Created by felix on 14.03.25.
//

#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "ESPAsyncWebServer.h"
#include "UpdateHandler.h"

#ifdef LED
#include "LedHandler.h"
class LedHandler;
using DeviceHandler = LedHandler;
#elif SWITCH
#include "SwitchHandler.h"
class SwitchHandler;
using DeviceHandler = SwitchHandler;
#elif GARDENDOOR
#include "GardenDoorHandler.h"
class GardenDoorHandler;
using DeviceHandler = GardenDoorHandler;
#else // fallback to switch
#include "SwitchHandler.h"
class SwitchHandler;
using DeviceHandler = SwitchHandler;
#endif

using namespace std;

class WebServerManager {
public:
    void start();
    void setupRouting();
    void reset();

private:
    AsyncWebServer* _server;
    UpdateHandler* _updateHandler;
    DeviceHandler* _deviceHandler;
};

#endif //WEBSERVERMANAGER_H
