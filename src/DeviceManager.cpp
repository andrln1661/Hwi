
#include "DeviceManager.h"
#include "ModbusHandler.h"
#include "Config.h"
#include "Globals.h"

void DeviceManager::begin() {
    pinMode(FAN_PIN, OUTPUT);
    pinMode(MIXER_PIN, OUTPUT);
    pinMode(DISPENSER_PIN, OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);

    // Optional: clear state
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(MIXER_PIN, LOW);
    digitalWrite(DISPENSER_PIN, LOW);
    digitalWrite(PUMP_PIN, LOW);
}

void DeviceManager::update(const TemperatureSensor& airSensor, 
                           const TemperatureSensor& waterSensor, 
                           const Motor* const* motors) {
    // --- Fan logic ---
    uint16_t airTemp = airSensor.getTemperature();
    uint16_t airLow = modbusHandler->getHreg(ModbusReg::AIR_TEMP_LOW);
    uint16_t airHigh = modbusHandler->getHreg(ModbusReg::AIR_TEMP_HIGH);
    bool fanState = false;

    if (airTemp != 0xFFFF) {
        fanState = (airTemp >= airHigh) || (airTemp <= airLow);
    }
    controlFan(fanState);

    // --- Mixer logic ---
    uint16_t waterTemp = waterSensor.getTemperature();
    uint16_t waterLow = modbusHandler->getHreg(ModbusReg::WATER_TEMP_LOW);
    uint16_t waterHigh = modbusHandler->getHreg(ModbusReg::WATER_TEMP_HIGH);
    bool mixerState = false;

    if (waterTemp != 0xFFFF) {
        mixerState = (waterTemp >= waterHigh) || (waterTemp <= waterLow);
    }
    controlMixer(mixerState);

    // --- Dispenser logic ---
    bool dispenserState = (waterTemp != 0xFFFF && waterTemp >= waterHigh);
    controlDispenser(dispenserState);

    // --- Pump logic ---
    bool pumpState = false;
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (motors[i]->getStatus() < 2 && 
            modbusHandler->getHreg(ModbusReg::DUTY_BASE + i) > 0) {
            pumpState = true;
            break;
        }
    }
    controlPump(pumpState);
}

// Device control methods with Modbus feedback

void DeviceManager::controlFan(bool state) {
    digitalWrite(FAN_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusReg::DEV_STATUS_BASE + 0, state ? 1 : 0);
}

void DeviceManager::controlMixer(bool state) {
    digitalWrite(MIXER_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusReg::DEV_STATUS_BASE + 1, state ? 1 : 0);
}

void DeviceManager::controlDispenser(bool state) {
    digitalWrite(DISPENSER_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusReg::DEV_STATUS_BASE + 2, state ? 1 : 0);
}

void DeviceManager::controlPump(bool state) {
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusReg::DEV_STATUS_BASE + 3, state ? 1 : 0);
}