/*
julian_offset = 2415018.5;
datetime_2000_01_01 = 36526;
datetime_1999_01_01 = 36161;
datetime_first_lunation = 8418;
*/

#include "astronomical.h"
#include "rayonnement.h"
#include "misc.h"
#include "vsop87.h"

#define low_accuracy

#define AU             149597869.0       // astronomical unit in km
#define mean_lunation  29.530589;        // Mean length of a month
#define tropic_year    365.242190;   	 // Tropic year length
#define earth_radius   6378.15;     	 // Radius of the earth

typedef struct
{
  double longitude, latitude, radius; // lambda, beta, R 
  double rektaszension, declination;  // alpha, delta 
  double parallax;
  double elevation, azimuth;          // h, A 
} T_Coord;

static double julian_offset = 2415018.5;    // julian_offset = 2451544.5-EncodeDate(2000,1,1);

// ********************************************
// Prototypes Fonctions locales
// ********************************************
double julian_date(TDateTime date);
double star_time(TDateTime date);
void calc_horizontal(TDateTime date, double longitude, double latitude, T_Coord *coord);
void calc_epsilon_phi(TDateTime date, double *delta_phi, double *epsilon);
void calc_geocentric(TDateTime date, T_Coord *coord);
void sun_coordinate(TDateTime date, T_Coord *result);

double interpolation(double y1, double y2, double y3, double n);
double correction(double m, uint8_t kind, T_Coord pos1, T_Coord pos2, T_Coord pos3,
		              double h0, double theta0, double longitude, double latitude);
TDateTime Calc_Set_Rise(TDateTime date, double latitude, double longitude, T_RiseSet kind);


// ********************************************
// Fonctions locales
// ********************************************

double julian_date(TDateTime date)
{
  return julian_offset+date;
}

double star_time(TDateTime date)
{
  double jd, t;
  double delta_phi, epsilon;

  jd = julian_date(date);
  t = (jd-2451545.0)/36525;
  calc_epsilon_phi(date, &delta_phi, &epsilon);
  return Put_in_360(280.46061837+360.98564736629*(jd-2451545.0) +
                     t*t*(0.000387933-t/38710000) + delta_phi*Cos_D(epsilon));
}

void calc_horizontal(TDateTime date, double longitude, double latitude, T_Coord *coord)
{
  double h;
  double sin_l, cos_l;

  h = Put_in_360(star_time(date)-coord->rektaszension-longitude);
  SinCos_D(latitude, &sin_l, &cos_l);

  coord->azimuth = ArcTan2_D(Sin_D(h), Cos_D(h)*sin_l - Tan_D(coord->declination)*cos_l);
  coord->elevation = ArcSin_D(sin_l*Sin_D(coord->declination) + cos_l*Cos_D(coord->declination)*Cos_D(h));
}

