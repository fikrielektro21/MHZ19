#ifndef MHZ19_H
#define MHZ19_H

#include <cstdint>
#include <stdbool.h>





// ================================================================
// ============ UART: PLATFORM INTERFACE & CORE LOGIC =============
// ================================================================


/**
* @brief Error codes untuk MH-Z19B sensor
*/
enum class MHZ19_Error : uint8_t {
	SUCCESS = 0,
	ERR_PLATFORM_NULL,
	ERR_TRANSMIT_FAILED,
	ERR_RECEIVE_TIMEOUT,
	ERR_INVALID_HEADER,
	ERR_CHECKSUM_MISMATCH,
	ERR_INVALID_RESPONSE,
	ERR_SPAN_TOO_LOW, 			// span < 1000
	ERR_NOT_WARMED_UP, 			// sensor not yet warmed up
	ERR_ABC_NOT_DISABLED, 	// ABC remains active for manual calibration
	ERR_HD_PIN_INVALID, 		// Invalid HD PIN
	ERR_UNKNOWN,
};
		/**
		*  @brief Platform interface for UART hardware abstraction.
		*  If using an STM32 or ESP32, you must subclass this class.
		* And implementing it in the hardware's transmit and receive functions.
		*/

class MHZ19_Platform_Interface {
	public:
		virtual ~MHZ19_Platform_Interface() {}
		/**
     * @brief Virtual function to send data via UART
     * @param data Pointer to the data buffer to be sent
     * @param length Number of bytes to be sent (for this sensor, always 9 bytes)
     * @return true if successful, false if failed/timeout
     */
	 	virtual bool uart_transmit(const uint8_t* data, uint16_t length) = 0;
	 /**
     * @brief Virtual function to receive data via UART
     * @param data Pointer to the buffer for storing incoming data
     * @param length Number of bytes expected (for this sensor, always 9 bytes)
     * @return true if successful, false if failed/timeout
     */
		virtual bool uart_receive(uint8_t* data, uint16_t length, uint32_t timeout_ms = 1000) = 0;

	 /**
    	* @brief A virtual function to introduce a delay in milliseconds (ms).
	 	* @param ms Number of milliseconds for the delay
	 	* @note This function is important to ensure the sensor has enough time to process commands and send data
	 	* @note The implementation of delay_ms must use a method appropriate for the platform (e.g., HAL_Delay() for STM32, delay() for Arduino/ESP32)	
  		* @return void
		*/
		virtual void delay_ms(uint32_t ms) = 0; 
		virtual uint32_t get_millis() = 0; // timer

		/**
		* @brief Set the pin mode to OUTPUT.
		*/
		virtual bool set_pin_mode_output(uint8_t pin) = 0;

		/**
		* @brief Pull the pin LOW.
		*/
		virtual bool set_pin_low(uint8_t pin) = 0;

		/** 
		* @brief Pull the pin high.
		*/
		virtual bool set_pin_high(uint8_t pin) = 0;
};

	/**
	 * @brief MH-Z19B/C Core Logic Main Driver
	 */
class MHZ19 {
	public:
		enum Command : uint8_t {
			CMD_READ_CO2				= 0x86,
			CMD_CALIBRATE_ZERO			= 0x87,
			CMD_CALIBRATE_SPAN			= 0x88,
			CMD_ABC_LOGIC				= 0x79,
			CMD_SET_RANGE				= 0X99,
		};
		// Range Deteksi Sensor [cite: 192]
		enum Range : uint16_t {
			RANGE_2000PPM = 2000, 
			RANGE_5000PPM = 5000, 
		};
		
		/**
		 * @brief MHZ19 Driver Constructor
		 * @param platform Pointer to the platform interface implementation (STM32 / ESP32)
		 */
		 // Konstruktor
		explicit MHZ19(MHZ19_Platform_Interface* platform);
		 
		 bool begin(uint32_t warmup_time_ms = 180000); // default 3 menit

		 /**
		 * @brief Request the latest CO2 data from the sensor
		 * @param co2_out Pointer to store the reading in PPM units
		 * @param temp_out Pointer to store the internal temperature (Celsius), optional
		 * @return true if the reading and checksum are valid, false otherwise
		 */
		 
		 bool readCO2(uint16_t* co2_out, int16_t* temp_out = nullptr);
		 
		 /**
		 * @brief Enable or disable ABC (Automatic Baseline Correction)
		 * @param enable true for ON (0xA0), false for OFF (0x00)
		 * @return true if the command was successfully sent
		 */
		 bool setABCFunction(bool enable);
		 
		 /**
		 * @brief Setting the maximum sensor reading limit (2000 or 5000 ppm)
		 * @param range Reading range limit
		 * @return true if the command was successfully sent
		 */

		 bool isABCEnabled(); // cek status ABC
		 
		 bool setDetectionRange(Range range);
		 
		 /**
		 * @brief Run zero-point calibration (400 PPM if outdoors, 600-1000 PPM otherwise) adjust RangeDetection
		 * @note Ensure the sensor is in a 400ppm stable environment for over 20 minutes
		 */
		 bool calibrateZeroPoint();

