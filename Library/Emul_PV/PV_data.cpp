#include "PV_data.h"
#include <stdio.h>
#include <math.h>
#include "rayonnement.h"
#include "RTCLocal.h"

static TGPSPosition PV_GPS;
static TSite PV_Site;

static double MaskMatin, MaskSoir;
static bool Mask = false;

// Energie solaire du jour
static double ESolarDay;
static TDateTime SunRise, SunSet;

static double PV_Puissance;

static double PV_NOCT, PV_TempCoeffPuissance;
static double Ond_PuissanceACMax, Ond_Rendement;

static double Rendement;
bool UsePOnduleurACMax = false;

// Private functions
void SetRendement(void);
double ComputeCellTemperature(double aTAmbiante, double aIrradiance);

//PV_data = {{43.49333333, -6.36166666, 209, 0, 5, 800, 0, 0, -0.41, 44, 700, 95.7}};
/**
 * Initialisation à partir d'un fichier de type ini.
 * Si le fichier n'existe pas, il est créé.
 */
void PV_Init_From_File(const char *file, PVSite_Struct *PV_data)
{
	IniFiles init_file = IniFiles(file);
	bool exist = init_file.Begin(true);

	PV_Init_From_IniData(PV_data, init_file);

	if (!exist)
		init_file.SaveFile("");
}

/**
 * Initialisation à partir d'un fichier de type ini déjà ouvert en mémoire.
 */
void PV_Init_From_IniData(PVSite_Struct *PV_data, IniFiles &init_file)
{
	PV_data->latitude = init_file.ReadFloat("GPS", "Latitude", 43.49333333);
	PV_data->longitude = init_file.ReadFloat("GPS", "Longitude", -6.36166666);
	PV_data->altitude = init_file.ReadFloat("GPS", "Altitude", 209);
	PV_data->orientation = init_file.ReadFloat("GPS", "Orientation", 0);
	PV_data->inclinaison = init_file.ReadFloat("GPS", "Inclinaison", 5);

	PV_data->PV_puissance = init_file.ReadFloat("PV", "Puissance", 1000);
	PV_data->PV_Coeff_Puissance = init_file.ReadFloat("PV", "Coeff_Puissance", -0.41);
	PV_data->PV_Noct = init_file.ReadFloat("PV", "NOCT", 44);
	PV_data->Ond_PowerACMax = init_file.ReadFloat("PV", "PowerACMax", 1000);
	PV_data->Ond_Rendement = init_file.ReadFloat("PV", "Rendement", 95.7);

	PV_data->Mask_Matin = init_file.ReadFloat("MASK", "Mask_Matin", 0);
	PV_data->Mask_Soir = init_file.ReadFloat("MASK", "Mask_Soir", 0);

	PV_Init_From_Array(*PV_data);
}

void PV_Save_To_File(PVSite_Struct data, IniFiles &init_file)
{
	init_file.WriteFloat("GPS", "Latitude", data.latitude, "");
	init_file.WriteFloat("GPS", "Longitude", data.longitude, "");
	init_file.WriteFloat("GPS", "Altitude", data.altitude, "");
	init_file.WriteFloat("GPS", "Orientation", data.orientation, "");
	init_file.WriteFloat("GPS", "Inclinaison", data.inclinaison, "");

	init_file.WriteFloat("PV", "Puissance", data.PV_puissance, "");
	init_file.WriteFloat("PV", "Coeff_Puissance", data.PV_Coeff_Puissance, "");
	init_file.WriteFloat("PV", "NOCT", data.PV_Noct, "");
	init_file.WriteFloat("PV", "PowerACMax", data.Ond_PowerACMax, "");
	init_file.WriteFloat("PV", "Rendement", data.Ond_Rendement, "");

	init_file.WriteFloat("MASK", "Mask_Matin", data.Mask_Matin, "");
	init_file.WriteFloat("MASK", "Mask_Soir", data.Mask_Soir, "");

	init_file.SaveFile("");
}

/**
 * Initialiation des données du site : position, caractéristiques, ...
 * Les données sont transmises dans une structure tableau de dimension 12 qui doit contenir dans l'ordre
 * GPS : Latitude, Longitude (négatif à l'Est de Greenwitch), Altitude
 * Installation : Orientation, Inclinaison, Puissance totale Wc
 * Masque : MaskMatin, MaskSoir (au format DateTime, 0 si pas de masque)
 * Module : coefficient de perte de puissance en température (-0.45 par défaut), NOCT (45 par défaut)
 * Onduleur : la puissance AC max de l'onduleur pour l'écrétage, rendement onduleur (pourcentage)
 */
