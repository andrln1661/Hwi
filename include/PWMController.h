
// PWMController.h
#pragma once
#include <Arduino.h>  // Provides types like uint8_t and utility macros like constrain()

// Class to control hardware PWM on Arduino Mega (ATmega2560)
class PWMController {
public:
    // Initializes all timers (0 to 5) into appropriate PWM mode
    static void initialize();

    // Sets the frequency for a specific pin by modifying timer prescaler or TOP
    static void setFrequency(uint8_t pin, uint32_t freq);

    // Sets the duty cycle (0–1000) for a specific pin
    static void setDutyCycle(uint8_t pin, uint16_t duty);
};