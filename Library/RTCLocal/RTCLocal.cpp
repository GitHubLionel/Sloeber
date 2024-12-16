#include "RTCLocal.h"
#include "Partition_utils.h"	// Some utils functions for LittleFS/SPIFFS/FatFS

#if defined(USE_NTP_SERVER)
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
#endif

#ifdef RTC_USE_TASK
#include "Tasks_utils.h"
#endif

//#define RTC_DEBUG

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

RTCLocal::RTCLocal()
{
	strcpy((char*) Time_Filename, TIME_FILENAME);
	for (uint8_t i = 0; i < MAX_TOP; i++)
		this->Top[i] = false;
	SetSystemTime();
}

void RTCLocal::SetSystemTime()
{
#ifdef ESP32
	struct timeval tv;
	tv.tv_sec = this->UNIX_time;  // mktime(&t);
	tv.tv_usec = 0;    // microseconds
	settimeofday(&tv, NULL);
#endif
}

/**
 * Fonction principale à mettre dans la boucle loop
 * Return true si la date est à jour et qu'une seconde s'est écoulée depuis le dernier appel
 */
bool RTCLocal::update()
{
	// On est en train de mettre l'heure à jour
	if (lockUpdate)
		return false;

	uint32_t deltatime;
	bool minutechange = false;

	// The number of seconds that have passed since boot
	this->timeNow = millis() / 1000;

	// One second is elapsed
	if ((deltatime = this->timeNow - this->lasttimeNow) > 0)
	{
		this->seconds_count += deltatime;
		this->UNIX_time += deltatime;

		// Calcul des Top
		// Le top est actualisé s'il a été précédement "mangé"
		for (uint8_t i = 0; i < this->nb_top; i++)
		{
			if (!this->Top[i])
				this->Top[i] = (this->seconds_count % this->top_duration[i] == 0);
		}
		this->lasttimeNow = this->timeNow;
	}
	else
		return false;

	// if one minute has passed, restart counting seconds from zero and add one minute
	if ((this->seconds = this->timeNow - this->timeLast) >= 60)
	{
		this->timeLast = this->timeNow;
		// We do not use this->seconds = 0 because there is maybe more than a second
		this->seconds -= 60;

		// if one hour has passed, restart counting minutes from zero and add one hour
		if (++this->minutes >= 60)
		{
			this->minutes = 0;
			this->_cb_minute = false;

			// if 24 hours have passed, add one day
			if (++this->hours >= 24)
			{
				this->hours = 0;

				// End of month
				if (++this->day > this->month_day[this->month - 1])
				{
					this->day = 1;

					// End of year
					if (++this->month == 13)
					{
						this->month = 1;
						this->year++;
						// Test année bissextile
						(IsLeapYear(this->year)) ? this->month_day[1] = 29 : this->month_day[1] = 28;
					}
				}
			}
#ifdef RTC_USE_CORRECTION
			else
			{
				//every time 24 hours have passed since the initial starting time and it has not been reset this day before,
				//add milliseconds or delay the progran with some milliseconds.
				//Change these variables according to the error of your board.
				if ((this->hours == (24 - this->startingHour)) && (!this->correctedToday))
				{
					if (this->dailyErrorFast != 0)
						delay(this->dailyErrorFast*1000);
					this->seconds += this->dailyErrorBehind;
					this->UNIX_time += this->dailyErrorBehind;
					this->seconds_count += this->dailyErrorBehind;

					this->timeNow = millis() / 1000;
					this->lasttimeNow = this->timeNow;
					this->timeLast = this->timeNow - this->seconds;
					this->correctedToday = true;
				}

				if (this->hours == 24 - this->startingHour + 2)
					this->correctedToday = false;
			}
#endif
		}
		else
		{
			MinuteOfTheDay = this->hours * 60 + this->minutes;
			if (this->minutes == 59)
				this->_cb_minute = true;
			minutechange = true;
		}
	}

	// Met à jour la string heure
	getTime();

	// Gestion des callback
	if (_cb_secondechange != NULL)
		_cb_secondechange();

	if (minutechange && (_cb_minutechange != NULL))
		_cb_minutechange(MinuteOfTheDay);

	// Activation de la callback à partir de minuit moins _cb_delay seconde
	if (_cb_endday != NULL)
	{
		if (this->_cb_minute && (this->hours == 23) && (this->seconds >= this->_cb_delay))
		{
			// Pour éviter un deuxième appel si _cb_delay est grand
			this->_cb_minute = false;
			_cb_endday(this->year, this->month, this->day);
		}
	}

	// Sauvegarde automatique de l'heure
	if (this->_IsTimeUpToDate && this->_AutoSave && (this->seconds_count % this->_SaveDelay == 0))
		this->saveDateTime();

	return this->_IsTimeUpToDate;
}

