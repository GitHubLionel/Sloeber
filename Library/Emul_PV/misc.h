#pragma once
#ifndef __MISC_H
#define __MISC_H

#include <stdint.h>
#include <stdbool.h>

typedef double TDateTime;

extern uint8_t GLOBAL_SUMMER_HOUR;

// Math functions
double DegToRad(const double Degrees);
double RadToDeg(const double Radians);
double Put_in_360(const double x);

void DivMod(int32_t Dividend, uint16_t Divisor, uint16_t *Result, uint16_t *Remainder);
double frac(double x);
void SinCos(const double Theta, double *Sin, double *Cos);
double Sin_D(double x);
double Cos_D(double x);
void SinCos_D(const double Theta, double *Sin, double *Cos);
double Tan_D(double x);
double ArcTan2_D(double a, double b);
double ArcSin_D(double x);
double ArcCos_D(double x);
double ArcTan_D(double x);

// Time functions
uint16_t DayOfTheYear(const TDateTime AValue);
TDateTime StartOfTheYear(const TDateTime AValue);
uint16_t YearOf(const TDateTime AValue);
bool IsLeapYear(uint16_t Year);
bool EncodeDate(uint16_t Year, uint16_t Month, uint16_t Day, TDateTime *Date);
bool DecodeDate(const TDateTime DateTime, uint16_t *Year, uint16_t *Month, uint16_t *Day);
TDateTime IncDay(const TDateTime AValue, const int32_t ANumberOfDays);
bool EncodeTime(uint16_t Hour, uint16_t Min, uint16_t Sec, uint16_t MSec, TDateTime *Time);
void DecodeTime(const TDateTime DateTime, uint16_t *Hour, uint16_t *Min, uint16_t *Sec, uint16_t *MSec);
TDateTime DoubleToDateTime(double t);
TDateTime IncHour(const TDateTime AValue, const int32_t ANumberOfHours);
TDateTime IncMinute(const TDateTime AValue, const int32_t ANumberOfMinutes);
TDateTime IncSecond(const TDateTime AValue, const int32_t ANumberOfSeconds);

// Print functions
char *DateTimeToTimeStr(const TDateTime DateTime, char *str);
char *DateTimeToDateStr(const TDateTime DateTime, char *str);

// Degré, minute, seconde functions
void DecodeDMS(const double val, int16_t *Deg, uint16_t *Min, uint16_t *Sec);
char *DoubleToDMS(const double val, char *str);

// Heure solaire
void SunHourToLocalTime(TDateTime *sunHour);
void LocalTimeToSunHour(TDateTime *localHour);


#endif /* __MISC_H */
