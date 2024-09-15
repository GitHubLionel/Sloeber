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
		float Voltage_ph1;
		float Voltage_ph2;
		float Voltage_ph3;
		float Power_ph1;
		float Power_ph2;
		float Power_ph3;
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

		// Extra
		float Cirrus1_PF;
		float Cirrus1_Freq;

		// Températures
		float Cirrus1_Temp;

} Data_Struct;

void Get_Data(void);
uint8_t Update_IHM(const char *first_text, const char *last_text, bool display = true);
bool Get_Last_Data(float *Energy, float *Surplus, float *Prod);

void onDaychange(uint8_t year, uint8_t month, uint8_t day);

