//
// Created by felix on 20.03.25.
//

#include "GardenDoorHandler.h"

#define PIN 14
#define ON HIGH
#define OFF LOW

using namespace std;

GardenDoorHandler::GardenDoorHandler(WebServerManager *webServerManager) {
    _webServerManager = webServerManager;
    _switchOnPin = new SwitchOnPin();
    _switchOnPin->setPin(PIN);
    writeBuffer();
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, OFF);
}

void GardenDoorHandler::handleGetServerType(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"type\": \"garden door\"}");
}

void GardenDoorHandler::handlePinListGet(AsyncWebServerRequest *request) {
    JsonDocument jsonDocument;
    JsonArray jsonArray = jsonDocument.to<JsonArray>();

    jsonArray.add(PIN);

    char buffer[512];
    serializeJson(jsonArray, buffer);
    request->send(200, "application/json", buffer);
}

void GardenDoorHandler::handleGet(AsyncWebServer* server) {
    string path = "/api/pin/";
    SwitchOnPin *switchOnPinTmp = _switchOnPin;
    path.append(to_string(switchOnPinTmp->getPin()));
    server->on(path.c_str(), HTTP_GET, [switchOnPinTmp](AsyncWebServerRequest *request) {
        request->send(200, "application/json", switchOnPinTmp->getBuffer());
    });
}

void GardenDoorHandler::handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    bool stateOn;
    if (jsonObject["stateOn"].is<JsonVariant>()) {
        stateOn = jsonObject["stateOn"];
    }
    else {
        request->send(400, "application/json", "{}");
        return;
    }

    stopCountdown();

    if (stateOn) {
        if (jsonObject["time"].is<JsonVariant>()) {
            _timeInSec = jsonObject["time"];
        }
        else {
            _timeInSec = 3;
        }

        digitalWrite(PIN, ON);

        xTaskCreate(
                startCountdown,             /* Task function. */
                "AnimationTask",             /* name of task. */
                10000,                   /* Stack size of task */
                this,                /* parameter of the task */
                1,                          /* priority of the task */
                &_countdownTask          /* Task handle to keep track of created task */
        );
    }
    else {
        digitalWrite(PIN, OFF);
    }

    _switchOnPin->setStateOn(stateOn);
    writeBuffer();

    request->send(200, "application/json", "{}");
}

void GardenDoorHandler::handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    request->send(405, "application/json", "{}");
}

void GardenDoorHandler::writeBuffer() {
    char buffer[512];
    serializeJson(_switchOnPin->getInfo(), buffer);
    _switchOnPin->setBuffer(buffer);
}

void GardenDoorHandler::startCountdown(void *pvParameters) {
    GardenDoorHandler *gardenDoorHandler = (GardenDoorHandler *) pvParameters;

    Helper::wait(gardenDoorHandler->_timeInSec*1000);

    gardenDoorHandler->_switchOnPin->setStateOn(false);
    gardenDoorHandler->writeBuffer();
    digitalWrite(PIN, OFF);

    gardenDoorHandler->_countdownTask = nullptr;
    vTaskDelete(nullptr);
}

void GardenDoorHandler::stopCountdown() {
    if (_countdownTask != nullptr) {
        vTaskDelete(_countdownTask);
        _countdownTask = nullptr;
    }
}