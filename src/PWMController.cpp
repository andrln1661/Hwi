// PWMController.cpp
#include "PWMController.h"
#include "Config.h"     // Contains MIN_PWM_FREQ and MAX_PWM_FREQ
#include <Arduino.h>
#include <avr/io.h>     // Direct access to AVR timer registers

// Initialize all timers (Timer0–Timer5) in PWM mode
void PWMController::initialize() {
    // Ensure pins are outputs
    // pinMode(13, OUTPUT); // PB7: OC1C (Timer1) — repurposed from OC0A
    // pinMode(4,  OUTPUT); // PG5: OC0B (Timer0)

    // ---------------- Timer 0: Pin 4 (OC0B) with OCR0A as TOP ----------------
    // Fast PWM with OCR0A as TOP (WGM02:0 = 7). OC0B enabled, OC0A disconnected.
    TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(COM0B1);
    TCCR0B = _BV(WGM02) | _BV(CS00); // prescaler = /1
    OCR0A  = 0xFF;
    OCR0B  = 0x00;

    // ---------------- Timer 1: Pins 11 (OC1A), 12 (OC1B), 13 (OC1C) ----------------
    // Mode 14 Fast PWM, TOP=ICR1
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1C1) | _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // prescaler = /1
    ICR1   = 2000;

    // ---------------- Timer 2: Pins 9,10 (OC2A,OC2B) ----------------
    // Fast PWM, fixed prescaler choices
    TCCR2A = _BV(WGM20) | _BV(WGM21) | _BV(COM2A1) | _BV(COM2B1);
    TCCR2B = _BV(CS20);
    OCR2A  = 0xFF;
    OCR2B  = 0x00;

    // ---------------- Timer 3: Pins 5,2,3 (OC3A/OC3B/OC3C) ----------------
    TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(WGM31);
    TCCR3B = _BV(WGM33) | _BV(WGM32) | _BV(CS30);
    ICR3   = 2000;

    // ---------------- Timer 4: Pins 6,7,8 (OC4A/OC4B/OC4C) ----------------
    TCCR4A = _BV(COM4A1) | _BV(COM4B1) | _BV(WGM41);
    TCCR4B = _BV(WGM43) | _BV(WGM42) | _BV(CS40);
    ICR4   = 2000;

    // ---------------- Timer 5: Pins 46 (OC5A), 45 (OC5B), 44 (OC5C) ----------------
    TCCR5A = _BV(COM5A1) | _BV(COM5B1) | _BV(WGM51);               // Fast PWM, Mode 14
    TCCR5B = _BV(WGM53) | _BV(WGM52) | _BV(CS50);                  // Prescaler = /1
    ICR5   = 2000;
}

// Default global frequency
uint32_t PWMController::currentGlobalFreq = 1000;
void PWMController::setGlobalFrequency(uint32_t freq) {
    currentGlobalFreq = constrain(freq, MIN_PWM_FREQ, MAX_PWM_FREQ);

    // Timer 0 (8-bit): Pin 4


    // Timer 2 (8-bit): Pins 9, 10 
    if (freq >= 30000)       TCCR2B = (TCCR2B & 0xF8) | 0x01;
    else if (freq >= 20000)  TCCR2B = (TCCR2B & 0xF8) | 0x02;
    else if (freq >= 10000)  TCCR2B = (TCCR2B & 0xF8) | 0x03;
    else if (freq >= 5000)   TCCR2B = (TCCR2B & 0xF8) | 0x04;
    else if (freq >= 1000)   TCCR2B = (TCCR2B & 0xF8) | 0x05;
    else if (freq >= 500)    TCCR2B = (TCCR2B & 0xF8) | 0x06;
    else                     TCCR2B = (TCCR2B & 0xF8) | 0x07;

    // Timers 1, 3, 4, 5 (16-bit): pins 2, 3, 5, 6, 7, 8, 11, 12...is switched from Timer 2 to Timer 1 channel C in initialization)
    // Let timebase code handle Timer1 prescaler to preserve time accounting.
    TCCR1B = (TCCR1B & 0xF8) | 0x01;
    TCCR3B = (TCCR3B & 0xF8) | 0x01;
    TCCR4B = (TCCR4B & 0xF8) | 0x01;
    TCCR5B = (TCCR5B & 0xF8) | 0x01;

    // f = F_CPU / (prescaler * (1 + TOP)), with prescaler = 1:
    uint32_t top = (F_CPU / (1UL * freq));
    if (top > 0) top -= 1;
    if (top < 1) top = 1;           // avoid TOP=0
    if (top > 65535UL) top = 65535; // with prescaler=1, this caps at ~244 Hz

    // Program Timer1 TOP via timebase API so timebase snapshots prior epoch
    ICR1 = uint16_t(top);
    ICR3 = uint16_t(top);
    ICR4 = uint16_t(top);
    ICR5 = uint16_t(top);
}

// ... rest of PWMController implementation remains unchanged ...
// (functions to set duty for channels, map pins to timers, etc.)


// Set PWM frequency for the given pin
// void PWMController::setFrequency(uint8_t pin, uint32_t freq) {
//     if (freq == 0) return;
//     freq = constrain(freq, MIN_PWM_FREQ, MAX_PWM_FREQ);  // Safety clamp

//     // -------- Timer 0 (Fast PWM, OCR0A as TOP): Pin 4 only; choose prescaler + TOP --------
//     if (pin == 4) {
//         // Try prescalers in ascending order to maximize TOP while staying within 8-bit limit.
//         const uint16_t prescVals[5] = {1, 8, 64, 256, 1024};
//         const uint8_t  csBits[5]    = {0x01, 0x02, 0x03, 0x04, 0x05};

