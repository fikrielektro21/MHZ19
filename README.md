# MH-Z19C NDIR CO2 Sensor Library
An advanced, cross-platform Arduino library for MH-Z19C (and MH-Z19B) CO2 sensors. 

Unlike many other libraries, this library is built using a hardware abstraction layer (HAL) interface. This means it flawlessly supports **ESP32, AVR (Arduino Uno, Nano, Mega), and STM32** without spaghetti `#ifdef` macros. It supports both UART and PWM interfaces simultaneously.

## Features
- **Cross-Platform**: Supports standard Arduino AVR, ESP32, and STM32 (via HAL layer).
- **UART & PWM Support**: Read CO2 concentration with high precision using the serial protocol or PWM capture.
- **Full Calibration Control**: Turn on/off Automatic Baseline Correction (ABC), perform zero-point (400ppm) and span calibrations.
- **Hardware-Backed PWM**: On ESP32 and Arduino, it uses interrupt-driven (`attachInterrupt`) PWM capturing without blocking the main loop until you read the value.
- **Error Handling**: Built-in timeout and checksum validation mechanisms so you don't get stuck with invalid data.

## Installation
1. Download this repository as a `.zip` file.
2. In Arduino IDE, go to **Sketch** -> **Include Library** -> **Add .ZIP Library...**
3. Select the downloaded `.zip` file.

## Platform Support & Wiring

### UART Connection
- **Vin**: 5V (Sensor requires 4.5V - 5.5V, minimum 150mA).
- **GND**: GND
- **Tx**: Connect to RX of your Microcontroller.
- **Rx**: Connect to TX of your Microcontroller.

> **Note**: For ESP32 and 3.3V logic boards, the sensor's UART is 3.3V compatible, so you can connect directly without a logic level converter.

### PWM Connection
- **PWM**: Connect to an Interrupt-capable pin (e.g., Pin 2 or 3 on Arduino Uno/Nano, or any GPIO on ESP32).

## Getting Started

### 1. Basic UART on ESP32 (Hardware Serial)
```cpp
#include "MHZ19.h"
#include "MHZ19_ESP32.h"

HardwareSerial MHZ19_UART(2); 
MHZ19_ESP32_Platform uartPlatform(&MHZ19_UART, 1000);
MHZ19 co2Sensor(&uartPlatform);

void setup() {
    Serial.begin(115200);
    MHZ19_UART.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
    
    // Initialize sensor (waits for warmup internally if needed)
    co2Sensor.begin(); 
}

void loop() {
    uint16_t co2 = 0;
    int16_t temp = 0;
    
    if (co2Sensor.readCO2WithRetry(&co2, &temp, 3)) {
        Serial.printf("CO2: %u ppm | Temp: %d C\n", co2, temp);
    } else {
        Serial.printf("Error: %s\n", co2Sensor.getErrorString(co2Sensor.getLastError()));
    }
    delay(2000);
}
```

### 2. Basic UART on Arduino AVR (Software Serial)
```cpp
#include "MHZ19.h"
#include "MHZ19_Arduino.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
MHZ19_Arduino_Platform uartPlatform(&mySerial);
MHZ19 co2Sensor(&uartPlatform);

void setup() {
    Serial.begin(9600);
    mySerial.begin(9600);
    co2Sensor.begin();
}

void loop() {
    uint16_t co2 = 0;
    int16_t temp = 0;
    if (co2Sensor.readCO2(&co2, &temp)) {
        Serial.print("CO2: ");
        Serial.print(co2);
        Serial.print(" ppm | Temp: ");
        Serial.println(temp);
    }
    delay(2000);
}
```

## API Reference

### `MHZ19` (UART Core)
- `bool begin(uint32_t warmup_time_ms = 180000)`: Initialize sensor (default 3 min warmup).
- `bool readCO2(uint16_t* co2_out, int16_t* temp_out = nullptr)`: Reads current CO2 and temperature.
- `bool readCO2WithRetry(uint16_t* co2_out, int16_t* temp_out = nullptr, uint8_t retries = 3)`: Reads CO2 with automatic retries on failure.
- `bool setABCFunction(bool enable)`: Turn Automatic Baseline Correction on or off. (Turn OFF for greenhouse/indoor farming).
- `bool setDetectionRange(Range range)`: Change range (2000 or 5000 ppm).
- `bool calibrateZeroPoint()`: Set current environment as 400ppm. **Wait >20 min outside before calling**.

### `MHZ19_PWM` (PWM Core)
- `bool begin(uint8_t pin)`: Start PWM capture on an interrupt-capable pin.
- `bool readCO2(uint16_t* co2_out, uint32_t timeout_ms = 1100)`: Blocking read of a full PWM cycle (takes ~1004ms).

# WIRING

This project was designed using the following popular design applications:

* **WIRING**: [DIAGRAM](https://app.cirkitdesigner.com/project/577139f5-62ed-48c8-83cd-7bf379076e08)

* [DATA SHEET](https://www.winsen-sensor.com/d/files/infrared-gas-sensor/mh-z19c-pins-type-co2-manual-ver1_0.pdf)

* ![Image](Assets\circuit_image.png)


## License
MIT License
