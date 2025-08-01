#pragma once
#include "ModbusRTU.h"

// Handles all Modbus RTU interaction and register mapping
class ModbusHandler {
public:
    // Constructor: takes a HardwareSerial port and slave address
    ModbusHandler(HardwareSerial& port, uint8_t slaveID);

    // Initializes all registers and sets up ModbusRTU
    void begin();

    // Main task loop — call in Arduino loop() to process Modbus state
    void task();

    // --- Register Access ---

    uint16_t getHreg(uint16_t addr);           // Read holding register
    uint16_t getIreg(uint16_t addr);           // Read input register (❗you should implement this)
    void setHreg(uint16_t addr, uint16_t value);  // Write holding register
    void setIreg(uint16_t addr, uint16_t value);  // Update input register

    void addHreg(uint16_t addr, uint16_t value = 0); // Define a holding register
    void addIreg(uint16_t addr, uint16_t value = 0); // Define an input register

    // --- Write Callback Registration ---
    void registerMotorCallbacks();   // Bind motor control regs
    void registerDeviceCallbacks();  // Bind actuator control regs
    void registerSystemCallbacks();  // Bind system-level actions (E-STOP)

private:
    ModbusRTU mb;                   // Internal RTU instance
    static ModbusRTU* mbInstance;  // For use in static callbacks

    // --- Callback Handlers for Register Writes ---
    static uint16_t handleMotorWrite(TRegister* reg, uint16_t val);   // Handles duty/freq write
    static uint16_t handleDeviceWrite(TRegister* reg, uint16_t val);  // Handles device on/off
    static uint16_t handleSystemWrite(TRegister* reg, uint16_t val);  // Handles emergency stop
};
