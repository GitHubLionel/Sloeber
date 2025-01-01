#include "rayonnement.h"
#include <math.h>

const char *cAngstromString[TAngstromCoeff_Size] = {"Bleu profond (B = 0,01)",
		"Bleu foncé (B = 0,02)", "Bleu pur (B = 0,035)", "Bleu (B = 0,05)", "Bleu délavé (B = 0,07)",
		"Bleu laiteux (B = 0,15)", "Bleu voilé (B = 0,25)", "Bleu blanc (B = 0,5)"};
const char *cAngstromShortString[TAngstromCoeff_Size] = {"Bleu profond", "Bleu foncé", "Bleu pur",
		"Bleu", "Bleu délavé", "Bleu laiteux", "Bleu voilé", "Bleu blanc"};
const double cAngstromCoeff[TAngstromCoeff_Size] = {0.01, 0.02, 0.035, 0.05, 0.07, 0.15, 0.25, 0.50};

TMaskPoint GlobalMask[37] = {0, 0};

// ********************************************
// Fonctions locales
// ********************************************

// Le rayonnement solaire direct arrive à traverser l'atmosphère mais subit malgré tout une atténuation de son intensité.
// Définir l'altitude du point local pour connaître la pression atmosphérique (PAtm)
// PAtm = 101325 x (1 - 2,26 x 10^-5 x z)^5,26, en Pa, où z est l'altitude en mètres
double Pression_Atm(double aAltitude)
{
	return 101325.0 * pow((1.0 - 2.26e-5 * aAltitude), 5.26);
}

// Définir la pression de vapeur saturante (Pvs), le taux moyen d'humidité relative (HR) et la pression partielle de vapeur d'eau (Pv) :
// Pvs = 2,165 x (1,098 + T / 100)^8,02, en mmHg (millimètre de mercure)
// HR moyen = 50% (0,5)
// Pv = Pvs x HR
// où T est le température de l'air en °C

double Pression_Vapeur(double aTemperature, double aHR)  // Par défaut aHR = 0.5
{
	double result = 2.165 * pow((1.098 + aTemperature / 100.0), 8.02);
	return result * aHR;
}

// Définir la masse d'air optique relative (m) d'où en découle l'épaisseur optique de Rayleigh (ER) qui détermine l'atténuation due à la diffusion :
// m = PAtm / (101325 x sin(h) + 15198,75 x (3,885 + h)^-1,253)
// ER = 1 / (0,9 x m + 9,4)
// où h est la hauteur du soleil en degrés

double MasseAirOptique(double aAltitude, double aHauteurSoleil)
{
	return Pression_Atm(aAltitude)
			/ (101325.0 * sin(DegToRad(aHauteurSoleil)) + 15198.75 * pow((3.885 + aHauteurSoleil), -1.253));
}

double EpaisseurRayleigh(double aAltitude, double aHauteurSoleil)
{
	return 1.0 / (0.9 * MasseAirOptique(aAltitude, aHauteurSoleil) + 9.4);
}

// Définir le facteur de trouble de Linke :
// TL = 2,4 + 14,6 x B + 0,4 x (1 + 2 x B) x ln(Pv)
// où B est le coefficient de trouble atmosphérique qui prend une valeur de :
// B = 0,02 pour un lieu situé en montagne
// B = 0,05 pour un lieu rural
// B = 0,10 pour un lieu urbain
// B = 0,20 pour un lieu industriel (atmosphère polluée)
// Ln est le logarithme népérien

double TroubleLinke(TAngstromCoeff aAngstrom, double aTemperature, double aHR) // Par défaut aHR = 0.5 
{
	return 2.4 + 14.6 * cAngstromCoeff[aAngstrom]
			+ 0.4 * (1.0 + 2.0 * cAngstromCoeff[aAngstrom]) * log(Pression_Vapeur(aTemperature, aHR));
}

// Le rayonnement solaire direct sur un plan récepteur normal à ce rayonnement vaut donc :
// I* = ESol x e^(-ER x m x TL), en W/m²
// Il est possible de simplifier l'obtention de ISol avec la formule suivante :
// I* = ESol x e^(-TL / (0,9 + 9,4 x sin(h))), en W/m²

