#pragma once

#include "Arduino.h"

class JSN_SR04T
{
private:
	bool JSN_USE_UART;
	uint8_t trigPin;
	uint8_t echoPin;
	uint8_t JSN_Count = 0;
	uint8_t JSN_Data[4];

	float JSN_Distance_cm = 0.0;
	float JSN_Cumul = 0.0;
	uint16_t JSN_Cumul_Count = 0;
	float JSN_Distance_mean = 0.0;
	bool JSN_Cumul_Error = false;

	void CheckJSNMessage(void);
	void computeDist(void);
	void triggerJSN(void);

public:
	JSN_SR04T(bool use_uart);
	void Mode(bool use_uart);
	void Initialize(uint8_t trig_Pin = 1, uint8_t echo_Pin = 3);
	void askJSN(void);

	float get_Distance(void)
	{
		return JSN_Distance_cm;
	}
	String get_Distance_Str(void)
	{
		return String(JSN_Distance_cm);
	}
	float get_DistanceMean(void);
	bool get_LastError(void)
	{
		return JSN_Cumul_Error;
	}
};

