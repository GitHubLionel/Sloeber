#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"

// Si on utilise le server NTP (include la librairie NTPClient)
//#define USE_NTP_SERVER=1  // 2 pour l'heure d'été
#ifdef USE_NTP_SERVER
#warning "Using NTP SERVER, you must include NTPClient library"
#endif

// Nombre maximum de top qu'on peut définir
#define MAX_TOP	5

// Si on veut corriger la dérive
//#define RTC_USE_CORRECTION

// La longueur max d'un nom de fichier LittleFS
#define LITTLEFS_MAX_LEN	32
#define TIME_FILENAME	"/time.dat"

// Unix Date time au premier janvier 2020
#define DT01_01_2020	1577833200UL

// Callback pour le changement de jour : pour minuit et la mise à jour date
typedef void (*RTC_daychange_cb)(uint8_t year, uint8_t month, uint8_t day);

// To use RTCLocal in a task
#ifdef RTC_USE_TASK
#define RTC_DATA_TASK	{true, "RTC_Task", 4096, 10, 100, CoreAny, RTC_Task_code}
void RTC_Task_code(void *parameter);
#else
#define RTC_DATA_TASK	{}
#endif

class RTCLocal
{
	private:
		uint8_t month_day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

		uint32_t lasttimeNow = 0;
		uint32_t timeNow = 0;
		uint32_t timeLast = 0;

		uint8_t seconds = 0;
		uint8_t minutes = 0;
		uint8_t hours = 12; // set your starting hour here, not below at int startingHour.
		uint8_t day = 1;
		uint8_t month = 1;
		uint8_t year = 20;

		uint32_t seconds_count = 0UL;	// Compteur de seconde depuis le démarrage
		// Temps Unix initialisé au 1er janvier 2020 (nombre de seconde depuis 1/1/1970 00:00:00 UTC
		uint32_t UNIX_time = DT01_01_2020;

		bool lockUpdate = false;

#ifdef RTC_USE_CORRECTION
		// Time start Settings:
		uint8_t startingHour = hours; // This ensures accurate daily correction of time

		// Accuracy settings
		// You must use only one of this two parameter : dailyErrorFast if microcontroller goes faster else dailyErrorBehind.
		int dailyErrorFast = 0; // set the average number of seconds your microcontroller's time is fast on a daily basis
		int dailyErrorBehind = 10; // set the average number of seconds your microcontroller's time is behind on a daily basis

		bool correctedToday = true; // do not change this variable, one means that the time has already been corrected
		// today for the error in your boards crystal.
		// This is true for the first day because you just set the time when you uploaded the sketch.
#endif

		// Top definition
		uint8_t nb_top = 0;
		uint32_t top_duration[MAX_TOP];
		bool Top[MAX_TOP];

		// Flag de mise à l'heure. Indique si l'heure initiale a été mise à jour
		bool _IsTimeUpToDate = false;

		// Sauvegarde automatique de l'heure dans le fichier Time_Filename tous les SaveDelay secondes
		bool _AutoSave = true;
		// Délais pour la sauvegarde automatique
		uint32_t _SaveDelay = 10;

		// Définition d'une action juste avant minuit (par défaut, une seconde avant minuit)
		RTC_daychange_cb _cb_endday = NULL;		// pointer to the callback function
		uint8_t _cb_delay = 58;
		bool _cb_minute = false;  // Indique qu'on est à 59 minutes

		// Définition d'une action si on change de jour (pour les mises à jour de la date)
		RTC_daychange_cb _cb_daychange = NULL;		// pointer to the callback function

		bool IsLeapYear(int year);
		void StartTime();
		void SetSystemTime();
		void UpdateDateTime(struct tm *t, uint32_t unix_time);

	public:
		char the_time[9];			// L'heure courante au format hh:nn:ss
		char Time_Filename[LITTLEFS_MAX_LEN]; // Le nom du fichier de sauvegarde de l'heure

		RTCLocal();
		RTCLocal(const char *filename)
		{
			RTCLocal();
			if (strlen(filename) < LITTLEFS_MAX_LEN)
				strcpy((char*) Time_Filename, filename);
		}

		void setFileName(const char *filename)
		{
			if (strlen(filename) < LITTLEFS_MAX_LEN)
				strcpy((char*) Time_Filename, filename);
		}

		void setAutoSave(const bool autosave)
		{
			_AutoSave = autosave;
		}

		void setSaveDelay(const uint32_t delay)
		{
			_SaveDelay = delay;
		}

		// A mettre dans la boucle loop principale
		bool update();
		// Met à jour l'heure à partir d'une string dd-mm-yyHhh-nn-ssUunix_timestamp
		void setDateTime(const char *time, bool with_epoch = true, bool default_format = true);
		// Met à jour l'heure à partir de l'heure UTC + décalage gmt
#if defined(USE_NTP_SERVER)
	bool setEpochTime(int8_t gmt = 1);
#endif

		// Flag de mise à l'heure. Indique si l'heure initiale a été mise à jour
		bool IsTimeUpToDate(void) const
		{
			return _IsTimeUpToDate;
		}

		// Gestion de la auvegarde de l'heure dans un fichier LittleFS
		void setupDateTime(const bool autosave = true);
		void saveDateTime(const char *datetime = "");

		// Gestion des top
		void setTop(uint32_t top_duration[], uint8_t nb_top);
		bool getTop(uint8_t id, bool reset = true);
		void clearTop(uint8_t id);

		char* getTime(char *time, const char sep = ':') const;
		char* getShortDate(char *date, const char sep = '/') const;
		char* getDate(char *date, bool millenium) const;
		char* getDateTime(char *datetime, bool millenium, const char sep = ' ') const;
		char* getFormatedDateTime(char *datetime) const;

		uint32_t getUNIXDateTime(void) const
		{
			return UNIX_time;
		}

		// Définition de la callback à faire à minuit moins cb_delay secondes
		// Par défaut, 2 secondes avant minuit pour être sûr d'avoir l'évènement
		void setEndDayCallBack(const RTC_daychange_cb &callback, const uint8_t cb_delay = 2)
		{
			_cb_endday = callback;
			_cb_delay = 60 - cb_delay;
		}

		void setDayChangeCallback(const RTC_daychange_cb &callback)
		{
			_cb_daychange = callback;
		}

};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RTCLocal)
extern RTCLocal RTC_Local;

time_t RTC_Local_Callback();
#endif