//         uint8_t chosenCS = csBits[4]; // default to largest prescaler
//         uint8_t top = 255;

//         for (uint8_t i = 0; i < 5; ++i) {
//             uint32_t calcTop = (F_CPU / (prescVals[i] * (uint32_t)freq));
//             if (calcTop == 0) continue;
//             calcTop -= 1;
//             if (calcTop >= 2 && calcTop <= 255) { // keep at least 2 for non-trivial duty resolution
//                 chosenCS = csBits[i];
//                 top = (uint8_t)calcTop;
//                 break; // first fit => largest TOP in range
//             }
//         }

//         // Program prescaler (preserve WGM02) and TOP
//         TCCR0B = (TCCR0B & 0xF8) | chosenCS; // keep upper bits incl. WGM02
//         OCR0A  = top;                        // new TOP for Timer0
//         return;
//     }

//     // -------- Timer 2 (8-bit): Pins 9, 10 (unchanged behavior) --------
//     else if (pin == 9 || pin == 10) {
//         if (freq >= 30000)       TCCR2B = (TCCR2B & 0xF8) | 0x01;
//         else if (freq >= 8000)   TCCR2B = (TCCR2B & 0xF8) | 0x02;
//         else if (freq >= 2000)   TCCR2B = (TCCR2B & 0xF8) | 0x03;
//         else if (freq >= 1000)   TCCR2B = (TCCR2B & 0xF8) | 0x04;
//         else if (freq >= 500)    TCCR2B = (TCCR2B & 0xF8) | 0x05;
//         else if (freq >= 250)    TCCR2B = (TCCR2B & 0xF8) | 0x06;
//         else                     TCCR2B = (TCCR2B & 0xF8) | 0x07;
//         return;
//     }

//     // -------- 16-bit timers (1,3,4,5): prescaler = 1; set ICRx as TOP --------
//     else {
//         // f = F_CPU / (N * (1 + TOP)), with N = 1
//         uint32_t top = (F_CPU / (1UL * freq));
//         if (top > 0) top -= 1;
//         if (top < 1) top = 1;           // avoid TOP=0
//         if (top > 65535UL) top = 65535; // with N=1, this caps at ~244 Hz

//         switch (pin) {
//             case 11: case 12: case 13:
//                 // Ensure prescaler = 1 (preserve WGM bits)
//                 TCCR1B = (TCCR1B & 0xF8) | 0x01;
//                 ICR1 = (uint16_t)top;
//                 break;
//             case 5: case 2: case 3:
//                 TCCR3B = (TCCR3B & 0xF8) | 0x01;
//                 ICR3 = (uint16_t)top;
//                 break;
//             case 6: case 7: case 8:
//                 TCCR4B = (TCCR4B & 0xF8) | 0x01;
//                 ICR4 = (uint16_t)top;
//                 break;
//             case 46: case 45: case 44:
//                 TCCR5B = (TCCR5B & 0xF8) | 0x01;
//                 ICR5 = (uint16_t)top;
//                 break;
//         }
//         return;
//     }
// }

// Set PWM duty cycle (0–1000) for the given pin
void PWMController::setDutyCycle(uint8_t pin, uint16_t duty) {
    noInterrupts();
    duty = constrain(duty, 0, 1000);  // per-mille (0–1000)
    uint16_t ocrValue = 0;

    switch (pin) {
        // --- Timer1 (ICR1 as TOP) ---
        case 11: ocrValue = (uint32_t)duty * ICR1 / 1000UL; OCR1A = ocrValue; break;
        case 12: ocrValue = (uint32_t)duty * ICR1 / 1000UL; OCR1B = ocrValue; break;
        case 13: ocrValue = (uint32_t)duty * ICR1 / 1000UL; OCR1C = ocrValue; break;

        // --- Timer3 (ICR3 as TOP) ---
        case 5:  ocrValue = (uint32_t)duty * ICR3 / 1000UL; OCR3A = ocrValue; break;
        case 2:  ocrValue = (uint32_t)duty * ICR3 / 1000UL; OCR3B = ocrValue; break;
        case 3:  ocrValue = (uint32_t)duty * ICR3 / 1000UL; OCR3C = ocrValue; break;

        // --- Timer4 (ICR4 as TOP) ---
        case 6:  ocrValue = (uint32_t)duty * ICR4 / 1000UL; OCR4A = ocrValue; break;
        case 7:  ocrValue = (uint32_t)duty * ICR4 / 1000UL; OCR4B = ocrValue; break;
        case 8:  ocrValue = (uint32_t)duty * ICR4 / 1000UL; OCR4C = ocrValue; break;

        // --- Timer2 (8-bit, TOP=255) ---
        case 9:  ocrValue = (uint32_t)duty * 255 / 1000UL; OCR2B = ocrValue; break;
        case 10: ocrValue = (uint32_t)duty * 255 / 1000UL; OCR2A = ocrValue; break;

        // --- Timer0 (Fast PWM, TOP=OCR0A) ---
        case 4:  ocrValue = (uint32_t)duty * OCR0A / 1000UL; OCR0B = ocrValue; break;

        // --- Timer5 (ICR5 as TOP) ---
        case 46: ocrValue = (uint32_t)duty * ICR5 / 1000UL; OCR5A = ocrValue; break;
        case 45: ocrValue = (uint32_t)duty * ICR5 / 1000UL; OCR5B = ocrValue; break;
        case 44: ocrValue = (uint32_t)duty * ICR5 / 1000UL; OCR5C = ocrValue; break;
    }
    interrupts();
}
