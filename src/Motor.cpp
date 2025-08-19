#include "Motor.h"
#include "PWMController.h"
#include "ModbusHandler.h"
#include "Config.h"
#include "Globals.h"

Motor::Motor(uint8_t id, uint8_t pwmPin, uint8_t currentPin, 
             TemperatureSensor* tempSensor, ModbusHandler& modbus)
    : id(id), pwmPin(pwmPin), 
      currentSensor(currentPin, ModbusReg::CURR_BASE + id),  // Initialize CurrentSensor
      tempSensor(tempSensor), modbusHandler(modbus),
      dutyCycle(0), status(0) {}

void Motor::begin() {
    pinMode(pwmPin, OUTPUT);
    currentSensor.begin();  // Initialize current sensor
    
    PWMController::setFrequency(pwmPin, 1000);
    PWMController::setDutyCycle(pwmPin, 0);
}

void Motor::update() {
    currentSensor.update();  // Update current reading
    
    int16_t temp = tempSensor->getTemperature();
    modbusHandler.setIreg(ModbusReg::TEMP_BASE + id, static_cast<uint16_t>(temp));

    uint8_t tempStatus = tempSensor->getStatus();
    uint16_t tempCritThreshold = modbusHandler.getHreg(ModbusReg::MOTOR_TEMP_CRIT);
    uint16_t currCritThreshold = modbusHandler.getHreg(ModbusReg::MOTOR_CURR_CRIT);
    uint16_t current = currentSensor.getCurrent();  // Get current from sensor

    if (tempStatus == 3) {
        status = 3;
        setDuty(0);
    } else if (temp >= static_cast<int16_t>(tempCritThreshold) || current >= currCritThreshold) {
        status = 2;
        setDuty(0);
    } else if (tempStatus > 0) {
        status = 1;
    } else {
        status = 0;
    }

    modbusHandler.setIreg(ModbusReg::STATUS_BASE + id, status);
}


void Motor::setDuty(uint16_t duty) {
    dutyCycle = (duty > 1000) ? 1000 : duty;  // Clamp to 1000
    PWMController::setDutyCycle(pwmPin, dutyCycle);
}

void Motor::setFrequency(uint32_t freq) {
    PWMController::setFrequency(pwmPin, freq);
}

uint8_t Motor::getStatus() const {
    return status;
}