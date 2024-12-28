#pragma once
#ifndef __PV_DATA_H
#define __PV_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "misc.h"
#include "rayonnement.h"
#include "IniFiles.h"

#define PVDATA_MAX   12

typedef union
{
		float Tab[PVDATA_MAX];
		struct
		{
				// GPS
				float latitude;
				float longitude;
				float altitude;

				// Installation
				float orientation;
				float inclinaison;
				float PV_puissance;

				// Masques
				float Mask_Matin;
				float Mask_Soir;

				// Module
				float PV_Coeff_Puissance;
				float PV_Noct;

				// Onduleur
				float Ond_PowerACMax;
				float Ond_Rendement;
		};
} PVSite_Struct;

void PV_Init_From_Array(PVSite_Struct data);
void PV_Init_From_File(const char *file, PVSite_Struct *PV_data);
void PV_Init_From_IniData(PVSite_Struct *PV_data, IniFiles &init_file);
void PV_Save_To_File(PVSite_Struct data, IniFiles &init_file);
void PV_Set_DayParameters(TAngstromCoeff ang, double temp, double hr);
void PV_Day_Init(TDateTime day);

TDateTime PV_Get_SunRise(bool toLocalTime);
TDateTime PV_Get_SunSet(bool toLocalTime);
TDateTime PV_Get_SunTransit(TDateTime day, bool toLocalTime);
void Sun_Rise_Set_Str(char *SunRise_SunSet);
TDateTime Now_DateTime(void);

double PV_Irradiance(TDateTime aDateSun);
double PV_Power(TDateTime aDateSun);

#endif /* __PV_DATA_H */
