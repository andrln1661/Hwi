#include "Motor.h"
#include "PWMController.h"
#include "ModbusHandler.h"
#include "Config.h"

extern ModbusHandler modbusHandler;  // Global Modbus interface

Motor::Motor(uint8_t id, uint8_t pwmPin, uint8_t currentPin, TemperatureSensor* tempSensor)
    : id(id), pwmPin(pwmPin), currentPin(currentPin), tempSensor(tempSensor),
      dutyCycle(0), status(0) {}

void Motor::begin() {
    pinMode(pwmPin, OUTPUT);
    pinMode(currentPin, INPUT);

    // Default PWM setup (1 kHz, 0% duty)
    PWMController::setFrequency(pwmPin, 1000);
    PWMController::setDutyCycle(pwmPin, 0);

    // Register Modbus mapping (holding = control, input = monitor)
    modbusHandler.addHreg(ModbusReg::DUTY_BASE + id, 0);      // 0–1000
    modbusHandler.addHreg(ModbusReg::FREQ_BASE + id, 1000);   // 100–30,000

    modbusHandler.addIreg(ModbusReg::CURR_BASE + id, 0);      // in ADC units
    modbusHandler.addIreg(ModbusReg::TEMP_BASE + id, 0);      // in 1/100 °C
    modbusHandler.addIreg(ModbusReg::STATUS_BASE + id, 0);    // 0–3
}

void Motor::update() {
    uint16_t current = readFilteredCurrent();  // Smoothed current
    modbusHandler.setIreg(ModbusReg::CURR_BASE + id, current);

    int16_t temp = tempSensor->getTemperature();  // in 1/100 °C
    modbusHandler.setIreg(ModbusReg::TEMP_BASE + id, static_cast<uint16_t>(temp));

    uint8_t tempStatus = tempSensor->getStatus();  // 0: OK, 1: warn, 2: crit, 3: sensor err
    uint16_t tempCritThreshold = modbusHandler.getHreg(ModbusReg::MOTOR_TEMP_CRIT);
    uint16_t currCritThreshold = modbusHandler.getHreg(ModbusReg::MOTOR_CURR_CRIT);

    if (tempStatus == 3) {
        status = 3;         // Sensor error
        setDuty(0);
    } else if (temp >= static_cast<int16_t>(tempCritThreshold) || current >= currCritThreshold) {
        status = 2;         // Critical overtemp or overcurrent
        setDuty(0);
    } else if (tempStatus > 0) {
        status = 1;         // Warning
        // Still running
    } else {
        status = 0;         // Normal

        uint16_t storedDuty = modbusHandler.getHreg(ModbusReg::DUTY_BASE + id);
        if (dutyCycle != storedDuty) {
            setDuty(storedDuty);  // Apply update
        }
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

uint16_t Motor::readFilteredCurrent() {
    static float filtered[NUM_MOTORS] = {0};  // One filter per motor
    uint16_t raw = analogRead(currentPin);    // 0–1023

    // Exponential moving average filter (low-pass)
    filtered[id] = 0.15f * raw + 0.85f * filtered[id];

    return (filtered[id] - 512) * 1000 / 66;  // For ACS712-5A, if 5V ref
}

