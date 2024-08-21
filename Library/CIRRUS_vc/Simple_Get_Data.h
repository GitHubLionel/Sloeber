#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include "CIRRUS.h"

/**
 * Un exemple de gestion des données extraites du Cirrus
 * On traite le cas où on a un ou deux Cirrus.
 * Pour le CS5480, on a également la lecture du deuxième channel
 * Lors d'un projet particulier, dupliquer cette unité en supprimant "Simple_"
 * Ne pas oublier d'exclure du build "Simple_Get_Data.c"
 */

// To create a basic task to check Cirrus data every 200 ms
#ifdef CIRRUS_USE_TASK
#define CIRRUS_DATA_TASK(start)	{(start), "CIRRUS_Task", 4096, 5, 200, CoreAny, CIRRUS_Task_code}
void CIRRUS_Task_code(void *parameter);
#endif

// Pour la gestion du SSR
//#define USE_SSR

#define SIZE_RMS_DATA  4
#ifdef CIRRUS_CS5480
#define SIZE_LOG_DATA  (SIZE_RMS_DATA*4 + 4 + 2)
#else
#define SIZE_LOG_DATA  (SIZE_RMS_DATA*2 + 4 + 2)
#endif

typedef struct
{
  float Voltage;
  float Current;
  float Power;
  float Energy;
} RMS_Data;

typedef struct
{
  uint32_t time_s;
  union
  {
    float Tab[SIZE_LOG_DATA];
    struct
    {
      RMS_Data Cirrus1_ch1;
#ifdef CIRRUS_CS5480
      RMS_Data Cirrus1_ch2;
#endif
      RMS_Data Cirrus2_ch1;
#ifdef CIRRUS_CS5480
      RMS_Data Cirrus2_ch2;
#endif

      // Extra
      float Cirrus1_PF;
      float Cirrus1_Freq;
      float Cirrus2_PF;
      float Cirrus2_Freq;
     
      // Températures
      float Cirrus1_Temp;
      float Cirrus2_Temp;
    };
  };
} Simple_Data_Struct;

/**
 * La liste des indices pour Log_Data_Struct
 * Permet de sélectionner les paramètres qu'on veut exporter dans le log
 */
typedef enum
{
  C1_ch1_Urms,
  C1_ch1_Irms,
  C1_ch1_Power,
  C1_ch1_Energy,
#ifdef CIRRUS_CS5480
  C1_ch2_Urms,
  C1_ch2_Irms,
  C1_ch2_Power,
  C1_ch2_Energy,
#endif
  C2_ch1_Urms,
  C2_ch1_Irms,
  C2_ch1_Power,
  C2_ch1_Energy,
#ifdef CIRRUS_CS5480
  C2_ch2_Urms,
  C2_ch2_Irms,
  C2_ch2_Power,
  C2_ch2_Energy,
#endif
  C1_PF,
  C1_Freq,
  C2_PF,
  C2_Freq,
  C1_Temp,
  C2_Temp,
  DATA_MAX
} Enum_Log_Data;

void Simple_Get_Data(void);
void Simple_Update_IHM(const char *first_text, const char *last_text);

