
#include <Arduino.h>
#include <SystemCore.h>
#include "Config.h"
#include <avr/wdt.h>

SystemCore systemCore;

void setup() {
  systemCore.setup();
  // wdt_enable(WDTO_2S);
}

void loop() {
  // wdt_enable(WDTO_2S);
  systemCore.loop();
}