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
    }, handleSystemUpdate);


    handleGet(server);

    server->on("/api/pin", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->url() != "/api/pin") {
            return;
        }
        handlePinListGet(request);
    });

    AsyncCallbackJsonWebHandler *addHandler = new AsyncCallbackJsonWebHandler("/api/pin", [](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        handlePost(request, jsonObject);
    });
    addHandler->setMethod(HTTP_POST);
    server->addHandler(addHandler);

    AsyncCallbackJsonWebHandler *deleteHandler = new AsyncCallbackJsonWebHandler("/api/pin", [](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        handleDelete(request, jsonObject);
    });
    deleteHandler->setMethod(HTTP_DELETE);
    server->addHandler(deleteHandler);

    server->begin();
}
