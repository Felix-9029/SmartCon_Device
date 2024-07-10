/**
 * @author <p>Felix Reichert</p>
 * <p>File: main.cpp</p>
 * <p>Creation date: 01.06.2022</p>
 * <p>Last update: 11.06.2024</p>
 * <p>Version: 2.1</p>
 */

#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <mbedtls/md.h>
#include <LedStripeOnPin.h>
#include <Credentials.h>
#include <mbedtls/pk.h>
#include <mbedtls/base64.h>
#include <map>

#define SYSTEM_LED 2

using namespace std;

// ------------------------------------- function prototypes -------------------------------------

void connectToWifi();

void setupRouting();

void handleSystemUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

bool verifySignature(const unsigned char *fullData, size_t fullDataLength, const unsigned char *signature, size_t signatureLength);

void handlePinListGet(AsyncWebServerRequest *request);

void handleLedPost(AsyncWebServerRequest *request, JsonObject &jsonObject);

void handleLedDelete(AsyncWebServerRequest *request, JsonObject &jsonObject);

void setColor(uint32_t color);

uint32_t wheel(byte wheelPos);

[[noreturn]] void animationSet(void *parameter);

void wait(int interval);

// ------------------------------------- global variables -------------------------------------

AsyncWebServer *server;
vector<LedStripeOnPin *> ledStripeOnPinList;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(0, 0, NEO_RGBW + NEO_KHZ800);

TaskHandle_t AnimationTask;

const unsigned char publicKey[] = R"rawliteral(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAroYF67mLl0qbzzC5UJDg
P9x7pSZ7lsxT7mZG0/C7ZGFqBnzEetuMJgiRtU/OywfCTm6KoC6TlRgblWYZov7x
CK84vRA1cVGZn8bBJH/jniqoATHAaBAZ0xFiyokxJWS6OXjbx9+toRmg0g9FbObV
ukRkum6DCUn32+ItIQ2yrdQZ6vSof1Z4nmlhJB2mA5wEOO+Hhricm+4jrXl4ckhj
Gendl9eO8Z7LfxkCWlxqE6b42H6SUWQNa/PG09BWRi/7063kphXtZP2GVZw7sRDh
71NWzmF4+n4E6jvhndTVY5qHVvuZ+DE6x2cE9ZUqQlxf85Rvsx6emDZa/4v1yk+2
sQIDAQAB
-----END PUBLIC KEY-----
)rawliteral";

// ------------------------------------- init -------------------------------------

void setup() {
    Serial.begin(115200);
    pinMode(SYSTEM_LED, OUTPUT);
    digitalWrite(SYSTEM_LED, LOW);
    connectToWifi();
    server = new AsyncWebServer(80);
    setupRouting();
}

void loop() {
    if (WiFiClass::status() != WL_CONNECTED) {
        Serial.println("Connection lost.");
        connectToWifi();
        wait(10);
    }
}

// ------------------------------------- Setup wifi -------------------------------------

void connectToWifi() {
    Serial.printf("Connecting to %s\n", SSID);

    byte mac[6];
    WiFi.macAddress(mac);
    char macPart[7 + 1];
    sprintf(macPart, "_%02x%02x%02x", mac[3], mac[4], mac[5]);

    std::string hostnameString = "EasyLED";
    hostnameString.append(macPart);

    WiFiClass::setHostname(hostnameString.c_str());
    WiFi.begin(SSID, PWD);
    int resetCounter = 0;
    while (WiFiClass::status() != WL_CONNECTED) {
        resetCounter++;
        if (resetCounter >= 100) {
            ESP.restart();
        }
        Serial.print(".");
        digitalWrite(SYSTEM_LED, !digitalRead(SYSTEM_LED));
        wait(800);
    }
    digitalWrite(SYSTEM_LED, 0);

    Serial.println("\nConnected.");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Hostname: %s\n", WiFiClass::getHostname());
    Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void setupRouting() {
    server->on("/api/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, handleSystemUpdate);


    for (const LedStripeOnPin *ledStripeOnPinTmp: ledStripeOnPinList) {
        std::string path = "/api/pin/";
        path.append(to_string(ledStripeOnPinTmp->getPin()));
        server->on(path.c_str(), HTTP_GET, [ledStripeOnPinTmp](AsyncWebServerRequest *request) {
            request->send(200, "application/json", ledStripeOnPinTmp->getBuffer());
        });
    }

    server->on("/api/pin", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->url() != "/api/pin") {
            return;
        }
        handlePinListGet(request);
    });

    AsyncCallbackJsonWebHandler *addLedHandler = new AsyncCallbackJsonWebHandler("/api/pin", [](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        handleLedPost(request, jsonObject);
    });
    addLedHandler->setMethod(HTTP_POST);
    server->addHandler(addLedHandler);

    AsyncCallbackJsonWebHandler *deleteLedHandler = new AsyncCallbackJsonWebHandler("/api/pin", [](AsyncWebServerRequest *request, JsonVariant const &json) {
        JsonObject jsonObject = json.as<JsonObject>();
        handleLedDelete(request, jsonObject);
    });
    deleteLedHandler->setMethod(HTTP_DELETE);
    server->addHandler(deleteLedHandler);

    server->begin();
}

