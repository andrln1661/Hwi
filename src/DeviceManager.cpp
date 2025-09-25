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
    uint16_t airLimit = modbusHandler->getHreg(ModbusHoldingReg::AIR_TEMP_LIMIT);
    bool fanState = false;

    if (airTemp != 0xFFFF) {
        fanState = (airTemp >= airLimit);
    }
    controlFan(fanState);

    // --- Mixer logic ---
    uint16_t waterTemp = waterSensor.getTemperature();
    uint16_t waterLimit = modbusHandler->getHreg(ModbusHoldingReg::WATER_TEMP_LIMIT);
    bool mixerState = false;

    if (waterTemp != 0xFFFF) {
        mixerState = (waterTemp >= waterLimit);
    }
    controlMixer(mixerState);

    // --- Dispenser logic ---
    bool dispenserState = (waterTemp != 0xFFFF && waterTemp >= waterLimit);
    controlDispenser(dispenserState);

    // --- Pump logic ---
    bool pumpState = false;
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (motors[i]->getStatus() < 2 && 
            modbusHandler->getHreg(ModbusHoldingReg::DUTY_BASE + i) > 0) {
            pumpState = true;
            break;
        }
    }
    controlPump(pumpState);
}

// Device control methods with Modbus feedback

void DeviceManager::controlFan(bool state) {
    digitalWrite(FAN_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusInputReg::FAN_REG, state ? 1 : 0);
}

void DeviceManager::controlMixer(bool state) {
    digitalWrite(MIXER_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusInputReg::MIXER_REG, state ? 1 : 0);
}

void DeviceManager::controlDispenser(bool state) {
    digitalWrite(DISPENSER_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusInputReg::DISPENSER_REG, state ? 1 : 0);
}

void DeviceManager::controlPump(bool state) {
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
    modbusHandler->setIreg(ModbusInputReg::PUMP_REG, state ? 1 : 0);
}