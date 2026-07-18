#ifndef MHZ19_STM32_H
#define MHZ19_STM32_H

#if defined(ARDUINO_ARCH_STM32)

#include "MHZ19.h"
#include <Arduino.h>

/**
 * @brief Concrete implementation of MHZ19_Platform_Interface specifically for STM32
 */
class MHZ19_STM32_Platform : public MHZ19_Platform_Interface {
    private:
        HardwareSerial* _serial;
        uint32_t _timeout_ms;

    public:
        explicit MHZ19_STM32_Platform(HardwareSerial* serial, uint32_t timeout_ms = 1000)
            : _serial(serial), _timeout_ms(timeout_ms) {}

        bool uart_transmit(const uint8_t* data, uint16_t length) override {
            if (!_serial) return false;
            while (_serial->available()) _serial->read(); 
            return (_serial->write(data, length) == length);
        }

        bool uart_receive(uint8_t* data, uint16_t length, uint32_t timeout_ms = 1000) override {
            if (!_serial) return false;
            uint32_t timeout = (timeout_ms > 0) ? timeout_ms : _timeout_ms;
            uint32_t start_time = millis();
            uint16_t idx = 0;

            while ((idx < length) && (millis() - start_time < timeout)) {
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
            delay(ms);
        }
};

/**
 * @brief PWM Implementation for STM32 (Arduino API-based attachInterrupt)
 */
class MHZ19_PWM_STM32_Platform : public MHZ19_PWM_Platform_Interface {
    private:
        uint8_t _pin = 255;
        volatile uint32_t _lastEdgeMicros = 0;
        volatile uint32_t _capturedHigh_us = 0;
        volatile uint32_t _capturedLow_us = 0;
        volatile bool _cycleReady = false;
        
        static MHZ19_PWM_STM32_Platform*& instancePtr() {
            static MHZ19_PWM_STM32_Platform* ptr = nullptr;
            return ptr;
        }
        
        static void isrHandler() {
            MHZ19_PWM_STM32_Platform* self = instancePtr();
            if (!self) return;
                    
            uint32_t now = micros();
            uint32_t duration = now - self->_lastEdgeMicros;
            self->_lastEdgeMicros = now;
                    
            if (digitalRead(self->_pin) == HIGH) {
                self->_capturedLow_us = duration;
                self->_cycleReady = true;
            } else {
                self->_capturedHigh_us = duration;
            }
        }
        
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
            
            while (!_cycleReady) {
                if (millis() - start > timeout_ms) return false;
                yield();
            }
            
            noInterrupts();
            *high_us = _capturedHigh_us;
            *low_us = _capturedLow_us;
            _cycleReady = false;
            interrupts();
            
            return true;
        }
        
        void end_pwm_capture() override {
            detachInterrupt(digitalPinToInterrupt(_pin));
            instancePtr() = nullptr;
        }
};

#endif // ARDUINO_ARCH_STM32

#endif // MHZ19_STM32_H
