#pragma once
#include <Arduino.h> // Needed for pin constants

// Error codes
enum ErrorCode
{
    ERR_NO_ERROR = 0,
    ERR_TEMP_LOW = 1,
    ERR_TEMP_HIGH = 2,
    ERR_SENSOR_DISCONNECTED = 3,
    ERR_OVERCURRENT = 4,
    ERR_MODBUS_CRC_FAIL = 5,
    ERR_MODBUS_TIMEOUT = 6
};

// Serial port configuration
constexpr uint32_t BAUDRATE = 250000;

// -------------------------
// Pin Configuration
// -------------------------

// Motor control PWM output pins (software/hardware PWM)
constexpr uint8_t PWM_PINS[15] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 44, 45, 46};
constexpr uint8_t SLAVE_ID = 1;

// Motor current sensor analog input pins (ACS712 or similar)
constexpr uint8_t CURRENT_PINS[15] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14};

// Motor temperature DS18B20 sensor digital input pins
constexpr uint8_t TEMP_PINS[15] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};

// System-wide sensors (also digital pins)
constexpr uint8_t WATER_TEMP_PIN = 20; // ⚠️ Also I2C SDA
constexpr uint8_t AIR_TEMP_PIN = 21;   // ⚠️ Also I2C SCL

// Output control pins for actuators
constexpr uint8_t FAN_PIN = 40;
constexpr uint8_t MIXER_PIN = 41;
constexpr uint8_t DISPENSER_PIN = 42;
constexpr uint8_t PUMP_PIN = 43;

// -------------------------
// Modbus Register Map
// -------------------------
namespace ModbusHoldingReg
{

    // --- Motor Parameters ---
    // Holding Registers (writeable by master)
    constexpr uint16_t DUTY_BASE = 1; // Holding: [1–15] — duty cycle control
    constexpr uint16_t GLOBAL_FREQ = 0;

    // Thresholds (Holding registers, writeable by master)
    constexpr uint16_t MOTOR_TEMP_CRIT = 61;
    constexpr uint16_t MOTOR_CURR_CRIT = 62;

    // --- System Parameters ---
    constexpr uint16_t START_REG_ADDR = 65;

    // Air Temp
    constexpr uint16_t AIR_TEMP_REG = 70;
    constexpr uint16_t AIR_TEMP_LIMIT = 72;

    // Water Temp
    constexpr uint16_t WATER_TEMP_REG = 80;
    constexpr uint16_t WATER_TEMP_LIMIT = 82;

}

namespace ModbusInputReg
{
    // Input Registers (read-only to master)
    constexpr uint16_t CURR_BASE = 16;   // Input: [16–30] — motor current values
    constexpr uint16_t TEMP_BASE = 31;   // Input: [31–45] — motor temperature values
    constexpr uint16_t STATUS_BASE = 46; // Input: [46–60] — motor status (e.g. overtemp, error)

    constexpr uint16_t DEV_STATUS_BASE = 90;
    constexpr uint16_t TIME_LOW = 66;

    // --- Device States ---
    constexpr uint16_t FAN_REG = 91;
    constexpr uint16_t MIXER_REG = 92;
    constexpr uint16_t DISPENSER_REG = 93;
    constexpr uint16_t PUMP_REG = 94;
}

// -------------------------
// Safety & Operational Limits
// -------------------------

constexpr uint8_t NUM_MOTORS = 15;

// Temperature values scaled (e.g., 5000 = 50.00°C if using hundredths of °C)
constexpr uint16_t TEMP_WARNING = 5000;
constexpr uint16_t TEMP_CRITICAL = 6000;
constexpr uint16_t CURR_CRITICAL = 9000; // mA or raw sensor units

// PWM frequency range (Hz)
constexpr uint16_t MIN_PWM_FREQ = 100;
constexpr uint16_t MAX_PWM_FREQ = 30000;