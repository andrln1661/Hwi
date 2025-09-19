
#pragma once
// Prevents multiple inclusions of this header file during compilation.

#include "ModbusHandler.h"     // Handles Modbus RTU communication (master-slave protocol)
#include "TemperatureSensor.h" // Interface for DS18B20 or similar temperature sensors
#include "Motor.h"             // Motor class: includes PWM, current sensing, and temperature safety
#include "DeviceManager.h"     // Manages auxiliary actuators like fan, pump, mixer, etc.
#include "Config.h"            // Global configuration constants (e.g., NUM_MOTORS)
#include <Arduino.h>           // Arduino core functions and types (pinMode, millis, digitalWrite, etc.)


// Main orchestrator of the embedded system. Handles setup, runtime control, and data flow.
class SystemCore {
public:
    // Constructor to initialize internal members (could also be used to preload configs)
    SystemCore();
    
    // Destructor to clean up dynamically allocated objects
    ~SystemCore();

    // Sets up all subsystems (sensors, motors, Modbus, etc.)
    void setup();

    // Main runtime loop. Should be called repeatedly from Arduino's loop()
    void loop();

    // Allows ModbusHandler to access private members like motors[] directly.
    // Useful for implementing indirect register callbacks.
    friend class ModbusHandler;

private:
    // --- Subsystems ---

    ModbusHandler modbus;               // Modbus RTU slave handler (interfaces with external master like Python GUI)

    TemperatureSensor airSensor;        // Ambient air temperature sensor (DS18B20 or similar)
    TemperatureSensor waterSensor;      // Water temperature sensor (same interface as above)

    TemperatureSensor* motorSensors[NUM_MOTORS]; // Array of pointers to per-motor temperature sensors

    Motor* motors[NUM_MOTORS];          // Array of pointers to core motor control objects

    DeviceManager deviceManager;        // Controls peripheral devices (fan, mixer, pump, etc.)

    uint64_t accumulated_ticks = 0;
    uint32_t last_overflow_snapshot = 0;
    uint16_t current_timer0_prescaler = 64;

    // --- Timekeeping ---

    // Custom millisecond counter (optionally extended to 64-bit for rollover-safe timing)
    uint64_t millisCustom();

    // Stores prescaler state for custom millis if using a timer overflow counter
    uint16_t prev_prescaler;
};

// Declares a global instance accessible throughout the codebase.
// Allows Modbus callbacks or interrupt handlers to refer to the central system state.
extern SystemCore systemCore;