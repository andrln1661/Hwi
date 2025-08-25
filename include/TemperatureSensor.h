
#pragma once  // Ensure the header is only included once during compilation

#include <OneWire.h>             // For communicating with 1-Wire devices like DS18B20
#include <DallasTemperature.h>   // High-level wrapper for 1-Wire temperature sensors
#include <Arduino.h>             // Core Arduino definitions and types

// Class to encapsulate a single DS18B20 temperature sensor
class TemperatureSensor {
private:
    OneWire oneWire;             // OneWire bus instance, bound to a specific pin
    DallasTemperature sensor;    // DallasTemperature library interface for reading sensor(s)
    
    uint16_t regTemp;            // Modbus register to store current temperature
    uint16_t regLowThreshold;    // Modbus register for minimum safe temperature
    uint16_t regHighThreshold;   // Modbus register for maximum safe temperature
    
    int16_t temperature;         // Last read temperature in 0.1°C precision (e.g., 254 = 25.4°C)
                                 // Changed from uint16_t to handle negative Celsius values
    uint8_t status;              // Encoded sensor status (e.g., 0=OK, 1=too low, 2=too high)
    uint64_t lastTemperatureRequest = 0; // Instance-specific timestamp

    uint32_t lastRequestTime;
    bool conversionPending;
public:
    // Constructor: specify GPIO pin and Modbus register mappings
    TemperatureSensor(uint8_t pin, uint16_t tempReg, uint16_t lowThresholdReg, uint16_t highThresholdReg);

    // Initializes OneWire and DallasTemperature libraries
    void begin();

    // Reads temperature from sensor and updates Modbus registers and status
    void update();

    // Returns latest temperature value in 0.1°C precision (e.g., 325 = 32.5°C)
    int16_t getTemperature() const;

    // Returns current status code
    uint8_t getStatus() const;

    // Read data from sensors (Take's a bit time so we need it to call before reading temps)
    void requestTemperatures(uint64_t now);

    void requestTemperaturesAsync(uint64_t now);
    bool isConversionComplete();
    void readTemperatureAsync();

};