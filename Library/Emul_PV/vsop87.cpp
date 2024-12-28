
/*
{  Calculates the planetary heliocentric coordinates according to the
   VSOP87 theory. Calculations according to chapter 32 (31) of Meeus. }
*/

#include "vsop87.h"
#include <math.h>
#include "misc.h"

#define meeus  //  Only use the accuracy as in the Meeus book

typedef struct
{
  double a;
  double b;
  double c;
} TVSOPEntry;
  
typedef void (*TVSOPCalcFunc) (int , uint8_t, TVSOPEntry *);

// *****************************
// Private functions prototype
// *****************************

double TVSOP_Calc(TVSOPCalcFunc Factor, uint8_t id);
void TVSOP_DynamicToFK5(double *longitude, double *latitude);
static void TVSOPEarth_LongitudeFactor(int nr, uint8_t index, TVSOPEntry *result);
static void TVSOPEarth_LatitudeFactor(int nr, uint8_t index, TVSOPEntry *result);
static void TVSOPEarth_RadiusFactor(int nr, uint8_t index, TVSOPEntry *result);

TDateTime datetime_2000_01_01 = 36526;
static double Tau;

// *****************************
// Public function
// *****************************

void earth_coord(TDateTime date, double *l, double *b, double *r)
{
  Tau = (date-datetime_2000_01_01-0.5)/365250.0;
  
  *r = TVSOP_Calc(TVSOPEarth_RadiusFactor, 1);
  *l = TVSOP_Calc(TVSOPEarth_LongitudeFactor, 2);
  *b = TVSOP_Calc(TVSOPEarth_LatitudeFactor, 3);
  TVSOP_DynamicToFK5(l,b);
  *l = Put_in_360(RadToDeg(*l));  // rad -> degree
  *b = RadToDeg(*b);
}

// *****************************
// Private functions
// *****************************

double TVSOP_Calc(TVSOPCalcFunc Factor, uint8_t id)
{
  double t;
  double current = 0;
  double r[6];
  int i;
  uint8_t j;
  TVSOPEntry result; 

  t = Tau;
  for (j = 0; j < 6; j++)
  {
    r[j] = 0;
    i = 0;
    do
    {
//      switch (id) {
//	case 1: TVSOPEarth_RadiusFactor(i, j, &result); break;
//	case 2: TVSOPEarth_LongitudeFactor(i, j, &result); break;
//	case 3: TVSOPEarth_LatitudeFactor(i, j, &result); break;
//      }
      Factor(i, j, &result);
      current = result.a*cos(result.b + result.c*t);
      r[j] += current;
      i++;
    } while (current != 0);
  }
  return (r[0]+t*(r[1]+t*(r[2]+t*(r[3]+t*(r[4]+t*r[5])))))*1e-8;
}

void TVSOP_DynamicToFK5(double *longitude, double *latitude)
{
  double coeff1 = (0.09033/3600) * (M_PI/180.0);
  double coeff2 = (0.03916/3600) * (M_PI/180.0);
  double lprime,t;
  double delta_l, delta_b;
  double sin_p, cos_p;

  t = 10*Tau;
  lprime = *longitude + DegToRad(-1.397-0.00031*t)*t;
  SinCos(lprime, &sin_p, &cos_p);
  delta_l = -coeff1 + coeff2*(cos_p + sin_p)*tan(*latitude);
  delta_b = coeff2*(cos_p - sin_p);
  *longitude += delta_l;
  *latitude += delta_b;
}

// *****************************
// Private Earth specific functions
// *****************************

extern const uint16_t vsop87_ear_r0_size;
extern const double vsop87_ear_r0[][3];
extern const uint16_t vsop87_ear_r1_size;
extern const double vsop87_ear_r1[][3];
extern const uint16_t vsop87_ear_r2_size;
extern const double vsop87_ear_r2[][3];
extern const uint16_t vsop87_ear_r3_size;
extern const double vsop87_ear_r3[][3];
extern const uint16_t vsop87_ear_r4_size;
extern const double vsop87_ear_r4[][3];
extern const uint16_t vsop87_ear_r5_size;
extern const double vsop87_ear_r5[][3];

static void TVSOPEarth_RadiusFactor(int nr, uint8_t index, TVSOPEntry *result)
{
  result->a = 0.0; result->b = 0.0; result->c = 0.0;
  switch (index) {
    case 0 : if ((nr>=0) && (nr<vsop87_ear_r0_size)) {
      result->a = vsop87_ear_r0[nr][0];  result->b = vsop87_ear_r0[nr][1];  result->c = vsop87_ear_r0[nr][2];
      }
      break;
    case 1 : if ((nr>=0) && (nr<vsop87_ear_r1_size)) {
      result->a = vsop87_ear_r1[nr][0];  result->b = vsop87_ear_r1[nr][1];  result->c = vsop87_ear_r1[nr][2];
      }
      break;
    case 2 : if ((nr>=0) && (nr<vsop87_ear_r2_size)) {
      result->a = vsop87_ear_r2[nr][0];  result->b = vsop87_ear_r2[nr][1];  result->c = vsop87_ear_r2[nr][2];
      }
      break;
    case 3 : if ((nr>=0) && (nr<vsop87_ear_r3_size)) {
      result->a = vsop87_ear_r3[nr][0];  result->b = vsop87_ear_r3[nr][1];  result->c = vsop87_ear_r3[nr][2];
      }
      break;
    case 4 : if ((nr>=0) && (nr<vsop87_ear_r4_size)) {
      result->a = vsop87_ear_r4[nr][0];  result->b = vsop87_ear_r4[nr][1];  result->c = vsop87_ear_r4[nr][2];
      }
      break;
    case 5 : if ((nr>=0) && (nr<vsop87_ear_r5_size)) {
      result->a = vsop87_ear_r5[nr][0];  result->b = vsop87_ear_r5[nr][1];  result->c = vsop87_ear_r5[nr][2];
      }
      break;
    }
}

