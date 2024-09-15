#include "Relay.h"


static int8_t GPIO_Relay[RELAY_MAX];
bool State_Relay[RELAY_MAX] = {false};
uint8_t Nb_Relay = 0;

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

// ********************************************************************************
// Local initialization functions
// ********************************************************************************

/**
 * Initialisation du relais.
 * gpio : values of gpio
 */
void Relay_Initialize(uint8_t nbRelay, const uint8_t gpio[])
{
	Nb_Relay = nbRelay;
	for (int i=0; i<nbRelay; i++)
	{
		GPIO_Relay[i] = gpio[i];
		pinMode(GPIO_Relay[i], OUTPUT);
		digitalWrite(GPIO_Relay[i], LOW);
	}
}

/**
 * Set the relay ON (state = true) or OFF (state = false)
 */
void Set_Relay_State(uint8_t n, bool state)
{
	if (n >= Nb_Relay)
		return;

	if (state)
	{
		State_Relay[n] = true;
		digitalWrite(GPIO_Relay[n], HIGH);
	}
	else
	{
		State_Relay[n] = false;
		digitalWrite(GPIO_Relay[n], LOW);
	}
}

/**
 * Get the state of the relay. Return true if ON.
 */
bool Get_Relay_State(uint8_t n)
{
	if (n >= Nb_Relay)
		return false;

	return State_Relay[n];
}

// ********************************************************************************
// End of file
// ********************************************************************************
