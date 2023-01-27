/**
 * @author <p>Felix Reichert</p>
 * <p>File: main.cpp</p>
 * <p>Creation date: 01.06.2022</p>
 * <p>Last update: 12.11.2022</p>
 * <p>Version: 2</p>
 */

#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include "LedStripeOnPin.h"
#include "credentials.h"

#define LED 2

using namespace std;


// ------------------------------------- global variables and classes -------------------------------------

WebServer server(80);
vector<LedStripeOnPin*> ledStripeOnPinList;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(0, 0, NEO_RGBW + NEO_KHZ800);

TaskHandle_t AnimationTask;

// delay replacement
void wait(int interval) {
    unsigned long endMillis = millis() + interval;
    while (true) {
        if (endMillis <= millis()) {
            break;
        }
    }
}

// ------------------------------------- Setup wifi -------------------------------------

void connectToWifi() {
    Serial.printf("Connecting to %s\n", SSID);

    byte mac[6];
    WiFi.macAddress(mac);
    char macPart[7 + 1];
    sprintf(macPart, "_%02x%02x%02x", mac[3], mac[4], mac[5]);

    string hostnameString = "EasyLED";
    hostnameString.append(macPart);

    char hostname[hostnameString.length() + 1];
    strcpy(hostname, hostnameString.c_str());

    WiFiClass::hostname(hostname);
    WiFi.begin(SSID, PWD);
    int resetCounter = 0;
    while (WiFiClass::status() != WL_CONNECTED) {
        resetCounter++;
        if (resetCounter >= 100) {
            ESP.restart();
        }
        Serial.print(".");
        digitalWrite(LED, !digitalRead(LED));
        wait(800);
    }

    Serial.println("\nConnected.");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Hostname: %s\n", WiFiClass::getHostname());
    Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


// ------------------------------------- OTA - INSECURE -------------------------------------

void handleUpdate() {

    // TODO handle uploading firmware file
    server.on(
            "/update", HTTP_POST, []() {
                server.sendHeader("Connection", "close");
                server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
                ESP.restart();
            },
            []() {
                HTTPUpload &upload = server.upload();
                if (upload.status == UPLOAD_FILE_START) {
                    Serial.printf("Update: %s\n", upload.filename.c_str());
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  // start with max available size
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_WRITE) {
                    /* flashing firmware to ESP*/
                    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_END) {
                    if (Update.end(
                            true)) {  // true to set the size to the current progress
                        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                    }
                    else {
                        Update.printError(Serial);
                    }
                }
            });
}

// ------------------------------------- strip set colors -------------------------------------

void setColor(uint32_t color) {
    int numPixels = strip.numPixels();
    for (uint16_t pixel = 0; pixel < numPixels; pixel++) {
        strip.setPixelColor(pixel, Adafruit_NeoPixel::gamma32(color));
    }
}

// ------------------------------------- strip set globalAnimation -------------------------------------

// rainbow animation -> r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return Adafruit_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return Adafruit_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return Adafruit_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


[[noreturn]] void animationSet(void *parameter) {
    // TODO add globalAnimation with duration, (multiple) colors, length
    uint16_t i, j;

    while (true) {
        for (j = 0; j < 256; j++) {
            for (i = 0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, Wheel((i + j) & 255));
            }
            strip.show();
            wait(100);
        }
    }
}


// ------------------------------------- handle values from app post -------------------------------------

StaticJsonDocument<512> jsonDocument;

