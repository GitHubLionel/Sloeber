#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <initializer_list>
#include <vector>

// To create a basic task to check alarm every 10 secondes
#ifdef ALARM_USE_TASK
// Task to update relay according alarm
#define ALARM_ACTION_TASK(start)	{start, "ALARM_Task", 1024 * 4, 10, 10000, CoreAny, ALARM_Task_code}
void ALARM_Task_code(void *parameter);
#else
#define ALARM_ACTION_TASK(start)	{}
#endif

// Number of minutes in a day
#define MINUTESINDAY	1440

/**
 * Callback definition for the action
 * @Param idAlarm: the identificator of the alarm who called the action
 * @Param state: the state active/start or inactive/end
 * @Param param: the parameter pass to the action
 */
typedef void (*AlarmFunction_t)(size_t idAlarm, bool state, int param);

/**
 * Alarm range
 * start: beginning of the alarm
 * end: ending of the alarm
 */
class Alarm_Range
{
	public:
		Alarm_Range() : start(-1), end(-1) { }
		virtual ~Alarm_Range() { }

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
		void get(int *_start, int *_end) const
		{
			*_start = start;
			*_end = end;
		}
		// We can have only start or end alarm
		bool isDefined(void) const
		{
			return ((start != -1) || (end != -1));
		}
		virtual String print() const
		{
			return "Start: " + toString(start) + ", end: " + toString(end);
		}
	protected:
		int start = -1;
		int end = -1;

	private:
		String toString(int val) const
		{
			char temp[20];
			if (val == -1)
				strcpy(temp, "(-1)");
			else
				sprintf(temp, "%d.%02d (%d)", val / 60, val % 60, val);
			return String(temp);
		}
};

/**
 * Alarm property.
 * An alarm contain:
 * - ID: unique identifier
 * - range: start and end
 * - active: true if the alarm is active (can do an action)
 * - action: the callback to make action
 * - param: the parameter to pass to the action
 */
class Alarm_Property : public Alarm_Range
{
	public:
		Alarm_Property(size_t id) : ID(id) { }
		~Alarm_Property() { }

		size_t getID(void) const
		{
			return ID;
		}

		void setAction(const AlarmFunction_t &pAlarmAction, int Param)
		{
			action = pAlarmAction;
			param = Param;
		}

		bool getActive(void) const
		{
			return active;
		}

		void setActive(bool Active)
		{
			active = Active;
		}

		bool getOnlyOne(void) const
		{
			return only_One;
		}

		void setOnlyOne(bool one)
		{
			only_One = one;
		}

		void setParam(int Param)
		{
			param = Param;
		}

		// Alarm is activated if time in [start, end[
		bool IsAlarmActivated(int time) const
		{
			if (start != -1)
			{
				if (time >= start)
				{
					// If end is defined else until end of day
					if (end != -1)
						return (time < end);
					else
						return true;
				}
				else
					return false;
			}
			else
			{
				// end must be defined
				return (time < end);
			}
		}

		// Is the time of an alarm
		inline bool IsAlarmTime(int time) const
		{
			return ((start == time) || (end == time));
		}

		inline bool IsStartTime(int time) const
		{
			return (start == time);
		}

		inline bool IsEndTime(int time) const
		{
			return (end == time);
		}

		virtual String print(void) const
		{
			return "Alarm ID: " + (String) ID + ", " + Alarm_Range::print();
		}

		/**
		 * Execute the action associated to an alarm. Conditions are:
		 * Action must exist
		 * Alarm is activated
		 * Send true or false according time = start or end of the alarm
		 * else send true only if time is in the range [start, end[
		 */
		void DoAction(int time) const
		{
			if (active && (action != NULL))
			{
				if (IsAlarmTime(time))
					action(ID, IsStartTime(time), param);
				else
				{
					if (IsAlarmActivated(time))
						action(ID, true, param);
					else
						if (time > end)
							action(ID, false, param);
				}
			}
		}

	private:
		size_t ID = 0;                  // Unique identifier
		bool active = true;             // Is the alarm active ?
		bool only_One = false;          // If true then suppress alarm after action
		AlarmFunction_t action = NULL;  // The action to do
		int param = 0;                  // The parameter to pass to the action
};

/**
 * Structure that contains the time of the alarm referenced by its identifier
 */
typedef struct
{
		uint16_t time;
		const Alarm_Property *alarm;
} TimeAlarmLimit_typedef;

typedef std::vector<Alarm_Property> AlarmList;
typedef std::vector<TimeAlarmLimit_typedef> TimeActionList;

/**
 * Class that maintain a list of alarm
 */
class Alarm_Minute
{
	public:
		Alarm_Minute();
		~Alarm_Minute();

		size_t size(void)
		{
			return _alarm.size();
		}

		void setState(size_t idAlarm, bool state);
		bool getState(size_t idAlarm);
		void toggleState(size_t idAlarm);
		String getAllState(void);

		Alarm_Property* getAlarmByID(size_t id);

		void updateTime(int _time = -1);
		void updateList(void)
		{
			UpdateTimeList(false);
		}

		int add(int start, int end, const AlarmFunction_t &pAlarmAction, int param, bool updateTimeList = true);
		int add_one(int start, int end, const AlarmFunction_t &pAlarmAction, int param, bool updateTimeList = true);
		int add_one(int end, const AlarmFunction_t &pAlarmAction, int param, bool updateTimeList = true);

		bool getRange(size_t idAlarm, int *start, int *end);
		void getRange(size_t idAlarm, String &start, String &end);
		void deleteAlarm(size_t idAlarm, bool updateTimeList = true);
		void updateAlarm(size_t idAlarm, int start, int end, bool updateTimeList = true);

		void printAlarm(void);

		String toString(int time, bool withtime = false) const;

	protected:
		AlarmList _alarm;        // The list of alarm
		TimeActionList _time;    // The list of time (minute of the day) with an alarm
		int currentTime;         // The current time in minute of the actual day
		size_t idTime;           // The current index in the list of time action
		bool isTimeInitialized;  // Is currentTime initialized ?

	private:
		size_t unique_ID;  // One ID to reference one alarm
		int busy = 0;      // Busy flag counter

		bool CheckMinuteRange(int minute);
		void UpdateTimeList(bool checkAlarm);
		void UpdateNextAlarm(bool updateLastAlarm);
		void CheckAlarmTime(void);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ALARM)
extern Alarm_Minute Alarm;
#endif
