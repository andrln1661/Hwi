#pragma once
#include "TemperatureSensor.h"
#include "Motor.h"

class DeviceManager {
public:
    void begin();

    // Main update logic — called every loop()
    void update(const TemperatureSensor& airSensor, 
                const TemperatureSensor& waterSensor, 
                const Motor motors[]);

    // Expose control methods (e.g. for override or testing)
    void controlFan(bool state);
    void controlMixer(bool state);
    void controlDispenser(bool state);
    void controlPump(bool state);
};
