#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <initializer_list>
#include <vector>

// To create a basic task to update Relay every 1 minute
#ifdef RELAY_USE_TASK
#define RELAY_DATA_TASK(start)	{(start), "RELAY_Task", 1024, 2, 60000, CoreAny, RELAY_Task_code}
void RELAY_Task_code(void *parameter);
#else
#define RELAY_DATA_TASK(start)	{}
#endif

typedef struct
{
		int start = -1;
		int end = -1;
} Alarm_typedef;

typedef struct
{
		uint8_t idRelay;
		uint8_t gpio;
		bool hasAlarm;
		Alarm_typedef alarm1;
		Alarm_typedef alarm2;
		bool state;
} Relay_typedef;

typedef struct
{
		uint16_t time;
		uint8_t idRelay;
} TimeAlarm_typedef;

typedef enum
{
	Alarm1 = 1,
	Alarm2
} AlarmNumber;

typedef std::vector<Relay_typedef> RelayList;
typedef std::vector<TimeAlarm_typedef> TimeList;

class Relay_Class
{
	public:
		Relay_Class();
		Relay_Class(const std::initializer_list<uint8_t> gpios);
		~Relay_Class();

		void add(uint8_t gpio);
		void updateTime(int _time = -1);

		size_t size(void)
		{
			return _relay.size();
		}

		void setState(uint8_t idRelay, bool state);
		bool getState(uint8_t idRelay);
		String getAllState(void);

		/**
		 * Return true if one relay is ON, false if all relay are OFF
		 */
		bool IsOneRelayON(void)
		{
			return (relaisOnCount > 0);
		}

		bool hasAlarm(uint8_t idRelay);
		void addAlarm(uint8_t idRelay, AlarmNumber num, int begin, int end);
		bool getAlarm(uint8_t idRelay, AlarmNumber num, int *begin, int *end);
		void getAlarm(uint8_t idRelay, AlarmNumber num, String &begin, String &end);

	protected:
		RelayList _relay;
		TimeList _time;
		int currentTime;
		int idTime;
		bool isTimeInitialized;
		int relaisOnCount;
	private:
		void UpdateTimeList(void);
		void UpdateNextAlarm(void);
		void CheckAlarmTime(void);
};
