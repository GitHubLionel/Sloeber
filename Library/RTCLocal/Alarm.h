#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <initializer_list>
#include <vector>

// To create a basic task to update Relay every 10 secondes
#ifdef ALARM_USE_TASK
// Task to update relay according alarm
#define ALARM_DATA_TASK(start)	{start, "ALARM_Task", 1024 * 4, 2, 10000, CoreAny, ALARM_Task_code}
void ALARM_Task_code(void *parameter);
#else
#define ALARM_DATA_TASK(start)	{}
#endif

typedef void (*AlarmFunction_t)(size_t, bool, int);

/**
 * Alarm limit
 * start: beginning of the alarm
 * end: ending of the alarm
 */
typedef struct
{
		int start = -1;
		int end = -1;
	public:
		void reset(void)
		{
			start = -1;
			end = -1;
		}
		void set(int _start, int _end)
		{
			start = _start;
			end = _end;
		}
		void get(int *_start, int *_end)
		{
			*_start = start;
			*_end = end;
		}
		// We can have only start or end alarm
		bool isDefined(void)
		{
			return ((start != -1) || (end != -1));
		}
} AlarmLimit_typedef;

/**
 * Alarm structure
 */
typedef struct
{
		size_t idAlarm;
		AlarmLimit_typedef alarm;
		bool active;
		AlarmFunction_t action = NULL;
		int Param;

	public:
		// Alarm is activated if time in [start, end[
		bool IsAlarmActivated(int time)
		{
			if (alarm.start != -1)
			{
				if (time >= alarm.start)
				{
					// If end is defined else until end of day
					if (alarm.end != -1)
						return (time < alarm.end);
					else
						return true;
				}
				else
					return false;
			}
			else
			{
				// end must be defined
				return (time < alarm.end);
			}
		}

		// Is the time of an alarm
		bool IsAlarmTime(int time)
		{
			return ((alarm.start == time) || (alarm.end == time));
		}

	private:

} AlarmAction_typedef;

/**
 * Structure that contains the time at which the relay referenced by its identifier has an alarm
 */
typedef struct
{
		uint16_t time;
		size_t idAlarm;
} TimeAlarmLimit_typedef;

typedef std::vector<AlarmAction_typedef> AlarmList;
typedef std::vector<TimeAlarmLimit_typedef> TimeActionList;

class Alarm_Class
{
	public:
		Alarm_Class();
		~Alarm_Class();

		size_t size(void)
		{
			return _alarm.size();
		}

		void setState(size_t idAlarm, bool state);
		bool getState(size_t idAlarm);
		void toggleState(size_t idAlarm);
		String getAllState(void);

		AlarmAction_typedef* getAlarmByID(size_t id);

		void updateTime(int _time = -1);

		int add(int start, int end, const AlarmFunction_t &pAlarmAction, int param, bool updateTimeList = true);

		bool getLimit(size_t idAlarm, int *start, int *end);
		void getLimit(size_t idAlarm, String &start, String &end);
		void deleteAlarm(size_t idAlarm, bool updateTimeList = true);
		void updateAlarm(size_t idAlarm, int start, int end, bool updateTimeList = true);

		void printAlarm(void);

		String toString(int time, bool withtime = false) const;

	protected:
		AlarmList _alarm;    // The list of alarm
		TimeActionList _time;      // The list of time (minute of the day) with an alarm (on or off a relay)
		int currentTime;     // The current time in minute of the day unit
		size_t idTime;          // The current index in the list of time action
		bool isTimeInitialized;  // Is currentTime initialized ?

	private:
		size_t unique_ID;  // An ID to reference one alarm

		bool CheckMinuteRange(int minute);
		void UpdateTimeList(void);
		void DoAction(size_t idAlarm);
		void UpdateNextAlarm(bool updateLastAlarm);
		void CheckAlarmTime(void);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ALARM)
extern Alarm_Class Alarm;
#endif
