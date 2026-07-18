/*
 * MHZ19 Sensor Example - PWM (ESP32)
 *
 * The ESP32 is highly flexible; attachInterrupt() can be used on almost any GPIO pin.
 * The PWM signal is read using the high-precision internal hardware timer 'micros()' function.
 * 
 * Wiring:
 * Sensor PWM -> ESP32 GPIO 4 (Or any digital pin)
 * Sensor Vin -> 5V
 * Sensor GND -> GND
 */

#include <Arduino.h>
#include "MHZ19.h"
#include "MHZ19_ESP32.h"

// Connect pin PWM sensor to this GPIO
#define MHZ19_PWM_PIN 4

// Initialize ESP32 PWM Platform
MHZ19_PWM_ESP32_Platform pwmPlatform;

// Initialize Core Logic PWM, adjust the default detection range according to the sensor
// Usually RANGE_2000PPM or RANGE_5000PPM
MHZ19_PWM co2PWM(&pwmPlatform, MHZ19::RANGE_5000PPM);

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("=== Tes MH-Z19 via PWM (ESP32) ===");

    // Activate interrupt for reading signal cycles
    if (co2PWM.begin(MHZ19_PWM_PIN)) {
        Serial.println("PWM capture siap.");
    } else {
        Serial.println("Failed to initialize PWM capture! Check the pin used.");
    }
}

void loop() {
    uint16_t co2_ppm = 0;

    // Membaca konsentrasi dari 1 siklus PWM (butuh waktu pemrosesan ~1004ms (1 detik) penuh)
    if (co2PWM.readCO2(&co2_ppm, 1100)) {
        Serial.printf("CO2 (via PWM): %u ppm\n", co2_ppm);
    } else {
        Serial.println("Gagal/timeout menangkap siklus sinyal PWM secara utuh.");
    }

    delay(2000);
}