void RTCLocal::StartTime()
{
	this->timeNow = millis() / 1000;
	this->lasttimeNow = this->timeNow;
	this->timeLast = this->timeNow - this->seconds;
	(IsLeapYear(this->year)) ? this->month_day[1] = 29 : this->month_day[1] = 28;

	// On initialise le compteur pour avoir des tops à la minute
	this->seconds_count = this->seconds;
	this->_IsTimeUpToDate = true;
#ifdef RTC_USE_CORRECTION
	this->startingHour = this->hours;
	this->correctedToday = true;
#endif
	// Met à jour la string heure
	getTime();
}

void RTCLocal::UpdateDateTime(struct tm *t, uint32_t unix_time)
{
	lockUpdate = true;

#ifdef RTC_DEBUG
	char buf[20] = {0};
	sprintf(buf, "ut = %lu", unix_time);
	print_debug(buf);
#define MAX_SIZE 80
	char buffer[MAX_SIZE];
	strftime(buffer, MAX_SIZE, "dt = %d/%m/%Y %H:%M:%S", t);
	print_debug(buffer);
#endif

	// Activation de la callback si on a changé de jour
	if (_cb_daychange != NULL)
	{
		if ((this->day != t->tm_mday) || (this->month != t->tm_mon + 1) || (this->year != t->tm_year - 100))
			_cb_daychange(this->year, this->month, this->day);
	}

	// Mise à jour de la date
	this->year = t->tm_year - 100;
	this->month = t->tm_mon + 1; // Month, where 0 = jan
	this->day = t->tm_mday;
	this->hours = t->tm_hour;
	this->minutes = t->tm_min;
	this->seconds = t->tm_sec;
	this->UNIX_time = unix_time;

	SetSystemTime();
	StartTime();

	lockUpdate = false;
}

/**
 * Set Date Time from a string
 * Message de la forme dd-mm-yyHhh-nn-ssUunix_timestamp (default : with_epoch = true)
 * ou
 * DD/MM/YY#hh:mm:ss (with_epoch = false, default_format = false)
 */
void RTCLocal::setDateTime(const char *time, bool with_epoch, bool default_format)
{
	struct tm t;
	uint8_t _month;
	unsigned int _Unix_time;

	if (with_epoch)
	{
		sscanf(time, "%d-%hhu-%dH%d-%d-%dU%u", &t.tm_mday, &_month, &t.tm_year, &t.tm_hour, &t.tm_min,
				&t.tm_sec, &_Unix_time);
	}
	else
	{
		if (default_format)
			sscanf(time, "%d-%hhu-%dH%d-%d-%d", &t.tm_mday, &_month, &t.tm_year, &t.tm_hour, &t.tm_min, &t.tm_sec);
		else
			sscanf(time, "%d/%hhu/%d#%d:%d:%d", &t.tm_mday, &_month, &t.tm_year, &t.tm_hour, &t.tm_min, &t.tm_sec);
	}
	t.tm_mon = _month - 1;  // Month, where 0 = jan
	t.tm_year += 100;       // Year start in 1900
	t.tm_isdst = 0;         // Is DST on? 1 = yes, 0 = no, -1 = unknown
	if (!with_epoch)
		_Unix_time = mktime(&t);

	// Mise à jour de la date
	UpdateDateTime(&t, _Unix_time);
}

#if defined(USE_NTP_SERVER)
/**
 * Update time form NTP server
 * WARNING: We assume that a connexion is present
 * The server is started just to get time and is stoped after
 * gmt : hour offset from GMT (default 1)
 */
