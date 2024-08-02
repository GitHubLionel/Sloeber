#include "DS18B20.h"

//#define DALLAS_DEBUG

#ifdef DS18B20_USE_TASK
#include "Tasks_utils.h"
#endif

// External function for debug message
extern void print_debug(String mess, bool ln = true);
extern void print_debug(int val, bool ln = true);

// ********************************************************************************
// Définition DS18B20
// ********************************************************************************

// ********************************************************************************
// Functions prototype
// ********************************************************************************

/**
 * oneWireBus : GPIO where the DS18B20 is connected to
 */
DS18B20::DS18B20(uint8_t oneWireBus)
{
	for (int i = 0; i < DS18B20_MAX; i++)
	{
		DS_Temp[i] = 0.0;
	}
	OWbus = oneWireBus;
}

DS18B20::~DS18B20()
{
	if (sensors)
	{
		delete sensors;
		delete oneWire;
		sensors = NULL;
		oneWire = NULL;
	}
}

/**
 * Initialize all the DS18B20 sensors
 * Precision : 9 (lower) to 12 (higher) resolution
 * Conversion is in none blocking mode
 * Return the number of sensor found
 */
uint8_t DS18B20::Initialize(uint8_t precision)
{
	// Setup a oneWire instance to communicate with any OneWire devices
	oneWire = new OneWire(OWbus);

	// Pass our oneWire reference to Dallas Temperature sensor
	sensors = new DallasTemperature(oneWire);

	// Don't block the program while the temperature sensor is reading
	// DS18B20 max conversion time is 750 ms
	sensors->setWaitForConversion(false);

	// Start the DS18B20 sensor
	sensors->begin();

	// Device found ?
	DS_Found = sensors->getDeviceCount();
	print_debug("DS found : ", false);
	print_debug(DS_Found, true);
	if (DS_Found > 0)
	{
		if (!sensors->getAddress(deviceAddress, 0))
			DS_Found = 0;
		else
		{
			sensors->setResolution(deviceAddress, precision);
			for (int i = 0; i < DS_Found; i++)
			{
				print_debug("DS : ", false);
				print_debug(deviceAddress[i], true);
			}
			// Demande de températures
			sensors->requestTemperatures();
		}
	}
	return DS_Found;
}

// ********************************************************************************
// DS18B20 measures
// ********************************************************************************

/**
 * Check if conversion is ended.
 * Don't call this function under 750 ms elapsed
 */
void DS18B20::check_dallas(void)
{
	float temp;

	if (DS_Found > 0)
	{
		yield();  // Background operation before
		if (sensors->isConversionComplete())
		{
			for (int i = 0; i < DS_Found; i++)
			{
				temp = sensors->getTempCByIndex(i);
				if (temp != -127.0)
				{
					DS_Temp[i] = temp;
#ifdef DALLAS_DEBUG
	  print_debug("DS " + String(i) + " : ", false);
	  print_debug(String(temp), true);
#endif
				}
			}

			// Relance la demande de températures
			sensors->requestTemperatures();
		}
		yield();  // Background operation after
	}
}

float DS18B20::get_Temperature(uint8_t id)
{
	if (id < DS_Found)
		return DS_Temp[id];
	else
		return 0.0;
}

String DS18B20::get_Temperature_Str(uint8_t id)
{
	return String(get_Temperature(id));
}

// ********************************************************************************
// Basic Task function to check dallas DS18B20
// ********************************************************************************
/**
 * A basic Task to check dallas DS18B20
 */
#ifdef DS18B20_USE_TASK

/**
 * We assume that DS18B20 instance is called DS
 */
extern DS18B20 DS;

void DS18B20_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("DS18B20_Task");
	for (EVER)
	{
		DS.check_dallas();
		END_TASK_CODE();
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
