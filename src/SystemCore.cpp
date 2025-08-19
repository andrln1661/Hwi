
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

    modbus.task();
    
    static uint64_t lastMotorUpdate = 0;
    static uint64_t lastTempUpdate = 0;
    uint64_t now = millisCustom() * 2;
    
    // Update motor status every 500ms
    if (now - lastMotorUpdate >= 100) {
        lastMotorUpdate = now;
        for (uint8_t i = 0; i < NUM_MOTORS; i += 3) { // Process 2 motors per cycle
            if (i < NUM_MOTORS) motors[i]->update();
            if (i+1 < NUM_MOTORS) motors[i+1]->update();
            if (i+2 < NUM_MOTORS) motors[i+2]->update();
            modbus.task(); // Handle Modbus between batches
        }
    }

    // Update temperature and devices every 1000ms
    if (now - lastTempUpdate >= 1000) {
        lastTempUpdate = now;

        // Request temperatures first
        for (uint8_t i = 0; i < NUM_MOTORS; i++) {
            motorSensors[i]->requestTemperatures(now);  // Pointer access
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
        modbus.setIreg(ModbusReg::TIME_LOW, uint16_t(now & 0xFFFF));
        modbus.setIreg(ModbusReg::TIME_LOW + 1, uint16_t((now >> 16) & 0xFFFF));
        modbus.setIreg(ModbusReg::TIME_LOW + 2, uint16_t((now >> 32) & 0xFFFF));
        modbus.setIreg(ModbusReg::TIME_LOW + 3, uint16_t((now >> 48) & 0xFFFF));
    }


}

uint64_t SystemCore::millisCustom() {
    extern volatile unsigned long timer0_overflow_count;
    
    uint8_t tcnt0_snapshot;
    uint32_t overflow_snapshot;

    // Determine the new prescaler directly from the register
    uint8_t prescaler_bits = TCCR0B & 0x07;
    uint16_t new_prescaler;
    switch (prescaler_bits) {
        case 1: new_prescaler = 1; break;
        case 2: new_prescaler = 8; break;
        case 3: new_prescaler = 64; break;
        case 4: new_prescaler = 256; break;
        case 5: new_prescaler = 1024; break;
        default: new_prescaler = 64; break;
    }

    noInterrupts();
    tcnt0_snapshot = TCNT0;
    overflow_snapshot = timer0_overflow_count;
    // This check is crucial to catch an overflow that happens right as we read the registers
    if ((TIFR0 & _BV(TOV0)) && (tcnt0_snapshot < 255)) {
        overflow_snapshot++;
    }
    interrupts();

    // The core logic: If the prescaler has changed, we finalize the time
    // accumulated with the OLD prescaler and add it to our master accumulator.
    if (new_prescaler != current_timer0_prescaler) {
        // Calculate overflows since last check and convert to ticks using the OLD prescaler
        uint32_t overflows_since_last = overflow_snapshot - last_overflow_snapshot;
        accumulated_ticks += (uint64_t)overflows_since_last * 256 * current_timer0_prescaler;
        
        // Update the last snapshot and the current prescaler for the next run
        last_overflow_snapshot = overflow_snapshot;
        current_timer0_prescaler = new_prescaler;
    }

    // Calculate total ticks: the master accumulator plus the ticks in the current cycle
    uint64_t overflows_since_last = overflow_snapshot - last_overflow_snapshot;
    uint64_t ticks_since_last_update = (overflows_since_last * 256 + tcnt0_snapshot) * current_timer0_prescaler;
    uint64_t total_ticks = accumulated_ticks + ticks_since_last_update;


    // Convert total CPU ticks to milliseconds using integer math
    return total_ticks / (F_CPU / 1000UL);
}