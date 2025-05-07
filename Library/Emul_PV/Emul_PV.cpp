#include "Emul_PV.h"
#include <stdio.h>
#include <math.h>
#include "rayonnement.h"
#include "RTCLocal.h"
#include "Debug_utils.h"

EmulPV_Class::EmulPV_Class()
{
	// default value
	PV_NOCT = 44;
	PV_TempCoeffPuissance = -0.41;
	Ond_Rendement = 95.7;
	Ond_PuissanceACMax = 1;
	PV_Puissance = 1;
	SetRendement();
}

/**
 * Initialisation à partir d'un fichier de type ini.
 * Si le fichier n'existe pas, il est créé.
 */
void EmulPV_Class::Init_From_File(const char *file)
{
	IniFiles init_file = IniFiles(file);
	bool exist = init_file.Begin(true);

	Init_From_IniData(init_file);

	if (!exist)
		init_file.SaveFile("");
}

/**
 * Initialisation à partir d'un fichier de type ini déjà ouvert en mémoire.
 */
void EmulPV_Class::Init_From_IniData(IniFiles &init_file)
{
	PVSite_Struct dataPV;

	dataPV.latitude = init_file.ReadFloat("GPS", "Latitude", 43.49333333);
	dataPV.longitude = init_file.ReadFloat("GPS", "Longitude", -6.36166666);

	dataPV.altitude = init_file.ReadFloat("GPS", "Altitude", 209);
	dataPV.orientation = init_file.ReadFloat("GPS", "Orientation", 0);
	dataPV.inclinaison = init_file.ReadFloat("GPS", "Inclinaison", 5);

	dataPV.PV_puissance = init_file.ReadFloat("PV", "Puissance", 1000);
	dataPV.PV_Coeff_Puissance = init_file.ReadFloat("PV", "Coeff_Puissance", -0.41);
	dataPV.PV_Noct = init_file.ReadFloat("PV", "NOCT", 44);
	dataPV.Ond_PowerACMax = init_file.ReadFloat("PV", "PowerACMax", 1000);
	dataPV.Ond_Rendement = init_file.ReadFloat("PV", "Rendement", 95.7);

	dataPV.Mask_Matin = init_file.ReadFloat("MASK", "Mask_Matin", 0);
	dataPV.Mask_Soir = init_file.ReadFloat("MASK", "Mask_Soir", 0);

	Init_From_Array(dataPV);
}

void EmulPV_Class::Save_To_File(IniFiles &init_file)
{
	init_file.WriteFloat("GPS", "Latitude", PV_Data.latitude, "");
	init_file.WriteFloat("GPS", "Longitude", PV_Data.longitude, "");

	init_file.WriteFloat("GPS", "Altitude", PV_Data.altitude, "");
	init_file.WriteFloat("GPS", "Orientation", PV_Data.orientation, "");
	init_file.WriteFloat("GPS", "Inclinaison", PV_Data.inclinaison, "");

	init_file.WriteFloat("PV", "Puissance", PV_Data.PV_puissance, "");
	init_file.WriteFloat("PV", "Coeff_Puissance", PV_Data.PV_Coeff_Puissance, "");
	init_file.WriteFloat("PV", "NOCT", PV_Data.PV_Noct, "");
	init_file.WriteFloat("PV", "PowerACMax", PV_Data.Ond_PowerACMax, "");
	init_file.WriteFloat("PV", "Rendement", PV_Data.Ond_Rendement, "");

	init_file.WriteFloat("MASK", "Mask_Matin", PV_Data.Mask_Matin, "");
	init_file.WriteFloat("MASK", "Mask_Soir", PV_Data.Mask_Soir, "");

	init_file.SaveFile("");
}

/**
 * Initialiation des données du site : position, caractéristiques, ...
 * Les données sont transmises dans une structure tableau de dimension 12 qui doit contenir dans l'ordre
 * GPS : Latitude (Nord), Longitude (négatif à l'Est du méridien de Greenwitch), Altitude
 * Installation : Orientation, Inclinaison, Puissance totale Wc
 * Masque : MaskMatin, MaskSoir (au format DateTime, 0 si pas de masque)
 * Module : coefficient de perte de puissance en température (-0.45 par défaut), NOCT (45 par défaut)
 * Onduleur : la puissance AC max de l'onduleur pour l'écrétage, rendement onduleur (pourcentage)
 * Exemple PV_data = {{43.49333333, -6.36166666, 209, 0, 5, 800, 0, 0, -0.41, 44, 700, 95.7}};
 */
