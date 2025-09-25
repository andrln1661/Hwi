#include "ModbusHandler.h"
#include "Motor.h"
#include "DeviceManager.h"
#include "Config.h"
#include "Globals.h"
#include "PWMController.h"

ModbusHandler::ModbusHandler(HardwareSerial &portRef, uint8_t slaveRef)
    : port(portRef), slaveID(slaveRef)
{
    modbusHandler = this;
}

void ModbusHandler::begin(unsigned long baudrate)
{
    port.begin(baudrate); // For logging and Modbus

    // Begin with explicit parity (even) for validation
    if (!ModbusRTUServer.begin(slaveID, baudrate, SERIAL_8E1))
    {
        while (1)
            ;
    }

    // Configure ranges (larger for safety)
    ModbusRTUServer.configureHoldingRegisters(0, 100);
    ModbusRTUServer.configureInputRegisters(0, 100);

    // Init registers
    ModbusRTUServer.holdingRegisterWrite(ModbusHoldingReg::START_REG_ADDR, 0);
    ModbusRTUServer.holdingRegisterWrite(ModbusHoldingReg::MOTOR_TEMP_CRIT, TEMP_CRITICAL);
    ModbusRTUServer.holdingRegisterWrite(ModbusHoldingReg::MOTOR_CURR_CRIT, CURR_CRITICAL);
    ModbusRTUServer.holdingRegisterWrite(ModbusHoldingReg::AIR_TEMP_LIMIT, TEMP_CRITICAL);
    ModbusRTUServer.holdingRegisterWrite(ModbusHoldingReg::WATER_TEMP_LIMIT, TEMP_WARNING);

    ModbusRTUServer.inputRegisterWrite(ModbusInputReg::TIME_LOW, 0);
    ModbusRTUServer.inputRegisterWrite(ModbusInputReg::TIME_LOW + 1, 0);
    ModbusRTUServer.inputRegisterWrite(ModbusInputReg::TIME_LOW + 2, 0);
    ModbusRTUServer.inputRegisterWrite(ModbusInputReg::TIME_LOW + 3, 0);

    // Init duty and freq shadows and registers
    for (uint8_t i = 0; i < NUM_MOTORS; i++)
    {
        ModbusRTUServer.holdingRegisterWrite(ModbusHoldingReg::DUTY_BASE + i, 0);
        dutyShadows[i] = 0;
        // ModbusRTUServer.holdingRegisterWrite(ModbusReg::FREQ_BASE + i, 1000);
        // freqShadows[i] = 1000;
    }
    memset(deviceShadows, 0, sizeof(deviceShadows));
    startShadow = 0;
}

void ModbusHandler::task()
{
    static uint8_t errorCount = 0;

    int pollResult = ModbusRTUServer.poll();
    if (pollResult == -1)
    {
        if (++errorCount > 10)
        {
            // Reset Modbus state after 10 errors
            ModbusRTUServer.begin(slaveID, BAUDRATE, SERIAL_8E1);
            errorCount = 0;
        }
    }
    else
    {
        errorCount = 0;
    }
    if (!port.available())
        return;

    // int pollResult = ModbusRTUServer.poll();
    // if (pollResult == -1) {
    //     // Error handling (e.g., frame/CRC error); increment counter or log
    // }

    // Check for changes in holding registers (reactive handling)
    uint16_t globalFreq = ModbusRTUServer.holdingRegisterRead(ModbusHoldingReg::GLOBAL_FREQ);
    if (globalFreqShadow != globalFreq)
    {
        handleMotorWrite(ModbusHoldingReg::GLOBAL_FREQ, globalFreq);
        globalFreqShadow = globalFreq;
    }
    for (uint8_t i = 0; i < NUM_MOTORS; i++)
    {
        uint16_t dutyVal = ModbusRTUServer.holdingRegisterRead(ModbusHoldingReg::DUTY_BASE + i);
        if (dutyVal != dutyShadows[i])
        {
            handleMotorWrite(ModbusHoldingReg::DUTY_BASE + i, dutyVal);
            dutyShadows[i] = dutyVal;
        }
        // uint16_t freqVal = ModbusRTUServer.holdingRegisterRead(ModbusReg::FREQ_BASE + i);
        // if (freqVal != freqShadows[i]) {
        // handleMotorWrite(ModbusReg::FREQ_BASE + i, freqVal);
        // freqShadows[i] = freqVal;
        // }
    }
    for (uint8_t i = 0; i < 4; i++)
    {
        uint16_t devVal = ModbusRTUServer.holdingRegisterRead(ModbusInputReg::FAN_REG + i);
        if (devVal != deviceShadows[i])
        {
            handleDeviceWrite(ModbusInputReg::FAN_REG + i, devVal);
            deviceShadows[i] = devVal;
        }
    }
    uint16_t startVal = ModbusRTUServer.holdingRegisterRead(ModbusHoldingReg::START_REG_ADDR);
    if (startVal != startShadow)
    {
        handleSystemWrite(ModbusHoldingReg::START_REG_ADDR, startVal);
        startShadow = startVal;
    }
}

uint16_t ModbusHandler::getHreg(uint16_t addr)
{
    return ModbusRTUServer.holdingRegisterRead(addr);
}

uint16_t ModbusHandler::getIreg(uint16_t addr)
{
    return ModbusRTUServer.inputRegisterRead(addr);
}

void ModbusHandler::setHreg(uint16_t addr, uint16_t value)
{
    ModbusRTUServer.holdingRegisterWrite(addr, value);
}

void ModbusHandler::setIreg(uint16_t addr, uint16_t value)
{
    ModbusRTUServer.inputRegisterWrite(addr, value);
}

void ModbusHandler::handleMotorWrite(uint16_t addr, uint16_t val)
{ // Changed parameter type to uint16_t
    if (addr >= ModbusHoldingReg::DUTY_BASE &&
        addr < ModbusHoldingReg::DUTY_BASE + NUM_MOTORS)
    {
        uint8_t id = addr - ModbusHoldingReg::DUTY_BASE;
        motors[id]->setDuty(val);
    }
    else if (addr == ModbusHoldingReg::GLOBAL_FREQ)
    {
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

void ModbusHandler::handleDeviceWrite(int addr, uint16_t val)
{
    if (addr == ModbusInputReg::FAN_REG)
        deviceManager->controlFan(val > 0);
    else if (addr == ModbusInputReg::MIXER_REG)
        deviceManager->controlMixer(val > 0);
    else if (addr == ModbusInputReg::DISPENSER_REG)
        deviceManager->controlDispenser(val > 0);
    else if (addr == ModbusInputReg::PUMP_REG)
        deviceManager->controlPump(val > 0);
}

void ModbusHandler::handleSystemWrite(int addr, uint16_t val)
{
    if (val == 1)
    {
        if (addr == ModbusHoldingReg::START_REG_ADDR)
        {
            PWMController::clearCount();
        }
    }

    if (val == 0)
    {
        Serial1.println("Stopped start");
        noInterrupts();
        for (int i = 0; i < NUM_MOTORS; i++)
        {
            // motors[i]->setDuty(0);
            modbusHandler->setHreg(ModbusHoldingReg::DUTY_BASE + i, 0);
        }
        deviceManager->controlFan(false);
        deviceManager->controlMixer(false);
        deviceManager->controlDispenser(false);
        deviceManager->controlPump(false);
        interrupts();
        Serial1.println("Stopped finished");
    }
}