extern const uint16_t vsop87_ear_b0_size;
extern const double vsop87_ear_b0[][3];
extern const uint16_t vsop87_ear_b1_size;
extern const double vsop87_ear_b1[][3];
extern const uint16_t vsop87_ear_b2_size;
extern const double vsop87_ear_b2[][3];
extern const uint16_t vsop87_ear_b3_size;
extern const double vsop87_ear_b3[][3];
extern const uint16_t vsop87_ear_b4_size;
extern const double vsop87_ear_b4[][3];

static void TVSOPEarth_LatitudeFactor(int nr, uint8_t index, TVSOPEntry *result)
{
  result->a = 0.0; result->b = 0.0; result->c = 0.0;
  switch (index) {
    case 0 : if ((nr>=0) && (nr<vsop87_ear_b0_size)) {
      result->a = vsop87_ear_b0[nr][0];  result->b = vsop87_ear_b0[nr][1];  result->c = vsop87_ear_b0[nr][2];
      }
      break;
    case 1 : if ((nr>=0) && (nr<vsop87_ear_b1_size)) {
      result->a = vsop87_ear_b1[nr][0];  result->b = vsop87_ear_b1[nr][1];  result->c = vsop87_ear_b1[nr][2];
      }
      break;
    case 2 : if ((nr>=0) && (nr<vsop87_ear_b2_size)) {
      result->a = vsop87_ear_b2[nr][0];  result->b = vsop87_ear_b2[nr][1];  result->c = vsop87_ear_b2[nr][2];
      }
      break;
    case 3 : if ((nr>=0) && (nr<vsop87_ear_b3_size)) {
      result->a = vsop87_ear_b3[nr][0];  result->b = vsop87_ear_b3[nr][1];  result->c = vsop87_ear_b3[nr][2];
      }
      break;
    case 4 : if ((nr>=0) && (nr<vsop87_ear_b4_size)) {
      result->a = vsop87_ear_b4[nr][0];  result->b = vsop87_ear_b4[nr][1];  result->c = vsop87_ear_b4[nr][2];
      }
      break;
    }
}

extern const uint16_t vsop87_ear_l0_size;
extern const double vsop87_ear_l0[][3];
extern const uint16_t vsop87_ear_l1_size;
extern const double vsop87_ear_l1[][3];
extern const uint16_t vsop87_ear_l2_size;
extern const double vsop87_ear_l2[][3];
extern const uint16_t vsop87_ear_l3_size;
extern const double vsop87_ear_l3[][3];
extern const uint16_t vsop87_ear_l4_size;
extern const double vsop87_ear_l4[][3];
extern const uint16_t vsop87_ear_l5_size;
extern const double vsop87_ear_l5[][3];

static void TVSOPEarth_LongitudeFactor(int nr, uint8_t index, TVSOPEntry *result)
{
  result->a = 0.0; result->b = 0.0; result->c = 0.0;
  switch (index) {
    case 0 : if ((nr>=0) && (nr<vsop87_ear_l0_size)) {
      result->a = vsop87_ear_l0[nr][0];  result->b = vsop87_ear_l0[nr][1];  result->c = vsop87_ear_l0[nr][2];
      }
      break;
    case 1 : if ((nr>=0) && (nr<vsop87_ear_l1_size)) {
      result->a = vsop87_ear_l1[nr][0];  result->b = vsop87_ear_l1[nr][1];  result->c = vsop87_ear_l1[nr][2];
      }
      break;
    case 2 : if ((nr>=0) && (nr<vsop87_ear_l2_size)) {
      result->a = vsop87_ear_l2[nr][0];  result->b = vsop87_ear_l2[nr][1];  result->c = vsop87_ear_l2[nr][2];
      }
      break;
    case 3 : if ((nr>=0) && (nr<vsop87_ear_l3_size)) {
      result->a = vsop87_ear_l3[nr][0];  result->b = vsop87_ear_l3[nr][1];  result->c = vsop87_ear_l3[nr][2];
      }
      break;
    case 4 : if ((nr>=0) && (nr<vsop87_ear_l4_size)) {
      result->a = vsop87_ear_l4[nr][0];  result->b = vsop87_ear_l4[nr][1];  result->c = vsop87_ear_l4[nr][2];
      }
      break;
    case 5 : if ((nr>=0) && (nr<vsop87_ear_l5_size)) {
      result->a = vsop87_ear_l5[nr][0];  result->b = vsop87_ear_l5[nr][1];  result->c = vsop87_ear_l5[nr][2];
      }
      break;
    }
}


