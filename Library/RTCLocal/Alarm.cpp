#include "Alarm.h"
#include "Debug_utils.h"		  // Some utils functions for debug

#ifdef ALARM_USE_TASK
#include "Tasks_utils.h"
#if defined(USE_RTCLocal)
#include "RTCLocal.h"		      // A pseudo RTC software library
#endif
#endif

#define DEBUG_ALARM

// Number of minutes in a day
#define MINUTESINDAY	1440

// ********************************************************************************
// Alarm_Class constructor
// ********************************************************************************

Alarm_Class::Alarm_Class()
{
	unique_ID = 0;
	currentTime = -1;
	idTime = 0;
	isTimeInitialized = false;
}

Alarm_Class::~Alarm_Class()
{
	_alarm.clear();
	_time.clear();
}

// ********************************************************************************
// Alarm_Class public functions
// ********************************************************************************

/**
 * Get an alarm by his ID
 * @Param id: the ID of the alarm
 * Note: ID is NOT the index in the alarm list. ID is the return value of the add function.
 */
AlarmAction_typedef* Alarm_Class::getAlarmByID(size_t id)
{
	size_t i = 0;

	for (AlarmAction_typedef alarm : _alarm)
	{
		if (alarm.idAlarm == id)
			return &_alarm[i];
		i++;
	}
	return NULL;
}

/**
 * Set the alarm active (state = true) or not active (state = false)
 */
void Alarm_Class::setState(size_t idAlarm, bool state)
{
	AlarmAction_typedef *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction == NULL)
		return;

	if (state != alarmAction->active)
	{
		alarmAction->active = state;
		UpdateTimeList();
	}
}

/**
 * Get the state of the alarm. Return true if the alarm is active
 */
bool Alarm_Class::getState(size_t idAlarm)
{
	AlarmAction_typedef *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction == NULL)
		return false;

	return alarmAction->active;
}

/**
 * Toggle the state of the alarm: active <-> not active
 */
void Alarm_Class::toggleState(size_t idAlarm)
{
	AlarmAction_typedef *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction == NULL)
		return;
	setState(idAlarm, !getState(idAlarm));
}

/**
 * Get the state of the all the alarms. Print ON if the alarm is active
 */
String Alarm_Class::getAllState(void)
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
inline bool Alarm_Class::CheckMinuteRange(int minute)
{
	return ((minute >= -1) && (minute < MINUTESINDAY));
}

/**
 * Update the current time in minute
 * _time in minute since 00h00 : _time in [0 .. 1440[
 * This function shoud be called at least every minute
 * If _time == -1 (default), currentTime is just incremented by 1.
 * _time must be provided to change day
 */
void Alarm_Class::updateTime(int _time)
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

/**
 * Add alarm to the list.
 * @Param start: time in minute to start the Alarm
 * @Param end: time in minute to stop the Alarm
 * @Param pAlarmAction: the callback function to do the action
 * @Param param: the parameter to pass to the callback
 * @Param updateTimeList: if true (default) then update the alarm list
 */
int Alarm_Class::add(int start, int end, const AlarmFunction_t &pAlarmAction, int param, bool updateTimeList)
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

	AlarmAction_typedef alarm;
	alarm.idAlarm = unique_ID;
	alarm.alarm.set(start, end);
	alarm.active = true;
	alarm.action = pAlarmAction;
	alarm.Param = param;
	_alarm.push_back(alarm);

	// Update alarm time list
	if (updateTimeList)
		UpdateTimeList();
	unique_ID++;

	return alarm.idAlarm;
}

bool Alarm_Class::getLimit(size_t idAlarm, int *start, int *end)
{
	*start = *end = -1;
	AlarmAction_typedef *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction != NULL)
	{
		alarmAction->alarm.get(start, end);
	}

	return (*start != -1) || (*end != -1);
}

void Alarm_Class::getLimit(size_t idAlarm, String &start, String &end)
{
	int _start, _end;
	start = "";
	end = "";
	if (getLimit(idAlarm, &_start, &_end))
	{
		start = toString(_start);
		end = toString(_end);
	}
}

void Alarm_Class::deleteAlarm(size_t idAlarm, bool updateTimeList)
{
	size_t i = 0;

	for (AlarmAction_typedef alarm : _alarm)
	{
		if (alarm.idAlarm == idAlarm)
			break;
		i++;
	}
	if (i < _alarm.size())
	{
		_alarm.erase(_alarm.begin() + i);
		if (updateTimeList)
			UpdateTimeList();
	}
}