void EmulPV_Class::Init_From_Array(const PVSite_Struct &data)
{
	PV_Data = data;

	PV_GPS.Latitude = data.Tab[0];
	PV_GPS.Longitude = data.Tab[1];

	PV_GPS.Altitude = data.Tab[2];
	PV_GPS.Orientation = data.Tab[3];
	PV_GPS.Inclinaison = data.Tab[4];

	PV_Site.GPS = PV_GPS;
	PV_Site.MaskMatin = DoubleToDateTime(data.Tab[6]);
	PV_Site.MaskSoir = DoubleToDateTime(data.Tab[7]);
	PV_Site.UseMask = false;
	PV_Site.Temperature = 25;
	PV_Site.HR = 0.75;
	PV_Site.Angstrom = acBleuPur;

	PV_Puissance = data.Tab[5];
	PV_TempCoeffPuissance = data.Tab[8] / 100.0;
	PV_NOCT = data.Tab[9];
	if (PV_TempCoeffPuissance == 0)
		PV_TempCoeffPuissance = -0.45 / 100.0; // Estimation moyenne
	if (PV_NOCT == 0)
		PV_NOCT = 45;     // Estimation moyenne
	Ond_PuissanceACMax = data.Tab[10];
	Ond_Rendement = data.Tab[11];

	SetRendement();
}

/**
 * Initialisation des paramètres du jour du site
 * 	ang : coefficient d'Angstrom (couleur du ciel)
 * 	temp : température
 * 	hr : humidité relative en pourcentage (0.75 par défaut)
 */
void EmulPV_Class::Set_DayParameters(TAngstromCoeff ang, double temp, double hr)
{
	PV_Site.HR = hr;
	if (PV_Site.HR <= 0)
		PV_Site.HR = 0.01;
	else
		if (PV_Site.HR > 1)
			PV_Site.HR = 1;

	PV_Site.Temperature = temp;
	PV_Site.Angstrom = ang;
}

/**
 * Initialise les données du jour commune (mask, énergie solaire)
 */
void EmulPV_Class::Day_Init()
{
	// Mask
	Mask = (PV_Site.MaskMatin != 0) || (PV_Site.MaskSoir != 0) || (PV_Site.UseMask);
	if (Mask)
	{
		MaskMatin = frac(PV_Site.MaskMatin);
		MaskSoir = frac(PV_Site.MaskSoir);
	}

	// L'énergie du jour
	ESolarDay = ESol(DayOfTheYear(CurrentDay));

	// Le lever et coucher du Soleil
	SunRise = Sun_Hour_GPS(CurrentDay, PV_GPS, sun_rise);
	SunSet = Sun_Hour_GPS(CurrentDay, PV_GPS, sun_set);
}

/**
 * Renvoie l'heure du lever du Soleil à l'heure solaire ou local
 */
TDateTime EmulPV_Class::getSunRise(bool toLocalTime)
{
	TDateTime dt = SunRise;
	if (toLocalTime)
		SunHourToLocalTime(&dt);
	return dt;
}

/**
 * Renvoie l'heure du coucher du Soleil à l'heure solaire ou local
 */
TDateTime EmulPV_Class::getSunSet(bool toLocalTime)
{
	TDateTime dt = SunSet;
	if (toLocalTime)
		SunHourToLocalTime(&dt);
	return dt;
}

/**
 * Renvoie l'heure du transit (midi) du Soleil à l'heure solaire ou local
 */
TDateTime EmulPV_Class::getSunTransit(TDateTime day, bool toLocalTime)
{
	TDateTime dt = Sun_Hour_GPS(day, PV_GPS, sun_transit);
	if (toLocalTime)
		SunHourToLocalTime(&dt);
	return dt;
}

/**
 * Renvoie une chaine contenant l'heure de lever et coucher du Soleil
 * La chaine SunRise_SunSet doit être définie avec 20 caractères
 */
char* EmulPV_Class::SunRise_SunSet(void)
{
	char rise[9], set[9];

	DateTimeToTimeStr(getSunRise(true), rise);
	DateTimeToTimeStr(getSunSet(true), set);
	sprintf(Sun_Rise_Sun_Set, "%s - %s", rise, set);
	return Sun_Rise_Sun_Set;
}

/**
 * Renvoie la data
 */
float EmulPV_Class::getData(PVData_Enum data)
{
	return PV_Data.Tab[data];
}

String EmulPV_Class::getData_str(PVData_Enum data)
{
	char buffer[10] = {0};

	if ((data == pvLatitude) || (data == pvLongitude))
		sprintf(buffer, "%.6f", PV_Data.Tab[data]);
	else
		sprintf(buffer, "%.2f", PV_Data.Tab[data]);

	return (String) buffer;
}

/**
 * Renvoie la date du jour au format TDateTime
 */
