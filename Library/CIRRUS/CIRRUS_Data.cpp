#include "CIRRUS.h"

#ifdef CIRRUS_USE_TASK
#include "Tasks_utils.h"
#endif

// ********************************************************************************
// Functions prototype
// ********************************************************************************

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(String mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

#define DeleteAndNull(p)	\
	{ if ((p) != NULL) \
		{ delete (p);	\
			(p) = NULL; }}

// ********************************************************************************
//
// ********************************************************************************

// ********************************************************************************
// Cirrus CS5490 initialization
// ********************************************************************************
CIRRUS_CS5490::~CIRRUS_CS5490()
{
	DeleteAndNull(RMSData);
}

void CIRRUS_CS5490::Initialize()
{
	RMSData = new CIRRUS_RMSData(this);
}

/**
 * Get RMS Data and temperature
 */
void CIRRUS_CS5490::GetData(void)
{
	RMSData->GetData();
}

/**
 * Return U RMS
 */
float CIRRUS_CS5490::GetURMS(void)
{
	return RMSData->GetURMS();
}

/**
 * Return P RMS
 */
float CIRRUS_CS5490::GetPRMSSigned(void)
{
	return RMSData->GetPRMSSigned();
}

/**
 * Return temperature
 */
float CIRRUS_CS5490::GetTemperature(void)
{
	return RMSData->GetTemperature();
}

/**
 * Return power factor (cosphi)
 */
float CIRRUS_CS5490::GetPowerFactor(void)
{
	return RMSData->GetPowerFactor();
}

/**
 * Return frequency
 */
float CIRRUS_CS5490::GetFrequency(void)
{
	return RMSData->GetFrequency();
}

/**
 * Return energies of the day
 */
void CIRRUS_CS5490::GetEnergy(float *conso, float *surplus)
{
	*conso = RMSData->GetEnergyConso();
	*surplus = RMSData->GetEnergySurplus();
}

/**
 * Return error count
 */
uint32_t CIRRUS_CS5490::GetErrorCount(void)
{
	return RMSData->GetErrorCount();
}

// ********************************************************************************
// Cirrus CS548x initialization
// ********************************************************************************
CIRRUS_CS548x::~CIRRUS_CS548x()
{
	DeleteAndNull(RMSData_ch1);
	DeleteAndNull(RMSData_ch2);
}

void CIRRUS_CS548x::Initialize()
{
	if (isCS5484)
	{
		config0_default = 0x400000;
		RMSData_ch1 = new CIRRUS_RMSData(this, false);
		RMSData_ch2 = new CIRRUS_RMSData(this, false);
	}
	else
	{
		config0_default = 0xC02000;
		RMSData_ch1 = new CIRRUS_RMSData(this);
		RMSData_ch2 = new CIRRUS_RMSData(this);
	}
}

/**
 * Get RMS Data and temperature of selected channel(s)
 */
bool CIRRUS_CS548x::GetData(CIRRUS_Channel channel)
{
	bool log = false;

	if ((channel == Channel_1) || (channel == Channel_all))
	{
		SelectChannel(Channel_1);
		log = RMSData_ch1->GetData();
	}
	else
		log = true;
	if ((channel == Channel_2) || (channel == Channel_all))
	{
		SelectChannel(Channel_2);
		log = log && RMSData_ch2->GetData();
	}
	return log;
}

/**
 * Return U RMS of the selected channel
 */
float CIRRUS_CS548x::GetURMS(CIRRUS_Channel channel)
{
	if (channel == Channel_1)
		return RMSData_ch1->GetURMS();
	else
		if (channel == Channel_2)
			return RMSData_ch2->GetURMS();
		else
			return 0;
}

/**
 * Return P RMS of the selected channel
 */
float CIRRUS_CS548x::GetPRMSSigned(CIRRUS_Channel channel)
{
	if (channel == Channel_1)
		return RMSData_ch1->GetPRMSSigned();
	else
		if (channel == Channel_2)
			return RMSData_ch2->GetPRMSSigned();
		else
			return 0;
}

/**
 * Return temperature
 */
float CIRRUS_CS548x::GetTemperature(void)
{
	return RMSData_ch1->GetTemperature();
}

/**
 * Return power factor (cosphi) of the selected channel
 */
float CIRRUS_CS548x::GetPowerFactor(CIRRUS_Channel channel)
{
	if (channel == Channel_1)
		return RMSData_ch1->GetPowerFactor();
	else
		if (channel == Channel_2)
			return RMSData_ch2->GetPowerFactor();
		else
			return 0;
}

/**
 * Return frequency
 */
float CIRRUS_CS548x::GetFrequency(void)
{
	return RMSData_ch1->GetFrequency();
}

/**
 * Return energies of the day
 */
void CIRRUS_CS548x::GetEnergy(float *conso, float *surplus, CIRRUS_Channel channel)
{
	if (channel == Channel_1)
	{
		*conso = RMSData_ch1->GetEnergyConso();
		*surplus = RMSData_ch1->GetEnergySurplus();
	}
	else
		if (channel == Channel_2)
		{
			*conso = RMSData_ch2->GetEnergyConso();
			*surplus = RMSData_ch2->GetEnergySurplus();
		}
		else
		{
			*conso = 0;
			*surplus = 0;
		}
}

RMS_Data CIRRUS_CS548x::GetLog(CIRRUS_Channel channel)
{
	return RMSData_ch1->GetLog();
}

/**
 * Return error count
 */
uint32_t CIRRUS_CS548x::GetErrorCount(void)
{
	return RMSData_ch1->GetErrorCount() + RMSData_ch2->GetErrorCount();
}

// ********************************************************************************
// CIRRUS_RMSData class : GetData
// ********************************************************************************
bool CIRRUS_RMSData::GetData(void)
{
	if (Cirrus->wait_for_ready(true))
	{
		if (_cumul_count == 0)
			_start = millis();
		if (_log_count == 0)
			_startLog = millis();

		Cirrus->get_data(CIRRUS_RMS_Voltage, &_data.Voltage);
		Cirrus->get_data(CIRRUS_RMS_Current, &_data.Current);
		Cirrus->get_data(CIRRUS_Active_Power, &_data.Power);
		_cumul += _data;
		if (_temperature)
		{
			_tempData = Cirrus->get_temperature();
			_tempCumul += _temperature;
		}

		// Gestion énergie, calcul sur la durée de la moyenne
		_cumul_count++;
		if (_cumul_count == _cumul_MAX)
		{
			_cumul /= _cumul_MAX;
			_data.Energy = _cumul.Power * ((millis() - _start) / 1000) / 3600;
			if (_data.Energy > 0.0)
				energy_day_conso += _data.Energy;
			else
				energy_day_surplus += fabs(_data.Energy);

			_log += _cumul;
			_log_count++;
			_cumul.Zero();

			if (_temperature)
			{
				_tempCumul /= _cumul_MAX;
				_tempLog += _tempCumul;
				_tempCumul = 0;
			}

			// Extra
			Cirrus->get_data(CIRRUS_Power_Factor, &Power_Factor);
			Cirrus->get_data(CIRRUS_Frequency, &Frequency);

			_cumul_count = 0;
		}

		// Gestion log graphe
		if (millis() - _startLog >= 120000) // 2 minutes
		{
			_logcumul = _log / _log_count;
			_log.Zero();
			_log_count = 0;
			_logAvailable = true;
		}

	}
	else
		_error_count++;

	return _logAvailable;
}

// ********************************************************************************
// End of file
// ********************************************************************************
