#pragma once
#ifndef __RAYONNEMENT_H
#define __RAYONNEMENT_H

#include <stdint.h>
#include <stdbool.h>

#include "misc.h"
#include "vsop87.h"
#include "astronomical.h"

// The observer's latitude is negative for the southern hemisphere and positive
// for the northern hemisphere; the longitude is positive for points west of Greenwich,
// negative for points east, and both given in degrees.

// Définition du coefficient d'Angstrom
//PRINT "      bleu fonc‚         0.015 … 0.025"
//PRINT "      bleu pur           0.025 … 0.050"
//PRINT "      bleu lav‚          0.050 … 0.10"
//PRINT "      bleu laiteux       0.10  … 0.20"
//PRINT "      voil‚ blanc        > 0.20"

#define TAngstromCoeff_Size     8

/**
 * Définition des coefficients d'Angström correspondant à la "transparence" du ciel
 */
typedef enum
{
	acBleuProfond,
	acBleuFonce,
	acBleuPur,
	acBleu,
	acBleuDelave,
	acBleuLaiteux,
	acBleuVoile,
	acBleuBlanc
} TAngstromCoeff;

typedef struct
{
		double Azimut;
		double Hauteur;
} TMaskPoint;

/**
 * Structure décrivant un site
 * 	Name : Nom du site (facultatif)
 * 	GPS : position du site au format TGPS
 * 	MaskMatin, MaskSoir : les heures (heure solaire) des masques matin et soir
 * 	UseMask : booléen indiquant si on tient compte des masques dans le tableau GlobalMask (37 points)
 * 	Temperature : Température moyenne du jour
 * 	HR : Humidité relative en pourcentage du jour
 * 	Angstrom : Coefficients d'Angström = couleur du ciel
 */
typedef struct
{
		char Name[20];
		TGPSPosition GPS;
		TDateTime MaskMatin;
		TDateTime MaskSoir;
		bool UseMask;
		double Temperature;
		double HR;
		TAngstromCoeff Angstrom;
} TSite;

double TroubleLinke(TAngstromCoeff aAngstrom, double aTemperature, double aHR);
double RSDirect(double aESol, double aHauteurSoleil, double aAltitude, TAngstromCoeff aAngstrom,
		double aTemperature, double aHR);
double RSDirect_Mer(double aESol, double aHauteurSoleil, TAngstromCoeff aAngstrom,
		double aTemperature, double aHR);
double RSDiffus(double aHauteurSoleil, double aInclinaison);
double Coefficient_Incidence(double aHauteurSoleil, double aAzimut, double aInclinaison,
		double aOrientation);
double ESol(int aJour);

double Irradiance(TDateTime aDateSun, TSite aSite);
void Sun_Irradiance_Day(TDateTime aDateSun, TSite aSite, double *aIrr);
double Time_Equation(TDateTime aDate);
TDateTime DateTimeToSunTime(TDateTime aDate, double aLongitude, int aDecalage);
TDateTime SunTimeToDateTime(TDateTime aSunDate, double aLongitude, int aDecalage);

double GetHauteurMask(double aAzimut);
void FillSampleMask(void);

#endif /* __RAYONNEMENT_H */

