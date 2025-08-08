#include "Globals.h"
#include "SystemCore.h"
#include "Config.h"
#include "PWMController.h"   // Add this line

SystemCore::SystemCore()
    : modbus(Serial, 1),
      airSensor(AIR_TEMP_PIN, 
                ModbusReg::AIR_TEMP_REG,
                ModbusReg::AIR_TEMP_LOW,
                ModbusReg::AIR_TEMP_HIGH),
      waterSensor(WATER_TEMP_PIN, 
                  ModbusReg::WATER_TEMP_REG,
                  ModbusReg::WATER_TEMP_LOW,
                  ModbusReg::WATER_TEMP_HIGH),
      motorSensors{
          TemperatureSensor(TEMP_PINS[0], ModbusReg::TEMP_BASE + 0, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[1], ModbusReg::TEMP_BASE + 1, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[2], ModbusReg::TEMP_BASE + 2, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[3], ModbusReg::TEMP_BASE + 3, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[4], ModbusReg::TEMP_BASE + 4, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[5], ModbusReg::TEMP_BASE + 5, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[6], ModbusReg::TEMP_BASE + 6, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[7], ModbusReg::TEMP_BASE + 7, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[8], ModbusReg::TEMP_BASE + 8, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[9], ModbusReg::TEMP_BASE + 9, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[10], ModbusReg::TEMP_BASE + 10, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[11], ModbusReg::TEMP_BASE + 11, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[12], ModbusReg::TEMP_BASE + 12, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[13], ModbusReg::TEMP_BASE + 13, ModbusReg::MOTOR_TEMP_CRIT, 0),
          TemperatureSensor(TEMP_PINS[14], ModbusReg::TEMP_BASE + 14, ModbusReg::MOTOR_TEMP_CRIT, 0)
      },
      motors{
          Motor(0, PWM_PINS[0], CURRENT_PINS[0], &motorSensors[0]),
          Motor(1, PWM_PINS[1], CURRENT_PINS[1], &motorSensors[1]),
          Motor(2, PWM_PINS[2], CURRENT_PINS[2], &motorSensors[2]),
          Motor(3, PWM_PINS[3], CURRENT_PINS[3], &motorSensors[3]),
          Motor(4, PWM_PINS[4], CURRENT_PINS[4], &motorSensors[4]),
          Motor(5, PWM_PINS[5], CURRENT_PINS[5], &motorSensors[5]),
          Motor(6, PWM_PINS[6], CURRENT_PINS[6], &motorSensors[6]),
          Motor(7, PWM_PINS[7], CURRENT_PINS[7], &motorSensors[7]),
          Motor(8, PWM_PINS[8], CURRENT_PINS[8], &motorSensors[8]),
          Motor(9, PWM_PINS[9], CURRENT_PINS[9], &motorSensors[9]),
          Motor(10, PWM_PINS[10], CURRENT_PINS[10], &motorSensors[10]),
          Motor(11, PWM_PINS[11], CURRENT_PINS[11], &motorSensors[11]),
          Motor(12, PWM_PINS[12], CURRENT_PINS[12], &motorSensors[12]),
          Motor(13, PWM_PINS[13], CURRENT_PINS[13], &motorSensors[13]),
          Motor(14, PWM_PINS[14], CURRENT_PINS[14], &motorSensors[14])
      },
      prev_prescaler(0) {
        ::modbusHandler = &modbus;
        ::deviceManager = &deviceManager;
        ::motors = this -> motors;
    
    // Initialize motor sensors
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        motorSensors[i] = TemperatureSensor(
            TEMP_PINS[i], 
            ModbusReg::TEMP_BASE + i,
            ModbusReg::MOTOR_TEMP_CRIT,
            0  // High threshold not used for motors
        );
    }
    
    // Initialize motors
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        motors[i] = Motor(
            i, 
            PWM_PINS[i], 
            CURRENT_PINS[i], 
            &motorSensors[i]
        );
    }
}

void SystemCore::setup() {
    PWMController::initialize();
    modbus.begin();
    
    // Initialize sensors
    airSensor.begin();
    waterSensor.begin();
    for (auto& sensor : motorSensors) {
        sensor.begin();
    }
    
    // Initialize motors
    for (auto& motor : motors) {
        motor.begin();
    }
    
    // Initialize devices
    deviceManager.begin();
}

void SystemCore::loop() {
    modbus.task();
    
    static uint64_t lastMotorUpdate = 0;
    static uint64_t lastTempUpdate = 0;
    uint64_t now = millisCustom();
    
    // Update motor status every 100ms
    if (now - lastMotorUpdate >= 100) {
        lastMotorUpdate = now;
        for (auto& motor : motors) {
            motor.update();
        }
    }
    
    // Update temperature and devices every 1000ms
    if (now - lastTempUpdate >= 1000) {
        lastTempUpdate = now;
        
        // Update all temperature sensors
        for (auto& sensor : motorSensors) {
            sensor.update();
        }
        airSensor.update();
        waterSensor.update();
        
        // Update devices
        deviceManager.update(airSensor, waterSensor, motors);
        
        // Update timestamp register
        modbus.setIreg(ModbusReg::TIME_LOW, uint16_t(now & 0xFFFF));
    }
}

uint64_t SystemCore::millisCustom() {
    // Implementation of custom millis function
    extern volatile unsigned long timer0_overflow_count;
    uint8_t tcnt0_snapshot;
    uint32_t overflow_snapshot;

    // Get current prescaler from Timer0 control register
    uint8_t prescalerBits = TCCR0B & 0x07;
    uint16_t prescaler;
    switch (prescalerBits) {
        case 1: prescaler = 1; break;
        case 2: prescaler = 8; break;
        case 3: prescaler = 64; break;
        case 4: prescaler = 256; break;
        case 5: prescaler = 1024; break;
        default: prescaler = 64; break;
    }

    // Adjust timer0_overflow_count if prescaler changes
    if (prescaler != prev_prescaler && prev_prescaler) {
        double value = (double)timer0_overflow_count * prev_prescaler / prescaler;
        timer0_overflow_count = (uint32_t)(value + 0.5); // Round to nearest integer
    }

    // Capture timer state safely
    noInterrupts();
    tcnt0_snapshot = TCNT0;
    overflow_snapshot = timer0_overflow_count;
    if ((TIFR0 & _BV(TOV0)) && (tcnt0_snapshot < 255)) {
        overflow_snapshot++;
    }
    interrupts();

    // Calculate milliseconds
    uint64_t ticks = (uint64_t)overflow_snapshot * 256ULL + tcnt0_snapshot;
    prev_prescaler = prescaler;
    return (ticks * prescaler * 1000ULL) / F_CPU;
}