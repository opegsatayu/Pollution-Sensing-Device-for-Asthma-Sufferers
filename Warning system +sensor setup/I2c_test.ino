#include <Wire.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("scanning for I2C devices...");
    Wire.begin(21 , 22);
}

void loop() {
    byte error, address;
    int nDevices = 0;
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            Serial.println(address, HEX);
            nDevices++;
        }
    }
    if (nDevices == 0) Serial.println("No I2C devices found.");
    delay(5000);
}
