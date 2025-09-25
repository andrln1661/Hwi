// PWMController.cpp
#include "PWMController.h"
#include "Config.h" // Contains MIN_PWM_FREQ and MAX_PWM_FREQ
#include <Arduino.h>
#include <avr/io.h> // Direct access to AVR timer registers

volatile uint64_t PWMController::_micros64 = 0;
uint16_t PWMController::_timer2_top = 0xFF;
uint16_t PWMController::_timer2_prescaler = 8;
uint32_t PWMController::_us_per_overflow = 1294854; // Default for 7812 Hz

// Timer 2 overflow interrupt
ISR(TIMER2_OVF_vect)
{
    PWMController::_micros64++;
}

uint64_t PWMController::microsCustom()
{
    uint64_t m;
    cli();
    m = _micros64 * _us_per_overflow;
    m = m / 1000;
    sei();
    return m;
}

uint64_t PWMController::millisCustom()
{
    return microsCustom() / 10000;
}

void PWMController::delayCustom(uint64_t ms)
{
    uint64_t start = millisCustom();
    while (millisCustom() - start < ms)
        ;
}

void PWMController::clearCount()
{
    PWMController::_micros64 = 0;
}

// Initialize all timers (Timer0–Timer5) in PWM mode
void PWMController::initialize()
{
    // Ensure pins are outputs
    // pinMode(13, OUTPUT); // PB7: OC1C (Timer1) — repurposed from OC0A
    // pinMode(4,  OUTPUT); // PG5: OC0B (Timer0)

    // ---------------- Timer 0: Pin 4 (OC0B) with OCR0A as TOP ----------------
    // Fast PWM with OCR0A as TOP (WGM02:0 = 7). OC0B enabled, OC0A disconnected.
    // TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(COM0B1);
    // TCCR0B = _BV(WGM02) | _BV(CS00); // prescaler = /1
    // OCR0A  = 0xFF;
    // OCR0B  = 0x00;

    TIMSK2 |= (1 << TOIE2);
    TIMSK0 &= ~(1 << TOIE0);

    // ---------------- Timer 1: Pins 11 (OC1A), 12 (OC1B), 13 (OC1C) ----------------
    // Mode 14 Fast PWM, TOP=ICR1
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1C1) | _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // prescaler = /1
    ICR1 = 2048;

    // ---------------- Timer 2: Pins 9,10 (OC2A,OC2B) ----------------
    // Fast PWM, fixed prescaler choices
    TCCR2A = _BV(WGM20) | _BV(WGM21) | _BV(COM2A1) | _BV(COM2B1);
    TCCR2B = _BV(CS21);

    // ---------------- Timer 3: Pins 5,2,3 (OC3A/OC3B/OC3C) ----------------
    TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(WGM31);
    TCCR3B = _BV(WGM33) | _BV(WGM32) | _BV(CS30);
    ICR3 = 2048;

    // ---------------- Timer 4: Pins 6,7,8 (OC4A/OC4B/OC4C) ----------------
    TCCR4A = _BV(COM4A1) | _BV(COM4B1) | _BV(WGM41);
    TCCR4B = _BV(WGM43) | _BV(WGM42) | _BV(CS40);
    ICR4 = 2048;

    // ---------------- Timer 5: Pins 46 (OC5A), 45 (OC5B), 44 (OC5C) ----------------
    TCCR5A = _BV(COM5A1) | _BV(COM5B1) | _BV(WGM51); // Fast PWM, Mode 14
    TCCR5B = _BV(WGM53) | _BV(WGM52) | _BV(CS50);    // Prescaler = /1
    ICR5 = 2048;
}

// Default global frequency
void PWMController::setGlobalFrequency(uint32_t freq)
{
    // Timer 0 (8-bit): Pin 4

    // Timer 2 (8-bit): Pins 9, 10
    if (freq >= 10000)
    {
        TCCR2B = (TCCR2B & 0xF8) | 0x01;
        _timer2_prescaler = 1;
    }
    else if (freq >= 5000)
    {
        TCCR2B = (TCCR2B & 0xF8) | 0x02;
        _timer2_prescaler = 8;
    }
    else if (freq >= 2000)
    {
        TCCR2B = (TCCR2B & 0xF8) | 0x03;
        _timer2_prescaler = 32;
    }
    else if (freq >= 1000)
    {
        TCCR2B = (TCCR2B & 0xF8) | 0x04;
        _timer2_prescaler = 64;
    }
    else if (freq >= 500)
    {
        TCCR2B = (TCCR2B & 0xF8) | 0x05;
        _timer2_prescaler = 128;
    }
    else if (freq >= 200)
    {
        TCCR2B = (TCCR2B & 0xF8) | 0x06;
        _timer2_prescaler = 256;
    }
    else
    {
        TCCR2B = (TCCR2B & 0xF8) | 0x07;
        _timer2_prescaler = 512;
    }
    _timer2_top = 0xFF;
    // _us_per_overflow = (1000000UL * _timer2_prescaler * (_timer2_top + 1)) / F_CPU + 1;
    _us_per_overflow = 1294854;

    // Ensure Timer2 interrupt is enabled
    TIMSK2 |= (1 << TOIE2);

    // Timers 1, 3, 4, 5 (16-bit): pins 2, 3, 5, 6, 7, 8, 11, 12...is switched from Timer 2 to Timer 1 channel C in initialization)
    // Let timebase code handle Timer1 prescaler to preserve time accounting.
    TCCR1B = (TCCR1B & 0xF8) | 0x01;
    TCCR3B = (TCCR3B & 0xF8) | 0x01;
    TCCR4B = (TCCR4B & 0xF8) | 0x01;
    TCCR5B = (TCCR5B & 0xF8) | 0x01;

    // f = F_CPU / (prescaler * (1 + TOP)), with prescaler = 1:
    uint32_t top = (F_CPU / (1UL * freq));
    if (top > 0)
        top -= 1;
    if (top < 1)
        top = 1; // avoid TOP=0
    if (top > 65535UL)
        top = 65535; // with prescaler=1, this caps at ~244 Hz

    // Program Timer1 TOP via timebase API so timebase snapshots prior epoch
    ICR1 = uint16_t(top);
    ICR3 = uint16_t(top);
    ICR4 = uint16_t(top);
    ICR5 = uint16_t(top);
}

