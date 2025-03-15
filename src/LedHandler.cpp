//
// Created by felix on 14.03.25.
//

#include "LedHandler.h"

using namespace std;

void handlePinListGet(AsyncWebServerRequest *request) {
    JsonDocument jsonDocument;
    JsonArray jsonArray = jsonDocument.to<JsonArray>();

    for (LedStripeOnPin const *ledStripeOnPinTmp: ledStripeOnPinList) {
        jsonArray.add(ledStripeOnPinTmp->getPin());
    }

    char buffer[512];
    serializeJson(jsonArray, buffer);
    request->send(200, "application/json", buffer);
}

void handleGet(AsyncWebServer* server) {
    for (const LedStripeOnPin *ledStripeOnPinTmp: ledStripeOnPinList) {
        string path = "/api/pin/";
        path.append(to_string(ledStripeOnPinTmp->getPin()));
        server->on(path.c_str(), HTTP_GET, [ledStripeOnPinTmp](AsyncWebServerRequest *request) {
            request->send(200, "application/json", ledStripeOnPinTmp->getBuffer());
        });
    }
}

void handlePost(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    short pin;
    if (jsonObject.containsKey("pin")) {
        pin = jsonObject["pin"];
    }
    else {
        request->send(404, "application/json", "{}");
        return;
    }

    // exclude unusable pins
    if (pin < 0 // does not exist
        || pin == 0 // boot mode (BOOT button)
        || pin == 1 // TX
        || pin == 2 // internal SYSTEM_LED
        || pin == 3 // RX
        || (pin >= 6 && pin <= 11) // flash pins

        // || pin == 12 // MUST be low on startup AND blocked only in dev mode -> debugging port
        // || pin == 13 // blocked only in dev mode -> debugging port
        // || pin == 14 // blocked only in dev mode -> debugging port
        // || pin == 15 // blocked only in dev mode -> debugging port

        || pin == 20 // does not exist
        || pin == 24 // does not exist
        || (pin >= 28 && pin <= 31) // does not exist
        || (pin >= 34 && pin <= 36) // input only
        || pin == 37 // does not exist
        || pin == 38 // does not exist
        || pin == 39 // input only
        || pin >= 40 /* does not exist */) {
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

    if (jsonObject.containsKey("colorMode")) {
        ledStripeOnPin->setColorMode(jsonObject["colorMode"]);
    }
    if (jsonObject.containsKey("pin")) {
        ledStripeOnPin->setPin(jsonObject["pin"]);
    }
    if (jsonObject.containsKey("ledCount")) {
        ledStripeOnPin->setLedCount(jsonObject["ledCount"]);
    }
    if (jsonObject.containsKey("stateOn")) {
        ledStripeOnPin->setStateOn(jsonObject["stateOn"]);
    }
    if (jsonObject.containsKey("brightness")) {
        ledStripeOnPin->setBrightness(jsonObject["brightness"]);
    }

    strip.setPin(ledStripeOnPin->getPin());
    strip.updateLength(ledStripeOnPin->getLedCount());

    if (ledStripeOnPin->getColorMode()) {
        if (jsonObject.containsKey("red")) {
            ledStripeOnPin->setRed(jsonObject["red"]);
        }
        if (jsonObject.containsKey("green")) {
            ledStripeOnPin->setGreen(jsonObject["green"]);
        }
        if (jsonObject.containsKey("blue")) {
            ledStripeOnPin->setBlue(jsonObject["blue"]);
        }
        if (jsonObject.containsKey("white")) {
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

void handleDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
    short pin;
    if (jsonObject.containsKey("pin")) {
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

void setColor(uint32_t color) {
    uint16_t numPixels = strip.numPixels();
    for (uint16_t pixel = 0; pixel < numPixels; pixel++) {
        strip.setPixelColor(pixel, Adafruit_NeoPixel::gamma32(color));
    }
}

// ------------------------------------- strip set globalAnimation -------------------------------------

// rainbow animation -> r - g - b - back to r.
uint32_t wheel(byte wheelPos) {
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


[[noreturn]] void animationSet(void *parameter) {
    // TODO add globalAnimation with duration, (multiple) colors, length
    uint16_t i, j;

    while (true) {
        for (j = 0; j < 256; j++) {
            for (i = 0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, wheel((i + j) & 255));
            }
            strip.show();
            wait(100);
        }
    }
}