/*
 * MHZ19 Sensor Example - PWM (ESP32)
 *
 * ESP32 sangat fleksibel; attachInterrupt() dapat dipasang di hampir semua pin GPIO.
 * Sinyal PWM dibaca menggunakan fitur 'micros()' hardware timer internal berpresisi tinggi.
 * 
 * Wiring:
 * Sensor PWM -> ESP32 GPIO 4 (Atau pin digital apa saja)
 * Sensor Vin -> 5V
 * Sensor GND -> GND
 */

#include <Arduino.h>
#include "MHZ19.h"
#include "MHZ19_ESP32.h"

// Sambungkan pin PWM sensor ke GPIO ini
#define MHZ19_PWM_PIN 4

// Inisialisasi Platform ESP32 PWM
MHZ19_PWM_ESP32_Platform pwmPlatform;

// Inisialisasi Core Logic PWM, sesuaikan rentang deteksi bawaan sensor
// Umumnya RANGE_2000PPM atau RANGE_5000PPM
MHZ19_PWM co2PWM(&pwmPlatform, MHZ19::RANGE_5000PPM);

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("=== Tes MH-Z19 via PWM (ESP32) ===");

    // Mengaktifkan interrupt untuk membaca siklus sinyal
    if (co2PWM.begin(MHZ19_PWM_PIN)) {
        Serial.println("PWM capture siap.");
    } else {
        Serial.println("Gagal menyiapkan PWM capture! Periksa pin yang digunakan.");
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
