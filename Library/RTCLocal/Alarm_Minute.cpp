#include "Alarm_Minute.h"
#include "Debug_utils.h"		  // Some utils functions for debug

#ifdef ALARM_USE_TASK
#include "Tasks_utils.h"
#if defined(USE_RTCLocal)
#include "RTCLocal.h"		      // A pseudo RTC software library
#endif
#endif

//#define DEBUG_ALARM

// ********************************************************************************
// Alarm_Minute constructor
// ********************************************************************************

Alarm_Minute::Alarm_Minute()
{
	unique_ID = 0;
	currentTime = -1;
	idTime = 0;
	isTimeInitialized = false;
}

Alarm_Minute::~Alarm_Minute()
{
	_alarm.clear();
	_time.clear();
}

// ********************************************************************************
// Alarm_Minute public functions
// ********************************************************************************

/**
 * Get an alarm by his ID
 * @Param id: the ID of the alarm
 * Note: ID is NOT the index in the alarm list. ID is the return value of the add function.
 */
Alarm_Property* Alarm_Minute::getAlarmByID(size_t id)
{
	size_t i = 0;

	for (Alarm_Property alarm : _alarm)
	{
		if (alarm.getID() == id)
			return &_alarm[i];
		i++;
	}
	return NULL;
}

/**
 * Set the alarm active (state = true) or not active (state = false)
 */
void Alarm_Minute::setState(size_t idAlarm, bool state)
{
	Alarm_Property *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction == NULL)
		return;

	if (state != alarmAction->getActive())
	{
		alarmAction->setActive(state);
		UpdateTimeList(false);
	}
}

/**
 * Get the state of the alarm. Return true if the alarm is active
 */
bool Alarm_Minute::getState(size_t idAlarm)
{
	Alarm_Property *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction == NULL)
		return false;

	return alarmAction->getActive();
}

/**
 * Toggle the state of the alarm: active <-> not active
 */
void Alarm_Minute::toggleState(size_t idAlarm)
{
	Alarm_Property *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction == NULL)
		return;
	setState(idAlarm, !getState(idAlarm));
}

/**
 * Get the state of the all the alarms. Print ON if the alarm is active
 */
String Alarm_Minute::getAllState(void)
{
	String state = "";
	for (size_t i = 0; i < _alarm.size(); i++)
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
inline bool Alarm_Minute::CheckMinuteRange(int minute)
{
	return ((minute >= -1) && (minute < MINUTESINDAY));
}

/**
 * Update the current time in minute
 * _time in minute since 00h00 : _time in [0 .. 1440[
 * This function shoud be called at least every minute
 * If _time == -1 (default), currentTime is just incremented by 1. In that case, if we use task, period must be one minute.
 * _time must be provided to change day
 */
void Alarm_Minute::updateTime(int _time)
{
	if ((busy > 0) || (currentTime == _time))
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

/**
 * Add alarm to the list.
 * @Param start: time in minute to start the Alarm
 * @Param end: time in minute to stop the Alarm
 * @Param pAlarmAction: the callback function to do the action
 * @Param param: the parameter to pass to the callback
 * @Param updateTimeList: if true (default) then update the alarm list
 */
int Alarm_Minute::add(int start, int end, const AlarmFunction_t &pAlarmAction, int param, bool updateTimeList)
{
	if ((start == -1) && (end == -1))
		return -1;

	// Alarm range
	if (!CheckMinuteRange(start) || !CheckMinuteRange(end))
	{
		print_debug("Alarm error: start or end not correct");
		return -1;
	}

	if ((start != -1) && (end != -1) && (start >= end))
	{
		print_debug("Alarm error: start >= end");
		return -1;
	}

	busy++;
	Alarm_Property alarm(unique_ID);
	alarm.set(start, end);
	alarm.setAction(pAlarmAction, param);
	alarm.setActive(true);
	_alarm.push_back(alarm);

	// Test action
	if (isTimeInitialized)
	  alarm.DoAction(currentTime);

	// Update alarm time list
	if (updateTimeList)
		UpdateTimeList(false);
	unique_ID++;
	busy--;

	return alarm.getID();
}

bool Alarm_Minute::getRange(size_t idAlarm, int *start, int *end)
{
	*start = *end = -1;
	Alarm_Property *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction != NULL)
	{
		alarmAction->get(start, end);
	}

	return (*start != -1) || (*end != -1);
}

void Alarm_Minute::getRange(size_t idAlarm, String &start, String &end)
{
	int _start, _end;
	start = "";
	end = "";
	if (getRange(idAlarm, &_start, &_end))
	{
		start = toString(_start);
		end = toString(_end);
	}
}

void Alarm_Minute::deleteAlarm(size_t idAlarm, bool updateTimeList)
{
	size_t i = 0;

	busy++;
	for (Alarm_Property alarm : _alarm)
	{
		if (alarm.getID() == idAlarm)
			break;
		i++;
	}
	if (i < _alarm.size())
	{
		_alarm.erase(_alarm.begin() + i);
		if (updateTimeList)
			UpdateTimeList(false);
	}
	busy--;
}

void Alarm_Minute::updateAlarm(size_t idAlarm, int start, int end, bool updateTimeList)
{
	busy++;
	Alarm_Property *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction != NULL)
	{
		alarmAction->set(start, end);
		// Actualise l'alarme
		alarmAction->DoAction(currentTime);
		if (updateTimeList)
			UpdateTimeList(false);
	}
	busy--;
}

