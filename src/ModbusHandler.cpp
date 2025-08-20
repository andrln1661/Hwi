
#include "ModbusHandler.h"
#include "Motor.h"
#include "DeviceManager.h"
#include "Config.h"
#include "Globals.h"

ModbusHandler::ModbusHandler(HardwareSerial& portRef, uint8_t slaveRef)
    : port(portRef), slaveID(slaveRef) {}

void ModbusHandler::begin(unsigned long baudrate) {
    port.begin(baudrate);  // For logging and Modbus

    // Begin with explicit parity (even) for validation
    if (!ModbusRTUServer.begin(slaveID, baudrate, SERIAL_8E1)) {
        while (1);
    }

    // Configure ranges (larger for safety)
    ModbusRTUServer.configureHoldingRegisters(0, 1000);
    ModbusRTUServer.configureInputRegisters(0, 1000);

    // Init registers
    ModbusRTUServer.holdingRegisterWrite(ModbusReg::START_REG_ADDR, 0);
    ModbusRTUServer.inputRegisterWrite(ModbusReg::TIME_LOW, 0);
    ModbusRTUServer.inputRegisterWrite(ModbusReg::TIME_LOW + 1, 0);
    ModbusRTUServer.inputRegisterWrite(ModbusReg::TIME_LOW + 2, 0);
    ModbusRTUServer.inputRegisterWrite(ModbusReg::TIME_LOW + 3, 0);
    ModbusRTUServer.holdingRegisterWrite(ModbusReg::MOTOR_TEMP_CRIT, TEMP_CRITICAL);
    ModbusRTUServer.holdingRegisterWrite(ModbusReg::MOTOR_CURR_CRIT, CURR_CRITICAL);
    ModbusRTUServer.holdingRegisterWrite(ModbusReg::AIR_TEMP_LOW, TEMP_WARNING);
    ModbusRTUServer.holdingRegisterWrite(ModbusReg::AIR_TEMP_HIGH, TEMP_CRITICAL);
    ModbusRTUServer.holdingRegisterWrite(ModbusReg::WATER_TEMP_LOW, TEMP_WARNING);
    ModbusRTUServer.holdingRegisterWrite(ModbusReg::WATER_TEMP_HIGH, TEMP_CRITICAL);

    // Init duty and freq shadows and registers
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        ModbusRTUServer.holdingRegisterWrite(ModbusReg::DUTY_BASE + i, 0);
        dutyShadows[i] = 0;
        // ModbusRTUServer.holdingRegisterWrite(ModbusReg::FREQ_BASE + i, 1000);
        // freqShadows[i] = 1000;
    }
    memset(deviceShadows, 0, sizeof(deviceShadows));
    startShadow = 0;

}

void ModbusHandler::task() {
    static uint8_t errorCount = 0;
    
    int pollResult = ModbusRTUServer.poll();
    if (pollResult == -1) {
        if (++errorCount > 10) {
            // Reset Modbus state after 10 errors
            ModbusRTUServer.begin(slaveID, BAUDRATE, SERIAL_8E1);
            errorCount = 0;
        }
    } else {
        errorCount = 0;
    }
    if (!port.available()) return;

    // int pollResult = ModbusRTUServer.poll();
    // if (pollResult == -1) {
    //     // Error handling (e.g., frame/CRC error); increment counter or log
    // }

    // Check for changes in holding registers (reactive handling)
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        uint16_t dutyVal = ModbusRTUServer.holdingRegisterRead(ModbusReg::DUTY_BASE + i);
        if (dutyVal != dutyShadows[i]) {
            handleMotorWrite(ModbusReg::DUTY_BASE + i, dutyVal);
            dutyShadows[i] = dutyVal;
        }
        // uint16_t freqVal = ModbusRTUServer.holdingRegisterRead(ModbusReg::FREQ_BASE + i);
        // if (freqVal != freqShadows[i]) {
            // handleMotorWrite(ModbusReg::FREQ_BASE + i, freqVal);
            // freqShadows[i] = freqVal;
        // }
    }
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t devVal = ModbusRTUServer.holdingRegisterRead(ModbusReg::DEV_STATUS_BASE + i);
        if (devVal != deviceShadows[i]) {
            handleDeviceWrite(ModbusReg::DEV_STATUS_BASE + i, devVal);
            deviceShadows[i] = devVal;
        }
    }
    uint16_t startVal = ModbusRTUServer.holdingRegisterRead(ModbusReg::START_REG_ADDR);
    if (startVal != startShadow) {
        handleSystemWrite(ModbusReg::START_REG_ADDR, startVal);
        startShadow = startVal;
    }
}

uint16_t ModbusHandler::getHreg(uint16_t addr) {
    return ModbusRTUServer.holdingRegisterRead(addr);
}

uint16_t ModbusHandler::getIreg(uint16_t addr) {
    return ModbusRTUServer.inputRegisterRead(addr);
}

void ModbusHandler::setHreg(uint16_t addr, uint16_t value) {
    ModbusRTUServer.holdingRegisterWrite(addr, value);
}

void ModbusHandler::setIreg(uint16_t addr, uint16_t value) {
    ModbusRTUServer.inputRegisterWrite(addr, value);
}

void ModbusHandler::handleMotorWrite(uint16_t addr, uint16_t val) {  // Changed parameter type to uint16_t
    if (addr >= ModbusReg::DUTY_BASE &&
        addr < ModbusReg::DUTY_BASE + NUM_MOTORS) {
        uint8_t id = addr - ModbusReg::DUTY_BASE;
        motors[id]->setDuty(val);
    } 
    else if (addr == ModbusReg::GLOBAL_FREQ) {
        globalFreqShadow = val;
        // PWMController::setGlobalFrequency(val);
        motors[1]->setFrequency(val);
    }
    // else if (addr >= ModbusReg::FREQ_BASE &&
            //    addr < ModbusReg::FREQ_BASE + NUM_MOTORS) {
        // uint8_t id = addr - ModbusReg::FREQ_BASE;
        // motors[id]->setFrequency(val);
    // }
}

void ModbusHandler::handleDeviceWrite(int addr, uint16_t val) {
    if (addr == ModbusReg::FAN_REG) deviceManager->controlFan(val > 0);
    else if (addr == ModbusReg::MIXER_REG) deviceManager->controlMixer(val > 0);
    else if (addr == ModbusReg::DISPENSER_REG) deviceManager->controlDispenser(val > 0);
    else if (addr == ModbusReg::PUMP_REG) deviceManager->controlPump(val > 0);
}

void ModbusHandler::handleSystemWrite(int addr, uint16_t val) {
    if (val == 0) {
        for (int i = 0; i < NUM_MOTORS; i++) {
            motors[i]->setDuty(0);
        }
        deviceManager->controlFan(false);
        deviceManager->controlMixer(false);
        deviceManager->controlDispenser(false);
        deviceManager->controlPump(false);
    }
}