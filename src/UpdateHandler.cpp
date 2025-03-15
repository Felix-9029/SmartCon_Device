//
// Created by felix on 14.03.25.
//

#include "UpdateHandler.h"

void UpdateHandler::handleSystemUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        if (!request->hasHeader("x-update-hash")) {
            request->send(400, "application/json", "{\"error\":\"Missing x-update-hash header\"}");
            return;
        }

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
            AsyncWebHeader const* xUpdateHashHeader = request->getHeader("x-update-hash");
            std::string encodedSignature = xUpdateHashHeader->value().c_str();

            // Determine the required buffer size for the decoded data
            size_t signatureLength = 0;
            mbedtls_base64_decode(nullptr, 0, &signatureLength, reinterpret_cast<const unsigned char *>(encodedSignature.c_str()), encodedSignature.length());

            // Allocate the buffer with the required size
            auto *signature = new unsigned char[signatureLength + 1]; // +1 for null-termination

            // Decode the base64 encoded string
            int returnCode = mbedtls_base64_decode(signature, signatureLength, &signatureLength, reinterpret_cast<const unsigned char *>(encodedSignature.c_str()), encodedSignature.length());

            if (returnCode == 0) {
                signature[signatureLength] = '\0';
            }
            else {
                request->send(500, "text/plain", "Failed to base64 decode expectedHashEncrypted.");
            }

            String updateHash = Update.md5String();
            Serial.print("MD5-Hash: ");
            Serial.println(updateHash);

            const unsigned char *updateHashUCStr = reinterpret_cast<const unsigned char *>(updateHash.c_str());

            mbedtls_pk_context pk;
            mbedtls_pk_init(&pk);
            mbedtls_pk_parse_public_key(&pk, reinterpret_cast<const unsigned char *>(publicKey), sizeof(publicKey));

            size_t hashLength = 16;

            // Verify the signature
            int verifyResult = mbedtls_pk_verify(&pk, MBEDTLS_MD_MD5, updateHashUCStr, 0, signature, signatureLength);

            // Clean up
            mbedtls_pk_free(&pk);
            delete[] signature;

            if (verifyResult != 0) {
                Serial.println("Signature verification failed.");
                Serial.println(verifyResult);
                Update.abort();
                request->send(400, "text/plain", "Signature invalid.");
            }
            else {
                Update.end(true);
                Serial.println("Signature verified and update complete.");
                request->send(200, "text/plain", "Successfully updated.");
            }

            ESP.restart();
        }
    }
}
