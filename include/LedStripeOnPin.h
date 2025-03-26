#ifndef LEDSTRIPEONPIN_H
#define LEDSTRIPEONPIN_H

#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include "ArduinoJson.h"

enum class Color {
    Green,
    Red,
    Blue
};

class LedStripeOnPin {
public:
    // ------------------------------------- getter and setter -------------------------------------
    bool getColorMode() const {
        return _colorMode;
    }

    void setColorMode(bool colorMode) {
        _colorMode = colorMode;
    }

    short getPin() const {
        return _pin;
    }

    void setPin(short pin) {
        _pin = pin;
    }

    int getLedCount() const {
        return _ledCount;
    }

    void setLedCount(int ledCount) {
        _ledCount = ledCount;
    }

    bool getStateOn() const {
        return _stateOn;
    }

    void setStateOn(bool stateOn) {
        _stateOn = stateOn;
    }

    uint32_t getBrightness() const {
        return _brightness;
    }

    void setBrightness(uint32_t brightness) {
        _brightness = brightness;
    }

    uint32_t getRed() const {
        return _red;
    }

    void setRed(uint32_t red) {
        _red = red;
    }

    uint32_t getGreen() const {
        return _green;
    }

    void setGreen(uint32_t green) {
        _green = green;
    }

    uint32_t getBlue() const {
        return _blue;
    }

    void setBlue(uint32_t blue) {
        _blue = blue;
    }

    uint32_t getWhite() const {
        return _white;
    }

    void setWhite(uint32_t white) {
        _white = white;
    }

    const std::string &getAnimation() const {
        return _animation;
    }

    void setAnimation(const std::string &animation) {
        _animation = animation;
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

    LedStripeOnPin() = default;

    LedStripeOnPin(const LedStripeOnPin &rhs) {
        _colorMode = rhs.getColorMode();
        _pin = rhs.getPin();
        _ledCount = rhs.getLedCount();
        _stateOn = rhs.getStateOn();
        _brightness = rhs.getBrightness();
        _red = rhs.getRed();
        _green = rhs.getGreen();
        _blue = rhs.getBlue();
        _white = rhs.getWhite();
        _animation = rhs.getAnimation();
    }

    virtual ~LedStripeOnPin() = default;

    bool operator==(const LedStripeOnPin &rhs) const {
        return _colorMode == rhs._colorMode &&
               _pin == rhs._pin &&
               _ledCount == rhs._ledCount &&
               _stateOn == rhs._stateOn &&
               _brightness == rhs._brightness &&
               _red == rhs._red &&
               _green == rhs._green &&
               _blue == rhs._blue &&
               _white == rhs._white &&
               _animation == rhs._animation;
    }

    bool operator!=(const LedStripeOnPin &rhs) const {
        return !(rhs == *this);
    }

    // ------------------------------------- calc brightness -------------------------------------
    uint32_t applyBrightnessToLight(Color color) const {
        static const std::unordered_map<Color, std::function<uint32_t(void)>> colorToFunction = {
                {Color::Green, [this] { return this->getGreen(); }},
                {Color::Red, [this] { return this->getRed(); }},
                {Color::Blue, [this] { return this->getBlue(); }}
        };

        uint32_t lightValue = colorToFunction.at(color)();

        if (this->getStateOn()) {
            auto value = (lightValue / 255) * this->getBrightness();
            return std::min(value, 255u);
        }

        return 0;
    }

    // ------------------------------------- put current values into jsonObject -------------------------------------
    JsonDocument getInfo() const {
        JsonDocument jsonDocument;
        jsonDocument.clear();
        JsonObject jsonObject = jsonDocument.add<JsonObject>();
        jsonObject["colorMode"] = this->getColorMode();
        jsonObject["ledCount"] = this->getLedCount();
        jsonObject["stateOn"] = this->getStateOn();
        jsonObject["brightness"] = this->getBrightness();
        if (getColorMode()) {
            jsonObject["red"] = this->getRed();
            jsonObject["green"] = this->getGreen();
            jsonObject["blue"] = this->getBlue();
            jsonObject["white"] = this->getWhite();
        }
        else {
            jsonObject["globalAnimation"] = this->getAnimation();
        }
        return jsonDocument;
    }

private:
    bool _colorMode = true;
    short _pin = 0;
    int _ledCount = 0;
    bool _stateOn = false;

    uint32_t _brightness = 0;
    uint32_t _red = 0;
    uint32_t _green = 0;
    uint32_t _blue = 0;
    uint32_t _white = 0;

    std::string _animation;

    char _buffer[512] = { };
};

#endif //LEDSTRIPEONPIN_H
