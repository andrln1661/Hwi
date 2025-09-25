#include "CurrentSensor.h"
#include "ModbusHandler.h"
#include "Config.h"
#include "Globals.h"

CurrentSensor::CurrentSensor(uint8_t pin, uint16_t regAddr)
    : pin(pin), regAddr(regAddr), filteredValue(0),
      smoothingFactor(DEFAULT_SMOOTHING), lastSampleTime(0) {}

void CurrentSensor::begin()
{
    pinMode(pin, INPUT);
    if (pin == A0)
        analogReference(INTERNAL1V1);

    // Initial reading to start filter
    uint16_t raw = analogRead(pin);
    // float current = float(raw - 512) / 1024 * 5 / 185 * 1000;
    current = raw;
    modbusHandler->setIreg(regAddr, raw);
}

void CurrentSensor::update(uint64_t now)
{
    if (now - lastSampleTime < SAMPLE_INTERVAL)
        return;

    lastSampleTime = now;
    uint16_t raw = analogRead(pin);

    // Exponential smoothing
    // filteredValue = smoothingFactor * raw + (1 - smoothingFactor) * filteredValue;
    // float voltage = (raw - 512) / 1024 * 5;
    // current = voltage / 66 * 1000; // 27.03 for 1V ref
    current = raw;

    // Update Modbus register
    modbusHandler->setIreg(regAddr, raw);
}

float CurrentSensor::getCurrent() const
{
    return current;
}

// void CurrentSensor::setSmoothingFactor(float factor)
// {
//     smoothingFactor = constrain(factor, 0.0f, 1.0f);
// }