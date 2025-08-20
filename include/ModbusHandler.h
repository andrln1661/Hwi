#pragma once
#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <Arduino.h>
#include "Config.h"

class ModbusHandler {
private:
    HardwareSerial& port;
    uint8_t slaveID;
    uint16_t dutyShadows[15];
    // uint16_t freqShadows[15];
    uint16_t globalFreqShadow;
    uint16_t deviceShadows[4];
    uint16_t startShadow;

    static constexpr uint8_t BUFFER_SIZE = 64;
    static uint8_t modbusBuffer[BUFFER_SIZE];
    static volatile uint8_t modbusIndex;
    static volatile bool frameReady;

public:
    ModbusHandler(HardwareSerial& portRef, uint8_t slaveRef);
    void begin(unsigned long baudrate = BAUDRATE);
    void task();

    uint16_t getHreg(uint16_t addr);
    uint16_t getIreg(uint16_t addr);
    void setHreg(uint16_t addr, uint16_t value);
    void setIreg(uint16_t addr, uint16_t value);

    void handleMotorWrite(uint16_t addr, uint16_t val);
    void handleDeviceWrite(int addr, uint16_t val);
    void handleSystemWrite(int addr, uint16_t val);
};