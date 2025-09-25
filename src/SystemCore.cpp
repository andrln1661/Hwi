#include "Globals.h"
#include "SystemCore.h"
#include "Config.h"
#include "PWMController.h" // Add this line

void uint64_to_string(uint64_t n, char *buf)
{
    if (n == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    char *p = &buf[21];
    *p = '\0';

    do
    {
        *--p = '0' + (n % 10);
        n /= 10;
    } while (n > 0);

    memmove(buf, p, &buf[21] - p + 1);
}
SystemCore::SystemCore()
    : modbus(Serial, SLAVE_ID),
      airSensor(AIR_TEMP_PIN,
                ModbusHoldingReg::AIR_TEMP_REG,
                ModbusHoldingReg::AIR_TEMP_LIMIT),
      waterSensor(WATER_TEMP_PIN,
                  ModbusHoldingReg::WATER_TEMP_REG,
                  ModbusHoldingReg::WATER_TEMP_LIMIT),
      prev_prescaler(0)
{
    ::modbusHandler = &modbus;
    ::deviceManager = &deviceManager;
    ::motors = this->motors; // Update global pointer to motor array

    // Initialize motor sensors with dynamic allocation
    for (uint8_t i = 0; i < NUM_MOTORS; i++)
    {
        motorSensors[i] = new TemperatureSensor(
            TEMP_PINS[i],
            ModbusInputReg::TEMP_BASE + i,
            ModbusHoldingReg::MOTOR_TEMP_CRIT);
    }

    // Initialize motors with dynamic allocation
    for (uint8_t i = 0; i < NUM_MOTORS; i++)
    {
        motors[i] = new Motor(
            i,
            PWM_PINS[i],
            CURRENT_PINS[i],
            motorSensors[i], // Pass pointer to sensor
            this->modbus);
    }
}

SystemCore::~SystemCore()
{
    // Clean up dynamically allocated objects
    for (uint8_t i = 0; i < NUM_MOTORS; i++)
    {
        delete motorSensors[i];
        delete motors[i];
    }
}

void SystemCore::setup()
{
    modbus.begin();
    Serial1.begin(250000);

    // Initialize sensors
    airSensor.begin();
    waterSensor.begin();
    for (uint8_t i = 0; i < NUM_MOTORS; i++)
    {
        motorSensors[i]->begin(); // Pointer access
    }

    // Initialize motors
    for (uint8_t i = 0; i < NUM_MOTORS; i++)
    {
        motors[i]->begin(); // Pointer access
    }

    PWMController::initialize();
    // Initialize devices
    deviceManager.begin();
    Serial1.println("Connection established");
}

void SystemCore::loop()
{
    static uint8_t currentSensor = 0;

    static uint64_t lastMotorUpdate = 0;
    static uint64_t lastTempUpdate = 0;
    static uint64_t lastSerialUpdate = 0;
    uint64_t now = PWMController::millisCustom();

    modbus.setIreg(ModbusInputReg::TIME_LOW, float(now & 0xFFFF));
    modbus.setIreg(ModbusInputReg::TIME_LOW + 1, float((now >> 16) & 0xFFFF));
    modbus.setIreg(ModbusInputReg::TIME_LOW + 2, uint16_t((now >> 32) & 0xFFFF));
    modbus.setIreg(ModbusInputReg::TIME_LOW + 3, uint16_t((now >> 48) & 0xFFFF));

    modbus.task();

    if (now - lastSerialUpdate >= 5000)
    {
        Serial1.println("\n\n\n\n=== MOTORS REGISTERS ===");
        for (uint8_t i = 0; i < NUM_MOTORS; i++) {
            Serial1.print("Motor ");
            Serial1.print(i);
            Serial1.print(" // DUTY: ");
            Serial1.print(motors[i]->getDuty());
            Serial1.print(" TEMP: ");
            Serial1.print(motors[i]->getTemp());
            Serial1.print(" CURRENT: ");
            Serial1.print(motors[i]->getCurr());
            Serial1.println();
        }
        Serial1.println("=== SYSTEM REGISTERS ===");
        Serial1.print("START_REG_ADDR: ");
        Serial1.println(modbus.getHreg(ModbusHoldingReg::START_REG_ADDR));

        lastSerialUpdate = now;
    }

    // Update motor status every 500ms
    if (now - lastMotorUpdate >= 500)
    {
        lastMotorUpdate = now;
        for (uint8_t i = 0; i < NUM_MOTORS; i += 3)
        { // Process 2 motors per cycle
            if (i < NUM_MOTORS)
                motors[i]->update(now);
            if (i + 1 < NUM_MOTORS)
                motors[i + 1]->update(now);
            if (i + 2 < NUM_MOTORS)
                motors[i + 2]->update(now);
            modbus.task(); // Handle Modbus between batches
        }
    }

    // Update temperature and devices every 1000ms
    if (now - lastTempUpdate >= 1000)
    {
        lastTempUpdate = now;

        // Request temperatures first
        for (uint8_t i = 0; i < NUM_MOTORS; i++)
        {
            motorSensors[i]->requestTemperaturesAsync(now); // Pointer access
        }

        // Then update all temperature readings
        for (uint8_t i = 0; i < NUM_MOTORS; i++)
        {
            motorSensors[i]->update(); // Pointer access
        }

        airSensor.update();
        waterSensor.update();

        // Update devices (pass pointer to motor array)
        deviceManager.update(airSensor, waterSensor, motors);

        // Update timestamp register
    }
}