#pragma once
#ifndef __PV_DATA_H
#define __PV_DATA_H

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"

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
				float latitude;  // Nord positif
				float longitude; // Est négatif de Greenwitch
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

typedef enum
{
	pvLatitude,  // Nord positif
	pvLongitude, // Est négatif de Greenwitch
	pvAltitude,

	// Installation
	pvOrientation,
	pvInclinaison,
	pvPV_Puissance,

	// Masques
	pvMask_Matin,
	pvMask_Soir,

	// Module
	pvCoeff_Puissance,
	pvNoct,

	// Onduleur
	pvOnd_PowerACMax,
	pvOnd_Rendement,

	// End of data
	pvMAXDATA
} PVData_Enum;

class EmulPV_Class
{
	public:
		EmulPV_Class();

		void Init_From_Array(const PVSite_Struct &data);
		void Init_From_File(const char *file);
		void Init_From_IniData(IniFiles &init_file);
		void Save_To_File(IniFiles &init_file);
		void Set_DayParameters(TAngstromCoeff ang, double temp, double hr);

		TDateTime getSunRise(bool toLocalTime);
		TDateTime getSunSet(bool toLocalTime);
		TDateTime getSunTransit(TDateTime day, bool toLocalTime);
		char *SunRise_SunSet(void);
		void setDateTime(void);

		float getData(PVData_Enum data);
		String getData_str(PVData_Enum data);
		TAngstromCoeff getAngstromCoeff(void) const
		{
			return PV_Site.Angstrom;
		}
		double getDayTemperature(void) const
		{
			return PV_Site.Temperature;
		}

		double Irradiance(TDateTime aDateSun);
		double Power(TDateTime aDateSun);
		double Compute_Power_TH(uint32_t daytime_s, bool summer_hour);

	private:
		TGPSPosition PV_GPS;
		TSite PV_Site;
		PVSite_Struct PV_Data;

		double MaskMatin = 0, MaskSoir = 0;
		bool Mask = false;

		// Energie solaire du jour
		double ESolarDay = 0;
		TDateTime SunRise = 0, SunSet = 0;

		double PV_Puissance;

		double PV_NOCT, PV_TempCoeffPuissance;
		double Ond_PuissanceACMax, Ond_Rendement;

		double Rendement;
		bool UsePOnduleurACMax = false;

		TDateTime CurrentDay = 0;
		char Sun_Rise_Sun_Set[20] = {0};

		void Day_Init();
		void SetRendement(void);
		double ComputeCellTemperature(double aTAmbiante, double aIrradiance);
};


#endif /* __PV_DATA_H */