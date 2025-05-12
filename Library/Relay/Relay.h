#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <initializer_list>
#include <vector>

// To create a basic task to update Relay every 10 secondes
#ifdef RELAY_USE_TASK
#define RELAY_DATA_TASK(start)	{start, "RELAY_Task", 1024 * 4, 2, 10000, CoreAny, RELAY_Task_code}
void RELAY_Task_code(void *parameter);
#else
#define RELAY_DATA_TASK(start)	{}
#endif

/**
 * Alarm structure for a relay
 * start: beginning of the ON status of the relay
 * end: ending of the ON status of the relay
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
} Alarm_typedef;

/**
 * Relay structure
 */
typedef struct
{
		uint8_t idRelay;
		uint8_t gpio;
		bool hasAlarm;
		bool hasAlarm2;
		Alarm_typedef alarm1;
		Alarm_typedef alarm2;
		bool state;
	public:
		bool IsAlarm1Activated(int time)
		{
			return _IsAlarmActivated(alarm1, time);
		}

		bool IsAlarm2Activated(int time)
		{
			return _IsAlarmActivated(alarm2, time);
		}

		bool IsAlarmActivated(int time)
		{
			bool activated = false;
			if (hasAlarm)
			{
				if (IsAlarm1Activated(time))
					activated = true;
				else
				{
					if (IsAlarm2Activated(time))
						activated = true;
				}
			}
			return activated;
		}

		bool FoundAlarm(int time, uint8_t *alarmNumber, bool *start, int *val)
		{
			if (hasAlarm)
			{
				if (alarm1.start == time)
				{
					*alarmNumber = 1;
					*start = true;
					*val = alarm1.start;
					return true;
				}
				if (alarm1.end == time)
				{
					*alarmNumber = 1;
					*start = false;
					*val = alarm1.end;
					return true;
				}
				if (alarm2.start == time)
				{
					*alarmNumber = 2;
					*start = true;
					*val = alarm2.start;
					return true;
				}
				if (alarm2.end == time)
				{
					*alarmNumber = 2;
					*start = false;
					*val = alarm2.end;
					return true;
				}
			}
			return false;
		}

	private:
		// Alarm is activated if time in [start, end[
		bool _IsAlarmActivated(Alarm_typedef alarm, int time)
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
} Relay_typedef;

/**
 * Structure that contains the time at which the relay referenced by its identifier has an alarm
 */
typedef struct
{
		uint16_t time;
		uint8_t idRelay;
} TimeAlarm_typedef;

enum AlarmNumber
{
	Alarm1 = 1,
	Alarm2
};

typedef std::initializer_list<uint8_t> RelayGPIOList;
typedef std::vector<Relay_typedef> RelayList;
typedef std::vector<TimeAlarm_typedef> TimeList;

class Relay_Class
{
	public:
		Relay_Class();
		Relay_Class(const RelayGPIOList &gpios);
		~Relay_Class();

		void Initialize(const RelayGPIOList &gpios);
		void add(uint8_t gpio);

		size_t size(void)
		{
			return _relay.size();
		}

		void setState(uint8_t idRelay, bool state);
		bool getState(uint8_t idRelay) const;
		void toggleState(uint8_t idRelay);
		String getAllState(void);

		/**
		 * Return true if one relay is ON, false if all relay are OFF
		 */
		bool IsOneRelayON(void) const
		{
			return (relaisOnCount > 0);
		}

		void updateTime(int _time = -1);
		bool hasAlarm(void) const;
		bool hasAlarm(uint8_t idRelay) const;
		bool addAlarm(uint8_t idRelay, AlarmNumber num, int start, int end, bool updateTimeList = true);
		bool addAlarm(uint8_t idRelay, int start1, int end1, int start2, int end2, bool updateTimeList = true);
		void deleteAlarm(uint8_t idRelay, AlarmNumber num, bool updateTimeList = true);
		bool getAlarm(uint8_t idRelay, AlarmNumber num, int *start, int *end);
		void getAlarm(uint8_t idRelay, AlarmNumber num, String &start, String &end);

		void printAlarm(void) const;

		String toString(int time, bool withtime = false) const;

	protected:
		RelayList _relay;    // The list of relay
		TimeList _time;      // The list of time (minute of the day) with an alarm (on or off a relay)
		int currentTime;     // The current time in minute of the day unit
		int idTime;          // The current index in the list of time action
		bool isTimeInitialized;  // Is currentTime initialized ?
		int relaisOnCount;       // Number of relay in state ON
	private:
		bool CheckMinuteRange(int minute);
		void UpdateTimeList(void);
		void UpdateNextAlarm(bool updateLastAlarm);
		void CheckAlarmTime(void);
};
