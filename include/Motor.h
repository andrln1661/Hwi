#pragma once
#include <Arduino.h>
#include "TemperatureSensor.h"
#include "CurrentSensor.h"  // Add include

class ModbusHandler;

class Motor {
public:
    Motor(uint8_t id, uint8_t pwmPin, uint8_t currentPin, 
          TemperatureSensor* tempSensor, ModbusHandler& modbus);
    void begin();
    void update(uint64_t now);
    void setDuty(uint16_t duty);
    void setFrequency(uint32_t freq);
    uint8_t getStatus() const;
    
private:
    uint8_t id;
    uint8_t pwmPin;
    CurrentSensor currentSensor;  // Replace currentPin with CurrentSensor
    TemperatureSensor* tempSensor;
    ModbusHandler& modbusHandler;
    uint16_t dutyCycle;
    uint8_t status;
};