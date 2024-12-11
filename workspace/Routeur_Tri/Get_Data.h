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
 * Liste des phases
 */
typedef enum
{
	Phase1 = 1,
	Phase2,
	Phase3
} Phase_ID;

/**
 * La structure pour le log dans le fichier
 * Données moyennées toutes les x minutes
 */
typedef struct
{
		float Voltage_ph1;
		float Voltage_ph2;
		float Voltage_ph3;
		float Power_ph1;
		float Power_ph2;
		float Power_ph3;
		float Power_prod;
		float Temp;
} Graphe_Data;

/**
 * La structure pour les données instantannées
 */
typedef struct
{
		uint32_t time_s;

		RMS_Data Phase1; // CS5484 channel 1
		RMS_Data Phase2; // CS5484 channel 2
		RMS_Data Phase3; // CS5480 channel 1
		RMS_Data Production; // CS5480 channel 2

		// La somme des énergies des 3 phases
		float energy_day_conso = 0.0;
		float energy_day_surplus = 0.0;

		// Un raccourci sur la production
		float *energy_day_prod = &Production.Energy;
		// Extra
		float Cirrus1_PF;
		float Cirrus1_Freq;

		// Températures
		float Cirrus1_Temp;

		// Somme des puissances des 3 phases pour avoir le surplus
		float get_total_power(void)
		{
			return Phase1.ActivePower + Phase2.ActivePower + Phase3.ActivePower;
		}

} Data_Struct;

// To access cirrus data
extern Data_Struct Current_Data;
extern volatile Graphe_Data log_cumul;

// Task to save log every 10 s
#define LOG_DATA_TASK	{true, "LOG_DATA_Task", 4096, 3, 10000, Core1, Log_Data_Task_code}
void Log_Data_Task_code(void *parameter);

void Set_PhaseCE(Phase_ID phase);
Phase_ID Get_PhaseCE(void);

void Get_Data(void);
uint8_t Update_IHM(const char *first_text, const char *last_text, bool display = true);
bool Get_Last_Data(float *Energy, float *Surplus, float *Prod);

void reboot_energy(void);
void onDaychange(uint8_t year, uint8_t month, uint8_t day);

