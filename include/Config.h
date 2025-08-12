#pragma once
#include <Arduino.h>  // Needed for pin constants
    
// Serial port configuration
constexpr uint32_t BAUDRATE = 9600;

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
constexpr uint8_t WATER_TEMP_PIN = 20;  // ⚠️ Also I2C SDA
constexpr uint8_t AIR_TEMP_PIN = 21;    // ⚠️ Also I2C SCL

// Output control pins for actuators
constexpr uint8_t FAN_PIN = 40;
constexpr uint8_t MIXER_PIN = 41;
constexpr uint8_t DISPENSER_PIN = 42;
constexpr uint8_t PUMP_PIN = 43;

// -------------------------
// Modbus Register Map
// -------------------------
namespace ModbusReg {

    // --- Motor Parameters ---
    // Holding Registers (writeable by master)
    constexpr uint16_t DUTY_BASE = 0;      // Holding: [0–14] — duty cycle control
    constexpr uint16_t FREQ_BASE = 100;    // Holding: [100–114] — frequency control

    // Input Registers (read-only to master)
    constexpr uint16_t CURR_BASE = 200;    // Input: [200–214] — motor current values
    constexpr uint16_t TEMP_BASE = 300;    // Input: [300–314] — motor temperature values
    constexpr uint16_t STATUS_BASE = 400;  // Input: [400–414] — motor status (e.g. overtemp, error)

    // Thresholds (Holding registers, writeable by master)
    constexpr uint16_t MOTOR_TEMP_CRIT = 500;
    constexpr uint16_t MOTOR_CURR_CRIT = 501;

    // --- System Parameters ---
    constexpr uint16_t START_REG_ADDR = 900;
    constexpr uint16_t TIME_LOW = 901;

    // Air Temp
    constexpr uint16_t AIR_TEMP_REG = 920;
    constexpr uint16_t AIR_TEMP_LOW = 921;
    constexpr uint16_t AIR_TEMP_HIGH = 922;

    // Water Temp
    constexpr uint16_t WATER_TEMP_REG = 930;
    constexpr uint16_t WATER_TEMP_LOW = 931;
    constexpr uint16_t WATER_TEMP_HIGH = 932;

    // --- Device States ---
    constexpr uint16_t DEV_STATUS_BASE = 911;
    constexpr uint16_t FAN_REG = 911;
    constexpr uint16_t MIXER_REG = 912;
    constexpr uint16_t DISPENSER_REG = 913;
    constexpr uint16_t PUMP_REG = 914;
}

// -------------------------
// Safety & Operational Limits
// -------------------------

constexpr uint8_t NUM_MOTORS = 15;

// Temperature values scaled (e.g., 5000 = 50.00°C if using hundredths of °C)
constexpr uint16_t TEMP_WARNING  = 5000;
constexpr uint16_t TEMP_CRITICAL = 6000;
constexpr uint16_t CURR_CRITICAL = 900;     // mA or raw sensor units

// PWM frequency range (Hz)
constexpr uint16_t MIN_PWM_FREQ = 100;
constexpr uint16_t MAX_PWM_FREQ = 30000;
