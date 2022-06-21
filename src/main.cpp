#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include "credentials.h"
#define LED 2

const char *HOSTNAME = "geile-Schnitte";

u32_t globalRed = 0;
u32_t globalGreen = 0;
u32_t globalBlue = 0;
u32_t globalWhite = 0;

short pin = 32;
int ledCount = 20;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(ledCount, pin, NEO_RGBW + NEO_KHZ800);

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

/*
 * setup function
 */


void colorSet(uint32_t c, uint8_t wait) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
    }
}

StaticJsonDocument<250> jsonDocument;
char buffer[250];
void create_json(char *tag, float value) {
    jsonDocument.clear();
    jsonDocument["type"] = tag;
    jsonDocument["value"] = value;
    serializeJson(jsonDocument, buffer);
}

void add_json_object(char *key, float value) {
    JsonObject obj = jsonDocument.createNestedObject();
    obj["type"] = key;
    obj["value"] = value;
}


WebServer server(80);
void connectToWifi() {
    Serial.printf("Connecting to %s\n", SSID);

    WiFiClass::hostname(HOSTNAME);
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
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.printf("MAC address: %x:%x:%x:%x:%x:%x\n", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
}

void handlePost() {
    if (!server.hasArg("plain")) {
        // TODO: error ausgeben, http code oder so
    }
    String body = server.arg("plain");
    deserializeJson(jsonDocument, body);
    if (ledCount != jsonDocument["ledCount"]) {
        strip.clear();
    }



    Serial.printf("delete me Pin: %d LED Count: %d\n", pin, ledCount);
    Serial.printf("delete me R: %d G: %d B: %d W: %d\n", globalRed, globalGreen, globalBlue, globalWhite);

    pin = jsonDocument["pin"];
    ledCount = jsonDocument["ledCount"];
    globalRed = jsonDocument["red"];
    globalGreen = jsonDocument["green"];
    globalBlue = jsonDocument["blue"];
    globalWhite = jsonDocument["white"];

    Serial.printf("Pin: %d LED Count: %d\n", pin, ledCount);
    Serial.printf("R: %d G: %d B: %d W: %d\n", globalRed, globalGreen, globalBlue, globalWhite);

    strip.setPin(pin);
    strip.updateLength(ledCount);
    colorSet(Adafruit_NeoPixel::Color(globalGreen, globalRed, globalBlue, globalWhite), 0);
    strip.show();

    server.send(200, "application/json", "{}");
}

// void handleGet() {
//
// }

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
void getInfo() {
    jsonDocument.clear();
    // FIXME: change casting [-Wwrite-strings] converting a string constant to 'char*' forbitten in C** ISO ## nils
    // might be a security vuln or some other shit, workes for now
    // TODO: replace this global shit with a nice get function -> might be more complex due to more than one color per strip (animations / fade etc)
    add_json_object("red", globalRed);
    add_json_object("green", globalGreen);
    add_json_object("blue", globalBlue);
    add_json_object("white", globalWhite);
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}

void setup_routing() {
    server.on("/led", HTTP_POST, handlePost);
    //server.on("/info", getInfo);
    //server.on("/led", HTTP_GET, handleGet);
    // handleUpdate(); // FIXME: WHAT THE FUCK IS THIS DO NOT! ENABLE
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