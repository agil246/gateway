#include "e220.h"

void setupE220() {
    Serial.println("E220 init (dummy)");
}

bool receiveData(char* buffer, size_t len) {
    static unsigned long last = 0;
    if (millis() - last > 5000) {
        last = millis();
        const char* dummy = "{\"id\":\"PJU-001\",\"state\":1,\"vin\":220,\"crc\":123}";
        strncpy(buffer, dummy, len);
        return true;
    }
    return false;
}