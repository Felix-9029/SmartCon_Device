//
// Created by felix on 14.03.25.
//

#ifndef LEDHANDLER_H
#define LEDHANDLER_H

#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "Helper.h"
#include "LedStripeOnPin.h"
#include "Adafruit_NeoPixel.h"
#include "WebServerManager.h"

class LedHandler {
public:
    LedHandler(WebServerManager &webServerManager);
    void handlePinListGet(AsyncWebServerRequest *request);
    void handleGet(AsyncWebServer* server);
    void handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject);
    void handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject);
    [[noreturn]] static void animationSet(void *parameter);

private:
    WebServerManager &webServerManager;
    void setColor(uint32_t color);
    uint32_t wheel(byte wheelPos);

    std::vector<LedStripeOnPin *> ledStripeOnPinList;
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(0, 0, NEO_RGBW + NEO_KHZ800);
    TaskHandle_t AnimationTask = nullptr;
};

#endif //LEDHANDLER_H
