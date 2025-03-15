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
#elif SWITCH
#include "SwitchHandler.h"
class SwitchHandler;
#else // fallback to switch
#include "SwitchHandler.h"
class SwitchHandler;
#endif

class WebServerManager {
public:
    void start();
    void setupRouting();
    void reset();

private:
    AsyncWebServer* server = nullptr;
    UpdateHandler* updateHandler = nullptr;
#ifdef LED
    LedHandler* deviceHandler = nullptr;
#elif SWITCH
    SwitchHandler* deviceHandler = nullptr;
#else // fallback to switch
    SwitchHandler* deviceHandler = nullptr;
#endif
};

#endif //WEBSERVERMANAGER_H
