//
// Created by felix on 14.03.25.
//

#include "SwitchHandler.h"

using namespace std;

void SwitchHandler::handlePinListGet(AsyncWebServerRequest *request) {
    JsonDocument jsonDocument;
    JsonArray jsonArray = jsonDocument.to<JsonArray>();

    for (const SwitchOnPin *switchOnPinTmp: this->switchOnPinList) {
        jsonArray.add(switchOnPinTmp->getPin());
    }

    char buffer[512];
    serializeJson(jsonArray, buffer);
    request->send(200, "application/json", buffer);
}

void SwitchHandler::handleGet(AsyncWebServer* server) {
    for (const SwitchOnPin *switchOnPinTmp: switchOnPinList) {
        string path = "/api/pin/";
        path.append(to_string(switchOnPinTmp->getPin()));
        server->on(path.c_str(), HTTP_GET, [switchOnPinTmp](AsyncWebServerRequest *request) {
            request->send(200, "application/json", switchOnPinTmp->getBuffer());
        });
    }
}

void SwitchHandler::handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    short pin;
    if (jsonObject["pin"].is<JsonVariant>()) {
        pin = jsonObject["pin"];
    }
    else {
        request->send(404, "application/json", "{}");
        return;
    }

    // exclude unusable pins
    if (Helper::isPinUnusable(pin)) {
        request->send(404, "application/json", "{}");
        return;
    }

    boolean containsPin = false;
    SwitchOnPin *switchOnPin;
    for (SwitchOnPin *switchOnPinTmp: switchOnPinList) {
        if (switchOnPinTmp->getPin() == pin) {
            switchOnPin = switchOnPinTmp;
            containsPin = true;
            break;
        }
    }

    if (!containsPin) {
        switchOnPin = new SwitchOnPin();
    }

    if (jsonObject["pin"].is<JsonVariant>()) {
        switchOnPin->setPin(jsonObject["pin"]);
    }
    if (jsonObject["stateOn"].is<JsonVariant>()) {
        switchOnPin->setStateOn(jsonObject["stateOn"]);
    }

    char buffer[512];
    serializeJson(switchOnPin->getInfo(), buffer);
    switchOnPin->setBuffer(buffer);

    if (!containsPin) {
        switchOnPinList.push_back(switchOnPin);
        webServerManager->reset();
    }

    request->send(200, "application/json", "{}");
}

void SwitchHandler::handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    // block deleting pins
    request->send(404, "application/json", "{}");
    return;

    short pin;
    if (jsonObject["pin"].is<JsonVariant>()) {
        pin = jsonObject["pin"];
    }
    else {
        request->send(404, "application/json", "{}");
        return;
    }

    for (SwitchOnPin *switchOnPinTmp: switchOnPinList) {
        if (switchOnPinTmp->getPin() == pin) {
            switchOnPinList.erase(std::remove(switchOnPinList.begin(), switchOnPinList.end(), switchOnPinTmp), switchOnPinList.end());
        	webServerManager->reset();
            break;
        }
    }

    request->send(200, "application/json", "{}");
}