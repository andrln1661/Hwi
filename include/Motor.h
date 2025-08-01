#pragma once
#include "TemperatureSensor.h"
#include <Arduino.h>  // Needed for pinMode, analogRead, etc.

class Motor {
public:
    // Constructor takes hardware + sensor bindings
    Motor(uint8_t id, uint8_t pwmPin, uint8_t currentPin, TemperatureSensor* tempSensor);

    // Initializes pins, PWM, and Modbus registers
    void begin();

    // Main update loop (to be called regularly)
    void update();

    // Update PWM duty cycle (0–1000 means 0–100%)
    void setDuty(uint16_t duty);

    // Update PWM frequency (e.g., 100 Hz – 30,000 Hz)
    void setFrequency(uint32_t freq);

    // Return internal status: 0–3 (OK, warning, fault, sensor error)
    uint8_t getStatus() const;

private:
    uint8_t id;             // Motor index (0–14)
    uint8_t pwmPin;         // PWM output pin
    uint8_t currentPin;     // Current sensor pin (analog)
    TemperatureSensor* tempSensor;  // Linked DS18B20 or similar

    uint16_t dutyCycle;     // Current duty cycle
    uint8_t status;         // Current motor status

    // Read current with exponential smoothing filter
    uint16_t readFilteredCurrent();
};
