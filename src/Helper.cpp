//
// Created by felix on 14.03.25.
//

#include "Helper.h"

void wait(int interval) {
    unsigned long endMillis = millis() + interval;
    while (true) {
        if (endMillis <= millis()) {
            break;
        }
    }
}