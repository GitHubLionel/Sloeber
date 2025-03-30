
#include "misc.h"
#include <stdio.h>
#include <math.h>

/**
 * Variable globale indiquant si on est en heure d'été ou d'hiver
 * Hiver = 1;  Eté = 2
 */
uint8_t GLOBAL_SUMMER_HOUR = 1;

double DegToRad(const double Degrees) // { Radians := Degrees * PI / 180 }
{
  return Degrees * (M_PI / 180.0);
}

double RadToDeg(const double Radians) // { Degrees := Radians * 180 / PI }
{
  return Radians * (180.0 / M_PI);
}

double Put_in_360(const double x)
{
  double result = x-round(x/360.0)*360.0;
  while (result<0) result += 360.0;
  return result;
}

void DivMod(int32_t Dividend, uint16_t Divisor, uint16_t *Result, uint16_t *Remainder)
{
  if (Dividend < 0) Dividend = -Dividend;
  *Result = (uint16_t) (Dividend / (uint32_t)Divisor);
  *Remainder = (uint16_t) (Dividend % (uint32_t)Divisor);
}

double frac(double x)
{
  double intpart;
  return modf(x, &intpart);
}

void SinCos(const double Theta, double *Sin, double *Cos)
{
  *Sin = sin(Theta);
  *Cos = cos(Theta);
}

double Sin_D(double x)
{
  return sin(DegToRad((x))); // Put_in_360
}

double Cos_D(double x)
{
  return cos(DegToRad((x))); // Put_in_360
}

void SinCos_D(const double Theta, double *Sin, double *Cos)
{
  double Theta_D = Theta*(M_PI/180.0);
  *Sin = sin(Theta_D);
  *Cos = cos(Theta_D);
}

double Tan_D(double x)
{
  return tan(DegToRad((x))); // Put_in_360
}

double ArcTan2_D(double a, double b)
{
  return RadToDeg(atan2(a,b));
}

double ArcSin_D(double x)
{
  return RadToDeg(asin(x));
}

double ArcCos_D(double x)
{
  return RadToDeg(acos(x));
}

double ArcTan_D(double x)
{
  return RadToDeg(atan(x));
}

// ********************************************
// Date Time functions
// ********************************************

typedef struct
{
  uint16_t tab[12];
} TDayTable;

//typedef *TDayTable PDayTable;

