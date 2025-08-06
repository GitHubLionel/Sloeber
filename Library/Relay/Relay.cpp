#include "Relay.h"
#include "Debug_utils.h"		  // Some utils functions for debug

#ifdef RELAY_USE_CB_TASK
#include "Tasks_utils.h"
#endif

// #define DEBUG_RELAY

// Number of minutes in a day
#define MINUTESINDAY	1440

// Check if id is correct
#define	CHECK_RELAY_SIZE(id)	if (!((id) < _relay.size())) return;
#define	CHECK_RELAY_SIZE_TRUE(id)	if (!((id) < _relay.size())) return false;

// ********************************************************************************
// Relay_Class constructor
// ********************************************************************************

Relay_Class::Relay_Class()
{
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
	relay.active = true;
	relay.IDAlarm1 = -1;
	relay.IDAlarm2 = -1;
	relay.state = false;
	pinMode(gpio, OUTPUT);
	digitalWrite(gpio, LOW);
	_relay.push_back(relay);
}

void Relay_Class::setActive(uint8_t idRelay, bool active)
{
	CHECK_RELAY_SIZE(idRelay);
	_relay[idRelay].active = active;
}

bool Relay_Class::getActive(uint8_t idRelay) const
{
	CHECK_RELAY_SIZE_TRUE(idRelay);
	return _relay[idRelay].active;
}

/**
 * Set the relay ON (state = true) or OFF (state = false)
 */
void Relay_Class::setState(uint8_t idRelay, bool state)
{
	CHECK_RELAY_SIZE(idRelay);

	// If relay is not active, return
	if (!_relay[idRelay].active)
		return;

	bool initial_State = _relay[idRelay].state;

	if (state != initial_State)
	{
		bool accept = true;
		if (_relay_on_before_change_cb)
			_relay_on_before_change_cb(idRelay, state, &accept);
		if (!accept)
			return;

		if (state)
		{
			_relay[idRelay].state = true;
			digitalWrite(_relay[idRelay].gpio, HIGH);
			relaisOnCount++;
		}
		else
		{
			_relay[idRelay].state = false;
			digitalWrite(_relay[idRelay].gpio, LOW);
			relaisOnCount--;
		}

		if (_relay_on_after_change_cb)
			_relay_on_after_change_cb(idRelay, state);

#ifdef DEBUG_RELAY
		String tmp = "Relay " + (String) idRelay + ". State: ";
		tmp += (initial_State) ? "ON" : "OFF";
		tmp += " ==> ";
		tmp += (state) ? "ON" : "OFF";
		print_debug(tmp);
#endif
	}
}

/**
 * Toggle the state of the relay: ON <-> OFF
 */
void Relay_Class::toggleState(uint8_t idRelay)
{
	CHECK_RELAY_SIZE(idRelay);
	setState(idRelay, !getState(idRelay));
}

/**
 * Get the state of the relay. Return true if ON.
 */
bool Relay_Class::getState(uint8_t idRelay) const
{
	CHECK_RELAY_SIZE_TRUE(idRelay);
	return _relay[idRelay].state;
}

String Relay_Class::getAllState(void) const
{
	String state = "";
	for (size_t i = 0; i < _relay.size(); i++)
	{
		if (!state.isEmpty())
			state += ",";
		state += (getState(i)) ? "ON" : "OFF";
	}
	return state;
}

// ********************************************************************************
// Basic Task function to update relay with a callback
// ********************************************************************************

#ifdef RELAY_USE_CB_TASK

/**
 * This functions should be redefined elsewhere with your analyse
 */
Relay_Class* __attribute__((weak)) RelayOperation_cb(uint8_t *idRelay, bool *state)
{
	return NULL;
}

void RELAY_CB_Task_code(void *parameter)
{
	Relay_Class *relay = NULL;
	uint8_t idRelay;
	bool state;

	BEGIN_TASK_CODE("RELAY_CB_Task");
	for (EVER)
	{
		relay = RelayOperation_cb(&idRelay, &state);
		if (relay != NULL)
			relay->setState(idRelay, state);
		END_TASK_CODE(false);
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
