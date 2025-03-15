//
// Created by felix on 14.03.25.
//

#include "LedHandler.h"

using namespace std;

void LedHandler::handlePinListGet(AsyncWebServerRequest *request) {
    JsonDocument jsonDocument;
    JsonArray jsonArray = jsonDocument.to<JsonArray>();

    for (LedStripeOnPin const *ledStripeOnPinTmp: ledStripeOnPinList) {
        jsonArray.add(ledStripeOnPinTmp->getPin());
    }

    char buffer[512];
    serializeJson(jsonArray, buffer);
    request->send(200, "application/json", buffer);
}

void LedHandler::handleGet(AsyncWebServer* server) {
    for (const LedStripeOnPin *ledStripeOnPinTmp: ledStripeOnPinList) {
        string path = "/api/pin/";
        path.append(to_string(ledStripeOnPinTmp->getPin()));
        server->on(path.c_str(), HTTP_GET, [ledStripeOnPinTmp](AsyncWebServerRequest *request) {
            request->send(200, "application/json", ledStripeOnPinTmp->getBuffer());
        });
    }
}

void LedHandler::handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    short pin;
    if (jsonObject["pin"].is<short>()) {
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
    LedStripeOnPin *ledStripeOnPin;
    for (LedStripeOnPin *ledStripeOnPinTmp: ledStripeOnPinList) {
        if (ledStripeOnPinTmp->getPin() == pin) {
            ledStripeOnPin = ledStripeOnPinTmp;
            containsPin = true;
            break;
        }
    }

    if (!containsPin) {
        ledStripeOnPin = new LedStripeOnPin();
    }

    if (jsonObject["colorMode"].is<boolean>()) {
        ledStripeOnPin->setColorMode(jsonObject["colorMode"]);
    }
    if (jsonObject["pin"].is<short>()) {
        ledStripeOnPin->setPin(jsonObject["pin"]);
    }
    if (jsonObject["ledCount"].is<int>()) {
        ledStripeOnPin->setLedCount(jsonObject["ledCount"]);
    }
    if (jsonObject["stateOn"].is<boolean>()) {
        ledStripeOnPin->setStateOn(jsonObject["stateOn"]);
    }
    if (jsonObject["brightness"].is<uint32_t>()) {
        ledStripeOnPin->setBrightness(jsonObject["brightness"]);
    }

    strip.setPin(ledStripeOnPin->getPin());
    strip.updateLength(ledStripeOnPin->getLedCount());

    if (ledStripeOnPin->getColorMode()) {
        if (jsonObject["red"].is<uint32_t>()) {
            ledStripeOnPin->setRed(jsonObject["red"]);
        }
        if (jsonObject["green"].is<uint32_t>()) {
            ledStripeOnPin->setGreen(jsonObject["green"]);
        }
        if (jsonObject["blue"].is<uint32_t>()) {
            ledStripeOnPin->setBlue(jsonObject["blue"]);
        }
        if (jsonObject["white"].is<uint32_t>()) {
            ledStripeOnPin->setWhite(jsonObject["white"]);
        }
        if (AnimationTask != nullptr) {
            vTaskDelete(AnimationTask);
            AnimationTask = nullptr;
        }

        setColor(Adafruit_NeoPixel::Color(ledStripeOnPin->applyBrightnessToLight(Color::Red),
                                          ledStripeOnPin->applyBrightnessToLight(Color::Green),
                                          ledStripeOnPin->applyBrightnessToLight(Color::Blue),
                                          ledStripeOnPin->getStateOn() ? ledStripeOnPin->getWhite() : 0));
        strip.show();
        Serial.printf("R: %d G: %d B: %d W: %d\n", ledStripeOnPin->getRed(), ledStripeOnPin->getGreen(), ledStripeOnPin->getBlue(), ledStripeOnPin->getWhite());
    }
    else {
        ledStripeOnPin->setAnimation(jsonObject["globalAnimation"].as<std::string>());
        if (AnimationTask != nullptr) {
            vTaskDelete(AnimationTask);
            AnimationTask = nullptr;
        }
        xTaskCreate(
                animationSet,             /* Task function. */
                "AnimationTask",             /* name of task. */
                10000,                   /* Stack size of task */
                nullptr,                /* parameter of the task */
                1,                          /* priority of the task */
                &AnimationTask          /* Task handle to keep track of created task */
        );
    }

    char buffer[512];
    serializeJson(ledStripeOnPin->getInfo(), buffer);
    ledStripeOnPin->setBuffer(buffer);

    if (!containsPin) {
        ledStripeOnPinList.push_back(ledStripeOnPin);
        server->reset();
        setupRouting();
    }

    request->send(200, "application/json", "{}");
}

void LedHandler::handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    short pin;
    if (jsonObject["pin"].is<short>()) {
        pin = jsonObject["pin"];
    }
    else {
        request->send(404, "application/json", "{}");
        return;
    }

    for (LedStripeOnPin *ledStripeOnPinTmp: ledStripeOnPinList) {
        if (ledStripeOnPinTmp->getPin() == pin) {
            ledStripeOnPinList.erase(std::remove(ledStripeOnPinList.begin(), ledStripeOnPinList.end(), ledStripeOnPinTmp), ledStripeOnPinList.end());
            server->reset();
            setupRouting();
            break;
        }
    }

    request->send(200, "application/json", "{}");
}

// ------------------------------------- strip set colors -------------------------------------

void LedHandler::setColor(uint32_t color) {
    uint16_t numPixels = strip.numPixels();
    for (uint16_t pixel = 0; pixel < numPixels; pixel++) {
        strip.setPixelColor(pixel, Adafruit_NeoPixel::gamma32(color));
    }
}

// ------------------------------------- strip set globalAnimation -------------------------------------

// rainbow animation -> r - g - b - back to r.
uint32_t LedHandler::wheel(byte wheelPos) {
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


[[noreturn]] void LedHandler::animationSet(void *parameter) {
    // TODO add globalAnimation with duration, (multiple) colors, length
    uint16_t i, j;

    while (true) {
        for (j = 0; j < 256; j++) {
            for (i = 0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, wheel((i + j) & 255));
            }
            strip.show();
            Helper::wait(100);
        }
    }
}