void EmulPV_Class::setDateTime(void)
{
	uint8_t day, month, year;
	TDateTime Date;

	RTC_Local.getDate(&day, &month, &year);
	EncodeDate(2000 + year, month, day, &Date);
	CurrentDay = Date;
	Day_Init();
}

/**
 * Calcul de l'irradiance dans le plan du capteur pour la date-heure donnée à l'heure solaire
 * Les données du jour doivent être initialisées avant (appel de PV_Day_Init)
 * Si on a une heure locale, on doit d'abord la convertir en heure solaire (appel de LocalTimeToSunHour)
 */
double EmulPV_Class::Irradiance(TDateTime aDateSun)
{
	double hauteur, azimuth;
	double lRSDirect;
	bool masked = false;
	double lMaskTime;
	double result = 0.0;

	if ((aDateSun <= SunRise) || (aDateSun >= SunSet))
		return 0.0;

	Sun_Position_Horizontal(aDateSun, PV_Site.GPS.Latitude, PV_Site.GPS.Longitude, &hauteur, &azimuth);
	if (hauteur > 0)
	{
		// Calcul du diffus
		result = RSDiffus(hauteur, PV_Site.GPS.Inclinaison);

		// Calcul du direct
		if (Mask)
		{
			lMaskTime = frac(aDateSun);
			if ((lMaskTime < MaskMatin) || (lMaskTime > MaskSoir))
				masked = true;

			if (PV_Site.UseMask && (!masked) && (GetHauteurMask(azimuth) > hauteur))
				masked = true;
		}
		if (!masked)
		{
			lRSDirect = RSDirect(ESolarDay, hauteur, PV_Site.GPS.Altitude, PV_Site.Angstrom, PV_Site.Temperature, PV_Site.HR);
			result += lRSDirect
					* Coefficient_Incidence(hauteur, azimuth, PV_Site.GPS.Inclinaison,
							PV_Site.GPS.Orientation);
		}

		if (result < 0)
			result = 0.0;
	}
	return result;
}

// Rendement
// Rendement du module = Puissance/Surface/1000 (car STC 1000 W/m²) : Sans dimension
// Rendement total = Rendement module * Rendement onduleur          : Sans dimension
// Irradiance                                                       : [W/m²]
// Puissance = Irradiance * Rendement total * Surface totale        : [W/m²] * [m²] = [W]
// La surface totale = Surface un module * Nb module
// On voit qu'on peut simplifier par la surface d'un module
// Puissance = Irradiance * (Puissance/1000) * (Rendement onduleur) * Nb module
void EmulPV_Class::SetRendement(void)
{
	Rendement = PV_Puissance / 1000.0; // Ici Puissance = Puissance module * Nb module
//  if FRendement = 0 then
//    FRendement := (145 / 1000) * NbModule;  // Estimation moyenne de 145 Wc/m²
	Rendement = Rendement * (Ond_Rendement / 100.0);
}

/**
 * Calcul de la température cellule basé sur le NOCT
 */
double EmulPV_Class::ComputeCellTemperature(double aTAmbiante, double aIrradiance)
{
	return aTAmbiante + aIrradiance / 800.0 * (PV_NOCT - 20.0);
}

/**
 * Calcul de la puissance théorique de l'installation pour une date-heure donnée à l'heure solaire
 * Les données du jour doivent être initialisées avant (appel de PV_Day_Init)
 * Si on a une heure locale, on doit d'abord la convertir en heure solaire (appel de LocalTimeToSunHour)
 */
double EmulPV_Class::Power(TDateTime aDateSun)
{
	double _Irradiance, TempCoeff, Puissance;

	_Irradiance = Irradiance(aDateSun);
	if (_Irradiance == 0.0)
		return 0.0;

	TempCoeff = 1.0 + PV_TempCoeffPuissance * (ComputeCellTemperature(PV_Site.Temperature, _Irradiance) - 25.0);
	Puissance = _Irradiance * Rendement * TempCoeff;
	if (UsePOnduleurACMax && (Puissance > Ond_PuissanceACMax))
		Puissance = Ond_PuissanceACMax;

	return Puissance;
}

/**
 * Calcul de la puissance théorique à une heure donnée (en seconde) du jour courant
 */
double EmulPV_Class::Compute_Power_TH(uint32_t daytime_s)
{
	TDateTime dt;
	int32_t time_s = daytime_s;
	double power = last_power;

	if (daytime_s != last_daytime_s)
	{
		// Décalage heure solaire d'été
		time_s -= 3600 * GLOBAL_SUMMER_HOUR;

		if (time_s > 0)
		{
			dt = IncSecond(CurrentDay, time_s);
			power = Power(dt);
		}
		else
			power = 0.0;
		last_daytime_s = daytime_s;
		last_power = power;
	}

	return power;
}

// ********************************************************************************
// End of file
// ********************************************************************************
