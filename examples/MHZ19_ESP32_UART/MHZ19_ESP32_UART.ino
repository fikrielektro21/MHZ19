/*
 * MHZ19 Sensor Example - UART (ESP32)
 *
 * The ESP32 has 3 HardwareSerial ports. We will use HardwareSerial 2 (Serial2).
 * 
 * Wiring (Adjust RX/TX pins to match your setup):
 * Sensor TX -> ESP32 GPIO 16 (RX2)
 * Sensor RX -> ESP32 GPIO 17 (TX2)
 * Sensor Vin -> 5V (Note: Ensure the ESP32 is safe from the sensor's 5V logic level on the TX line if using specific boards, though most are safe).
 * Sensor GND -> GND
 */
 
#include <Arduino.h>
#include "MHZ19.h"
#include "MHZ19_ESP32.h"

// Define pin koneksi
#define MHZ19_UART_RX 16
#define MHZ19_UART_TX 17

// Initialize HardwareSerial 2
HardwareSerial MHZ19_UART(2);

// Initialize ESP32 UART Platform
MHZ19_ESP32_Platform uartPlatform(&MHZ19_UART, 1000); // Timeout 1000ms

// Initialize Core Logic UART
MHZ19 co2UART(&uartPlatform);

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("=== Tes MH-Z19 via UART (ESP32) ===");

    // Start port serial for sensor (Baudrate 9600)
    MHZ19_UART.begin(9600, SERIAL_8N1, MHZ19_UART_RX, MHZ19_UART_TX);
    delay(100); // take time for UART settle
    
    co2UART.begin();
    
    // (Opsional) OFF Auto-Calibration if using in AC environment
    // co2UART.setABCFunction(false);
    
    Serial.println("Sensor ready, reading data...");
}

void loop() {
    uint16_t co2_ppm = 0;
    int16_t temperature = 0;

    // Read with auto-retry mechanism 3 times if there is a communication failure
    if (co2UART.readCO2WithRetry(&co2_ppm, &temperature, 3)) {
        Serial.printf("CO2: %u ppm | Temp: %d C\n", co2_ppm, temperature);
    } else {
        Serial.printf("Failed to read: %s\n", co2UART.getErrorString(co2UART.getLastError()));
    }

    delay(2000); // Wait 2 seconds
}