void calc_epsilon_phi(TDateTime date, double *delta_phi, double *epsilon)
{
#ifdef low_accuracy
  double l,ls;
#else
  const int8_t arg_mul[31][5] = {
       { 0, 0, 0, 0, 1},
       {-2, 0, 0, 2, 2},
       { 0, 0, 0, 2, 2},
       { 0, 0, 0, 0, 2},
       { 0, 1, 0, 0, 0},
       { 0, 0, 1, 0, 0},
       {-2, 1, 0, 2, 2},
       { 0, 0, 0, 2, 1},
       { 0, 0, 1, 2, 2},
       {-2,-1, 0, 2, 2},
       {-2, 0, 1, 0, 0},
       {-2, 0, 0, 2, 1},
       { 0, 0,-1, 2, 2},
       { 2, 0, 0, 0, 0},
       { 0, 0, 1, 0, 1},
       { 2, 0,-1, 2, 2},
       { 0, 0,-1, 0, 1},
       { 0, 0, 1, 2, 1},
       {-2, 0, 2, 0, 0},
       { 0, 0,-2, 2, 1},
       { 2, 0, 0, 2, 2},
       { 0, 0, 2, 2, 2},
       { 0, 0, 2, 0, 0},
       {-2, 0, 1, 2, 2},
       { 0, 0, 0, 2, 0},
       {-2, 0, 0, 2, 0},
       { 0, 0,-1, 2, 1},
       { 0, 2, 0, 0, 0},
       { 2, 0,-1, 0, 1},
       {-2, 2, 0, 2, 2},
       { 0, 1, 0, 0, 1}
      };

  const int32_t arg_phi[31][2] = {
       {-171996,-1742},
       { -13187,  -16},
       {  -2274,   -2},
       {   2062,    2},
       {   1426,  -34},
       {    712,    1},
       {   -517,   12},
       {   -386,   -4},
       {   -301,    0},
       {    217,   -5},
       {   -158,    0},
       {    129,    1},
       {    123,    0},
       {     63,    0},
       {     63,    1},
       {    -59,    0},
       {    -58,   -1},
       {    -51,    0},
       {     48,    0},
       {     46,    0},
       {    -38,    0},
       {    -31,    0},
       {     29,    0},
       {     29,    0},
       {     26,    0},
       {    -22,    0},
       {     21,    0},
       {     17,   -1},
       {     16,    0},
       {    -16,    1},
       {    -15,    0}
      };

  const int32_t arg_eps[31][2] = {
       { 92025,   89},
       {  5736,  -31},
       {   977,   -5},
       {  -895,    5},
       {    54,   -1},
       {    -7,    0},
       {   224,   -6},
       {   200,    0},
       {   129,   -1},
       {   -95,    3},
       {     0,    0},
       {   -70,    0},
       {   -53,    0},
       {     0,    0},
       {   -33,    0},
       {    26,    0},
       {    32,    0},
       {    27,    0},
       {     0,    0},
       {   -24,    0},
       {    16,    0},
       {    13,    0},
       {     0,    0},
       {   -12,    0},
       {     0,    0},
       {     0,    0},
       {   -10,    0},
       {     0,    0},
       {    -8,    0},
       {     7,    0},
       {     9,    0}
      };

  double d,m,ms,f,s;
  int i;
#endif 
  double t,omega;
  double epsilon_0, delta_epsilon;

  t = (julian_date(date)-2451545.0)/36525;

  // longitude of rising knot 
  omega = Put_in_360(125.04452+(-1934.136261+(0.0020708+1/450000*t)*t)*t);

#ifdef low_accuracy 
  // //@/// delta_phi and delta_epsilon - low accuracy 
  // mean longitude of sun (l) and moon (ls) 
  l = 280.4665+36000.7698*t;
  ls = 218.3165+481267.8813*t;

  // correction due to nutation 
  delta_epsilon = 9.20*Cos_D(omega)+0.57*Cos_D(2*l)+0.10*Cos_D(2*ls)-0.09*Cos_D(2*omega);

  // longitude correction due to nutation 
  *delta_phi = (-17.20*Sin_D(omega)-1.32*Sin_D(2*l)-0.23*Sin_D(2*ls)+0.21*Sin_D(2*omega))/3600;
  
#else 
  // //@/// delta_phi and delta_epsilon - higher accuracy 
  // mean elongation of moon to sun 
  d = Put_in_360(297.85036+(445267.111480+(-0.0019142+t/189474)*t)*t);

  // mean anomaly of the sun 
  m = Put_in_360(357.52772+(35999.050340+(-0.0001603-t/300000)*t)*t);

  // mean anomly of the moon 
  ms = Put_in_360(134.96298+(477198.867398+(0.0086972+t/56250)*t)*t);

  // argument of the latitude of the moon 
  f = Put_in_360(93.27191+(483202.017538+(-0.0036825+t/327270)*t)*t);

  *delta_phi = 0;
  delta_epsilon = 0;

  for (i = 0; i <= 30; i++)
  {
    s = arg_mul[i][0]*d + arg_mul[i][1]*m + arg_mul[i][2]*ms +
        arg_mul[i][3]*f + arg_mul[i][4]*omega;
    *delta_phi += (arg_phi[i][0] + arg_phi[i][1]*t*0.1)*Sin_D(s);
    delta_epsilon = delta_epsilon + (arg_eps[i][0] + arg_eps[i][1]*t*0.1)*Cos_D(s);
  }

  *delta_phi = (*delta_phi)*0.0001/3600;
  delta_epsilon = delta_epsilon*0.0001/3600;
  
#endif 

  // angle of ecliptic 
  epsilon_0 = 84381.448+(-46.8150+(-0.00059+0.001813*t)*t)*t;

  *epsilon = (epsilon_0+delta_epsilon)/3600;
}