void Alarm_Class::updateAlarm(size_t idAlarm, int start, int end, bool updateTimeList)
{
	AlarmAction_typedef *alarmAction = getAlarmByID(idAlarm);
	if (alarmAction != NULL)
	{
		alarmAction->alarm.set(start, end);
		if (updateTimeList)
			UpdateTimeList();
	}
}

void Alarm_Class::printAlarm(void)
{
	for (AlarmAction_typedef alarm : _alarm)
	{
		print_debug(alarm.print());
	}

	String tmp = "";
	int i = 0;
	for (TimeAlarmLimit_typedef time : _time)
	{
		tmp = "Time ID: " + (String) i + " : At time = " + toString(time.time, true) + " Alarm: " + (String) time.idAlarm;
		AlarmAction_typedef *alarmAction = getAlarmByID(time.idAlarm);
		if (alarmAction != NULL)
		{
			AlarmLimit_typedef alarm = alarmAction->alarm;
			tmp += " - Alarm ";
			tmp += (alarm.start == time.time) ? "start: " : "end: ";
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
}

String Alarm_Class::toString(int time, bool withtime) const
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
// Alarm_Class private functions
// ********************************************************************************

bool sort_time(TimeAlarmLimit_typedef t1, TimeAlarmLimit_typedef t2)
{
	return (t1.time < t2.time);
}

void Alarm_Class::UpdateTimeList(void)
{
	_time.clear();
	idTime = 0;
	isTimeInitialized = false;

	for (AlarmAction_typedef alarm : _alarm)
	{
		if (alarm.active)
		{
			TimeAlarmLimit_typedef time;
			time.idAlarm = alarm.idAlarm;
			if (alarm.alarm.start != -1)
			{
				time.time = alarm.alarm.start;
				_time.push_back(time);
			}
			if (alarm.alarm.end != -1)
			{
				time.time = alarm.alarm.end;
				_time.push_back(time);
			}
		}
	}

	if (_time.size() > 0)
	{
		// Sort alarm time
		std::sort(_time.begin(), _time.end(), sort_time);
	}

//	if (isTimeInitialized)
//		UpdateNextAlarm(false);
}

void Alarm_Class::DoAction(size_t idAlarm)
{
	AlarmAction_typedef *alarmAction = getAlarmByID(idAlarm);
	if ((alarmAction != NULL) && (alarmAction->active) && (alarmAction->action != NULL))
	{
		if (alarmAction->IsAlarmTime(currentTime))
			alarmAction->action(idAlarm, (currentTime == alarmAction->alarm.start), alarmAction->Param);
		else
			if (alarmAction->IsAlarmActivated(currentTime))
				alarmAction->action(idAlarm, true, alarmAction->Param);

//		AlarmLimit_typedef alarm = alarmAction->alarm;
//		if (currentTime == alarm.start)
//		  alarmAction->action(idAlarm, true, alarmAction->Param);
//		else
//			if (currentTime == alarm.end)
//			  alarmAction->action(idAlarm, false, alarmAction->Param);
//			else
//			{
//				if ((currentTime > alarm.start) && (currentTime < alarm.end))
//					alarmAction->action(idAlarm, true, alarmAction->Param);
//			}
	}
}

void Alarm_Class::UpdateNextAlarm(bool updateLastAlarm)
{
	idTime = 0;
	if (_time.size() == 0)
		return;

#ifdef DEBUG_ALARM
	String tmp = "Time: " + toString(currentTime, true);
	print_debug(tmp);
#endif

	// Find the next alarm time to execute
	while ((idTime < _time.size()) && (_time[idTime].time < currentTime))
	{
		if (updateLastAlarm)
		{
			DoAction(_time[idTime].idAlarm);
		}
		idTime++;
	}
#ifdef DEBUG_ALARM
	tmp = "Current id time = " + (String) idTime;
	print_debug(tmp);
#endif
}

void Alarm_Class::CheckAlarmTime(void)
{
	if ((_time.size() == 0) || (idTime == _time.size()))
		return;

	// We can have the same alarm for several actions
	while ((idTime < _time.size()) && (_time[idTime].time == currentTime))
	{
#ifdef DEBUG_ALARM
		String tmp = "Time: " + toString(currentTime, true);
		print_debug(tmp);
#endif
		DoAction(_time[idTime].idAlarm);
		idTime++;
#ifdef DEBUG_ALARM
		tmp = "Current id time = " + (String) idTime;
		print_debug(tmp);
#endif
	}
}

/**
 * A global instance of Alarm_Class
 */
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ALARM)
Alarm_Class Alarm = Alarm_Class();
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
