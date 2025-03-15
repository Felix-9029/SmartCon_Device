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
#elif SWITCH
#include "SwitchHandler.h"
#else // fallback to switch
#include "SwitchHandler.h"
#endif

class WebServerManager {
public:
    void start();
    void setupRouting();
    void reset();

private:
    AsyncWebServer* server;
    UpdateHandler updateHandler;
#ifdef LED
    LedHandler deviceHandler;
#elif SWITCH
    SwitchHandler deviceHandler;
#else // fallback to switch
    SwitchHandler deviceHandler;
#endif
};

#endif //WEBSERVERMANAGER_H
