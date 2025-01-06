#include "Relay.h"

#ifdef RELAY_USE_TASK
#include "Tasks_utils.h"
#if defined(USE_RTCLocal)
#include "RTCLocal.h"		      // A pseudo RTC software library
#endif
#endif

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

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
Relay_Class::Relay_Class(const std::initializer_list<uint8_t> gpios) :
		Relay_Class()
{
	for (auto gpio : gpios)
	{
		add(gpio);
	}
}

Relay_Class::~Relay_Class()
{
	_relay.clear();
	_time.clear();
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
	relay.state = false;
	pinMode(gpio, OUTPUT);
	digitalWrite(gpio, LOW);
	_relay.push_back(relay);
}

/**
 * Update the current time in minute
 * This function shoud be called every minute
 */
void Relay_Class::updateTime(int _time)
{
	int lastcurrentTime = currentTime;

	if (_time != -1)
	{
		if (currentTime == _time)
			return;
		currentTime = _time;
	}
	else
		currentTime++;

	// Time not initialized or new time is before currentTime (change day)
	if ((!isTimeInitialized) || (lastcurrentTime > _time))
	{
		UpdateNextAlarm();
		isTimeInitialized = true;
	}
	CheckAlarmTime();
}

/**
 * Set the relay ON (state = true) or OFF (state = false)
 */
void Relay_Class::setState(uint8_t idRelay, bool state)
{
	if (idRelay >= _relay.size())
		return;

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
bool Relay_Class::getState(uint8_t idRelay)
{
	if (idRelay >= _relay.size())
		return false;

	return _relay[idRelay].state;
}

String Relay_Class::getAllState(void)
{
	String state = "";
	for (int i = 0; i < _relay.size(); i++)
	{
		if (!state.isEmpty())
			state += ",";
		(getState(i)) ? state += "ON" : state += "OFF";
	}
	return state;
}

bool Relay_Class::hasAlarm(uint8_t idRelay)
{
	if (idRelay >= _relay.size())
		return false;

	return _relay[idRelay].hasAlarm;
}

void Relay_Class::addAlarm(uint8_t idRelay, AlarmNumber num, int begin, int end)
{
	if (idRelay >= _relay.size())
		return;

	if ((begin >= end) && (begin != -1) && (end != -1))
	{
		print_debug("Alarm error: begin >= end");
		return;
	}

	if (num == Alarm1)
	{
		_relay[idRelay].alarm1.start = begin;
		_relay[idRelay].alarm1.end = end;
		// We can have only begin or end alarm
		_relay[idRelay].hasAlarm = ((begin != -1) || (end != -1));
	}
	else
	{
		// Second alarm only if first alarm exist
		if (_relay[idRelay].hasAlarm)
		{
			if ((begin != -1) && (begin <= _relay[idRelay].alarm1.end))
			{
				print_debug("Alarm2 error: begin <= end of Alarm1");
				_relay[idRelay].alarm2.start = -1;
				_relay[idRelay].alarm2.end = -1;
			}
			else
			{
				_relay[idRelay].alarm2.start = begin;
				_relay[idRelay].alarm2.end = end;
			}
		}
		else
		{
			_relay[idRelay].alarm2.start = -1;
			_relay[idRelay].alarm2.end = -1;
		}
	}
	UpdateTimeList();
}

bool Relay_Class::getAlarm(uint8_t idRelay, AlarmNumber num, int *begin, int *end)
{
	*begin = *end = -1;
	if (num == Alarm1)
	{
		*begin = _relay[idRelay].alarm1.start;
		*end = _relay[idRelay].alarm1.end;
	}
	else
	{
		*begin = _relay[idRelay].alarm2.start;
		*end = _relay[idRelay].alarm2.end;
	}
	return (*begin != -1);
}

void Relay_Class::getAlarm(uint8_t idRelay, AlarmNumber num, String &begin, String &end)
{
	int _begin, _end;
	char temp[50];
	if (getAlarm(idRelay, num, &_begin, &_end))
	{
		sprintf(temp, "%d.%02d", _begin / 60, _begin % 60);
		begin = String(temp);
		sprintf(temp, "%d.%02d", _end / 60, _end % 60);
		end = String(temp);
	}
	else
	{
		begin = "";
		end = "";
	}
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
			if (relay.alarm1.end != -1)
			{
				time.time = relay.alarm1.end;
				_time.push_back(time);
			}
			// check if second alarm
			if (relay.alarm2.start != -1)
			{
				time.time = relay.alarm2.start;
				_time.push_back(time);
			}
			if (relay.alarm2.end != -1)
			{
				time.time = relay.alarm2.end;
				_time.push_back(time);
			}
		}
	}

	if (_time.size() > 0)
	{
		// Sort alarm time
		std::sort(_time.begin(), _time.end(), sort_time);
		UpdateNextAlarm();
	}
}

void Relay_Class::UpdateNextAlarm(void)
{
	// Find the next alarm time to execute
	if (_time.size() > 0)
	{
		idTime = 0;
		while (_time[idTime].time < currentTime)
		{
			idTime++;
			if (idTime == (int)_time.size())
				break;
		}
	}
}

void Relay_Class::CheckAlarmTime(void)
{
	if ((idTime == -1) || (idTime == (int)_time.size()))
		return;

	// We can have the same alarm for several relays
	while (currentTime == _time[idTime].time)
	{
		Relay_typedef relay = _relay[_time[idTime].idRelay];
		if ((relay.alarm1.start == currentTime) || (relay.alarm2.start == currentTime))
			setState(relay.idRelay, true);
		else
		{
			if ((relay.alarm1.end == currentTime) || (relay.alarm2.end == currentTime))
				setState(relay.idRelay, false);
		}
		idTime++;
		if (idTime == (int)_time.size())
			break;
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
