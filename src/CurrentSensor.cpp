#include "CurrentSensor.h"
#include "ModbusHandler.h"
#include "Config.h"
#include "Globals.h"

CurrentSensor::CurrentSensor(uint8_t pin, uint16_t regAddr)
    : pin(pin), regAddr(regAddr), filteredValue(0),
      smoothingFactor(DEFAULT_SMOOTHING), lastSampleTime(0) {}

void CurrentSensor::begin() {
    pinMode(pin, INPUT);
    
    // Initial reading to start filter
    uint16_t raw = analogRead(pin);
    filteredValue = raw;
}

void CurrentSensor::update(uint64_t now) {
    if (now - lastSampleTime < SAMPLE_INTERVAL) return;
    
    lastSampleTime = now;
    uint16_t raw = analogRead(pin);
    
    // Exponential smoothing
    filteredValue = smoothingFactor * raw + (1 - smoothingFactor) * filteredValue;
    
    // Convert to mA (ACS712-5A specific conversion)
    uint16_t current = (filteredValue - 512) * 1000 / 66;

    // Update Modbus register
    modbusHandler->setIreg(regAddr, current);
}

uint16_t CurrentSensor::getCurrent() const {
    return (filteredValue - 512) * 1000 / 66;
}

void CurrentSensor::setSmoothingFactor(float factor) {
    smoothingFactor = constrain(factor, 0.0f, 1.0f);
}