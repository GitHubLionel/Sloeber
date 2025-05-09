#include "Relay.h"
#include "Debug_utils.h"		  // Some utils functions for debug

#ifdef RELAY_USE_TASK
#include "Tasks_utils.h"
#if defined(USE_RTCLocal)
#include "RTCLocal.h"		      // A pseudo RTC software library
#endif
#endif

#define RELAY_DEBUG

// ********************************************************************************
// Relay_Class constructor
// ********************************************************************************

Relay_Class::Relay_Class()
{
	currentTime = -1;
	idTime = -1;
	isTimeInitialized = false;
	relaisOnCount = 0;
}

/**
 * Initialization of the class relay
 * gpios : values of gpio
 */
Relay_Class::Relay_Class(const RelayGPIOList &gpios) :
		Relay_Class()
{
	Initialize(gpios);
}

Relay_Class::~Relay_Class()
{
	_relay.clear();
	_time.clear();
}

void Relay_Class::Initialize(const RelayGPIOList &gpios)
{
	for (auto gpio : gpios)
	{
		add(gpio);
	}
}

// ********************************************************************************
// Relay_Class public functions
// ********************************************************************************

/**
 * Add relay to the list
 * Relay is initialized off
 */
void Relay_Class::add(uint8_t gpio)
{
	Relay_typedef relay;
	relay.idRelay = _relay.size();
	relay.gpio = gpio;
	relay.hasAlarm = false;
	relay.hasAlarm2 = false;
	relay.state = false;
	pinMode(gpio, OUTPUT);
	digitalWrite(gpio, LOW);
	_relay.push_back(relay);
}

/**
 * Set the relay ON (state = true) or OFF (state = false)
 */
void Relay_Class::setState(uint8_t idRelay, bool state)
{
	if (idRelay >= _relay.size())
		return;

#ifdef RELAY_DEBUG
	String tmp = "";
	tmp = "Relay " + (String) idRelay + ". Initial state: ";
	tmp += (_relay[idRelay].state) ? "ON" : "OFF";
	tmp += " - Final state: ";
	tmp += (state) ? "ON" : "OFF";
	print_debug(tmp);
#endif

	if (state)
	{
		if (!_relay[idRelay].state)
		{
			_relay[idRelay].state = true;
			digitalWrite(_relay[idRelay].gpio, HIGH);
			relaisOnCount++;
		}
	}
	else
	{
		if (_relay[idRelay].state)
		{
			_relay[idRelay].state = false;
			digitalWrite(_relay[idRelay].gpio, LOW);
			relaisOnCount--;
		}
	}
}

/**
 * Toggle the state of the relay: ON <-> OFF
 */
void Relay_Class::toggleState(uint8_t idRelay)
{
	if (idRelay >= _relay.size())
		return;
	setState(idRelay, !getState(idRelay));
}

/**
 * Get the state of the relay. Return true if ON.
 */
bool Relay_Class::getState(uint8_t idRelay) const
{
	if (idRelay >= _relay.size())
		return false;

	return _relay[idRelay].state;
}

String Relay_Class::getAllState(void)
{
	String state = "";
	for (unsigned int i = 0; i < _relay.size(); i++)
	{
		if (!state.isEmpty())
			state += ",";
		state += (getState(i)) ? "ON" : "OFF";
	}
	return state;
}

// ********************************************************************************
// Alarm functions
// ********************************************************************************

/**
 * Check if minute is in the range [-1 .. 1440[
 * Value -1 is used for special operation
 */
inline bool Relay_Class::CheckMinuteRange(int minute)
{
	return ((minute >= -1) && (minute < 1440));
}

/**
 * Update the current time in minute
 * _time in minute since 00h00 : _time in [0 .. 1440[
 * This function shoud be called at least every minute
 * If _time == -1 (default), currentTime is just incremented by 1.
 * _time must be provided to change day
 */