void handlePost() {
    if (!server.hasArg("plain")) {
        server.send(404, "application/json", "{}");
        return;
    }

    String body = server.arg("plain");
    deserializeJson(jsonDocument, body);

    short pin;
    if (jsonDocument.containsKey("pin")) {
        pin = jsonDocument["pin"];
    }
    else {
        server.send(404, "application/json", "{}");
        return;
    }

    // exclude unusable pins
    if (pin < 0 // does not exist
            || pin == 0 // boot mode (BOOT button)
            || pin == 1 // TX
            || pin == 2 // internal LED
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
        server.send(404, "application/json", "{}");
        return;
    }

    boolean containsPin = false;
    LedStripeOnPin* ledStripeOnPin;
    LedStripeOnPin* ledStripeOnPinOld;
    for (LedStripeOnPin* ledStripeOnPinTmp : ledStripeOnPinList) {
        if (ledStripeOnPinTmp->getPin() == pin) {
            ledStripeOnPinOld = ledStripeOnPinTmp;
            ledStripeOnPin = ledStripeOnPinTmp;
            containsPin = true;
            break;
        }
    }

    if (!containsPin) {
        ledStripeOnPin = new LedStripeOnPin();
    }

    if (jsonDocument.containsKey("colorMode")) {
        ledStripeOnPin->setColorMode(jsonDocument["colorMode"]);
    }
    if (jsonDocument.containsKey("pin")) {
        ledStripeOnPin->setPin(jsonDocument["pin"]);
    }
    if (jsonDocument.containsKey("ledCount")) {
        ledStripeOnPin->setLedCount(jsonDocument["ledCount"]);
    }
    if (jsonDocument.containsKey("stateOn")) {
        ledStripeOnPin->setStateOn(jsonDocument["stateOn"]);
    }
    if (jsonDocument.containsKey("brightness")) {
        ledStripeOnPin->setBrightness(jsonDocument["brightness"]);
    }

    strip.setPin(ledStripeOnPin->getPin());
    strip.updateLength(ledStripeOnPin->getLedCount());

    if (ledStripeOnPin->getColorMode()) {
        if (jsonDocument.containsKey("red")) {
            ledStripeOnPin->setRed(jsonDocument["red"]);
        }
        if (jsonDocument.containsKey("green")) {
            ledStripeOnPin->setGreen(jsonDocument["green"]);
        }
        if (jsonDocument.containsKey("blue")) {
            ledStripeOnPin->setBlue(jsonDocument["blue"]);
        }
        if (jsonDocument.containsKey("white")) {
            ledStripeOnPin->setWhite(jsonDocument["white"]);
        }
        if (AnimationTask != nullptr) {
            vTaskDelete(AnimationTask);
            AnimationTask = nullptr;
        }

        Serial.printf("R: %d G: %d B: %d W: %d\n", ledStripeOnPin->getRed(), ledStripeOnPin->getGreen(), ledStripeOnPin->getBlue(), ledStripeOnPin->getWhite());
        setColor(Adafruit_NeoPixel::Color(ledStripeOnPin->applyBrightnessToLight(0),
                                          ledStripeOnPin->applyBrightnessToLight(1),
                                          ledStripeOnPin->applyBrightnessToLight(2),
                                          ledStripeOnPin->getStateOn() ? ledStripeOnPin->getWhite() : 0));
        strip.show();
    }
    else {
        ledStripeOnPin->setAnimation(jsonDocument["globalAnimation"].as<String>());
        if (AnimationTask != nullptr) {
            vTaskDelete(AnimationTask);
            AnimationTask = nullptr;
        }
        xTaskCreatePinnedToCore(
                animationSet,             /* Task function. */
                "AnimationTask",             /* name of task. */
                10000,                   /* Stack size of task */
                nullptr,                /* parameter of the task */
                1,                          /* priority of the task */
                &AnimationTask,          /* Task handle to keep track of created task */
                1                             /* pin task to core 0 */
        );
    }

    char buffer[512];
    serializeJson(ledStripeOnPin->getInfo(), buffer);
    ledStripeOnPin->setBuffer(buffer);

    if (containsPin) {
        replace(ledStripeOnPinList.begin(), ledStripeOnPinList.end(), ledStripeOnPinOld, ledStripeOnPin);
    }
    else {
        ledStripeOnPinList.push_back(ledStripeOnPin);
    }

    server.send(200, "application/json", "{}");
}

void handleDelete() {
    if (!server.hasArg("plain")) {
        server.send(404, "application/json", "{}");
        return;
    }

    String body = server.arg("plain");
    deserializeJson(jsonDocument, body);

    short pin;
    if (jsonDocument.containsKey("pin")) {
        pin = jsonDocument["pin"];
    }
    else {
        server.send(404, "application/json", "{}");
        return;
    }

    for (LedStripeOnPin* ledStripeOnPinTmp : ledStripeOnPinList) {
        if (ledStripeOnPinTmp->getPin() == pin) {
            ledStripeOnPinList.erase(std::remove(ledStripeOnPinList.begin(), ledStripeOnPinList.end(), ledStripeOnPinTmp), ledStripeOnPinList.end());
            break;
        }
    }

    server.send(200, "application/json", "{}");
}


// ------------------------------------- setup -------------------------------------

u32_t currentListSize = 0;
void setup_routing() {
    server.on("/api/led", HTTP_POST, handlePost);
    for (const LedStripeOnPin* ledStripeOnPinTmp : ledStripeOnPinList) {
        String path = "/api/pin/";
        path.concat(ledStripeOnPinTmp->getPin());
        server.on(path, [ledStripeOnPinTmp]() {
            server.send(200, "application/json", ledStripeOnPinTmp->getBuffer());
        });
    }
    server.on("/api/delete", HTTP_DELETE, handleDelete);
    // handleUpdate();
    server.begin();
}

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    connectToWifi();
    setup_routing();
}

void loop() {
    server.handleClient();
    if (WiFiClass::status() != WL_CONNECTED) {
        Serial.println("Connection lost.");
        connectToWifi();
        wait(10);
    }
    if (ledStripeOnPinList.size() != currentListSize) {
        server.close();
        setup_routing();
        currentListSize = ledStripeOnPinList.size();
    }
}