bool RTCLocal::setEpochTime(int8_t gmt)
{
	// Start the timeClient
	timeClient.begin();
	timeClient.setTimeOffset(3600 * gmt);

	if (!timeClient.update())
	{
		timeClient.end();
		return false;
	}

	// Beware, time_t is "long long int" if ESP_IDF >= 5 else time_t is "long int"
	time_t epochTime = timeClient.getEpochTime();

	// End timeClient
	timeClient.end();

	// Just verify that time is newer than year 2020
	if (epochTime > DT01_01_2020)
	{
		struct tm ptm;

		// Set timezone to Paris Standard Time (not usefull here)
//		setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
//		tzset();
		localtime_r(&epochTime, &ptm);

		// Mise à jour de la date
		UpdateDateTime(&ptm, epochTime);
	}
	else
		return false;

	return true;
}
#endif

bool RTCLocal::IsLeapYear(int year)
{
	return ((year % 4) == 0) || (((year % 100) == 0) && ((year % 400) == 0));
}

/**
 * Get time from LittleFS file.
 * Format expected is : dd-mm-yyHhh-nn-ssUunix_timestamp
 * 28 characters length
 * Set autosave to true (default) for saving datetime
 */
void RTCLocal::setupDateTime(const bool autosave)
{
	_AutoSave = autosave;
	// Ouvre le fichier time en lecture
	print_debug("Read last time : ", false);
	File time = FS_Partition->open(Time_Filename, "r");
	if (time)
	{
		char lasttime[30] = {0};
		time.readBytes(lasttime, 28);
		// Mise à l'heure
		setDateTime(lasttime);
		time.close();
		print_debug(lasttime);
	}
	else
		print_debug("failed !");
}

/**
 * Save time to LittleFS file
 * Format expected is : dd-mm-yyHhh-nn-ssUunix_timestamp
 * 28 characters length
 * if datetime = "", use the current datetime
 */
void RTCLocal::saveDateTime(const char *datetime)
{
	File time = FS_Partition->open(Time_Filename, "w");
	if (time)
	{
		char temp[30];
		if (strlen(datetime) == 0)
			getFormatedDateTime(temp);
		else
			strcpy(temp, datetime);
		//    print_debug(temp, true);
		time.print(temp);
		time.close();
	}
}

void RTCLocal::setTop(uint32_t top_duration[], uint8_t nb_top)
{
	if (nb_top > MAX_TOP)
		nb_top = MAX_TOP;
	this->nb_top = nb_top;
	for (uint8_t i = 0; i < nb_top; i++)
		this->top_duration[i] = top_duration[i];
}

bool RTCLocal::getTop(uint8_t id, bool reset)
{
	if (id > nb_top)
		return false;
	bool tmp = this->Top[id];
	if (reset)
		this->Top[id] = false;
	return tmp;
}

void RTCLocal::clearTop(uint8_t id)
{
	this->Top[id] = false;
}

void RTCLocal::getTime(void)
{
	char *ptime = this->time;

	*ptime++ = (char) (48 + (this->hours / 10)); // or digits[this->hours / 10];
	*ptime++ = (char) (48 + (this->hours % 10));
	*ptime++ = time_separator;
	*ptime++ = (char) (48 + (this->minutes / 10));
	*ptime++ = (char) (48 + (this->minutes % 10));
	*ptime++ = time_separator;
	*ptime++ = (char) (48 + (this->seconds / 10));
	*ptime++ = (char) (48 + (this->seconds % 10));
	*ptime = 0; // end string
//	strcpy(this->thetime, this->time); // It is really necessary ?
}

/**
 * Return the time in string format hh:nn:ss
 * sep is the separator. By défault it is : " "
 * Note : this version is about 7 times faster than use of sprintf
 * Use of an array of digits is a little smaller than conversion
 */