void Relay_Class::updateTime(int _time)
{
	if (currentTime == _time)
		return;

	if (!CheckMinuteRange(_time))
		return;

	int lastcurrentTime = currentTime;
	(_time != -1) ? currentTime = _time : currentTime++;

	// Time not initialized
	if (!isTimeInitialized)
	{
		UpdateNextAlarm(true);
		isTimeInitialized = true;
	}
	else
		// new time is before currentTime (change day)
		if (lastcurrentTime > currentTime)
			idTime = 0;

	CheckAlarmTime();
}

bool Relay_Class::hasAlarm(void) const
{
	bool result = false;
	for (int i = 0; i < _relay.size(); i++)
	{
		if (_relay[i].hasAlarm)
		{
			result = true;
			break;
		}
	}
	return result;
}

bool Relay_Class::hasAlarm(uint8_t idRelay) const
{
	if (idRelay >= _relay.size())
		return false;

	return _relay[idRelay].hasAlarm;
}

bool Relay_Class::addAlarm(uint8_t idRelay, AlarmNumber num, int start, int end, bool updateTimeList)
{
	if (idRelay >= _relay.size())
		return false;
	Relay_typedef *relais = &_relay[idRelay];

	if (num == AlarmNumber::Alarm1)
		return addAlarm(idRelay, start, end, relais->alarm2.start, relais->alarm2.end, updateTimeList);
	else
		if (num == AlarmNumber::Alarm2)
			return addAlarm(idRelay, relais->alarm1.start, relais->alarm1.end, start, end, updateTimeList);
	return false;
}

bool Relay_Class::addAlarm(uint8_t idRelay, int start1, int end1, int start2, int end2, bool updateTimeList)
{
	if (idRelay >= _relay.size())
		return false;

	bool result = true;
	Relay_typedef *relay = &_relay[idRelay];

	// First clean alarm for this relay
	relay->hasAlarm = false;
	relay->hasAlarm2 = false;
	relay->alarm1.reset();
	relay->alarm2.reset();
	UpdateTimeList();

	// Alarm1
	if (!CheckMinuteRange(start1) || !CheckMinuteRange(end1))
	{
		print_debug("Alarm1 error: start or end not correct");
		result = false;
	}

	if ((start1 >= end1) && (start1 != -1) && (end1 != -1))
	{
		print_debug("Alarm1 error: start >= end");
		result = false;
	}

	if (result)
	{
		relay->alarm1.set(start1, end1);
		relay->hasAlarm = relay->alarm1.isDefined();
	}

	// Alarm2 if Alarm1 exist
	if (result && relay->hasAlarm)
	{
		if (!CheckMinuteRange(start2) || !CheckMinuteRange(end2))
		{
			print_debug("Alarm2 error: start or end not correct");
			result = false;
		}

		if ((start2 >= end2) && (start2 != -1) && (end2 != -1))
		{
			print_debug("Alarm2 error: start >= end");
			result = false;
		}
		else
		{
			if ((start2 != -1) && (start2 <= relay->alarm1.end))
			{
				print_debug("Alarm2 error: start <= end of Alarm1");
				result = false;
			}
		}

		if (result)
		{
			relay->alarm2.set(start2, end2);
			relay->hasAlarm2 = relay->alarm2.isDefined();
		}
		else
		{
			// Suppress first alarm if we have error on second alarm
			relay->hasAlarm = false;
			relay->alarm1.reset();
		}
	}

	// Update alarm time list
	if (result)
		UpdateTimeList();
	return result;
}

bool Relay_Class::getAlarm(uint8_t idRelay, AlarmNumber num, int *start, int *end)
{
	if (idRelay >= _relay.size())
		return false;

	*start = *end = -1;
	if (num == AlarmNumber::Alarm1)
	{
		_relay[idRelay].alarm1.get(start, end);
	}
	else
		if (num == AlarmNumber::Alarm2)
		{
			_relay[idRelay].alarm2.get(start, end);
		}
	return (*start != -1) || (*end != -1);
}

void Relay_Class::getAlarm(uint8_t idRelay, AlarmNumber num, String &start, String &end)
{
	int _start, _end;
	start = "";
	end = "";
	if (getAlarm(idRelay, num, &_start, &_end))
	{
		start = toString(_start);
		end = toString(_end);
	}
}

