//
// Created by felix on 14.03.25.
//

#include "Helper.h"

void Helper::wait(int interval) {
    unsigned long endMillis = millis() + interval;
    while (true) {
        if (endMillis <= millis()) {
            break;
        }
    }
}

boolean Helper::isPinUnusable(short pin) {
    return pin < 0 // does not exist
        || pin == 0 // boot mode (BOOT button)
        || pin == 1 // TX
        || pin == 2 // internal SYSTEM_LED
        || pin == 3 // RX
        || (pin >= 6 && pin <= 11) // flash pins

        // || pin == 12 // MUST be low on startup AND blocked only in dev mode -> debugging port
        // || pin == 13 // blocked only in dev mode -> debugging port
        // || pin == 14 // blocked only in dev mode -> debugging port
        // || pin == 15 // blocked only in dev mode -> debugging port

        || pin == 20 // does not exist
        || pin == 24 // does not exist
        || (pin >= 28 && pin <= 31) // does not exist
        || (pin >= 34 && pin <= 36) // input only
        || pin == 37 // does not exist
        || pin == 38 // does not exist
        || pin == 39 // input only
        || pin >= 40; /* does not exist */
}