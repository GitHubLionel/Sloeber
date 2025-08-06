#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <initializer_list>
#include <vector>

// To create a basic task to update Relay with a callback
#ifdef RELAY_USE_CB_TASK
// Task to update relay according condition in a callback
#define RELAY_CB_DATA_TASK(start, delay)	{start, "RELAY_CB_Task", 1024 * 4, 5, delay, CoreAny, RELAY_CB_Task_code}
void RELAY_CB_Task_code(void *parameter);
#else
#define RELAY_CB_DATA_TASK(start, delay) {}
#endif

/**
 * Relay structure
 */
typedef struct
{
		uint8_t idRelay;    // The id of the relay
		uint8_t gpio;       // The gpio that command the relay
		bool active = true; // Is the state of relay can be change ?
		bool state = false; // The state of the relay. True = relay ON, False = relay OFF
		int IDAlarm1 = -1;  // The first alarm ID to command the relay
		int IDAlarm2 = -1;  // The second alarm ID to command the relay
} Relay_typedef;

typedef std::initializer_list<uint8_t> RelayGPIOList;
typedef std::vector<Relay_typedef> RelayList;

// Callback when a relay change
typedef void (*Relay_on_before_change_cb)(uint8_t id, bool new_state, bool *accept);
typedef void (*Relay_on_after_change_cb)(uint8_t id, bool new_state);

class Relay_Class
{
	public:
		Relay_Class();
		Relay_Class(const RelayGPIOList &gpios);
		~Relay_Class();

		void Initialize(const RelayGPIOList &gpios);
		void add(uint8_t gpio);

		size_t size(void) const
		{
			return _relay.size();
		}

		Relay_typedef* getRelay(uint8_t idRelay)
		{
			if (idRelay < _relay.size())
				return &_relay[idRelay];
			else
				return NULL;
		}

		void setActive(uint8_t idRelay, bool active);
		bool getActive(uint8_t idRelay) const;

		void setState(uint8_t idRelay, bool state);
		bool getState(uint8_t idRelay) const;
		void toggleState(uint8_t idRelay);
		String getAllState(void) const;

		/**
		 * Return true if one relay is ON, false if all relay are OFF
		 */
		bool IsOneRelayON(void) const
		{
			return (relaisOnCount > 0);
		}

		void setOnBeforeChangeCallback(const Relay_on_before_change_cb &callback)
		{
			_relay_on_before_change_cb = callback;
		}

		void setOnAfterChangeCallback(const Relay_on_after_change_cb &callback)
		{
			_relay_on_after_change_cb = callback;
		}

	protected:
		RelayList _relay;    // The list of relay
		int relaisOnCount;   // Number of relay in state ON

	private:
		Relay_on_before_change_cb _relay_on_before_change_cb = NULL;
		Relay_on_after_change_cb _relay_on_after_change_cb = NULL;
};
