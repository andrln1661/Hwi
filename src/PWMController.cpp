
// PWMController.cpp
#include "PWMController.h"
#include "Config.h"     // Contains MIN_PWM_FREQ and MAX_PWM_FREQ
#include <Arduino.h>
#include <avr/io.h>     // Direct access to AVR timer registers

// Initialize all timers (Timer0–Timer5) in PWM mode
void PWMController::initialize() {
    // ---------------- Timer 0: Pins 13 (OC0A), 4 (OC0B) ----------------
    TCCR0A = _BV(WGM00) | _BV(COM0A1) | _BV(COM0B1);  // Phase Correct PWM mode, clear OC0A/OC0B on compare match
    TCCR0B = _BV(CS01) | _BV(CS00);                   // Clock prescaler = /64

    // ---------------- Timer 1: Pins 11 (OC1A), 12 (OC1B) ----------------
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);  // Phase and frequency correct PWM
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11);     // Prescaler = /8
    ICR1 = 2000;                                      // TOP value (sets PWM period)

    // ---------------- Timer 2: Pins 9 (OC2B), 10 (OC2A) ----------------
    TCCR2A = _BV(WGM20) | _BV(WGM21) | _BV(COM2A1) | _BV(COM2B1); // Fast PWM
    TCCR2B = _BV(CS22);                                            // Prescaler = /64

    // ---------------- Timer 3: Pins 2 (OC3B), 3 (OC3C), 5 (OC3A) ----------------
    TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(WGM31); // PWM, Phase & Frequency Correct
    TCCR3B = _BV(WGM33) | _BV(WGM32) | _BV(CS31);    // Prescaler = /8
    ICR3 = 2000;                                     // TOP

    // ---------------- Timer 4: Pins 6 (OC4A), 7 (OC4B), 8 (OC4C) ----------------
    TCCR4A = _BV(COM4A1) | _BV(COM4B1) | _BV(WGM41);
    TCCR4B = _BV(WGM43) | _BV(WGM42) | _BV(CS41);
    ICR4 = 2000;

    // ---------------- Timer 5: Pins 44 (OC5C), 45 (OC5B), 46 (OC5A) ----------------
    TCCR5A = _BV(COM5A1) | _BV(COM5B1) | _BV(WGM51);
    TCCR5B = _BV(WGM53) | _BV(WGM52) | _BV(CS51);
    ICR5 = 2000;
}

// Set PWM frequency for the given pin
void PWMController::setFrequency(uint8_t pin, uint32_t freq) {
    if (freq == 0) return;
    freq = constrain(freq, MIN_PWM_FREQ, MAX_PWM_FREQ);  // Safety clamp

    // -------- Timer 0 (8-bit): Pins 4, 13 --------
    if (pin == 13 || pin == 4) {
        // Change prescaler to modify frequency (limited resolution)
        if (freq >= 30000)       TCCR0B = (TCCR0B & 0xF8) | 0x01; // ~31.3 kHz
        else if (freq >= 4000)   TCCR0B = (TCCR0B & 0xF8) | 0x02; // ~3.9 kHz
        else if (freq >= 1000)   TCCR0B = (TCCR0B & 0xF8) | 0x03; // ~490 Hz
        else if (freq >= 300)    TCCR0B = (TCCR0B & 0xF8) | 0x04; // ~122 Hz
        else                     TCCR0B = (TCCR0B & 0xF8) | 0x05; // ~30 Hz
    }

    // -------- Timer 2 (8-bit): Pins 9, 10 --------
    else if (pin == 9 || pin == 10) {
        if (freq >= 30000)       TCCR2B = (TCCR2B & 0xF8) | 0x01;
        else if (freq >= 8000)   TCCR2B = (TCCR2B & 0xF8) | 0x02;
        else if (freq >= 2000)   TCCR2B = (TCCR2B & 0xF8) | 0x03;
        else if (freq >= 1000)   TCCR2B = (TCCR2B & 0xF8) | 0x04;
        else if (freq >= 500)    TCCR2B = (TCCR2B & 0xF8) | 0x05;
        else if (freq >= 250)    TCCR2B = (TCCR2B & 0xF8) | 0x06;
        else                     TCCR2B = (TCCR2B & 0xF8) | 0x07;
    }

    // -------- 16-bit Timers (1,3,4,5): Set ICRx as TOP --------
    else {
        uint32_t top = F_CPU / (8UL * freq) - 1; // F_CPU = 16MHz, Prescaler = /8
        switch (pin) {
            case 11: case 12:          ICR1 = top; break;
            case 2: case 3: case 5:    ICR3 = top; break;
            case 6: case 7: case 8:    ICR4 = top; break;
            case 44: case 45: case 46: ICR5 = top; break;
        }
    }
}

// Set PWM duty cycle (0–1000) for the given pin
void PWMController::setDutyCycle(uint8_t pin, uint16_t duty) {
    noInterrupts();
    duty = constrain(duty, 0, 1000);  // Duty expressed in per-mille (0–1000)
    uint16_t ocrValue = 0;

    switch (pin) {
        // --- Timer1 (ICR1) ---
        case 11: ocrValue = (uint32_t)duty * ICR1 / 1000UL; OCR1A = ocrValue; break;
        case 12: ocrValue = (uint32_t)duty * ICR1 / 1000UL; OCR1B = ocrValue; break;

        // --- Timer3 (ICR3) ---
        case 2:  ocrValue = (uint32_t)duty * ICR3 / 1000UL; OCR3B = ocrValue; break;
        case 3:  ocrValue = (uint32_t)duty * ICR3 / 1000UL; OCR3C = ocrValue; break;
        case 5:  ocrValue = (uint32_t)duty * ICR3 / 1000UL; OCR3A = ocrValue; break;

        // --- Timer4 (ICR4) ---
        case 6:  ocrValue = (uint32_t)duty * ICR4 / 1000UL; OCR4A = ocrValue; break;
        case 7:  ocrValue = (uint32_t)duty * ICR4 / 1000UL; OCR4B = ocrValue; break;
        case 8:  ocrValue = (uint32_t)duty * ICR4 / 1000UL; OCR4C = ocrValue; break;

        // --- Timer2 (8-bit, TOP=255) ---
        case 9:  ocrValue = (uint32_t)duty * 255 / 1000UL; OCR2B = ocrValue; break;
        case 10: ocrValue = (uint32_t)duty * 255 / 1000UL; OCR2A = ocrValue; break;

        // --- Timer0 (8-bit, TOP=255) ---
        case 13: ocrValue = (uint32_t)duty * 255 / 1000UL; OCR0A = ocrValue; break;
        case 4:  ocrValue = (uint32_t)duty * 255 / 1000UL; OCR0B = ocrValue; break;

        // --- Timer5 (ICR5) ---
        case 44: ocrValue = (uint32_t)duty * ICR5 / 1000UL; OCR5C = ocrValue; break;
        case 45: ocrValue = (uint32_t)duty * ICR5 / 1000UL; OCR5B = ocrValue; break;
        case 46: ocrValue = (uint32_t)duty * ICR5 / 1000UL; OCR5A = ocrValue; break;
    }
    interrupts();
}