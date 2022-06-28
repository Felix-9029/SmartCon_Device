#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <string>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include "credentials.h"

#define LED 2

using namespace std;


// ------------------------------------- global variables -------------------------------------

boolean colorMode = true;
short pin = 13;
int ledCount = 25;
boolean stateOn = true;

u32_t globalBrightness = 0;
u32_t globalRed = 0;
u32_t globalGreen = 0;
u32_t globalBlue = 0;
u32_t globalWhite = 0;
string globalAnimation;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(ledCount, pin, NEO_RGBW + NEO_KHZ800);

TaskHandle_t AnimationTask;

/* TODO
typedef struct Data_t {
    Adafruit_NeoPixel strip;
    string globalAnimation;
} CurrentData_t;
*/


// ------------------------------------- Setup wifi -------------------------------------

WebServer server(80);

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
    while (WiFiClass::status() != WL_CONNECTED) {
        Serial.print(".");
        digitalWrite(LED, HIGH);
        delay(200);
        digitalWrite(LED, LOW);
        delay(200);
    }

    Serial.println("\nConnected.");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Hostname: %s\n", WiFiClass::getHostname());
    Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


// ------------------------------------- OTA - INSECURE -------------------------------------

const char *loginIndex =
        "<form name='loginForm'>"
        "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
        "<td colspan=2>"
        "<center><font size=4><b>ESP32 Login Page</b></font></center>"
        "<br>"
        "</td>"
        "<br>"
        "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
        "<td>Password:</td>"
        "<td><input type='Password' size=25 name='pwd'><br></td>"
        "<br>"
        "<br>"
        "</tr>"
        "<tr>"
        "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
        "</table>"
        "</form>"
        "<script>"
        "function check(form)"
        "{"
        "if(form.userid.value=='admin' && form.pwd.value=='admin')"
        "{"
        "window.open('/serverIndex')"
        "}"
        "else"
        "{"
        " alert('Error Password or Username')/*displays error message*/"
        "}"
        "}"
        "</script>";

/*
 * Server Index Page
 */

const char *serverIndex =
        "<script "
        "src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></"
        "script>"
        "<form method='POST' action='#' enctype='multipart/form-data' "
        "id='upload_form'>"
        "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
        "</form>"
        "<div id='prg'>progress: 0%</div>"
        "<script>"
        "$('form').submit(function(e){"
        "e.preventDefault();"
        "var form = $('#upload_form')[0];"
        "var data = new FormData(form);"
        " $.ajax({"
        "url: '/update',"
        "type: 'POST',"
        "data: data,"
        "contentType: false,"
        "processData:false,"
        "xhr: function() {"
        "var xhr = new window.XMLHttpRequest();"
        "xhr.upload.addEventListener('progress', function(evt) {"
        "if (evt.lengthComputable) {"
        "var per = evt.loaded / evt.total;"
        "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
        "}"
        "}, false);"
        "return xhr;"
        "},"
        "success:function(d, s) {"
        "console.log('success!')"
        "},"
        "error: function (a, b, c) {"
        "}"
        "});"
        "});"
        "</script>";

void handleUpdate() {
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
    });
    server.on("/serverIndex", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
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
                    if (!Update.begin(
                            UPDATE_SIZE_UNKNOWN)) {  // start with max available size
                        Update.printError(Serial);
                    }
                } else if (upload.status == UPLOAD_FILE_WRITE) {
                    /* flashing firmware to ESP*/
                    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                        Update.printError(Serial);
                    }
                } else if (upload.status == UPLOAD_FILE_END) {
                    if (Update.end(
                            true)) {  // true to set the size to the current progress
                        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                    } else {
                        Update.printError(Serial);
                    }
                }
            });
}


// ------------------------------------- strip set colors -------------------------------------

void colorSet(uint32_t color) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
}


