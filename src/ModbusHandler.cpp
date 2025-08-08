#include "ModbusHandler.h"
#include "Motor.h"
#include "DeviceManager.h"
#include "Config.h"
#include "Globals.h"

// Static pointer to allow static callback access to Modbus instance
ModbusRTU* ModbusHandler::mbInstance = nullptr;

// Externally defined motor and device objects (e.g., in SystemCore.cpp)
// extern Motor motors[NUM_MOTORS];
// extern DeviceManager deviceManager;

ModbusHandler::ModbusHandler(HardwareSerial& port, uint8_t slaveID)
    : mb() {
    mbInstance = &mb;             // Link static instance pointer (for callbacks if needed)
    mb.begin(&port, -1);          // -1 means no DE/RE pin used (e.g. for RS-485)
    mb.slave(slaveID);            // Set this device’s Modbus slave ID
}

void ModbusHandler::begin() {
    // System startup control and time tracking
    addHreg(ModbusReg::START_REG_ADDR, 0);  // Master writes here to start/stop
    addIreg(ModbusReg::TIME_LOW, 0);        // System time LSB (MSB not implemented yet)

    // Motor fault thresholds (settable by Modbus master)
    addHreg(ModbusReg::MOTOR_TEMP_CRIT, TEMP_CRITICAL);
    addHreg(ModbusReg::MOTOR_CURR_CRIT, CURR_CRITICAL);

    // Temperature thresholds for control logic (fan/mixer)
    addHreg(ModbusReg::AIR_TEMP_LOW, TEMP_WARNING);
    addHreg(ModbusReg::AIR_TEMP_HIGH, TEMP_CRITICAL);
    addHreg(ModbusReg::WATER_TEMP_LOW, TEMP_WARNING);
    addHreg(ModbusReg::WATER_TEMP_HIGH, TEMP_CRITICAL);

    // Bind Modbus register ranges to control callbacks
    registerMotorCallbacks();
    registerDeviceCallbacks();
    registerSystemCallbacks();
}

void ModbusHandler::task() {
    mb.task();  // Must be called regularly from `loop()` to process Modbus requests
}

uint16_t ModbusHandler::getHreg(uint16_t addr) {
    return mb.Hreg(addr);  // Read a holding register (for internal logic)
}

void ModbusHandler::setHreg(uint16_t addr, uint16_t value) {
    mb.Hreg(addr, value);  // Write a holding register
}

void ModbusHandler::setIreg(uint16_t addr, uint16_t value) {
    mb.Ireg(addr, value);  // Write an input register (read-only to master)
}

void ModbusHandler::addHreg(uint16_t addr, uint16_t value) {
    mb.addHreg(addr, value);  // Add and initialize holding register
}

void ModbusHandler::addIreg(uint16_t addr, uint16_t value) {
    mb.addIreg(addr, value);  // Add and initialize input register
}

void ModbusHandler::registerMotorCallbacks() {
    // Handle duty and frequency register updates for all motors
    mb.onSetHreg(ModbusReg::DUTY_BASE, handleMotorWrite, NUM_MOTORS);
    mb.onSetHreg(ModbusReg::FREQ_BASE, handleMotorWrite, NUM_MOTORS);
}

void ModbusHandler::registerDeviceCallbacks() {
    // Individual control for fan, mixer, dispenser, pump
    mb.onSetHreg(ModbusReg::FAN_REG, handleDeviceWrite, 1);
    mb.onSetHreg(ModbusReg::MIXER_REG, handleDeviceWrite, 1);
    mb.onSetHreg(ModbusReg::DISPENSER_REG, handleDeviceWrite, 1);
    mb.onSetHreg(ModbusReg::PUMP_REG, handleDeviceWrite, 1);
}

void ModbusHandler::registerSystemCallbacks() {
    // Handle system start/stop
    mb.onSetHreg(ModbusReg::START_REG_ADDR, handleSystemWrite, 1);
}

uint16_t ModbusHandler::handleMotorWrite(TRegister* reg, uint16_t val) {
    if (!reg) return 0;  // Safety: avoid null pointer crash

    uint16_t addr = reg->address.address;

    // Handle duty cycle set
    if (addr >= ModbusReg::DUTY_BASE && addr < ModbusReg::DUTY_BASE + NUM_MOTORS) {
        uint8_t motorId = addr - ModbusReg::DUTY_BASE;
        motors[motorId].setDuty(val);  // Actual logic in Motor.cpp
        return val;
    }
    // Handle frequency set
    else if (addr >= ModbusReg::FREQ_BASE && addr < ModbusReg::FREQ_BASE + NUM_MOTORS) {
        uint8_t motorId = addr - ModbusReg::FREQ_BASE;
        motors[motorId].setFrequency(val);
        return val;
    }

    return 0;  // Invalid address — no action
}

uint16_t ModbusHandler::handleDeviceWrite(TRegister* reg, uint16_t val) {
    if (!reg) return 0;

    uint16_t addr = reg->address.address;

    // Fan control
    if (addr == ModbusReg::FAN_REG) {
        deviceManager->controlFan(val > 0);
        return val;
    }
    // Mixer control
    else if (addr == ModbusReg::MIXER_REG) {
        deviceManager->controlMixer(val > 0);
        return val;
    }
    // Dispenser control
    else if (addr == ModbusReg::DISPENSER_REG) {
        deviceManager->controlDispenser(val > 0);
        return val;
    }
    // Pump control
    else if (addr == ModbusReg::PUMP_REG) {
        deviceManager->controlPump(val > 0);
        return val;
    }

    return 0;  // Unknown device address
}

uint16_t ModbusHandler::handleSystemWrite(TRegister* reg, uint16_t val) {
    if (!reg) return 0;

    if (val == 0) {  // Emergency stop triggered by master
        for (int i = 0; i < NUM_MOTORS; i++) {
            motors[i].setDuty(0);  // Force stop all motors
            // Optionally: motors[i].stop(); if frequency reset is needed
        }

        // Turn off all system actuators
        deviceManager->controlFan(false);
        deviceManager->controlMixer(false);
        deviceManager->controlDispenser(false);
        deviceManager->controlPump(false);
    }

    return 1;  // Acknowledge successful write
}
