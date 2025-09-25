#include "Motor.h"
#include "PWMController.h"
#include "ModbusHandler.h"
#include "Config.h"
#include "Globals.h"

Motor::Motor(uint8_t id, uint8_t pwmPin, uint8_t currentPin,
             TemperatureSensor *tempSensor, ModbusHandler &modbus)
    : id(id), pwmPin(pwmPin),
      currentSensor(currentPin, ModbusInputReg::CURR_BASE + id), // Initialize CurrentSensor
      tempSensor(tempSensor), modbusHandler(modbus),
      dutyCycle(0), status(0)
{
}

void Motor::begin()
{
    pinMode(pwmPin, OUTPUT);
    currentSensor.begin(); // Initialize current sensor

    // PWMController::setFrequency(pwmPin, 1000);
    PWMController::setGlobalFrequency(7812);
    PWMController::setDutyCycle(pwmPin, 0);
}

void Motor::update(uint64_t now)
{
    currentSensor.update(now); // Update current reading

    int16_t temp = tempSensor->getTemperature();
    modbusHandler.setIreg(ModbusInputReg::TEMP_BASE + id, static_cast<uint16_t>(temp));

    uint8_t tempStatus = tempSensor->getStatus();
    uint16_t current = currentSensor.getCurrent(); // Get current from sensor

    modbusHandler.setIreg(ModbusInputReg::STATUS_BASE + id, status);
}

void Motor::setDuty(uint16_t duty)
{
    PWMController::setDutyCycle(pwmPin, duty);
}

void Motor::setFrequency(uint32_t freq)
{
    PWMController::setGlobalFrequency(freq);
}

uint8_t Motor::getStatus() const
{
    return status;
}

float Motor::getCurr() const
{
    return currentSensor.getCurrent();
}

float Motor::getTemp() const
{
    return tempSensor->getTemperature();
}

float Motor::getDuty() const
{
    return PWMController::getDuty(pwmPin);
}