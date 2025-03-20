//
// Created by felix on 20.03.25.
//

#include "GardenDoorHandler.h"

#define PIN 13

using namespace std;

GardenDoorHandler::GardenDoorHandler(WebServerManager *webServerManager) {
    _webServerManager = webServerManager;
    pinMode(PIN, OUTPUT);
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
    path.append(to_string(PIN));
    bool stateOn = _stateOn;
    server->on(path.c_str(), HTTP_GET, [stateOn](AsyncWebServerRequest *request) {
        char buffer[512];
        JsonDocument jsonDocument;
        jsonDocument.clear();
        JsonObject jsonObject = jsonDocument.add<JsonObject>();
        jsonObject["stateOn"] = stateOn;
        serializeJson(jsonDocument, buffer);
        request->send(200, "application/json", buffer);
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

    if (stateOn) {
        if (jsonObject["time"].is<JsonVariant>()) {
            _timeInSec = jsonObject["time"];
        }

        digitalWrite(PIN, HIGH);

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
        vTaskDelete(_countdownTask);
        _countdownTask = nullptr;

        digitalWrite(PIN, LOW);
    }

    request->send(200, "application/json", "{}");
}

void GardenDoorHandler::handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    request->send(405, "application/json", "{}");
}

void GardenDoorHandler::startCountdown(void *pvParameters) {
    GardenDoorHandler *gardenDoorHandler = (GardenDoorHandler *) pvParameters;

    Helper::wait(gardenDoorHandler->_timeInSec*1000);

    gardenDoorHandler->_stateOn = false;
    digitalWrite(PIN, LOW);
}