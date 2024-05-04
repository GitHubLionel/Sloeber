#pragma once

#include "Arduino.h"
#include <OneWire.h>		// Include OneWire library
#include <DallasTemperature.h>	// Include Dallas library

/**
 * Class for DS18B20 sensor
 * Need to include OneWire and DallasTemperature to the project
 */

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

