
#include "Globals.h"
#include "SystemCore.h"
#include "Config.h"
#include "PWMController.h"   // Add this line

SystemCore::SystemCore()
    : modbus(Serial, SLAVE_ID),
      airSensor(AIR_TEMP_PIN, 
                ModbusReg::AIR_TEMP_REG,
                ModbusReg::AIR_TEMP_LOW,
                ModbusReg::AIR_TEMP_HIGH),
      waterSensor(WATER_TEMP_PIN, 
                  ModbusReg::WATER_TEMP_REG,
                  ModbusReg::WATER_TEMP_LOW,
                  ModbusReg::WATER_TEMP_HIGH),
      prev_prescaler(0) {
    ::modbusHandler = &modbus;
    ::deviceManager = &deviceManager;
    ::motors = this->motors;  // Update global pointer to motor array

    // Initialize motor sensors with dynamic allocation
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        motorSensors[i] = new TemperatureSensor(
            TEMP_PINS[i], 
            ModbusReg::TEMP_BASE + i,
            ModbusReg::MOTOR_TEMP_CRIT,
            0  // High threshold not used for motors
        );
    }
    
    // Initialize motors with dynamic allocation
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        motors[i] = new Motor(
            i, 
            PWM_PINS[i], 
            CURRENT_PINS[i], 
            motorSensors[i],  // Pass pointer to sensor
            this->modbus
        );
    }
}

SystemCore::~SystemCore() {
    // Clean up dynamically allocated objects
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        delete motorSensors[i];
        delete motors[i];
    }
}

void SystemCore::setup() {
    modbus.begin();
    
    // Initialize sensors
    airSensor.begin();
    waterSensor.begin();
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        motorSensors[i]->begin();  // Pointer access
    }
    
    // Initialize motors
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        motors[i]->begin();  // Pointer access
    }
    
    PWMController::initialize();
    // Initialize devices
    deviceManager.begin();
}

void SystemCore::loop() {
    static uint8_t currentSensor = 0;

    
    static uint64_t lastMotorUpdate = 0;
    static uint64_t lastTempUpdate = 0;
    uint64_t now = PWMController::millisCustom();

        modbus.setIreg(ModbusReg::TIME_LOW, uint16_t(now & 0xFFFF));
        modbus.setIreg(ModbusReg::TIME_LOW + 1, uint16_t((now >> 16) & 0xFFFF));
        modbus.setIreg(ModbusReg::TIME_LOW + 2, uint16_t((now >> 32) & 0xFFFF));
        modbus.setIreg(ModbusReg::TIME_LOW + 3, uint16_t((now >> 48) & 0xFFFF));

    modbus.task();
    
    // Update motor status every 500ms
    if (now - lastMotorUpdate >= 500) {
        lastMotorUpdate = now;
        for (uint8_t i = 0; i < NUM_MOTORS; i += 3) { // Process 2 motors per cycle
            if (i < NUM_MOTORS) motors[i]->update(now);
            if (i+1 < NUM_MOTORS) motors[i+1]->update(now);
            if (i+2 < NUM_MOTORS) motors[i+2]->update(now);
            modbus.task(); // Handle Modbus between batches
        }
    }

    // Update temperature and devices every 1000ms
    if (now - lastTempUpdate >= 1000) {
        lastTempUpdate = now;

        // Request temperatures first
        for (uint8_t i = 0; i < NUM_MOTORS; i++) {
            motorSensors[i]->requestTemperaturesAsync(now);  // Pointer access
        }

        // Then update all temperature readings
        for (uint8_t i = 0; i < NUM_MOTORS; i++) {
            motorSensors[i]->update();  // Pointer access
        }

        airSensor.update();
        waterSensor.update();
        
        // Update devices (pass pointer to motor array)
        deviceManager.update(airSensor, waterSensor, motors);
        
        // Update timestamp register
    }
}

