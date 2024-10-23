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

// Fonction d'action
typedef void (*Gestion_SSR_TypeDef)(void);

void SSR_Initialize(uint8_t ZC_Pin, uint8_t SSR_Pin, int8_t LED_Pin = -1);
float SSR_Compute_Dump_power(float default_Power = 0.0);

void SSR_Action(SSR_Action_typedef do_action, bool restart = false);
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