void calc_geocentric(TDateTime date, T_Coord *coord)
{
  double epsilon;
  double delta_phi;
  double sin_e, cos_e;

  calc_epsilon_phi(date, &delta_phi, &epsilon);
  coord->longitude = Put_in_360(coord->longitude+delta_phi);

  // geocentric coordinates 
  SinCos_D(epsilon, &sin_e, &cos_e);

  coord->rektaszension = ArcTan2_D( Sin_D(coord->longitude)*cos_e - Tan_D(coord->latitude)*sin_e, Cos_D(coord->longitude));
  coord->declination = ArcSin_D( Sin_D(coord->latitude)*cos_e + Cos_D(coord->latitude)*sin_e*Sin_D(coord->longitude));
}

void sun_coordinate(TDateTime date, T_Coord *result)
{
  double l,b,r;
  double lambda,t;

  earth_coord(date, &l, &b, &r);
  // convert earth coordinate to sun coordinate 
  l = l+180;
  b = -b;
  // conversion to FK5 
  t = (julian_date(date)-2451545.0)/365250.0*10;
  lambda = l+(-1.397-0.00031*t)*t;
  l = l-0.09033/3600;
  b = b+0.03916/3600*(Cos_D(lambda)-Sin_D(lambda));
  // aberration 
  l = l-20.4898/3600/r;

  result->longitude = Put_in_360(l);
  result->latitude = b;
  result->radius = r*AU;
  calc_geocentric(date, result);
}

// ********************************************
// Fonctions parcours du Soleil
// ********************************************

double interpolation(double y1, double y2, double y3, double n)
{
  double a,b,c;

  a = y2-y1;
  b = y3-y2;
  if (a>100)  a = a-360;
  if (a<-100)  a = a+360;
  if (b>100)  b = b-360;
  if (b<-100)  b = b+360;
  c = b-a;
  return y2+0.5*n*(a+b+n*c);
}

double correction(double m, uint8_t kind, T_Coord pos1, T_Coord pos2, T_Coord pos3,
		              double h0, double theta0, double longitude, double latitude)
{
  double alpha,delta,h, height;
  double result;

  alpha = interpolation(pos1.rektaszension,
                       pos2.rektaszension,
                       pos3.rektaszension,
                       m);
  delta = interpolation(pos1.declination,
                       pos2.declination,
                       pos3.declination,
                       m);
  h = Put_in_360((theta0+360.985647*m)-longitude-alpha);
  if (h>180) h = h-360;

  height = ArcSin_D(Sin_D(latitude)*Sin_D(delta)
                   +Cos_D(latitude)*Cos_D(delta)*Cos_D(h));

  switch (kind) {
    case 0:   result = -h/360; break;
    case 1:   result = (height-h0)/(360*Cos_D(delta)*Cos_D(latitude)*Sin_D(h)); break;
    case 2:   result = (height-h0)/(360*Cos_D(delta)*Cos_D(latitude)*Sin_D(h)); break;
    default:  result = 0;   //(* this cannot happen *)
  }
  return result;
}

