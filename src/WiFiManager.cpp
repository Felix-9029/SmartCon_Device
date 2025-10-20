//
// Created by felix on 14.03.25.
//

#include "WiFiManager.h"

#ifndef SYSTEM_LED
#define SYSTEM_LED 2
#endif

void WiFiManager::init() {
    WiFi.macAddress(mac);
    char macPart[7 + 1];
    sprintf(macPart, "_%02x%02x%02x", mac[3], mac[4], mac[5]);

    std::string hostnameString = "EasyLED";
    hostnameString.append(macPart);

    WiFiClass::setHostname(hostnameString.c_str());
}

void WiFiManager::connectToWifi(const char* ssid, const char* password) {
    Serial.printf("Connecting to %s\n", ssid);

    WiFi.begin(ssid, password);
    int resetCounter = 0;
    while (WiFiClass::status() != WL_CONNECTED) {
        resetCounter++;
        if (resetCounter >= 100) {
            ESP.restart();
        }
        Serial.print(".");
        digitalWrite(SYSTEM_LED, !digitalRead(SYSTEM_LED));
        Helper::wait(800);
    }
    digitalWrite(SYSTEM_LED, 0);

    Serial.println("\nConnected.");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Hostname: %s\n", WiFiClass::getHostname());
    Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}