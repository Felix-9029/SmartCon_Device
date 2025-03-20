//
// Created by felix on 14.03.25.
//

#include "WebServerManager.h"

void WebServerManager::start() {
    _server = new AsyncWebServer(80);
    _deviceHandler = new DeviceHandler(this);
}

void WebServerManager::setupRouting() {
    _server->on("/api/device", HTTP_GET, [this](AsyncWebServerRequest *request) {
        _deviceHandler->handleGetServerType(request);
    });

    _server->on("/api/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        _updateHandler->handleSystemUpdate(request, filename, index, data, len, final);
    });

    _deviceHandler->handleGet(_server);

    _server->on("/api/pin", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->url() != "/api/pin") {
            request->send(404, "text/plain", "Page not found!");
            return;
        }
        _deviceHandler->handlePinListGet(request);
    });

    AsyncCallbackJsonWebHandler *addHandler = new AsyncCallbackJsonWebHandler("/api/pin", [this](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        _deviceHandler->handlePost(request, jsonObject);
    });
    addHandler->setMethod(HTTP_POST);
    _server->addHandler(addHandler);

    AsyncCallbackJsonWebHandler *deleteHandler = new AsyncCallbackJsonWebHandler("/api/pin", [this](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        _deviceHandler->handleDelete(request, jsonObject);
    });
    deleteHandler->setMethod(HTTP_DELETE);
    _server->addHandler(deleteHandler);

    _server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Page not found!");
    });

    _server->begin();
}

void WebServerManager::reset() {
    _server->reset();
    setupRouting();
}