void Alarm_Minute::printAlarm(void)
{
	busy++;
	for (Alarm_Property &alarm : _alarm)
	{
		print_debug(alarm.print());
	}
	vTaskDelay(1);

	String tmp = "";
	int i = 0;
	for (TimeAlarmLimit_typedef &time : _time)
	{
		tmp = "Time ID: " + (String) i + " : At time = " + toString(time.time, true) + " Alarm: " + (String) time.alarm->getID();
		const Alarm_Property *alarmAction = time.alarm; // getAlarmByID(time.idAlarm);
		if (alarmAction != NULL)
		{
			tmp += " - Alarm ";
			tmp += (alarmAction->IsStartTime(time.time)) ? "start: " : "end: ";
			tmp += toString(time.time, true);
		}
		else
			tmp += " - Alarm error.";
		print_debug(tmp);
		i++;
	}
	if (i == 0)
		print_debug("No alarm.");
	else
	{
		tmp = "Current time = " + toString(currentTime, true);
		print_debug(tmp);
		tmp = "Current id time = " + (String) idTime;
		print_debug(tmp);
	}
	busy--;
}

String Alarm_Minute::toString(int time, bool withtime) const
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
// Alarm_Minute private functions
// ********************************************************************************

bool sort_time(TimeAlarmLimit_typedef t1, TimeAlarmLimit_typedef t2)
{
	return (t1.time < t2.time);
}

/**
 * Create the ordered list of time of the alarms
 */
void Alarm_Minute::UpdateTimeList(bool checkAlarm)
{
	_time.clear();
	idTime = 0;
	int start, end;

	busy++;
	for (const Alarm_Property &alarm : _alarm)
	{
		if (alarm.getActive())
		{
			TimeAlarmLimit_typedef time;
			time.alarm = &alarm;

			alarm.get(&start, &end);
			if (start != -1)
			{
				time.time = start;
				_time.push_back(time);
			}
			if (end != -1)
			{
				time.time = end;
				_time.push_back(time);
			}
		}
	}

	if (_time.size() > 0)
	{
		// Sort alarm time
		std::sort(_time.begin(), _time.end(), sort_time);
	}

#ifdef DEBUG_ALARM
	printAlarm();
#endif

	if (isTimeInitialized)
		UpdateNextAlarm(checkAlarm);
	busy--;
}

void Alarm_Minute::UpdateNextAlarm(bool updateLastAlarm)
{
	idTime = 0;
	if (_time.size() == 0)
		return;

#ifdef DEBUG_ALARM
	String tmp = "Time: " + toString(currentTime, true);
	print_debug(tmp);
#endif

	// Find the next alarm time to execute
	busy++;
	while ((idTime < _time.size()) && (_time[idTime].time <= currentTime))
	{
		if (updateLastAlarm)
		{
			_time[idTime].alarm->DoAction(currentTime);
		}
		idTime++;
	}
	busy--;
#ifdef DEBUG_ALARM
	tmp = "Current id time = " + (String) idTime;
	print_debug(tmp);
#endif
}

void Alarm_Minute::CheckAlarmTime(void)
{
	if ((_time.size() == 0) || (idTime == _time.size()))
		return;

	bool needUpdate = false;

	busy++;
	// We can have the same alarm for several actions
	while ((idTime < _time.size()) && (_time[idTime].time == currentTime))
	{
#ifdef DEBUG_ALARM
		String tmp = "Time: " + toString(currentTime, true);
		print_debug(tmp);
#endif
		_time[idTime].alarm->DoAction(currentTime);
		if (_time[idTime].alarm->getOnlyOne())
		{
			deleteAlarm(_time[idTime].alarm->getID(), false);
			needUpdate = true;
		}
		idTime++;
#ifdef DEBUG_ALARM
		tmp = "Current id time = " + (String) idTime;
		print_debug(tmp);
#endif
	}
	if (needUpdate)
		UpdateTimeList(false);
	busy--;
}

/**
 * A global instance of Alarm_Minute
 */
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ALARM)
Alarm_Minute Alarm = Alarm_Minute();
#endif

// ********************************************************************************
// Basic Task function to update alarm
// ********************************************************************************
/**
 * A basic Task to update alarm
 */
#ifdef ALARM_USE_TASK

void ALARM_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("ALARM_Task");
	for (EVER)
	{
#if defined(USE_RTCLocal)
		Alarm.updateTime(RTC_Local.getMinuteOfTheDay());
#else
		Alarm.updateTime();
#endif
		END_TASK_CODE(false);
	}
}

#endif

// ********************************************************************************
// End of file
// ********************************************************************************