uint16_t MonthDays[2][12] =
    {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
     {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

#define  DateDelta      693594
#define  HoursPerDay    24
#define  MinsPerHour    60
#define  SecsPerMin     60
#define  MSecsPerSec    1000
#define  MinsPerDay     (HoursPerDay * MinsPerHour)
#define  SecsPerDay     (MinsPerDay * SecsPerMin)
#define  SecsPerHour    (SecsPerMin * MinsPerHour)
#define  MSecsPerDay    (SecsPerDay * MSecsPerSec)

/**
 * Renvoie le numéro du jour dans l'année
 * Premier janvier = 1;
 */
uint16_t DayOfTheYear(const TDateTime AValue)
{
  return trunc(AValue - StartOfTheYear(AValue)) + 1;
}

/**
 * Renvoie un datetime correspondant au premier janvier à 00h00
 * de la date donnée par AValue
 */
TDateTime StartOfTheYear(const TDateTime AValue)
{
  TDateTime dt = 0;
  EncodeDate(YearOf(AValue), 1, 1, &dt);
  return dt;
}

uint16_t YearOf(const TDateTime AValue)
{
  uint16_t LMonth, LDay, Result = 0;

  DecodeDate(AValue, &Result, &LMonth, &LDay);
  return Result;
}

/**
 * Encode la date au format TDateTime
 * Year en 4 chiffres
 */
bool EncodeDate(uint16_t Year, uint16_t Month, uint16_t Day, TDateTime *Date)
{
  int32_t I;
  uint16_t *DayTable;
  bool Result = false;

  DayTable = &MonthDays[IsLeapYear(Year)][0];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
      (Day >= 1) && (Day <= DayTable[Month-1]))
  {
    for (I = 1; I<Month; I++) Day += DayTable[I-1];
    I = Year - 1;
    *Date = (TDateTime)(I * 365 + I / 4 - I / 100 + I / 400 + Day) - DateDelta;
    Result = true;
  }
  return Result;
}

/**
 * Renvoie true si l'année est bisextile
 */
bool IsLeapYear(uint16_t Year)
{
  return ((Year % 4) == 0) && (((Year % 100) != 0) || ((Year % 400) == 0));
}

bool DecodeDate(const TDateTime DateTime, uint16_t *Year, uint16_t *Month, uint16_t *Day)
{
#define  D1    365
#define  D4    (D1 * 4 + 1)
#define  D100  (D4 * 25 - 1)
#define  D400  (D100 * 4 + 1)

  uint16_t Y, M, D, I;
  int32_t T;
  uint16_t *DayTable;
  bool Result;

  T = ((int32_t) DateTime) + DateDelta;
  if (T <= 0)
  {
    Year = 0;
    Month = 0;
    Day = 0;
    Result = false;
  }
  else
  {
    T--;
    Y = 1;
    while (T >= D400)
    {
      T -= D400;
      Y += 400;
    }
    DivMod(T, D100, &I, &D);
    if (I == 4)
    {
      I--;
      D += D100;
    }
    Y += (I * 100);
    DivMod(D, D4, &I, &D);
    Y += (I * 4);
    DivMod(D, D1, &I, &D);
    if (I == 4)
    {
      I--;
      D += D1;
    }
    Y += I;
    Result = IsLeapYear(Y);
    DayTable = &MonthDays[(uint8_t)Result][0];
    M = 1;
    while (1)
    {
      I = DayTable[M-1];
      if (D < I) break;
      D -= I;
      M++;
    }
    *Year = Y;
    *Month = M;
    *Day = D + 1;
  }
  return Result;
}

TDateTime IncDay(const TDateTime AValue, const int32_t ANumberOfDays)
{
  return AValue + ANumberOfDays;
}

bool EncodeTime(uint16_t Hour, uint16_t Min, uint16_t Sec, uint16_t MSec, TDateTime *Time)
{
  bool Result = false;
  if ((Hour < HoursPerDay) && (Min < MinsPerHour) && (Sec < SecsPerMin) && (MSec < MSecsPerSec))
  {
    *Time = ((TDateTime)Sec + SecsPerMin *((TDateTime)Min + MinsPerHour*((TDateTime)Hour)))/SecsPerDay +
	    ((TDateTime)MSec) / MSecsPerDay;
    Result = true;
  }
  return Result;
}

void DecodeTime(const TDateTime DateTime, uint16_t *Hour, uint16_t *Min, uint16_t *Sec, uint16_t *MSec)
{
  uint16_t MinCount, MSecCount;

  DivMod((int32_t)(frac(DateTime)*MSecsPerDay), SecsPerMin * MSecsPerSec, &MinCount, &MSecCount);
  DivMod(MinCount, MinsPerHour, Hour, Min);
  DivMod(MSecCount, MSecsPerSec, Sec, MSec);
}

/**
 * Transforme un double en un TDateTime
 * Le double est au format hh.mm avec hh [0..23] et mm [0..59]
 */
TDateTime DoubleToDateTime(double t)
{
  TDateTime result=0;
  uint16_t h,m;

  h = (int)t;
  m = (int)((t-h)*100.0);

  EncodeTime(h, m, 0, 0, &result);
  return result;
}

TDateTime IncHour(const TDateTime AValue, const int32_t ANumberOfHours)
{
  if (AValue > 0)
    return ((AValue * HoursPerDay) + ANumberOfHours) / HoursPerDay;
  else
    return ((AValue * HoursPerDay) - ANumberOfHours) / HoursPerDay;
}

TDateTime IncMinute(const TDateTime AValue, const int32_t ANumberOfMinutes)
{
  if (AValue > 0)
    return ((AValue * MinsPerDay) + ANumberOfMinutes) / MinsPerDay;
  else
    return ((AValue * MinsPerDay) - ANumberOfMinutes) / MinsPerDay;
}

TDateTime IncSecond(const TDateTime AValue, const int32_t ANumberOfSeconds)
{
  if (AValue > 0)
    return ((AValue * SecsPerDay) + ANumberOfSeconds) / SecsPerDay;
  else
    return ((AValue * SecsPerDay) - ANumberOfSeconds) / SecsPerDay;
}

/**
 * Formate l'heure d'un TDateTime au format hh:mm:ss
 * Le buffer str doit être alloué avant appel
 */
char *DateTimeToTimeStr(const TDateTime DateTime, char *str)
{
  uint16_t hour, min, sec, msec;

  DecodeTime(DateTime, &hour, &min, &sec, &msec);
  sprintf(str,"%.2d:%.2d:%.2d",hour, min, sec);
  return str;
}

/**
 * Formate la date d'un TDateTime au format jj/mm/yyyy
 * Le buffer str doit être alloué avant appel
 */
char *DateTimeToDateStr(const TDateTime DateTime, char *str)
{
  uint16_t year=0, month=0, day=0;

  DecodeDate(DateTime, &year, &month, &day);
  sprintf(str,"%.2d/%.2d/%.4d",day, month, year);
  return str;
}

/**
 * Converti une heure solaire en heure locale (GMT +1)
 * GLOBAL_SUMMER_HOUR doit être défini pour l'heure d'été
 */
void SunHourToLocalTime(TDateTime *sunHour)
{
  TDateTime dt = *sunHour;

  *sunHour = IncHour(dt, GLOBAL_SUMMER_HOUR);
}

/**
 * Converti une heure locale (GMT +1) en heure solaire
 * GLOBAL_SUMMER_HOUR doit être défini pour l'heure d'été
 */
void LocalTimeToSunHour(TDateTime *localHour)
{
  TDateTime dt = *localHour;

  *localHour = IncHour(dt, -(GLOBAL_SUMMER_HOUR));
}

void DecodeDMS(const double val, int16_t *Deg, uint16_t *Min, uint16_t *Sec)
{
  double MinCount;

  MinCount = frac(val);
  *Deg = (int16_t)(val - MinCount);
  DivMod((int32_t)(MinCount*SecsPerHour), SecsPerMin, Min, Sec);
}

char *DoubleToDMS(const double val, char *str)
{
  int16_t Deg;
  uint16_t Min, Sec;

  DecodeDMS(val, &Deg, &Min, &Sec);
  sprintf(str,"%.2d°%.2d'%.2d''",Deg, Min, Sec);
  return str;
}

