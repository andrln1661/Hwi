#include "TemperatureSensor.h"
#include "ModbusHandler.h"
#include "Config.h"
#include "Globals.h"

// Constructor: initializes sensor objects and register mappings
TemperatureSensor::TemperatureSensor(uint8_t pin, uint16_t tempReg,
                                     uint16_t tempLimit)
    : oneWire(pin),     // Initialize OneWire bus on specified pin
      sensor(&oneWire), // Bind DallasTemperature instance to OneWire bus
      regTemp(tempReg), // Register for current temperature value
      limitTemp(tempLimit),
      temperature(0), // Initialize temperature to 0
      status(0)
{
} // Initial status: normal

// Initializes the temperature sensor hardware
void TemperatureSensor::begin()
{
    sensor.begin();           // Initialize DallasTemperature library
    sensor.setResolution(10); // Set sensor resolution (11 bits = 0.125°C precision)
    sensor.setWaitForConversion(false);
}

void TemperatureSensor::requestTemperatures(uint64_t now)
{
    if (now - lastTemperatureRequest > 1000)
    {
        sensor.requestTemperatures(); // Trigger temperature reading from DS18B20
        lastTemperatureRequest = now;
    }
}

void TemperatureSensor::requestTemperaturesAsync(uint64_t now)
{
    if (!conversionPending && (now - lastRequestTime > 1000))
    {
        sensor.requestTemperatures();
        conversionPending = true;
        lastRequestTime = now;
    }
}

bool TemperatureSensor::isConversionComplete()
{
    if (conversionPending)
    {
        conversionPending = !sensor.isConversionComplete();
        return !conversionPending;
    }
    return false;
}

// Reads temperature, checks thresholds, and updates Modbus registers
void TemperatureSensor::update()
{
    float tempC = sensor.getTempCByIndex(0); // Read the first sensor on the bus
    if (conversionPending && sensor.isConversionComplete())
    {
        temperature = tempC;
        // Serial1.println(tempC);

        // Write temperature to input register (read-only from master perspective)
        modbusHandler->setIreg(regTemp, tempC);
        // Note: If temperature is negative, casting to uint16_t will wrap around,
        // which may need special handling on Modbus master side.
        conversionPending = false;
    }
    else if (tempC == DEVICE_DISCONNECTED_C)
    {
        modbusHandler->setIreg(regTemp, 999.0);
        temperature = 999.0;
    }
}

// Getter: returns raw centi-degree temperature value (can be negative)
// int16_t TemperatureSensor::getTemperature() const {
//     if (temperature < 0) {
//         return 0;
//     }
//     return temperature;
// }
int16_t TemperatureSensor::getTemperature() const
{
    return temperature;
}

// Getter: returns current status flag (0=OK, 1=Low, 2=High, 3=Disconnected)
uint8_t TemperatureSensor::getStatus() const
{
    return status;
}