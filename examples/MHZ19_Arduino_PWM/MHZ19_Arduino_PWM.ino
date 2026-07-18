/*
 * MHZ19 Sensor Example - PWM (Arduino AVR: Uno, Nano, etc.)
 *
 * Note:
 * On Arduino Uno/Nano, the attachInterrupt() function is available 
 * only on Digital Pin 2 (Interrupt 0) and Digital Pin 3 (Interrupt 1).
 * Therefore, the sensor's PWM pin MUST be connected to Pin 2 or Pin 3.
 * 
 * Wiring:
 * Sensor PWM -> Arduino Pin 2
 * Sensor Vin -> 5V
 * Sensor GND -> GND
 */

#include <Arduino.h>
#include "MHZ19.h"
#include "MHZ19_Arduino.h"

// Pins that support interrupts on Arduino Uno/Nano (2 or 3)
#define MHZ19_PWM_PIN 2

// Initialize Arduino PWM Platform
MHZ19_PWM_Arduino_Platform pwmPlatform;

// Initialize PWM Core Logic, adjust range
// (Depending on the default sensor type from the factory, usually RANGE_2000PPM or RANGE_5000PPM)
MHZ19_PWM co2PWM(&pwmPlatform, MHZ19::RANGE_5000PPM);

void setup() {
    // Start Serial Monitor
    Serial.begin(9600);
    while (!Serial) { ; }
    
    Serial.println("=== Tes MH-Z19 via PWM (Arduino) ===");

    // Initialize PWM capture on the specified pin
    if (co2PWM.begin(MHZ19_PWM_PIN)) {
        Serial.println("Successfully activated PWM Capture.");
    } else {
        Serial.println("Failed to activate PWM Capture! Check the pin used.");
    }

    Serial.println("Waiting for sensor data...");
}

void loop() {
    uint16_t co2_ppm = 0;

    // Read CO2 concentration from 1 PWM cycle
    // Requires approximately ~1004ms (1 full cycle)
    if (co2PWM.readCO2(&co2_ppm, 1100)) { // timeout 1100ms
        Serial.print("CO2 (via PWM): ");
        Serial.print(co2_ppm);
        Serial.println(" ppm");
    } else {
        Serial.println("Failed to read PWM, Timeout or sensor not connected.");
    }

    // Delay 2 seconds before next reading
    delay(2000);
}
