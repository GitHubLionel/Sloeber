#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <OneWire.h>		// Include OneWire library
#include <DallasTemperature.h>	// Include Dallas library

/**
 * Class for DS18B20 sensor
 * Need to include OneWire and DallasTemperature to the project
 */

// To create a basic task to check DS18B20 temperature running every 2 s
#ifdef DS18B20_USE_TASK
#define DS18B20_DATA_TASK(start)	{(start), "DS18B20_Task", 1536, 5, 2000, CoreAny, DS18B20_Task_code}
void DS18B20_Task_code(void *parameter);
#else
#define DS18B20_DATA_TASK(start)	{}
#endif

// Max DS18B20 sensor allowed
#define DS18B20_MAX	5

class DS18B20
{
	private:
		// OneWire instance
		OneWire *oneWire = NULL;
		// Dallas Temperature sensors
		DallasTemperature *sensors = NULL;
		// arrays to hold device address
		DeviceAddress deviceAddress;

		uint8_t OWbus;
		float DS_Temp[DS18B20_MAX];
		uint8_t DS_Found = 0;

	public:
		DS18B20(uint8_t oneWireBus);
		~DS18B20();
		uint8_t Initialize(uint8_t precision);
		void check_dallas(void);

		float get_Temperature(uint8_t id);
		String get_Temperature_Str(uint8_t id);
};