double RSDirect(double aESol, double aHauteurSoleil, double aAltitude, TAngstromCoeff aAngstrom,
		double aTemperature, double aHR)  // Par défaut aHR = 0.5
{
	return aESol
			* exp(-EpaisseurRayleigh(aAltitude, aHauteurSoleil) * MasseAirOptique(aAltitude, aHauteurSoleil)
							* TroubleLinke(aAngstrom, aTemperature, aHR));
}

double RSDirect_Mer(double aESol, double aHauteurSoleil, TAngstromCoeff aAngstrom,
		double aTemperature, double aHR)  // Par défaut aHR = 0.5
{
	return aESol
			* exp(-TroubleLinke(aAngstrom, aTemperature, aHR) / (0.9 + 9.4 * sin(DegToRad(aHauteurSoleil))));
}

// Le rayonnement solaire diffus arrive sur le plan récepteur après avoir été réfléchi par les
// nuages, les poussières, les aérosols et le sol. On suppose que le rayonnement solaire diffus
// n'a pas de direction prédominante (donc isotrope) de ce fait, l'orientation du plan récepteur
// n'a pas d'importance, seule son inclinaison en a. Ainsi sur un plan récepteur d'inclinaison i,
// D* est égal à :
// D* = 125 x sin(h)^0,4 x ((1 + cos(i)) / 2) + 211,86 x sin(h)^1,22 x ((1 - cos(i)) / 2), en W/m²

double RSDiffus(double aHauteurSoleil, double aInclinaison)
{
	double lSin_Haut, lCos_Inc;

	lSin_Haut = sin(DegToRad(aHauteurSoleil));
	lCos_Inc = cos(DegToRad(aInclinaison));
	return 125.0 * pow(lSin_Haut, 0.4) * ((1.0 + lCos_Inc) * 0.5)
			+ 211.86 * pow(lSin_Haut, 1.22) * ((1.0 - lCos_Inc) * 0.5);
}

// Le calcul du coefficient d'incidence est obtenu avec la formule suivante :
// CI = sin(i) x cos(h) x cos(o - a) + cos(i) x sin(h)

double Coefficient_Incidence(double aHauteurSoleil, double aAzimut, double aInclinaison,
		double aOrientation)
{
	double lSin_Inc, lCos_Inc, lSin_haut, lCos_Haut;
	double result = 0.0;

	SinCos(DegToRad(aInclinaison), &lSin_Inc, &lCos_Inc);
	SinCos(DegToRad(aHauteurSoleil), &lSin_haut, &lCos_Haut);
	result = lSin_Inc * lCos_Haut * cos(DegToRad(aOrientation - aAzimut)) + lCos_Inc * lSin_haut;
	if (result < 0.0)
		result = 0.0;
	return result;
}

// A la distance moyenne du soleil à la terre (environ 150 x 10^6 kms), une surface normale
// au rayonnement solaire (perpendiculaire à ce rayonnement) hors atmosphère reçois
// environ 1367 W/m². Cet éclairement est appelée constante solaire. Compte tenu de
// la trajectoire elliptique de la terre autour du soleil, la distance d'éloignement
// la plus grande se produisant le 3 juillet avec environ 153 x 10^6 kms et la plus
// petite se produisant le 3 janvier avec environ 147 x 10^6 kms, cette constante varie
// de +-3,4% en passant par un maximum en janvier avec environ 1413 W/m² et un minimum
// en juin avec environ 1321 W/m². L'énergie reçu en fonction du jour de l'année peut
// être calculée avec la formule suivante :
// ESol = 1367 x (1 + 0,0334 x cos(360 x (j - 2,7206) / 365,25)), en W/m²
// j étant le numéro d'ordre du jour dans l'année (1 pour le 1er janvier)

inline double ESol(int aJour)
{
	return 1367 * (1.0 + 0.0334 * cos(2 * M_PI * (aJour - 2.7206) / 365.25));
}

// ********************************************
// Fonctions publiées
// ********************************************

void FillSampleMask(void)
{
	int i;

	for (i = 0; i <= 36; i++)
	{
		GlobalMask[i].Azimut = -180 + i * 10;
		GlobalMask[i].Hauteur = 0;
	}
	for (i = 9; i <= 18; i++)
		GlobalMask[i].Hauteur = (i - 9) * 7;
	for (i = 19; i <= 27; i++)
		GlobalMask[i].Hauteur = (27 - i) * 7;
}