void PV_Init_From_Array(PVSite_Struct data)
{
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
void PV_Set_DayParameters(TAngstromCoeff ang, double temp, double hr)
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
void PV_Day_Init(TDateTime day)
{
	// Mask
	Mask = (PV_Site.MaskMatin != 0) || (PV_Site.MaskSoir != 0) || (PV_Site.UseMask);
	if (Mask)
	{
		MaskMatin = frac(PV_Site.MaskMatin);
		MaskSoir = frac(PV_Site.MaskSoir);
	}

	// L'énergie du jour
	ESolarDay = ESol(DayOfTheYear(day));

	// Le lever et coucher du Soleil
	SunRise = Sun_Hour_GPS(day, PV_GPS, sun_rise);
	SunSet = Sun_Hour_GPS(day, PV_GPS, sun_set);
}

/**
 * Renvoie l'heure du lever du Soleil à l'heure solaire ou local
 */
TDateTime PV_Get_SunRise(bool toLocalTime)
{
	TDateTime dt = SunRise;
	if (toLocalTime)
		SunHourToLocalTime(&dt);
	return dt;
}

/**
 * Renvoie l'heure du coucher du Soleil à l'heure solaire ou local
 */
TDateTime PV_Get_SunSet(bool toLocalTime)
{
	TDateTime dt = SunSet;
	if (toLocalTime)
		SunHourToLocalTime(&dt);
	return dt;
}

/**
 * Renvoie l'heure du transit (midi) du Soleil à l'heure solaire ou local
 */
TDateTime PV_Get_SunTransit(TDateTime day, bool toLocalTime)
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
void Sun_Rise_Set_Str(char *SunRise_SunSet)
{
	char rise[9], set[9];

	DateTimeToTimeStr(PV_Get_SunRise(true), rise);
	DateTimeToTimeStr(PV_Get_SunSet(true), set);
	sprintf(SunRise_SunSet, "%s - %s", rise, set);
}

/**
 * Renvoie la date du jour au format TDateTime
 */
TDateTime Now_DateTime(void)
{
	uint8_t day, month, year;
	TDateTime Date;

	RTC_Local.getDate(&day, &month, &year);
	EncodeDate(2000 + year, month, day, &Date);
	return Date;
}

/**
 * Calcul de l'irradiance dans le plan du capteur pour la date-heure donnée à l'heure solaire
 * Les données du jour doivent être initialisées avant (appel de PV_Day_Init)
 * Si on a une heure locale, on doit d'abord la convertir en heure solaire (appel de LocalTimeToSunHour)
 */
double PV_Irradiance(TDateTime aDateSun)
{
	double hauteur, azimuth;
	double lRSDirect;
	bool masked = false;
	double lMaskTime;
	double result = 0.0;

	if ((aDateSun < SunRise) || (aDateSun > SunSet))
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
			result += lRSDirect * Coefficient_Incidence(hauteur, azimuth, PV_Site.GPS.Inclinaison, PV_Site.GPS.Orientation);
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
void SetRendement(void)
{
	Rendement = PV_Puissance / 1000.0; // Ici Puissance = Puissance module * Nb module
//  if FRendement = 0 then
//    FRendement := (145 / 1000) * NbModule;  // Estimation moyenne de 145 Wc/m²
	Rendement = Rendement * (Ond_Rendement / 100.0);
}

double ComputeCellTemperature(double aTAmbiante, double aIrradiance)
{
	return aTAmbiante + aIrradiance / 800.0 * (PV_NOCT - 20.0);
}

/**
 * Calcul de la puissance théorique de l'installation pour une date-heure donnée à l'heure solaire
 * Les données du jour doivent être initialisées avant (appel de PV_Day_Init)
 * Si on a une heure locale, on doit d'abord la convertir en heure solaire (appel de LocalTimeToSunHour)
 */
double PV_Power(TDateTime aDateSun)
{
	double Irradiance, TempCoeff, Puissance;

	Irradiance = PV_Irradiance(aDateSun);

	TempCoeff = 1.0 + PV_TempCoeffPuissance * (ComputeCellTemperature(PV_Site.Temperature, Irradiance) - 25.0);
	Puissance = Irradiance * Rendement * TempCoeff;
	if (UsePOnduleurACMax && (Puissance > Ond_PuissanceACMax))
		Puissance = Ond_PuissanceACMax;

	return Puissance;
}

// ********************************************************************************
// End of file
// ********************************************************************************
