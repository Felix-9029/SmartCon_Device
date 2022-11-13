#include <list>
#include <ArduinoJson.h>

class LedStripeOnPin {
    boolean _colorMode = true;
    short _pin = 0;
    int _ledCount = 0;
    boolean _stateOn = false;

    u32_t _brightness = 0;
    u32_t _red = 0;
    u32_t _green = 0;
    u32_t _blue = 0;
    u32_t _white = 0;

    String _animation;

    char _buffer[512] = { };

public:
    // ------------------------------------- getter and setter -------------------------------------
    boolean getColorMode() const {
        return _colorMode;
    }

    void setColorMode(boolean colorMode) {
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

    boolean getStateOn() const {
        return _stateOn;
    }

    void setStateOn(boolean stateOn) {
        _stateOn = stateOn;
    }

    u32_t getBrightness() const {
        return _brightness;
    }

    void setBrightness(u32_t brightness) {
        _brightness = brightness;
    }

    u32_t getRed() const {
        return _red;
    }

    void setRed(u32_t red) {
        _red = red;
    }

    u32_t getGreen() const {
        return _green;
    }

    void setGreen(u32_t green) {
        _green = green;
    }

    u32_t getBlue() const {
        return _blue;
    }

    void setBlue(u32_t blue) {
        _blue = blue;
    }

    u32_t getWhite() const {
        return _white;
    }

    void setWhite(u32_t white) {
        _white = white;
    }

    const String &getAnimation() const {
        return _animation;
    }

    void setAnimation(const String &animation) {
        _animation = animation;
    }

    const char* getBuffer() const {
        return _buffer;
    }

    void setBuffer(char buffer[512]) {
        strcpy(_buffer, buffer);
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
    u32_t applyBrightnessToLight(int color) const {
        u32_t lightValue = 0;
        switch (color) {
            case 0:
                lightValue = this->getGreen();
                break;
            case 1:
                lightValue = this->getRed();
                break;
            case 2:
                lightValue = this->getBlue();
                break;
            default:
                break;
        }
        if (this->getStateOn()) {
            // FIXME fix these nearly illegal casts
            auto value = static_cast<u32_t>((static_cast<double>(lightValue) / 255) * this->getBrightness());
            if (value > 255) {
                return 255;
            }
            else {
                return value;
            }
        }
        else {
            return 0;
        }
    }

    // ------------------------------------- put current values into jsonObject -------------------------------------
    StaticJsonDocument<512> getInfo() const {
        StaticJsonDocument<512> jsonDocument;
        jsonDocument.clear();
        JsonObject jsonObject = jsonDocument.createNestedObject();
        jsonObject["colorMode"] = this->getColorMode();
        jsonObject["pin"] = this->getPin();
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
};