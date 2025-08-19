
#include "TemperatureSensor.h"
#include "ModbusHandler.h"
#include "Config.h"
#include "Globals.h"


// Constructor: initializes sensor objects and register mappings
TemperatureSensor::TemperatureSensor(uint8_t pin, uint16_t tempReg, 
                     uint16_t lowThresholdReg, uint16_t highThresholdReg) 
    : oneWire(pin),                // Initialize OneWire bus on specified pin
      sensor(&oneWire),           // Bind DallasTemperature instance to OneWire bus
      regTemp(tempReg),           // Register for current temperature value
      regLowThreshold(lowThresholdReg),   // Register for low temperature threshold
      regHighThreshold(highThresholdReg), // Register for high temperature threshold
      temperature(0),             // Initialize temperature to 0
      status(0) {}                // Initial status: normal

// Initializes the temperature sensor hardware
void TemperatureSensor::begin() {
    sensor.begin();               // Initialize DallasTemperature library
    sensor.setResolution(10);     // Set sensor resolution (11 bits = 0.125°C precision)
    sensor.setWaitForConversion(false);
}

void TemperatureSensor::requestTemperatures(uint64_t now) {
    if (now - lastTemperatureRequest > 1000) {
        sensor.requestTemperatures(); // Trigger temperature reading from DS18B20
        lastTemperatureRequest = now;
    }
}

// Reads temperature, checks thresholds, and updates Modbus registers
void TemperatureSensor::update() {
    if (sensor.isConversionComplete()) {
        float tempC = sensor.getTempCByIndex(0); // Read the first sensor on the bus
    
        if (tempC == DEVICE_DISCONNECTED_C) {
            // Handle sensor disconnect case
            temperature = -32768;     // INT16_MIN sentinel value to indicate error
            status = 3;               // Status code for disconnected
        } else {
            // Convert temperature to centi-degrees (e.g., 25.34°C → 2534)
            temperature = static_cast<int16_t>(tempC * 100); 
            
            // Read threshold values from holding registers
            uint16_t lowThreshold = modbusHandler->getHreg(regLowThreshold);
            uint16_t highThreshold = modbusHandler->getHreg(regHighThreshold);

            // Compare and classify temperature status
            if (temperature >= static_cast<int16_t>(highThreshold))
                status = 2; // Temperature above upper limit
            else if (temperature <= static_cast<int16_t>(lowThreshold))
                status = 1; // Temperature below lower limit
            else
                status = 0; // Temperature within acceptable range
        }

        // Write temperature to input register (read-only from master perspective)
        modbusHandler->setIreg(regTemp, static_cast<uint16_t>(temperature));
        // Note: If temperature is negative, casting to uint16_t will wrap around,
        // which may need special handling on Modbus master side.
    }
}

// Getter: returns raw centi-degree temperature value (can be negative)
// int16_t TemperatureSensor::getTemperature() const {
//     if (temperature < 0) {
//         return 0;
//     }
//     return temperature;
// }
int16_t TemperatureSensor::getTemperature() const {
    return temperature;
}

// Getter: returns current status flag (0=OK, 1=Low, 2=High, 3=Disconnected)
uint8_t TemperatureSensor::getStatus() const {
    return status;
}