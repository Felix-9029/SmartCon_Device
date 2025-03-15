//
// Created by felix on 14.03.25.
//

#include "WebServerManager.h"

void WebServerManager::start() {
    server = new AsyncWebServer(80);
}

void WebServerManager::setupRouting() {
    server->on("/api/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        updateHandler.handleSystemUpdate(request, filename, index, data, len, final);
    });


    deviceHandler.handleGet(server);

    server->on("/api/pin", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->url() != "/api/pin") {
            return;
        }
        deviceHandler.handlePinListGet(request);
    });

    AsyncCallbackJsonWebHandler *addHandler = new AsyncCallbackJsonWebHandler("/api/pin", [this](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        deviceHandler.handlePost(request, jsonObject);
    });
    addHandler->setMethod(HTTP_POST);
    server->addHandler(addHandler);

    AsyncCallbackJsonWebHandler *deleteHandler = new AsyncCallbackJsonWebHandler("/api/pin", [this](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        deviceHandler.handleDelete(request, jsonObject);
    });
    deleteHandler->setMethod(HTTP_DELETE);
    server->addHandler(deleteHandler);

    server->begin();
}

void WebServerManager::reset() {
    server->reset();
    setupRouting();
}