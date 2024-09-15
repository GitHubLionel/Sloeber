#pragma once

#include <Arduino.h>

typedef enum
{
	SSR_OFF = 0,
	SSR_ON = 1
} SSR_State_typedef;

typedef enum
{
	LED_SSR_OFF = 0,
	LED_SSR_ON = 1
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
float SSR_Compute_Dump_power(float default_Power);

void SSR_Action(SSR_Action_typedef do_action, bool restart = false);
void SSR_Set_Dump_Power(float dump);
float SSR_Get_Dump_Power(void);
void SSR_Set_Dimme_Target(float target);
float SSR_Get_Dimme_Target(void);

void SSR_Enable(void);
void SSR_Disable(void);

bool SSR_Set_Percent(float percent);
float SSR_Get_Percent(void);
bool SSR_Get_StateON(void);

uint32_t ZC_Get_Count(void);
bool ZC_Top200ms(void);

SSR_Action_typedef SSR_Get_Action(void);