double GetHauteurMask(double aAzimut)
{
	int i;
	double a;

//  if (! Assigned(GlobalMask)) || (High(GlobalMask) < 2))
//    return 0.0;
//  else
	{
		i = 1;
		while ((i < sizeof(GlobalMask)) && (GlobalMask[i].Azimut < aAzimut))
			i++;
		// Interpolation linéaire : y = ax + b;  Hauteur = a.Azimut + b
		a = (GlobalMask[i].Hauteur - GlobalMask[i - 1].Hauteur)
				/ (GlobalMask[i].Azimut - GlobalMask[i - 1].Azimut);
		return a * (aAzimut - GlobalMask[i].Azimut) + GlobalMask[i].Hauteur;
	}
}

/**
 // Rayonnement solaire global à l'HEURE SOLAIRE.
 // La somme de ces deux rayonnements représente le rayonnement global :
 // G* = S* + D*
 // où S* est la valeur du rayonnement solaire direct sur un plan récepteur (o, i) et qui est égal à :
 // S* = I* x CI
 // CI étant le coefficient d'orientation présenté plus haut
 // D* est la valeur du rayonnement solaire diffus
 *
 */
double Irradiance(TDateTime aDateSun, TSite aSite)
{
	double hauteur, azimuth;
	double lRSDirect;
	bool mask = false;
	bool masked = false;
	double lMaskMatin, lMaskSoir, lMaskTime;
	double aESol, result = 0.0;

	// Mask
	mask = (aSite.MaskMatin != 0) || (aSite.MaskSoir != 0) || (aSite.UseMask);
	masked = false;
	if (mask)
	{
		lMaskMatin = frac(aSite.MaskMatin);
		lMaskSoir = frac(aSite.MaskSoir);
		lMaskTime = frac(aDateSun);
		if ((lMaskTime < lMaskMatin) || (lMaskTime > lMaskSoir))
			masked = true;
	}

	// L'énergie du jour
	aESol = ESol(DayOfTheYear(aDateSun));

	Sun_Position_Horizontal(aDateSun, aSite.GPS.Latitude, aSite.GPS.Longitude, &hauteur, &azimuth);
	if (hauteur > 0)
	{
		// Calcul du diffus
		result = RSDiffus(hauteur, aSite.GPS.Inclinaison);

		// Calcul du direct
		if (mask && (!masked))
		{
			if (aSite.UseMask && (GetHauteurMask(azimuth) > hauteur))
				masked = true;
		}
		if (!masked)
		{
			lRSDirect = RSDirect(aESol, hauteur, aSite.GPS.Altitude, aSite.Angstrom, aSite.Temperature, aSite.HR);
			result += lRSDirect * Coefficient_Incidence(hauteur, azimuth, aSite.GPS.Inclinaison, aSite.GPS.Orientation);
		}

		if (result < 0)
			result = 0.0;
	}
	return result;
}

/**
 * Calcul de l'irradiance pour un jour complet avec un pas de 5 minutes (288 valeurs)
 * Les masques sont pris en compte
 */
