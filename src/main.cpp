/**
 * @author <p>Felix Reichert</p>
 * <p>File: main.cpp</p>
 * <p>Creation date: 01.06.2022</p>
 * <p>Last update: 11.06.2024</p>
 * <p>Version: 2.1</p>
 */

#include "Preferences.h"
#include "Adafruit_NeoPixel.h"
#include "AsyncJson.h"
#include "Update.h"
#include "ESPAsyncWebServer.h"
#include "WiFi.h"
#include "mbedtls/md.h"
#include "Credentials.h"
#include "mbedtls/pk.h"

#include "WiFiManager.h"
#include "WebServerManager.h"
#include "Helper.h"

#define SYSTEM_LED 2

using namespace std;

WiFiManager wifiManager;
WebServerManager webServerManager;

void setup() {
    Serial.begin(115200);
    pinMode(SYSTEM_LED, OUTPUT);
    digitalWrite(SYSTEM_LED, LOW);
    wifiManager.init();
    wifiManager.connectToWifi(SSID, PWD);
    webServerManager.start();
    webServerManager.setupRouting();
}

void loop() {
    if (WiFiClass::status() != WL_CONNECTED) {
        Serial.println("Connection lost.");
        wifiManager.connectToWifi(SSID, PWD);
        Helper::wait(10);
    }
}