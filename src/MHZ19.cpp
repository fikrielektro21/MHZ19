#include "MHZ19.h"

MHZ19::MHZ19(MHZ19_Platform_Interface* platform) 
	: _platform(platform),
		_lastError(MHZ19_Error::SUCCESS),
		_startTime_ms(0),
		_warmupTime_ms(0),
		_abcState(true) // default dari pabrik ON
{
	if (!_platform) {
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
	}
}

// ====== initialization =============

bool MHZ19::begin(uint32_t warmup_time_ms) {
	if (!_platform) {
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false;
	}

	_warmupTime_ms = warmup_time_ms;
	_startTime_ms = _platform->get_millis();
	_lastError = MHZ19_Error::SUCCESS;

	// Check if the sensor responds with an initial reading.
	uint16_t co2;
	if (readCO2(&co2)) {
		return true;
	}

// If it fails, it might still be warming up; return true for initialization anyway.
// Errors will be handled in readCO2() by checking isWarmedUp().
// Clear the last error.
	resetError();
	return true;
}


uint8_t MHZ19::calculatedChecksum(const uint8_t* packet, uint8_t len) {
	uint8_t checksum = 0;
	// Accumulate Byte 1 through Byte 2 based on the datasheet instructions
	for (uint8_t i = 1; i < len - 1; i++) {
		checksum += packet[i]; // [cite: 206, 208]
	}
	return (uint8_t)(0 - checksum); // Two's complement negative (equivalent to 0x100 - sum)
}

void MHZ19::buildRequestPacket(uint8_t* packet, Command cmd, uint8_t b3, uint8_t b4) {
	packet[0] = 0xFF; 		  	// Byte 0: Header start byte
	packet[1] = 0x01; 		  	// Byte 1: ID sensor
	packet[2] = static_cast<uint8_t>(cmd); 	// Byte 2: Command 
	packet[3] = b3; 			// Additional Argument High Byte / Configuration 
	packet[5] = 0x00; 			// Reserved 
	packet[6] = 0x00; 			// Reserved 
	packet[7] = 0x00; 			// Reserved
	packet[8] = calculatedChecksum(packet, 9);  // Calculate & lock checksum to Byte 8
}

bool MHZ19::readCO2(uint16_t* co2_out, int16_t* temp_out) {
	if (!_platform  || !co2_out) {
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false; // Check if the pointer is valid
	}
	uint8_t request[9];
	uint8_t response[9] = {0}; // 9-byte buffer for sensor clone compatibility

	// build request packet for command 0x86
	buildRequestPacket(request, CMD_READ_CO2);

	// transmit 9-byte data packet to sensor via platform interface
	if (!_platform->uart_transmit(request, 9)) {
		setError(MHZ19_Error::ERR_TRANSMIT_FAILED);
		return false; 
	} // if failed to transmit, return false
	
	// Receive response packet data 9-byte from sensor
	if (!_platform->uart_receive(response, 9)) { 
		setError(MHZ19_Error::ERR_RECEIVE_TIMEOUT);
		return false; 
	}

	 // Validasi Header & Echo Command
	if (response[0] != 0xFF || response[1] != static_cast<uint8_t>(Command::CMD_READ_CO2)) {
		setError(MHZ19_Error::ERR_INVALID_HEADER);
		return false; //Check the header and sensor ID for any errors
	}

	// Validate Checksum (byte 1-7 sum, byte 8 must match)
	if (response[8] != calculatedChecksum(response, 9)) {
		setError(MHZ19_Error::ERR_CHECKSUM_MISMATCH);
		return false; // Check the checksum for any errors
	}

	// Data extraction using the datasheet formula: HIGH * 256 + LOW
	*co2_out = (static_cast<uint16_t>(response[2]) << 8) | response[3];

	if (temp_out !=  nullptr) {
		*temp_out = static_cast<int16_t>(response[4]) - 40;
	}
	setError(MHZ19_Error::SUCCESS);

	return true; // Successfully read CO2 data
}

bool MHZ19::setABCFunction(bool enable) {
	if (!_platform ) { 
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false; 
	}
	
	uint8_t request[9];
	
	// byte 3 is 0x00 if OFF
	uint8_t b3 = enable ? 0xA0 : 0x00; 

	buildRequestPacket(request, Command::CMD_ABC_LOGIC, enable ? 0xA0 : 0x00);

	if (!_platform->uart_transmit(request, 9)) {
		setError(MHZ19_Error::ERR_TRANSMIT_FAILED);
		return false;
	}

	_platform->delay_ms(100);
	_abcState = enable; // cache status
	_lastError = MHZ19_Error::SUCCESS;
	return true; 
}


