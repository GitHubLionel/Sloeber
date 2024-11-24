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
} Data_Struct;

void Get_Data(void);
uint8_t Update_IHM(const char *first_text, const char *last_text, bool display = true);
bool Get_Last_Data(float *Energy, float *Surplus, float *Prod);

void reboot_energy(void);
void onDaychange(uint8_t year, uint8_t month, uint8_t day);

