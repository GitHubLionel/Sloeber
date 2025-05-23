#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include "CIRRUS.h"

/**
 * Uniquement pour le Cirrus CS5480 : 2 channel
 * Dérivé du fichier Simple_Get_Data.h
 * Ne pas oublier d'exclure du build "Simple_Get_Data.c"
 */

// Pour la gestion du SSR
//#define USE_SSR

/**
 * La structure pour le log dans le fichier
 * Données moyennées toutes les x minutes
 */
typedef struct
{
		float Voltage;
		float Power_ch1;
		float Power_ch2;
		float Temp;
} Graphe_Data;

/**
 * La structure pour les données instantannées
 */
typedef struct
{
		uint32_t time_s;
		RMS_Data Cirrus_ch1;
		RMS_Data Cirrus_ch2;
		float Talema_Current = 0.0;
		float Talema_Power = 0.0;
		float Talema_Energy = 0.0;

		float energy_day_conso = 0.0;
		float energy_day_surplus = 0.0;
		float energy_day_prod = 0.0;

		// Extra data
		float DS18B20_Int = 0.0;
		float DS18B20_Ext = 0.0;
		float Prod_Th = 0.0;

		uint32_t TI_Counter = 0;
		uint32_t TI_Energy = 0;
		uint32_t TI_Power = 0;
} Data_Struct;

// To access cirrus data
extern Data_Struct Current_Data;
extern volatile Graphe_Data log_cumul;

// Task to save log every 10 s
#define LOG_DATA_TASK	{condCreate, "LOG_DATA_Task", 4096, 3, 10000, Core1, Log_Data_Task_code}
void Log_Data_Task_code(void *parameter);

void Get_Data(void);
uint8_t Update_IHM(const char *first_text, const char *last_text, bool display = true);
bool Get_Last_Data(float *Energy, float *Surplus, float *Prod);

void reboot_energy(void);
void onDaychange(uint8_t year, uint8_t month, uint8_t day);

void FillListFile(void);
void PrintListFile(void);
void GZListFile(void);