void Relay_Class::printAlarm(void) const
{
	String tmp = "";

	tmp = "Current time = " + toString(currentTime, true);
	print_debug(tmp);
	tmp = "Current id time = " + (String) idTime;
	print_debug(tmp);

	int i = 0;
	uint8_t alarmNumber;
	bool start;
	int val;
	for (TimeAlarm_typedef time : _time)
	{
		tmp = "ID: " + (String) i + " : At time = " + toString(time.time, true) + " Relay: " + (String) time.idRelay;
		Relay_typedef relay = _relay[time.idRelay];
		if (relay.FoundAlarm(time.time, &alarmNumber, &start, &val))
		{
			tmp += " - Alarm" + (String) alarmNumber + " ";
			tmp += (start) ? "start: " : "end: ";
			tmp += toString(val, true);
		}
		else
			tmp += " - Alarm error.";
		print_debug(tmp);
		i++;
	}
}

String Relay_Class::toString(int time, bool withtime) const
{
	char temp[20];
	String result = "";
	if (time != -1)
	{
		sprintf(temp, "%d.%02d", time / 60, time % 60);
		result = String(temp);
		if (withtime)
			result += " (" + (String) time + ")";
	}
	return result;
}

// ********************************************************************************
// Relay_Class private functions
// ********************************************************************************

bool sort_time(TimeAlarm_typedef t1, TimeAlarm_typedef t2)
{
	return (t1.time < t2.time);
}

void Relay_Class::UpdateTimeList(void)
{
	_time.clear();
	idTime = -1;

	for (Relay_typedef relay : _relay)
	{
		if (relay.hasAlarm)
		{
			TimeAlarm_typedef time;
			time.idRelay = relay.idRelay;
			if (relay.alarm1.start != -1)
			{
				time.time = relay.alarm1.start;
				_time.push_back(time);
			}
			else
			{
				// Special case, start is not defined, so start at 0
				time.time = 0;
				_time.push_back(time);
			}
			if (relay.alarm1.end != -1)
			{
				time.time = relay.alarm1.end;
				_time.push_back(time);
			}

			// check if second alarm
			if (relay.hasAlarm2)
			{
				if (relay.alarm2.start != -1)
				{
					time.time = relay.alarm2.start;
					_time.push_back(time);
				}
				else
				{
					// Special case, start is not defined, so start at 0
					time.time = 0;
					_time.push_back(time);
				}
				if (relay.alarm2.end != -1)
				{
					time.time = relay.alarm2.end;
					_time.push_back(time);
				}
			}
		}
	}

	if (_time.size() > 0)
	{
		// Sort alarm time
		std::sort(_time.begin(), _time.end(), sort_time);
		UpdateNextAlarm(true);
	}
}

void Relay_Class::UpdateNextAlarm(bool updateLastAlarm)
{
	// Find the next alarm time to execute
	idTime = 0;
	while ((idTime < _time.size()) && (_time[idTime].time < currentTime))
	{
		if (updateLastAlarm)
		{
			Relay_typedef *relay = &_relay[_time[idTime].idRelay];
			setState(relay->idRelay, relay->IsAlarmActivated(currentTime));
		}
		idTime++;
	}
}

void Relay_Class::CheckAlarmTime(void)
{
	if ((idTime == -1) || (idTime == (int) _time.size()))
		return;

	// We can have the same alarm for several relays
	while ((idTime < _time.size()) && (_time[idTime].time == currentTime))
	{
		Relay_typedef *relay = &_relay[_time[idTime].idRelay];
		setState(relay->idRelay, relay->IsAlarmActivated(currentTime));
		idTime++;
	}
}

// ********************************************************************************
// Basic Task function to update relay
// ********************************************************************************
/**
 * A basic Task to check Relay
 */
#ifdef RELAY_USE_TASK

/**
 * We assume that Relay_Class instance is called Relay
 */
extern Relay_Class Relay;

void RELAY_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("RELAY_Task");
	for (EVER)
	{
#if defined(USE_RTCLocal)
		Relay.updateTime(RTC_Local.getMinuteOfTheDay());
#else
		Relay.updateTime();
#endif
		END_TASK_CODE(false);
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
