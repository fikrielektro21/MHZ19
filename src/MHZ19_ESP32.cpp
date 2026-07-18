#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

#include "MHZ19_ESP32.h"

MHZ19_PWM_ESP32_Platform*& MHZ19_PWM_ESP32_Platform::instancePtr() {
    static MHZ19_PWM_ESP32_Platform* ptr = nullptr;
		return ptr;
    }
		
void IRAM_ATTR MHZ19_PWM_ESP32_Platform::isrHandler() {
	MHZ19_PWM_ESP32_Platform* self = instancePtr();
	if (!self) return;
			
	uint32_t now = micros();
	uint32_t duration = now - self->_lastEdgeMicros;
	self->_lastEdgeMicros = now;
			
	//if (duration > 110000) return;
			
	if (digitalRead(self->_pin) == HIGH) {
		self->_capturedLow_us = duration;
		self->_cycleReady = true;
	} else {
	}
}

#endif // ESP32 || ARDUINO_ARCH_ESP32