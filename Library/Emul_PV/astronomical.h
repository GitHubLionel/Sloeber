#pragma once
#ifndef __ASTRONOMICAL_H
#define __ASTRONOMICAL_H

#include <stdint.h>
#include <stdbool.h>

#include "misc.h"

/**
 * Structure contenant les coordonnées GPS : latitude, longitude, altitude
 * 	Latitude (parallèle) positive pour l'hémisphère nord : ~45° N
 * 	Longitude positive à l'ouest du méridien de Greenwich
 * L'orientation et l'inclinaison du plan à considérer
 */
typedef struct
{
  double Latitude;    // Latitude positive pour l'hémisphère nord
  double Longitude;   // Longitude positive à l'ouest du méridien de Greenwich
  int Altitude;
  int Orientation;
  int Inclinaison;
} TGPSPosition;

typedef enum
{
  sun_rise,
  sun_set,
  sun_transit,
  sun_rise_civil,
  sun_rise_nautical,
  sun_rise_astro,
  sun_set_civil,
  sun_set_nautical,
  sun_set_astro
} T_RiseSet;

void Sun_Position_Horizontal(TDateTime date, double latitude, double longitude, double *elevation, double *azimuth);
void Sun_Position_Horizontal_GPS(TDateTime date, TGPSPosition aPosition, double *elevation, double *azimuth);

TDateTime Sun_Hour(TDateTime date, double latitude, double longitude, T_RiseSet special_hour);
TDateTime Sun_Hour_GPS(TDateTime date, TGPSPosition aPosition, T_RiseSet special_hour);

#endif /* __ASTRONOMICAL_H */

