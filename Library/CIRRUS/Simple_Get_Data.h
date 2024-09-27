/**
 * A simple example of a get data process
 * You must define CIRRUS_SIMPLE_IS_CS5490 to true or false following your Cirrus
 * Only channel 1 is read
 * Don't forget to exclude from build "Simple_Get_Data.cpp" if you don't use this file
 */
#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include "CIRRUS.h"

/**
 * La structure pour le log dans le fichier
 * Données moyennées toutes les x minutes
 */
typedef struct
{
		float Voltage;
		float Power_ch1;
		float Temp;
} Graphe_Data;

/**
 * La structure pour les données instantannées
 */
typedef struct
{
		uint32_t time_s;

		RMS_Data Cirrus_ch1;

		// Extra
		float Cirrus_PF;
		float Cirrus_Freq;

		// Températures
		float Cirrus_Temp;

} Simple_Data_Struct;

void Simple_Set_Cirrus(const CIRRUS_Base &cirrus);
void Simple_Get_Data(void);
uint8_t Simple_Update_IHM(const char *first_text, const char *last_text, bool display = true);