TDateTime Calc_Set_Rise(TDateTime date, double latitude, double longitude, T_RiseSet kind)
{
#define sun_diameter  0.8333
#define civil_twilight_elevation  (-6.0)
#define nautical_twilight_elevation  (-12.0)
#define astronomical_twilight_elevation  (-18.0)

  double h;
  T_Coord pos1, pos2, pos3;
  double h0, theta0, cos_h0, cap_h0;
  double m0,m1,m2;
  TDateTime result = 0;

  switch (kind) {
    case sun_rise:          h0 = -sun_diameter; break;
    case sun_set:           h0 = -sun_diameter; break;
    case sun_rise_civil:    h0 = civil_twilight_elevation; break;
    case sun_set_civil:     h0 = civil_twilight_elevation; break;
    case sun_rise_nautical: h0 = nautical_twilight_elevation; break;
    case sun_set_nautical:  h0 = nautical_twilight_elevation; break;
    case sun_rise_astro:    h0 = astronomical_twilight_elevation; break;
    case sun_set_astro:     h0 = astronomical_twilight_elevation; break;
    default :               h0 = 0;  // (* don't care for _transit *)
  }

  h = (int)date;
  theta0 = star_time(h);
  sun_coordinate(h-1, &pos1);
  sun_coordinate(h, &pos2);
  sun_coordinate(h+1, &pos3);

  cos_h0 = (Sin_D(h0)-Sin_D(latitude)*Sin_D(pos2.declination))/
           (Cos_D(latitude)*Cos_D(pos2.declination));
  if ((cos_h0<-1) || (cos_h0>1))
    return 0.0;   // raise E_NoRiseSet.Create('No rises or sets calculable');
  cap_h0 = ArcCos_D(cos_h0);

  m0 = (pos2.rektaszension+longitude-theta0)/360;
  m1 = m0-cap_h0/360;
  m2 = m0+cap_h0/360;

  if ((kind == sun_rise) || (kind == sun_rise_civil) || (kind == sun_rise_nautical) || (kind == sun_rise_astro))
  {
    m1 = frac(m1);
    if (m1<0) m1 = m1+1;
    m1 += correction(m1,1,pos1, pos2, pos3, h0, theta0, longitude, latitude);
    result = h+m1;
  }
  else
    if ((kind == sun_set) || (kind == sun_set_civil) || (kind == sun_set_nautical) || (kind == sun_set_astro))
    {
      m2 = frac(m2);
      if (m2<0) m2 = m2+1;
      m2 += correction(m2,2,pos1, pos2, pos3, h0, theta0, longitude, latitude);
      result = h+m2;
    }
    else
      if (kind == sun_transit)
      {
	m0 = frac(m0);
        if (m0<0) m0 = m0+1;
	m0 += correction(m0,0,pos1, pos2, pos3, h0, theta0, longitude, latitude);
	result = h+m0;
      }

  return result;
}

// ********************************************
// Fonctions publiées
// ********************************************

/**
 * La position du Soleil dans le ciel
 * date : la date avec l'heure SOLAIRE du moment choisi
 * latitude, longitude : les coordonnées GPS
 * *elevation : la hauteur du Soleil
 * *azimuth : l'azimuth du Soleil
 */
void Sun_Position_Horizontal(TDateTime date, double latitude, double longitude, double *elevation, double *azimuth)
{
  static T_Coord pos1;

  sun_coordinate(date, &pos1);
  calc_horizontal(date, longitude, latitude, &pos1);
  *elevation = pos1.elevation;
  *azimuth = pos1.azimuth;
}

/**
 * La position du Soleil dans le ciel
 * date : la date avec l'heure SOLAIRE du moment choisi
 * aPosition : les coordonnées GPS
 * *elevation : la hauteur du Soleil
 * *azimuth : l'azimuth du Soleil
 */
void Sun_Position_Horizontal_GPS(TDateTime date, TGPSPosition aPosition, double *elevation, double *azimuth)
{
  Sun_Position_Horizontal(date, aPosition.Latitude, aPosition.Longitude, elevation, azimuth);
}

/**
 * Heure du Soleil à l'heure solaire
 * date : la date du jour
 * latitude, longitude : les coordonnées GPS
 * special_hour : Lever, coucher, transit
 */
TDateTime Sun_Hour(TDateTime date, double latitude, double longitude, T_RiseSet special_hour)
{
  return Calc_Set_Rise(date, latitude, longitude, special_hour);
}

/**
 * Heure du Soleil à l'heure solaire
 * date : la date du jour
 * aPosition : les coordonnées GPS
 * special_hour : Lever, coucher, transit
 */
TDateTime Sun_Hour_GPS(TDateTime date, TGPSPosition aPosition, T_RiseSet special_hour)
{
  return Sun_Hour(date, aPosition.Latitude, aPosition.Longitude, special_hour);
}

