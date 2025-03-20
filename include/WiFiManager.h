//
// Created by felix on 14.03.25.
//

#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include "WiFi.h"
#include "Helper.h"

class WiFiManager {
public:
    void init();
    void connectToWifi(const char* ssid, const char* password);

private:
    byte _mac[6] = {};
};

#endif //WIFIMANAGER_H
