#include <Arduino.h>
#include <SystemCore.h>
#include "Config.h"

SystemCore systemCore;

void setup() {
  systemCore.setup();
}

void loop() {
  systemCore.loop();
}