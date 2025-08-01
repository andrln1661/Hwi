#include <Arduino.h>
#include <SystemCore.h>

SystemCore systemCore;

void setup() {
  systemCore.setup();
}

void loop() {
  systemCore.loop();
}