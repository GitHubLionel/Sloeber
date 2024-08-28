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
// CIRRUS_RMSData class : GetData
// ********************************************************************************
bool CIRRUS_RMSData::GetData(unsigned long reftime, bool reset_ready)
{
#define _cumul_MAX 5

	if (_inst_count == 0)
		_start = reftime;
	if (_log_count == 0)
		_startLog = reftime;

	if (Cirrus->wait_for_ready(reset_ready))
	{
		Cirrus->get_data(CIRRUS_RMS_Voltage, &_inst_data.Voltage);
#ifdef CIRRUS_RMS_FULL
		Cirrus->get_data(CIRRUS_RMS_Current, &_inst_data.Current);
#endif
		Cirrus->get_data(CIRRUS_Active_Power, &_inst_data.Power);
		_inst_data_cumul += _inst_data;
		if (_temperature)
		{
			_inst_temp = Cirrus->get_temperature();
			_inst_temp_Cumul += _inst_temp;
		}

		// Gestion énergie, calcul sur la durée de la moyenne
		_inst_count++;
		if (_inst_count == _cumul_MAX)
		{
			_inst_data_cumul /= _cumul_MAX;
			_inst_data.Energy = _inst_data_cumul.Power * ((reftime - _start) / 1000.0) / 3600.0;
			if (_inst_data.Energy > 0.0)
				energy_day_conso += _inst_data.Energy;
			else
				energy_day_surplus += fabs(_inst_data.Energy);

			_log_cumul_data += _inst_data_cumul;
			_log_count++;
			_inst_data_cumul.Zero();

			if (_temperature)
			{
				_log_cumul_temp += (_inst_temp_Cumul / _cumul_MAX);
				_inst_temp_Cumul = 0;
			}

			// Extra, no mean
			if (_WantPower_Factor)
				Cirrus->get_data(CIRRUS_Power_Factor, &Power_Factor);
			if (_WantFrequency)
				Cirrus->get_data(CIRRUS_Frequency, &Frequency);

			_inst_count = 0;
		}

		// Gestion log graphe
		if (reftime - _startLog >= 120000) // 2 minutes
		{
			_log_data = _log_cumul_data / _log_count;
			_log_cumul_data.Zero();
			if (_temperature)
			{
				_log_temp = _log_cumul_temp / _log_count;
				_log_cumul_temp = 0;
			}
			_log_count = 0;
			_logAvailable = true;
		}
	}
	else
		_error_count++;

	return _logAvailable;
}

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
 * Main function that get the RMS data and others from Cirrus
 * Get RMS Data and temperature
 */
bool CIRRUS_CS5490::GetData(void)
{
	unsigned long reftime = millis();
	return RMSData->GetData(reftime);
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
		RMSData_ch2 = new CIRRUS_RMSData(this, false);
	}
}

void CIRRUS_CS548x::RestartEnergy(void)
{
	RMSData_ch1->RestartEnergy();
	RMSData_ch2->RestartEnergy();
}

/**
 * Main function that get the RMS data and others from Cirrus
 * Get RMS Data, temperature and others of selected channel(s)
 */
bool CIRRUS_CS548x::GetData(CIRRUS_Channel channel)
{
	bool log = false;
	unsigned long reftime = millis();

	if (channel == Channel_all)
	{
		SelectChannel(Channel_1);
		log = RMSData_ch1->GetData(reftime, false);
		SelectChannel(Channel_2);
		log &= RMSData_ch2->GetData(reftime);
	}
	else
		if (channel == Channel_1)
		{
			SelectChannel(Channel_1);
			log = RMSData_ch1->GetData(reftime);
		}
		else
			if (channel == Channel_2)
			{
				SelectChannel(Channel_2);
				log = RMSData_ch2->GetData(reftime);
			}
	return log;
}

/**
 * Return U RMS of the selected channel
 */
float CIRRUS_CS548x::GetURMS(CIRRUS_Channel channel) const
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
 * Return I RMS of the selected channel
 */
float CIRRUS_CS548x::GetIRMS(CIRRUS_Channel channel) const
{
#ifdef CIRRUS_RMS_FULL
	if (channel == Channel_1)
		return RMSData_ch1->GetIRMS();
	else
		if (channel == Channel_2)
			return RMSData_ch2->GetIRMS();
		else
#endif
			return 0;
}


/**
 * Return P RMS of the selected channel
 */
float CIRRUS_CS548x::GetPRMSSigned(CIRRUS_Channel channel) const
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
float CIRRUS_CS548x::GetTemperature(void) const
{
	return RMSData_ch1->GetTemperature();
}

/**
 * Return power factor (cosphi) of the selected channel
 */
float CIRRUS_CS548x::GetPowerFactor(CIRRUS_Channel channel) const
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
float CIRRUS_CS548x::GetFrequency(void) const
{
	return RMSData_ch1->GetFrequency();
}

/**
 * Return energies of the day. Select one channel.
 */
void CIRRUS_CS548x::GetEnergy(float *conso, float *surplus, CIRRUS_Channel channel)
{
	if (channel == Channel_1)
	{
		*conso = RMSData_ch1->GetEnergyConso();
		if (surplus != NULL)
			*surplus = RMSData_ch1->GetEnergySurplus();
	}
	else
		if (channel == Channel_2)
		{
			*conso = RMSData_ch2->GetEnergyConso();
			if (surplus != NULL)
				*surplus = RMSData_ch2->GetEnergySurplus();
		}
		else
		{
			*conso = 0;
			if (surplus != NULL)
				*surplus = 0;
		}
}

/**
 * Get log data. Reset the flag log available.
 */
RMS_Data CIRRUS_CS548x::GetLog(CIRRUS_Channel channel, double *temp)
{
	if (channel == Channel_1)
		return RMSData_ch1->GetLog(temp);
	else
		if (channel == Channel_2)
			return RMSData_ch2->GetLog(temp);
		else
			return RMS_Data();
}

/**
 * Return error count
 */
uint32_t CIRRUS_CS548x::GetErrorCount(void) const
{
	return RMSData_ch1->GetErrorCount() + RMSData_ch2->GetErrorCount();
}

// ********************************************************************************
// Basic Task function to get data from Cirrus
// ********************************************************************************
/**
 * A basic Task to get data from Cirrus
 */
#ifdef CIRRUS_USE_TASK

/**
 * This functions should be redefined elsewhere with your analyse
 */
void __attribute__((weak)) Get_Data(void)
{
	static bool Data_acquisition = false;

	// Prevent reentrant acquisition
	if (Data_acquisition)
		return;

	Data_acquisition = true;

	// Do get data you want
	// log = CS5490.GetData();
	// Power = CS5490.GetPRMSSigned();
	// ....

	Data_acquisition = false;
}

void CIRRUS_Task_code(void *parameter)
{
	BEGIN_TASK_CODE_UNTIL("CIRRUS_Task");
	for (EVER)
	{
		Get_Data();
		END_TASK_CODE_UNTIL();
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
