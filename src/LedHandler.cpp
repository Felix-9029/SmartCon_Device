//
// Created by felix on 14.03.25.
//

#include "LedHandler.h"

using namespace std;

LedHandler::LedHandler(WebServerManager *webServerManager) {
    _webServerManager = webServerManager;
}

void LedHandler::handleGetServerType(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"type\": \"led\"}");
}

void LedHandler::handlePinListGet(AsyncWebServerRequest *request) {
    JsonDocument jsonDocument;
    JsonArray jsonArray = jsonDocument.to<JsonArray>();

    for (const LedStripeOnPin *ledStripeOnPinTmp: _ledStripeOnPinList) {
        jsonArray.add(ledStripeOnPinTmp->getPin());
    }

    char buffer[512];
    serializeJson(jsonArray, buffer);
    request->send(200, "application/json", buffer);
}

void LedHandler::handleGet(AsyncWebServer* server) {
    for (const LedStripeOnPin *ledStripeOnPinTmp: _ledStripeOnPinList) {
        string path = "/api/pin/";
        path.append(to_string(ledStripeOnPinTmp->getPin()));
        server->on(path.c_str(), HTTP_GET, [ledStripeOnPinTmp](AsyncWebServerRequest *request) {
            request->send(200, "application/json", ledStripeOnPinTmp->getBuffer());
        });
    }
}

void LedHandler::handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject) {
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
        request->send(400, "application/json", "{}");
        return;
    }

    bool containsPin = false;
    LedStripeOnPin *ledStripeOnPin;
    for (LedStripeOnPin *ledStripeOnPinTmp: _ledStripeOnPinList) {
        if (ledStripeOnPinTmp->getPin() == pin) {
            ledStripeOnPin = ledStripeOnPinTmp;
            containsPin = true;
            break;
        }
    }

    if (!containsPin) {
        ledStripeOnPin = new LedStripeOnPin();
    }

    if (jsonObject["colorMode"].is<JsonVariant>()) {
        ledStripeOnPin->setColorMode(jsonObject["colorMode"]);
    }
    if (jsonObject["pin"].is<JsonVariant>()) {
        ledStripeOnPin->setPin(jsonObject["pin"]);
    }
    if (jsonObject["ledCount"].is<JsonVariant>()) {
        ledStripeOnPin->setLedCount(jsonObject["ledCount"]);
    }
    if (jsonObject["stateOn"].is<JsonVariant>()) {
        ledStripeOnPin->setStateOn(jsonObject["stateOn"]);
    }
    if (jsonObject["brightness"].is<JsonVariant>()) {
        ledStripeOnPin->setBrightness(jsonObject["brightness"]);
    }

    _strip.setPin(ledStripeOnPin->getPin());
    _strip.updateLength(ledStripeOnPin->getLedCount());

    if (ledStripeOnPin->getColorMode()) {
        if (jsonObject["red"].is<JsonVariant>()) {
            ledStripeOnPin->setRed(jsonObject["red"]);
        }
        if (jsonObject["green"].is<JsonVariant>()) {
            ledStripeOnPin->setGreen(jsonObject["green"]);
        }
        if (jsonObject["blue"].is<JsonVariant>()) {
            ledStripeOnPin->setBlue(jsonObject["blue"]);
        }
        if (jsonObject["white"].is<JsonVariant>()) {
            ledStripeOnPin->setWhite(jsonObject["white"]);
        }
        if (_animationTask != nullptr) {
            vTaskDelete(_animationTask);
            _animationTask = nullptr;
        }

        setColor(Adafruit_NeoPixel::Color(ledStripeOnPin->applyBrightnessToLight(Color::Red),
                                          ledStripeOnPin->applyBrightnessToLight(Color::Green),
                                          ledStripeOnPin->applyBrightnessToLight(Color::Blue),
                                          ledStripeOnPin->getStateOn() ? ledStripeOnPin->getWhite() : 0));
        _strip.show();
        Serial.printf("R: %d G: %d B: %d W: %d\n", ledStripeOnPin->getRed(), ledStripeOnPin->getGreen(), ledStripeOnPin->getBlue(), ledStripeOnPin->getWhite());
    }
    else {
        ledStripeOnPin->setAnimation(jsonObject["globalAnimation"].as<std::string>());
        if (_animationTask != nullptr) {
            vTaskDelete(_animationTask);
            _animationTask = nullptr;
        }
        xTaskCreate(
                runAnimation,             /* Task function. */
                "AnimationTask",             /* name of task. */
                10000,                   /* Stack size of task */
                this,                /* parameter of the task */
                1,                          /* priority of the task */
                &_animationTask          /* Task handle to keep track of created task */
        );
    }

    ledStripeOnPin->writeBuffer();

    if (!containsPin) {
        _ledStripeOnPinList.push_back(ledStripeOnPin);
        _webServerManager->reset();
    }

    request->send(200, "application/json", "{}");
}

void LedHandler::handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    short pin;
    if (jsonObject["pin"].is<JsonVariant>()) {
        pin = jsonObject["pin"];
    }
    else {
        request->send(404, "application/json", "{}");
        return;
    }

    for (LedStripeOnPin *ledStripeOnPinTmp: _ledStripeOnPinList) {
        if (ledStripeOnPinTmp->getPin() == pin) {
            _ledStripeOnPinList.erase(std::remove(_ledStripeOnPinList.begin(), _ledStripeOnPinList.end(), ledStripeOnPinTmp), _ledStripeOnPinList.end());
        	_webServerManager->reset();
            break;
        }
    }

    request->send(200, "application/json", "{}");
}

// ------------------------------------- strip set colors -------------------------------------

void LedHandler::setColor(uint32_t color) {
    uint16_t numPixels = _strip.numPixels();
    for (uint16_t pixel = 0; pixel < numPixels; pixel++) {
        _strip.setPixelColor(pixel, Adafruit_NeoPixel::gamma32(color));
    }
}

// ------------------------------------- strip set globalAnimation -------------------------------------

// rainbow animation -> r - g - b - back to r.
uint32_t LedHandler::colorRainbowWheel(byte wheelPos) {
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return Adafruit_NeoPixel::Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    if (wheelPos < 170) {
        wheelPos -= 85;
        return Adafruit_NeoPixel::Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    wheelPos -= 170;
    return Adafruit_NeoPixel::Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}


[[noreturn]] void LedHandler::runAnimation(void *parameter) {
    // TODO add globalAnimation with duration, (multiple) colors, length
    LedHandler* ledHandler = static_cast<LedHandler*>(parameter);
    uint16_t i, j;

    while (true) {
        for (j = 0; j < 256; j++) {
            for (i = 0; i < ledHandler->_strip.numPixels(); i++) {
                ledHandler->_strip.setPixelColor(i, ledHandler->colorRainbowWheel((i + j) & 255));
            }
            ledHandler->_strip.show();
            Helper::wait(100);
        }
    }
}
