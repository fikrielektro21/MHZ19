#ifndef MHZ19_ESP32_H
#define MHZ19_ESP32_H

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

#include "MHZ19.h"
#include "Arduino.h"
//#include <HardwareSerial.h>

/**
 * @brief Concrete implementation of MHZ19_Platform_Interface specifically for ESP32
 */
 class MHZ19_ESP32_Platform : public MHZ19_Platform_Interface {
    private:
        Stream* _serial;
        uint32_t _timeout_ms; ///< Timeout for UART read/write in milliseconds

    public:

        /**
        * @brief Constructor for ESP32 Platform
        * @param serial Pointer to ESP32 HardwareSerial object (e.g., &Serial2)
        * @param timeout Timeout for UART read/write operations in milliseconds
        */
        explicit MHZ19_ESP32_Platform(Stream* serial, uint32_t timeout_ms = 1000)
            : _serial(serial), _timeout_ms(timeout_ms) {}


        bool uart_transmit(const uint8_t* data, uint16_t length) override {
            if (!_serial) return false;
            while (_serial->available()) _serial->read(); // Clear RX buffer
            return (_serial->write(data, length) == length);
        }

        
        bool uart_receive(uint8_t* data, uint16_t length, uint32_t timeout_ms = 1000) override {
            if (!_serial) return false;
            uint32_t timeout = (timeout_ms > 0) ? timeout_ms : _timeout_ms;
            uint32_t start_time = millis();
            uint16_t idx = 0;

            while ((idx < length) && (millis() - start_time < timeout_ms)) {
                if (_serial->available() > 0) {
                    data[idx] = _serial->read();
                    idx++;
                }
            }
            return (idx == length);
        }

        bool set_pin_mode_output(uint8_t pin) override {
            pinMode(pin, OUTPUT);
            return true;
        }

        bool set_pin_low(uint8_t pin) override {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW);
            return true;
        }

        bool set_pin_high(uint8_t pin) override {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            return true;
        }

        uint32_t get_millis() override {
            return millis();
        }
        void delay_ms(uint32_t ms) override {
        delay(ms); // Native Arduino delay (blocking, fine for sensor spacing)
        }
    };
    
// ================================================================
// ============ PWM: PLATFORM ESP32/ARDUINO =======================
// ================================================================

/**
 * @brief Concrete implementation of MHZ19_PWM_Platform_Interface for ESP32/Arduino.
 *
 * Uses attachInterrupt() + micros(). This is NOT a regular polling approach: micros()
 * on ESP32 is backed by an internal 64-bit hardware timer, so its precision
 * is equivalent to hardware capture without needing to manually program the MCPWM Capture Unit.
 * (The API varies between different versions of the ESP-IDF Arduino core, hence it's intentionally
 * not used here to maintain portability and stability).
* @note ESP32: attachInterrupt() works on ALMOST ALL GPIO pins (GPIO Matrix).
 * not used here so it remains portable & stable).
 *
 * @note ESP32: attachInterrupt() WORKS on ALMOST ALL GPIO pins (GPIO Matrix).
 * @note Arduino AVR (Uno/Nano): attachInterrupt() Only works on specific pins
 *       (Uno: D2 & D3 only). If you connect to a different pin
 *       on the AVR board, this function will fail - it's a chip limitation, not a bug.
 */

class MHZ19_PWM_ESP32_Platform : public MHZ19_PWM_Platform_Interface {
    private:
        uint8_t _pin = 255;
        volatile uint32_t _lastEdgeMicros = 0;
        volatile uint32_t _capturedHigh_us = 0;
        volatile uint32_t _capturedLow_us = 0;
        volatile bool _cycleReady = false;
        
        static MHZ19_PWM_ESP32_Platform*& instancePtr();
        static void IRAM_ATTR isrHandler();
        
	public:
		bool begin_pwm_capture(uint8_t pin) override {
			_pin = pin;
			pinMode(_pin, INPUT);
			instancePtr() = this;
			_lastEdgeMicros = micros();
			_cycleReady = false;
			
			attachInterrupt(digitalPinToInterrupt(_pin), isrHandler, CHANGE);
			return true;
		}
		
		bool read_pwm(uint32_t* high_us, uint32_t* low_us, uint32_t timeout_ms) override {
			uint32_t start = millis();
			//
			
			while (!_cycleReady) {
				if (millis() - start > timeout_ms) return false;
				yield();
			}
			portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
			portENTER_CRITICAL(&myMutex);
			//noInterrupts();
			*high_us = _capturedHigh_us;
			*low_us = _capturedLow_us;
			_cycleReady = false;
			portEXIT_CRITICAL(&myMutex);
			//interrupts();
			return true;
		}
		
		void end_pwm_capture() override {
			detachInterrupt(digitalPinToInterrupt(_pin));
			instancePtr() = nullptr;
		}
};

#endif // ESP32 || ARDUINO_ARCH_ESP32

#endif // MHZ19_ESP32_H