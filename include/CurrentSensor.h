#pragma once
#include <Arduino.h>


class CurrentSensor
{
public:
    CurrentSensor(uint8_t pin, uint16_t regAddr);
    void begin();
    void update(uint64_t now);
    float getCurrent() const;
    void setSmoothingFactor(float factor);

private:
    uint8_t pin;
    uint16_t regAddr;
    float filteredValue;
    float smoothingFactor;
    uint32_t lastSampleTime;
    float current;

    static constexpr uint32_t SAMPLE_INTERVAL = 10; // ms
    static constexpr float DEFAULT_SMOOTHING = 0.15f;
};