		 /**
		 * @brief Perform zero-point calibration (400 ppm)
		 * @note Ensure the sensor is in a 400ppm stable environment for over 20 minutes
		 */
		 bool calibrateSpanPoint(uint16_t span_ppm);

		 bool calibrateFull(uint16_t span_ppm = 2000); // zero + span sequence

		 bool calibrateZeroByPin(uint8_t hd_pin); // manual calibration via pin HD

		 MHZ19_Error getLastError() const { return _lastError; }

		 const char* getErrorString(MHZ19_Error error);

		 bool isWarmedUp() const;

		 uint32_t getUptime() const;
		 
		 void resetError();

		// ============ RETRY MECHANISM ============
		 bool readCO2WithRetry(uint16_t* co2_out, int16_t* temp_out = nullptr, uint8_t retries = 3);
		// ============ UTILITY ============
		bool softReset(); // Reset sensor via UART (if supported)

	private:
		MHZ19_Platform_Interface* _platform;

		MHZ19_Error _lastError;
		uint32_t _startTime_ms;
		uint32_t _warmupTime_ms;
		bool _abcState; // cache status ABC
		
		static uint8_t calculatedChecksum(const uint8_t* packet, uint8_t len);
		
		void buildRequestPacket(uint8_t* packet, Command cmd, uint8_t b3 = 0x00, uint8_t b4 = 0x00);

		//bool validateResponse(const uint8_t* response, Command expected_cmd);
		void setError(MHZ19_Error error);
};



// ================================================================
// ============ PWM: PLATFORM INTERFACE & CORE LOGIC =============
// ================================================================

/**
 * @brief Interface for hardware abstraction of MH-Z19B PWM signal reading.
 * The implementation must automatically select the best available method for the PIN.
 * Provided: hardware timer capture (if supported) or attachInterrupt.
 * as a fallback if not.
 */

class MHZ19_PWM_Platform_Interface {
	public:
		virtual ~MHZ19_PWM_Platform_Interface() {}
		/**
		* @brief Prepare pin for capturing PWM signal.
		* @param pin GPIO pin number connected to the sensor's PWM output
		* @return true if successful
		*/
		virtual bool begin_pwm_capture(uint8_t pin) = 0;

		/**
		* @brief MengamHere are the complete measurement results for the latest single PWM cycle.
		* @param high_us [output] HIGH duration in microseconds (TH)
		* @param los_us [output] LOW duration in microseconds (TL)
		* @param timeout_ms Waiting period if the new cycle has not yet been completed.
		* @return true if a full cycle is successfully obtained, false if a timeout occurs.
		*/
		virtual bool read_pwm(uint32_t* high_us, uint32_t* low_us, uint32_t timeout_ms) = 0;

		/**
		* @brief Release the resource capture (interrupt/timer) from this pin.
		*/
		virtual void end_pwm_capture() = 0;
};

/**
* @brief Core driver for reading CO2 from the MH-Z19C/B/D sensor via PWM output.
* @note Different from MHZ19 class (UART) - no request/response/checksum,
* 			The sensor sends a continuous signal; we simply listen.
*/
class MHZ19_PWM {
	public:
		/**
		* @param platform Implementation of the PWM platform (ESP32/Arduino/STM32)
		* @param range Detection range of the sensor, which must match
		* 							the setting on the UART interface, or default to 2000 ppm from the factory
		*/
		explicit MHZ19_PWM(MHZ19_PWM_Platform_Interface* platform, MHZ19::Range range = MHZ19::RANGE_2000PPM);

		bool begin(uint8_t pin);

		/**
		* @brief Reading the CO2 concentration from one full PWM cycle.
		* @note Must wait for 1 cycle (`1004ms), so this call can block until timeout_ms before returning.
		*/
		bool readCO2(uint16_t* co2_out, uint32_t timeout_ms = 1100);

		void setRange(MHZ19::Range range);

	private:
		MHZ19_PWM_Platform_Interface* _platform;
		MHZ19::Range _range;
};

/**
 * @brief A helper to synchronize the detection range across both UART and PWM channels simultaneously.
 *
 * MHZ19 (UART) and MHZ19_PWM are not aware of each other by design,
 * so changing the range in one does NOT automatically change the other — if
 * forgot to update one of them, MHZ19_PWM::readCO2() will calculate ppm with
 * An incorrect range assumption without triggering any errors (a value is still produced, but it is wrong).
 * Use this function so that both change simultaneously.
 *
 * @param uartSensor Reference to the MHZ19 object (UART) already connected to the sensor.
 * @param pwmSensor  Reference to the MHZ19_PWM object that reads the SAME sensor.
 * @param range      The new range to be implemented
 * @return true if the UART command is successfully sent to the sensor
 *         (the update on the PWM side never fails, it's just a local variable update)
 */
bool setDetectionRangeBoth(MHZ19& uartSensor, MHZ19_PWM& pwmSensor, MHZ19::Range range);


#endif // MHZ19_H
	 