char* RTCLocal::getTime(char *time, const char sep) const
{
	//  static const char *digits = "0123456789";  // Les chiffres
	char *ptime = time;

	*ptime++ = (char) (48 + (this->hours / 10)); // or digits[this->hours / 10];
	*ptime++ = (char) (48 + (this->hours % 10));
	*ptime++ = sep;
	*ptime++ = (char) (48 + (this->minutes / 10));
	*ptime++ = (char) (48 + (this->minutes % 10));
	*ptime++ = sep;
	*ptime++ = (char) (48 + (this->seconds / 10));
	*ptime++ = (char) (48 + (this->seconds % 10));
	*ptime = 0; // end string

	return time;

	// 7 times faster than :
	//  sprintf(time, "%02d:%02d:%02d", this->hours, this->minutes, this->seconds);
}

/**
 * Return the date in string format dd/mm
 */
char* RTCLocal::getShortDate(char *date, const char sep) const
{
	char *pdate = date;

	*pdate++ = (char) (48 + (this->day / 10));
	*pdate++ = (char) (48 + (this->day % 10));
	*pdate++ = sep;
	*pdate++ = (char) (48 + (this->month / 10));
	*pdate++ = (char) (48 + (this->month % 10));
	*pdate = 0; // end string

	return date;
}

/**
 * Return the date in string format dd-mm-yyyy if millenium = true
 * else return dd-mm-yy
 */
char* RTCLocal::getDate(char *date, bool millenium) const
{
	char *pdate = date;

	*pdate++ = (char) (48 + (this->day / 10));
	*pdate++ = (char) (48 + (this->day % 10));
	*pdate++ = '-';
	*pdate++ = (char) (48 + (this->month / 10));
	*pdate++ = (char) (48 + (this->month % 10));
	*pdate++ = '-';
	if (millenium)
	{
		*pdate++ = '2';
		*pdate++ = '0';
	}
	*pdate++ = (char) (48 + (this->year / 10));
	*pdate++ = (char) (48 + (this->year % 10));
	*pdate = 0; // end string

	return date;

	// 7 times faster than :
	//  sprintf(date, "%02d-%02d-20%02d", this->day, this->month, this->year);
}

/**
 * Return date time in format dd-mm-yy(yy if millenium)(sep)hh:nn:ss
 * by default sep = " "
 */
char* RTCLocal::getDateTime(char *datetime, bool millenium, const char sep) const
{
	char tmp[12] = {0};

	strcpy(datetime, getDate(tmp, millenium));
	datetime[strlen(datetime)] = sep;
	datetime[strlen(datetime) + 1] = 0;
	strcat(datetime, this->time);
	return datetime;
}

/**
 * La datetime formatée comme pour son initialisation
 * Format : dd-mm-yyHhh-nn-ssUunix_timestamp
 * A besoin d'un buffer de 30 char
 */
char* RTCLocal::getFormatedDateTime(char *datetime) const
{
	//  char date[40] = {0};
	//  sprintf(date, "%.2d-%.2d-%.2dH%.2d-%.2d-%.2dU%ld", this->day, this->month, this->year, this->hours, this->minutes, this->seconds,
	//	  this->UNIX_time);
	//  return String(date);

	char tmp[12] = {0};

	strcpy(datetime, getDate(tmp, false));
	strcat(datetime, "H");
	strcat(datetime, getTime(tmp, '-'));
	strcat(datetime, "U");
	sprintf(tmp, "%u", (unsigned int) this->UNIX_time);
	strcat(datetime, tmp);

	return datetime;
}

/**
 * A global instance of RTCLocal
 */
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RTCLocal)
RTCLocal RTC_Local = RTCLocal();

// Calcul UNIX timestamp : Nombre de secondes depuis le 1er janvier 1970 00:00:00 UTC
time_t RTC_Local_Callback()
{
	return (time_t) RTC_Local.getUNIXDateTime();
}

// ********************************************************************************
// Basic Task function to update RTC_Local
// ********************************************************************************

#ifdef RTC_USE_TASK
void RTC_Task_code(void *parameter)
{
	BEGIN_TASK_CODE_UNTIL("RTC_Task");
	for (EVER)
	{
		RTC_Local.update();
		END_TASK_CODE_UNTIL(false);
	}
}
#endif

#endif

// ********************************************************************************
// End of file
// ********************************************************************************
