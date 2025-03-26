//
// Created by felix on 14.03.25.
//

#include "SwitchHandler.h"

using namespace std;

SwitchHandler::SwitchHandler(WebServerManager *webServerManager) {
    _webServerManager = webServerManager;
}

void SwitchHandler::handleGetServerType(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"type\": \"led\"}");
}

void SwitchHandler::handlePinListGet(AsyncWebServerRequest *request) {
    JsonDocument jsonDocument;
    JsonArray jsonArray = jsonDocument.to<JsonArray>();

    for (const SwitchOnPin *switchOnPinTmp: _switchOnPinList) {
        jsonArray.add(switchOnPinTmp->getPin());
    }

    char buffer[512];
    serializeJson(jsonArray, buffer);
    request->send(200, "application/json", buffer);
}

void SwitchHandler::handleGet(AsyncWebServer* server) {
    for (const SwitchOnPin *switchOnPinTmp: _switchOnPinList) {
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
        request->send(400, "application/json", "{}");
        return;
    }

    // exclude unusable pins
    if (Helper::isPinUnusable(pin)) {
        request->send(404, "application/json", "{}");
        return;
    }

    boolean containsPin = false;
    SwitchOnPin *switchOnPin;
    for (SwitchOnPin *switchOnPinTmp: _switchOnPinList) {
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

    digitalWrite(switchOnPin->getPin(), switchOnPin->getStateOn());

    switchOnPin->writeBuffer();

    if (!containsPin) {
        _switchOnPinList.push_back(switchOnPin);
        _webServerManager->reset();
    }

    request->send(200, "application/json", "{}");
}

void SwitchHandler::handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    short pin;
    if (jsonObject["pin"].is<JsonVariant>()) {
        pin = jsonObject["pin"];
    }
    else {
        request->send(404, "application/json", "{}");
        return;
    }

    for (SwitchOnPin *switchOnPinTmp: _switchOnPinList) {
        if (switchOnPinTmp->getPin() == pin) {
            _switchOnPinList.erase(std::remove(_switchOnPinList.begin(), _switchOnPinList.end(), switchOnPinTmp), _switchOnPinList.end());
        	_webServerManager->reset();
            break;
        }
    }

    request->send(200, "application/json", "{}");
}