// ------------------------------------- OTA - INSECURE -------------------------------------
std::map<String, std::vector<uint8_t>> clientDataMap;
void handleSystemUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        Serial.println("Update started");
        size_t update_content_len = request->contentLength();

        if (!Update.begin(update_content_len, U_FLASH)) {
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len) {
        Update.printError(Serial);
    }

    if (final) {
        if (!Update.end(true)) {
            Update.printError(Serial);
        }
        else {
            Serial.println("Update complete");
            ESP.restart();
        }
    }

    return;



    // TODO WIP

    String clientKey = request->client()->remoteIP().toString(); // Eindeutige Kennung des Clients

    if (!index) {
        Serial.println("Update started");
        size_t update_content_len = request->contentLength();

        if (!Update.begin(update_content_len, U_FLASH)) {
            Update.printError(Serial);
            return;
        }
        clientDataMap[clientKey].clear(); // Initialisiere den Vektor für diesen Client
    }

    // Füge die empfangenen Daten zum Vektor dieses Clients hinzu
    clientDataMap[clientKey].insert(clientDataMap[clientKey].end(), data, data + len);

    if (final) {
        std::vector<uint8_t>& fullDataVector = clientDataMap[clientKey];
        size_t fullDataLength = fullDataVector.size();

        uint8_t *fullData = new unsigned char[fullDataLength];
        std::copy(fullDataVector.begin(), fullDataVector.end(), fullData);

        std::string expectedHashEncryptedBase64;
        size_t maxDecodedLength = expectedHashEncryptedBase64.length() * 3 / 4;
        unsigned char *outputBuffer = new unsigned char[maxDecodedLength + 1]; // +1 für Null-Terminierung
        size_t outputLength = 0;
        int ret = mbedtls_base64_decode(outputBuffer, maxDecodedLength, &outputLength, reinterpret_cast<const unsigned char *>(expectedHashEncryptedBase64.c_str()), expectedHashEncryptedBase64.length());

        if (ret == 0) {
            outputBuffer[outputLength] = '\0';
        }
        else {
            request->send(500, "text/plain", "Failed to base64 decode expectedHashEncrypted.");
        }

        bool isValid = verifySignature(fullData, fullDataLength, outputBuffer, outputLength);
        clientDataMap.erase(clientKey);
        delete[] outputBuffer;

        if (!isValid) {
            request->send(400, "text/plain", "Signature invalid.");
            Serial.println("Signature invalid.");
            Update.abort();
        }
        else {
            Update.write(fullData, fullDataLength);
            Update.end(true);
            Serial.println("Signature verified and update complete.");
        }

        delete[] fullData;
        ESP.restart();
    }
}

bool verifySignature(const unsigned char *fullData, size_t fullDataLength, const unsigned char *signature, size_t signatureLength) {
    int ret;
    mbedtls_pk_context pk;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    unsigned char hash[32];

    mbedtls_pk_init(&pk);

    // Lade den öffentlichen Schlüssel
    if ((ret = mbedtls_pk_parse_public_key(&pk, publicKey, sizeof(publicKey))) != 0) {
        mbedtls_pk_free(&pk);
        return false;
    }

    // Berechne den Hash der Daten
    if ((ret = mbedtls_md(mbedtls_md_info_from_type(md_type), fullData, fullDataLength, hash)) != 0) {
        mbedtls_pk_free(&pk);
        return false;
    }

    Serial.println("Hash:");
    for (int i = 0; i < 32; i++) {
        Serial.printf("%02x", hash[i]);
    }
    Serial.println();

    Serial.println("Signature:");
    for (int i = 0; i < signatureLength; i++) {
        Serial.printf("%02x", signature[i]);
    }
    Serial.println();

    // Verifiziere die Signatur
    ret = mbedtls_pk_verify(&pk, md_type, hash, 0, signature, signatureLength);

    mbedtls_pk_free(&pk);

    return ret == 0; // Gibt true zurück, wenn die Verifizierung erfolgreich war
}

// ------------------------------------- handle values from net get -------------------------------------

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

// ------------------------------------- handle values from net post -------------------------------------

void handleLedPost(AsyncWebServerRequest *request, JsonObject &jsonObject) {
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

// ------------------------------------- handle values from net delete -------------------------------------

void handleLedDelete(AsyncWebServerRequest *request, JsonObject &jsonObject) {
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

// ------------------------------------- helper -------------------------------------

void wait(int interval) {
    unsigned long endMillis = millis() + interval;
    while (true) {
        if (endMillis <= millis()) {
            break;
        }
    }
}
