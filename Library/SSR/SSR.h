#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include <Arduino.h>

typedef enum
{
	SSR_OFF = 0,
	SSR_ON = 1,
	SSR_ON_ACTIF = 2
} SSR_State_typedef;

typedef enum
{
	SSR_LED_OFF = 0,
	SSR_LED_ON = 1
} SSR_Led_State_typedef;

typedef enum
{
	SSR_Action_OFF,
	SSR_Action_FULL,
	SSR_Action_Percent,
	SSR_Action_Surplus,
	SSR_Action_Dimme
} SSR_Action_typedef;

/**
 * Definition of task for Boost and Dump operations
 */
#ifdef SSR_USE_TASK
// Task to boost the SSR (Full load) for 1 hour
#define SSR_BOOST_TASK	{condSuspended, "SSR_BOOST_Task", 4096, 3, 3600 * 1000, Core0, SSR_Boost_Task_code}
void SSR_Boost_Task_code(void *parameter);

// Task to compute dump power
#define SSR_DUMP_TASK	{condSuspended, "SSR_DUMP_Task", 4096, 3, 150, Core0, SSR_Dump_Task_code}
void SSR_Dump_Task_code(void *parameter);

extern void onDumpComputed(float power);
#else
#define SSR_BOOST_TASK {}
#define SSR_DUMP_TASK	{}
#endif

/**
 * Functions definition
 * - Action to do for the SSR, see SSR_Action_typedef
 * - Action for the led
 */
typedef void (*Gestion_SSR_TypeDef)(const float, const float);
typedef void (*SetLedPinValue_Typedef)(const uint16_t);

/**
 * SSR functions
 */
void SSR_Initialize(uint8_t ZC_Pin, uint8_t SSR_Pin, int8_t LED_Pin = -1);
void SSR_SetLedPinValue(const SetLedPinValue_Typedef &fonc = NULL);
float SSR_Compute_Dump_power(float default_Power = 0.0);

void SSR_Set_Action(SSR_Action_typedef do_action, bool restart = false);
SSR_Action_typedef SSR_Get_Action(void);

SSR_State_typedef SSR_Get_State(void);
void SSR_Enable(void);
void SSR_Disable(void);

void SSR_Set_Dump_Power(float dump);
float SSR_Get_Dump_Power(void);

void SSR_Set_Target(float target);
float SSR_Get_Target(void);

void SSR_Set_Percent(float percent);
float SSR_Get_Percent(void);
float SSR_Get_Current_Percent(void);

void SSR_Set_Dimme_Target(float target);
float SSR_Get_Dimme_Target(void);

// Fonction pour avoir un top zéro cross
#if defined(ZERO_CROSS_TOP_Xms)
uint32_t ZC_Get_Count(void);
bool ZC_Top_Xms(void);
#endif