void Sun_Irradiance_Day(TDateTime aDateSun, TSite aSite, double *aIrr)
{
	double hauteur, azimuth;
	double lRSDirect;
	double aESol;
	bool mask = false;
	bool masked = false;
	TDateTime lSunRise, lSunSet, lTime;
	double lMaskMatin = 0, lMaskSoir = 0, lMaskTime;
	uint16_t t = 0;

	// Heure de lever et de coucher du Soleil
	lSunRise = Sun_Hour_GPS(aDateSun, aSite.GPS, sun_rise);
	lSunSet = Sun_Hour_GPS(aDateSun, aSite.GPS, sun_set);

	// Mask
	mask = (aSite.MaskMatin != 0) || (aSite.MaskSoir != 0) || (aSite.UseMask);
	if (mask)
	{
		lMaskMatin = frac(aSite.MaskMatin);
		lMaskSoir = frac(aSite.MaskSoir);
	}

	// L'énergie du jour
	aESol = ESol(DayOfTheYear(aDateSun));

	lTime = (int) aDateSun;
	for (t = 0; t < 288; t++)
		aIrr[t] = 0.0;

	// On se positionne sur l'heure de lever du Soleil
	t = 0;
	while (lTime < lSunRise)
	{
		t++;
		lTime = IncMinute(lTime, 5);
	}

	do
	{
		// Position du Soleil
		Sun_Position_Horizontal(lTime, aSite.GPS.Latitude, aSite.GPS.Longitude, &hauteur, &azimuth);
		if (hauteur > 0)
		{
			// Calcul du diffus
			aIrr[t] = RSDiffus(hauteur, aSite.GPS.Inclinaison);

			// Calcul du direct
			masked = false;
			if (mask)
			{
				lMaskTime = frac(lTime);
				if ((lMaskTime < lMaskMatin) || (lMaskTime > lMaskSoir))
					masked = true;
				if (aSite.UseMask && (GetHauteurMask(azimuth) > hauteur))
					masked = true;
			}
			if (!masked)
			{
				lRSDirect = RSDirect(aESol, hauteur, aSite.GPS.Altitude, aSite.Angstrom, aSite.Temperature, aSite.HR);
				aIrr[t] += lRSDirect * Coefficient_Incidence(hauteur, azimuth, aSite.GPS.Inclinaison, aSite.GPS.Orientation);
			}

			if (aIrr[t] < 0)
				aIrr[t] = 0.0;
		}

		t++;
		lTime = IncMinute(lTime, 5);
	} while (lTime < lSunSet);
}

// Formules de calcul de l'équation du temps :
// Ellipticité (courbe bleue) :
// C = (1,9148 x sin(357,5291 + 0,98560028 + j) + 0,02 x sin(2 x (357,5291 + 0,98560028 x j)) + 0,0003 x sin(3 x (357,5291+0,98560028 x j))) x 4
// Obliquité (courbe verte) :
// O = (-2,468 x sin(2 x (280,4665 + 0,98564736 x j)) + 0,053 x sin(4 x (280,4665 + 0,98564736 x j)) - 0,0014 x sin(6 x (280,4665 + 0,98564736 x j))) x 4
// j étant le numéro d'ordre du jour dans l'année (1 pour le 1er janvier)
// Equation du temps : E = C + O

double Time_Equation(TDateTime aDate)
{
	const double cDegRad = M_PI / 180.0;

	double C, O;
	int j;

	j = DayOfTheYear(aDate);
	C = (1.9148 * sin((357.5291 + 0.98560028 + j) * cDegRad)
			+ 0.02 * sin((2 * (357.5291 + 0.98560028 * j)) * cDegRad)
			+ 0.0003 * sin((3 * (357.5291 + 0.98560028 * j)) * cDegRad)) * 4;
	O = (-2.468 * sin((2 * (280.4665 + 0.98564736 * j)) * cDegRad)
			+ 0.053 * sin((4 * (280.4665 + 0.98564736 * j)) * cDegRad)
			- 0.0014 * sin((6 * (280.4665 + 0.98564736 * j)) * cDegRad)) * 4;
	return C + O;
}

// H légale = Hsun + Décalage horaire + 4 * Décalage longitude + Equation temps
TDateTime DateTimeToSunTime(TDateTime aDate, double aLongitude, int aDecalage)
{
	double lEquaTime;
	int lEquaTimeSec, lLongTimeSec;
	TDateTime result;

	lEquaTime = Time_Equation(aDate);
	lEquaTimeSec = (int) (lEquaTime * 60);
	lLongTimeSec = (int) (4 * aLongitude * 60);
	result = IncMinute(aDate, -aDecalage);  // Décalage horaire
	return IncSecond(result, -(lLongTimeSec + lEquaTimeSec)); // Longitude + Equ Temps
}

TDateTime SunTimeToDateTime(TDateTime aSunDate, double aLongitude, int aDecalage)
{
	double lEquaTime;
	int lEquaTimeSec, lLongTimeSec;
	TDateTime result;

	lEquaTime = Time_Equation(aSunDate);
	lEquaTimeSec = round(lEquaTime * 60);
	lLongTimeSec = round(4 * aLongitude * 60);
	result = IncMinute(aSunDate, aDecalage);  // Décalage horaire
	return IncSecond(result, (lLongTimeSec + lEquaTimeSec)); // Longitude + Equ Temps
}

