
#pragma once
#include "TemperatureSensor.h"
#include "Motor.h"

class DeviceManager {
public:
    void begin();

    // Changed third parameter type to const Motor* const*
    void update(const TemperatureSensor& airSensor, 
                const TemperatureSensor& waterSensor, 
                const Motor* const* motors); // Fixed parameter type

    void controlFan(bool state);
    void controlMixer(bool state);
    void controlDispenser(bool state);
    void controlPump(bool state);
};