/*
 * MHZ19 Sensor Example - UART (Arduino AVR: Uno, Nano, dll)
 *
 * Note:
 * Because Arduino Uno/Nano only has 1 HardwareSerial (Pin 0 & 1)
 * which is usually used for connection to PC/Serial Monitor,
 * we will use SoftwareSerial to communicate with the sensor.
 * 
 * Wiring:
 * Sensor TX -> Arduino Pin 4 (RX)
 * Sensor RX -> Arduino Pin 5 (TX)
 * Sensor Vin -> 5V
 * Sensor GND -> GND
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "MHZ19.h"
#include "MHZ19_Arduino.h"

// Initialize SoftwareSerial for MHZ19
#define MHZ19_RX_PIN 4
#define MHZ19_TX_PIN 5
SoftwareSerial MHZ19_Serial(MHZ19_RX_PIN, MHZ19_TX_PIN);

// Initialize UART Arduino Platform
MHZ19_Arduino_Platform uartPlatform(&MHZ19_Serial, 1000); // 1000ms timeout
// Inisialisasi Core Logic Sensor
MHZ19 co2UART(&uartPlatform);

void setup() {
    // Initialize Serial Monitor
    Serial.begin(9600);
    while (!Serial) { ; }
    
    Serial.println("=== Test MH-Z19 via UART (Arduino) ===");

    // Initialize SoftwareSerial with default sensor baudrate (9600)
    MHZ19_Serial.begin(9600);
    delay(100);

    // Initialize sensor
    co2UART.begin();

    // Wait for sensor to be ready (optional, or turn off ABC if needed)
    // co2UART.setABCFunction(false);
    
    Serial.println("Sensor Ready, reading data...");
}

void loop() {
    uint16_t co2_ppm = 0;
    int16_t temperature = 0;

    // Reading CO2 and temperature data via UART
    if (co2UART.readCO2WithRetry(&co2_ppm, &temperature, 3)) {
        Serial.print("CO2: ");
        Serial.print(co2_ppm);
        Serial.print(" ppm | Temp: ");
        Serial.print(temperature);
        Serial.println(" C");
    } else {
        Serial.print("Gagal membaca dari sensor. Error: ");
        Serial.println(co2UART.getErrorString(co2UART.getLastError()));
    }

    // Delay 2 seconds before next reading
    delay(2000);
}