bool MHZ19::setDetectionRange(Range range) {
	if (!_platform) {
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false;
	}

	uint8_t request[9]; 

	uint16_t val = static_cast<uint16_t>(range);

	buildRequestPacket(request, Command::CMD_SET_RANGE, 
                       static_cast<uint8_t>(val >> 8), 
                       static_cast<uint8_t>(val & 0xFF));
	
	if (!_platform->uart_transmit(request, 9)) {
		setError(MHZ19_Error::ERR_TRANSMIT_FAILED);
		return false;
	}
	
	_platform->delay_ms(100);
	_lastError = MHZ19_Error::SUCCESS;

	return true;
}

/**
 * @brief Performs zero-point calibration (the sensor treats the CURRENT AIR condition as 400 ppm).
 * @warning This function ONLY verifies the electronic warm-up period (~3 minutes) via isWarmedUp().
 *          It CANNOT verify whether the environment is actually at 400 ppm —
 *          that is the user's responsibility. You MUST place the sensor outdoors or in an area
 *          with excellent ventilation for >20 minutes BEFORE calling this function.
 *          Calibrating in an enclosed space (e.g., a bedroom or air-conditioned office)
 *          will cause all subsequent readings to be permanently biased downwards.
 */
bool MHZ19::calibrateZeroPoint(){
	if (!_platform) { 
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false; 
	}
	// ABC check must be OFF for manual calibration

	if(_abcState) {
		setError(MHZ19_Error::ERR_ABC_NOT_DISABLED);
		return false;
	}

	// cek warm up
	if (!isWarmedUp()) {
		setError(MHZ19_Error::ERR_NOT_WARMED_UP);
		return false;
	}

	uint8_t request[9];

	buildRequestPacket(request, Command::CMD_CALIBRATE_ZERO);

	if (!_platform->uart_transmit(request, 9)) {
		setError(MHZ19_Error::ERR_TRANSMIT_FAILED);
		return false;
	}

	_platform->delay_ms(100);
	_lastError = MHZ19_Error::SUCCESS;

	return true;
}

bool MHZ19::calibrateSpanPoint(uint16_t span_ppm) {
    if (!_platform) { 
			setError(MHZ19_Error::ERR_PLATFORM_NULL);
			return false; 
		}
		// span validation: minimum 1000 ppm, per datasheet
		if (span_ppm < 1000) {
			setError(MHZ19_Error::ERR_SPAN_TOO_LOW);
			return false;
		}

		// Check ABC must be OFF
		if (_abcState) {
			setError(MHZ19_Error::ERR_ABC_NOT_DISABLED);
			return false;
		}

		// Check warm-up 
		if (!isWarmedUp()){
			setError(MHZ19_Error::ERR_NOT_WARMED_UP);
			return false;
		}

    uint8_t request[9];
    buildRequestPacket(request, Command::CMD_CALIBRATE_SPAN, 
                       static_cast<uint8_t>(span_ppm >> 8), 
                       static_cast<uint8_t>(span_ppm & 0xFF));
    if (!_platform->uart_transmit(request, 9)) { 
			setError(MHZ19_Error::ERR_TRANSMIT_FAILED);
			return false;
		}
    _platform->delay_ms(100);
		_lastError = MHZ19_Error::SUCCESS;
    return true;
}

bool MHZ19::calibrateZeroByPin(uint8_t hd_pin) {
	if (!_platform) {
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false;
	}
	
	// cek warm up
	if (!isWarmedUp()) {
		setError(MHZ19_Error::ERR_NOT_WARMED_UP);
		return false;
	}
	// Use GPIO pin mode (this will be implemented in the platform layer)
	// For now, we assume the platform provides pin control functions
	// Or we can issue a warning that this feature requires additional implementation

	// Simulation: Pull HD pin low for >7 seconds
  	// Note: This requires implementation at the platform layer
	_platform->set_pin_mode_output(hd_pin);
	_platform->set_pin_low(hd_pin);
	_platform->delay_ms(8000);
	_platform->set_pin_high(hd_pin);
	_lastError = MHZ19_Error::SUCCESS;
	return true;
}