// Set PWM duty cycle (0–1000) for the given pin
void PWMController::setDutyCycle(uint8_t pin, uint16_t duty)
{
    noInterrupts();
    duty = constrain(duty, 0, 100); // per-mille (0–1000)
    uint16_t ocrValue = 0;

    switch (pin)
    {
    // --- Timer1 (ICR1 as TOP) ---
    case 11:
        ocrValue = (uint32_t)duty * ICR1 / 100UL;
        OCR1A = ocrValue;
        break;
    case 12:
        ocrValue = (uint32_t)duty * ICR1 / 100UL;
        OCR1B = ocrValue;
        break;
    case 13:
        ocrValue = (uint32_t)duty * ICR1 / 100UL;
        OCR1C = ocrValue;
        break;

    // --- Timer3 (ICR3 as TOP) ---
    case 5:
        ocrValue = (uint32_t)duty * ICR3 / 100UL;
        OCR3A = ocrValue;
        break;
    case 2:
        ocrValue = (uint32_t)duty * ICR3 / 100UL;
        OCR3B = ocrValue;
        break;
    case 3:
        ocrValue = (uint32_t)duty * ICR3 / 100UL;
        OCR3C = ocrValue;
        break;

    // --- Timer4 (ICR4 as TOP) ---
    case 6:
        ocrValue = (uint32_t)duty * ICR4 / 100UL;
        OCR4A = ocrValue;
        break;
    case 7:
        ocrValue = (uint32_t)duty * ICR4 / 100UL;
        OCR4B = ocrValue;
        break;
    case 8:
        ocrValue = (uint32_t)duty * ICR4 / 100UL;
        OCR4C = ocrValue;
        break;

    // --- Timer2 (8-bit, TOP=255) ---
    case 9:
        ocrValue = (uint32_t)duty * 255 / 100UL;
        OCR2B = ocrValue;
        break;
    case 10:
        ocrValue = (uint32_t)duty * 255 / 100UL;
        OCR2A = ocrValue;
        break;

    // --- Timer0 (Fast PWM, TOP=OCR0A) ---
    // case 4:  ocrValue = (uint32_t)duty * OCR0A / 1000UL; OCR0B = ocrValue; break;

    // --- Timer5 (ICR5 as TOP) ---
    case 46:
        ocrValue = (uint32_t)duty * ICR5 / 100UL;
        OCR5A = ocrValue;
        break;
    case 45:
        ocrValue = (uint32_t)duty * ICR5 / 100UL;
        OCR5B = ocrValue;
        break;
    case 44:
        ocrValue = (uint32_t)duty * ICR5 / 100UL;
        OCR5C = ocrValue;
        break;
    }
    interrupts();
}

float PWMController::getDuty(uint8_t pin)
{
    switch (pin)
    {
    // --- Timer1 (ICR1 as TOP) ---
    case 11:
        return OCR1A / ICR1 * 100;
        break;
    case 12:
        return OCR1B / ICR1 * 100;
        break;
    case 13:
        return OCR1C / ICR1 * 100;
        break;

    // --- Timer3 (ICR3 as TOP) ---
    case 5:
        return OCR3A / ICR3 * 100;
        break;
    case 2:
        return OCR3B / ICR3 * 100;
        break;
    case 3:
        return OCR3C / ICR3 * 100;
        break;

    // --- Timer4 (ICR4 as TOP) ---
    case 6:
        return OCR4A / ICR4 * 100;
        break;
    case 7:
        return OCR4B / ICR4 * 100;
        break;
    case 8:
        return OCR4C / ICR4 * 100;
        break;

    // --- Timer2 (8-bit, TOP=255) ---
    case 9:
        return OCR2A / 255 * 100;
        break;
    case 10:
        return OCR2B / 255 * 100;
        break;

    case 46:
        return OCR5A / ICR5 * 100;
        break;
    case 45:
        return OCR5B / ICR5 * 100;
        break;
    case 44:
        return OCR5C / ICR5 * 100;
        break;
    }
}
