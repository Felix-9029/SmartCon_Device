/**
 * @author <p>Felix Reichert</p>
 * <p>File: server.cpp</p>
 * <p>Creation date: 08.06.2024</p>
 * <p>Last update: 08.06.2024</p>
 * <p>Version: 1</p>
 */
/*
#include <Update.h>
#include <WebServer.h>

class ServerService {
    WebServer* _server;

public:
    ServerService() : _server(new WebServer(80)) {}

    ~ServerService() {
        delete _server;
    }
// ------------------------------------- OTA - INSECURE -------------------------------------
    void handleSystemUpdate() {

    }

    u32_t currentListSize = 0;
    void setupRouting() {
        _server->on("/api/led", HTTP_GET, handleGetLedList);
        _server->on("/api/led", HTTP_POST, handlePost);
        for (const LedStripeOnPin* ledStripeOnPinTmp : ledStripeOnPinList) {
            String path = "/api/pin/";
            path.concat(ledStripeOnPinTmp->getPin());
            _server->on(path, [ledStripeOnPinTmp]() {
                _server->send(200, "application/json", ledStripeOnPinTmp->getBuffer().data());
            });
        }
        _server->on("/api/delete", HTTP_DELETE, handleDelete);
        // handleUpdate();
        _server->begin();
    }
};
*/