// ------------------------------------- strip set globalAnimation -------------------------------------

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if(WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


[[noreturn]] void animationSet(void *parameter) {
    // TODO add globalAnimation with duration, (multiple) colors, length

    /* TODO
    auto *data = (CurrentData_t *) parameter;
    */
    unsigned long previousMillis = 0;
    int interval = 50;

    unsigned long currentMillis;


    uint16_t i, j;

    while(true) {
        for (j = 0; j < 256; j++) {
            for (i = 0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, Wheel((i + j) & 255));
            }
            strip.show();
            while (true) {
                currentMillis = millis();
                if (currentMillis - previousMillis > interval) {
                    previousMillis = currentMillis;
                    break;
                }
            }
        }
    }
}


// ------------------------------------- Host current values to update app instances -------------------------------------

StaticJsonDocument<500> jsonDocument;
char buffer[500];

void getInfo() {
    jsonDocument.clear();
    // TODO: replace this global shit with a nice get function -> might be more complex due to more than one color per strip (animations / fade etc)
    JsonObject jsonObject = jsonDocument.createNestedObject();
    jsonObject["colorMode"] = colorMode;
    jsonObject["pin"] = pin;
    jsonObject["ledCount"] = ledCount;
    jsonObject["stateOn"] = stateOn;
    jsonObject["brightness"] = globalBrightness;
    if (colorMode) {
        jsonObject["red"] = globalRed;
        jsonObject["green"] = globalGreen;
        jsonObject["blue"] = globalBlue;
        jsonObject["white"] = globalWhite;
    }
    else {
        jsonObject["globalAnimation"] = globalAnimation;
    }
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}


// ------------------------------------- handle values from app post -------------------------------------

u32_t lightApplyBrightness(u32_t light) {
    if (stateOn) {
        // FIXME fix these nearly illegal casts
        auto value = static_cast<u32_t>((static_cast<double>(light) / 255) * globalBrightness);
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

void handlePost() {
    if (!server.hasArg("plain")) {
        // TODO: error ausgeben, http code oder so
    }
    String body = server.arg("plain");
    deserializeJson(jsonDocument, body);

    colorMode = jsonDocument["colorMode"];
    pin = jsonDocument["pin"];
    ledCount = jsonDocument["ledCount"];
    stateOn = jsonDocument["stateOn"];
    globalBrightness = jsonDocument["brightness"];
    strip.setPin(pin);
    strip.updateLength(ledCount);

    if (colorMode) {
        globalRed = jsonDocument["red"];
        globalGreen = jsonDocument["green"];
        globalBlue = jsonDocument["blue"];
        globalWhite = jsonDocument["white"];
        if (AnimationTask != nullptr) {
            vTaskDelete(AnimationTask);
            AnimationTask = nullptr;
        }

        Serial.printf("R: %d G: %d B: %d W: %d\n", globalRed, globalGreen, globalBlue, globalWhite);
        colorSet(Adafruit_NeoPixel::Color(lightApplyBrightness(globalGreen), lightApplyBrightness(globalRed), lightApplyBrightness(globalBlue), stateOn ? globalWhite : 0));
        strip.show();
    } else {
        globalAnimation = jsonDocument["globalAnimation"].as<std::string>();
        if (AnimationTask != nullptr) {
            vTaskDelete(AnimationTask);
            AnimationTask = nullptr;
        }
        /* TODO
        CurrentData_t currentData = {strip, globalAnimation};
        */
        xTaskCreatePinnedToCore(
                animationSet,             /* Task function. */
                "AnimationTask",             /* name of task. */
                10000,                   /* Stack size of task */
                /* TODO
                (void *) &currentData,   /* parameter of the task */
                nullptr,
                1,                          /* priority of the task */
                &AnimationTask,          /* Task handle to keep track of created task */
                1                             /* pin task to core 0 */
        );
    }

    server.send(200, "application/json", "{}");
}


// ------------------------------------- setup -------------------------------------

void setup_routing() {
    server.on("/led", HTTP_POST, handlePost);
    server.on("/info", getInfo);
    handleUpdate(); // FIXME: WHAT THE FUCK IS THIS DO NOT! ENABLE
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
        delay(1);
    }
}