#include <unity.h>
#include "Motor.h"
#include "ModbusHandler.h"

// Mock PWM and current reading
namespace PWMController {
    void setDutyCycle(uint8_t, uint16_t) {}
    void setFrequency(uint8_t, uint32_t) {}
}
uint16_t analogRead(uint8_t) { return 512; } // Mid-range

void test_motor_overcurrent() {
    Motor motor(0, 9, A0, nullptr);
    MockModbusHandler mb;
    modbusHandler = &mb;
    mb.thresholds[ModbusReg::MOTOR_CURR_CRIT] = 5000; // 5A
    
    motor.update();
    TEST_ASSERT_EQUAL(2, motor.getStatus()); // Critical state
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_motor_overcurrent);
    return UNITY_END();
}
