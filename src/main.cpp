#include <Arduino.h>

void setupGateway();
void loopGateway();

void setup() {
  Serial.begin(115200);
  setupGateway();
}

void loop() {
  loopGateway();
}