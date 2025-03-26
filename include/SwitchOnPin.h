//
// Created by felix on 15.03.25.
//

#ifndef SWITCHONPIN_H
#define SWITCHONPIN_H

#pragma once

#include "ArduinoJson.h"

class SwitchOnPin {
public:
    // ------------------------------------- getter and setter -------------------------------------
    short getPin() const {
        return _pin;
    }

    void setPin(short pin) {
        _pin = pin;
    }

    bool getStateOn() const {
        return _stateOn;
    }

    void setStateOn(bool stateOn) {
        _stateOn = stateOn;
    }

    const char* getBuffer() const {
        return _buffer;
    }

    void setBuffer(char buffer[512]) {
        strcpy(_buffer, buffer);
    }

    void writeBuffer() {
        serializeJson(getInfo(), _buffer);
    }

    SwitchOnPin() = default;

    SwitchOnPin(const SwitchOnPin &rhs) {
        _pin = rhs.getPin();
        _stateOn = rhs.getStateOn();
    }

    virtual ~SwitchOnPin() = default;

    bool operator==(const SwitchOnPin &rhs) const {
        return _pin == rhs._pin &&
               _stateOn == rhs._stateOn;
    }

    bool operator!=(const SwitchOnPin &rhs) const {
        return !(rhs == *this);
    }

    // ------------------------------------- put current values into jsonObject -------------------------------------
    JsonDocument getInfo() const {
        JsonDocument jsonDocument;
        jsonDocument.clear();
        JsonObject jsonObject = jsonDocument.add<JsonObject>();
        jsonObject["stateOn"] = this->getStateOn();
        return jsonDocument;
    }

private:
    short _pin = 0;
    bool _stateOn = false;

    char _buffer[512] = { };
};

#endif //SWITCHONPIN_H
