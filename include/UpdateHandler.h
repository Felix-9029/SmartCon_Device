//
// Created by felix on 14.03.25.
//

#ifndef UPDATEHANDLER_H
#define UPDATEHANDLER_H

#include "ESPAsyncWebServer.h"
#include "Update.h"
#include "mbedtls/pk.h"
#include "mbedtls/base64.h"

class UpdateHandler {
public:
    void handleSystemUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

private:
    const char publicKey[453] = R"rawliteral(
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
};

#endif //UPDATEHANDLER_H