bool MHZ19::calibrateFull(uint16_t span_ppm) {
	if (!_platform) {
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false;
	}

	// Step 1: Disable ABC
	if (!setABCFunction(false)) {
		return false;
	}

	// Step 2: Zero calibration
	if (!calibrateZeroPoint()) {
		return false;
	}

	// Step 3: Span calibration
	if (!calibrateSpanPoint(span_ppm)) {
		return false;
	}
	_lastError = MHZ19_Error::SUCCESS;
	return true;
}


bool MHZ19::isABCEnabled() {
	return _abcState;
}

bool MHZ19::isWarmedUp() const {
	if(!_platform) return false;
	return (_platform->get_millis() - _startTime_ms) >= _warmupTime_ms;
}

uint32_t MHZ19::getUptime() const {
	if (!_platform) return 0;
	return _platform->get_millis() - _startTime_ms;
}

void MHZ19::resetError() {
	_lastError = MHZ19_Error::SUCCESS;
}

const char* MHZ19::getErrorString(MHZ19_Error error) {
	switch(error) {
		case MHZ19_Error::SUCCESS:								return "Succsess";
		case MHZ19_Error::ERR_PLATFORM_NULL: 			return "Platfrom interface is NULL";
		case MHZ19_Error::ERR_TRANSMIT_FAILED: 		return "UART transmit failed";
		case MHZ19_Error::ERR_RECEIVE_TIMEOUT: 		return "UART receive timeout";
		case MHZ19_Error::ERR_INVALID_HEADER: 		return "Invalid response header";
		case MHZ19_Error::ERR_CHECKSUM_MISMATCH: 	return "Checksum mismatch";
		case MHZ19_Error::ERR_INVALID_RESPONSE: 	return "Invalid response format";
		case MHZ19_Error::ERR_SPAN_TOO_LOW: 			return "Span calibration value";
		case MHZ19_Error::ERR_NOT_WARMED_UP: 			return "Sensor not Warmed Up yet";
		case MHZ19_Error::ERR_ABC_NOT_DISABLED: 	return "ABC must be disabled for manual calibration";
		case MHZ19_Error::ERR_HD_PIN_INVALID: 		return "Invalid HD pin";
		default: 																	return "Unknown error";
	}
}


bool MHZ19::readCO2WithRetry(uint16_t* co2_out, int16_t* temp_out, uint8_t retries) {
	if (!_platform) {
		setError(MHZ19_Error::ERR_PLATFORM_NULL);
		return false;
	}
	// soft reset via UART (bisa berbeda tiap firmware)
	for (uint8_t i = 0; i < retries; i++) {
		if (readCO2(co2_out, temp_out)) {
			return true;
		}
		_platform->delay_ms(50);
	}
	return false;
}

// ================================================================
// ============ PWM: CORE LOGIC IMPLEMENTATION ====================
// ================================================================

MHZ19_PWM::MHZ19_PWM(MHZ19_PWM_Platform_Interface* platform, MHZ19::Range range)
	: _platform(platform), _range(range) {}

bool MHZ19_PWM::begin(uint8_t pin) {
	if (!_platform) return false;
	return _platform->begin_pwm_capture(pin);
}

void MHZ19_PWM::setRange(MHZ19::Range range) {
	_range = range;
}

bool MHZ19_PWM::readCO2(uint16_t* co2_out, uint32_t timeout_ms) {
	if (!_platform) return false;
	
	uint32_t high_us = 0, low_us = 0;
	
	if (!_platform->read_pwm(&high_us, &low_us, timeout_ms)) {
		return false; // timeout atau sinyal tidak terdeteksi 
	}

	// Datasheet formula: Cppm = Range x (TH - 2ms) / (TH + TL - 4ms)
	// Calculated in microseconds (2ms=2000us, 4ms=4000us) for precision,
	// No floating-point arithmetic used.
	if (high_us <= 2000 || (high_us + low_us) <= 4000) {
		return false; // not yet stable
	}

	uint64_t numerator  = (uint64_t) (high_us - 2000) * static_cast<uint16_t>(_range);
	uint32_t denumerator = high_us + low_us - 4000;
	
	if (denumerator == 0 ) return false;
	*co2_out = static_cast<uint16_t>(numerator / denumerator);

	return true;
}



// ===========Privat Function ==========
void MHZ19::setError(MHZ19_Error error) {
	_lastError = error;
}

bool setDetectionRangaBoth(MHZ19& uartSensor, MHZ19_PWM& pwmSensor, MHZ19::Range range) {
	bool uart_ok = uartSensor.setDetectionRange(range);
	pwmSensor.setRange(range); // selalu jalankan, murni update varibale lokal
	return uart_ok;
}