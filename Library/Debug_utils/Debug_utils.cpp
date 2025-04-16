#include "Debug_utils.h"
#include "Partition_utils.h"		// Some utils functions for LittleFS/SPIFFS/FatFS
#ifdef USE_RTCLocal
#include "RTCLocal.h"		      // A pseudo RTC software library
#endif

#ifdef USE_SAVE_CRASH
#ifdef ESP8266
#include "EspSaveCrash.h"
#else
#include "esp_core_dump.h"
#endif
#endif

#ifdef ESP32
#include "esp_err.h"
#include "esp_task_wdt.h"
#endif

#ifdef UART_USE_TASK
#include "Tasks_utils.h"
#endif

/*
 * Serial note ESP8266 :
 * UART 0 possible options are (1, 3), (2, 3) or (15, 13)
 * UART 1 allows only TX on 2 if UART 0 is not (2, 3)
 */

// data Log dans un fichier
const String LOG_Filename = "/log.txt";

// Global boolean to stop the debug print
bool GLOBAL_PRINT_DEBUG = true;

#ifdef USE_SAVE_CRASH
#ifdef ESP8266
EspSaveCrash SaveCrash(0x0010, 0x0800);
#else
void readCoreDump();
esp_err_t esp_core_dump_image_erase_2();
#endif
#endif

// ********************************************************************************
// Ne pas modifier
// ********************************************************************************
#if (defined(SERIAL_DEBUG) || defined(LOG_DEBUG))
#define DEBUG
#endif

#ifdef SERIAL_DEBUG
HardwareSerial *Serial_Info = &Serial; // Other choice Serial1
#endif

// ********************************************************************************
// Initialization of Serial UART if directive USE_UART or SERIAL_DEBUG is defined
// baud: default 115200, NB_BIT = 8, PARITY NONE, NB_STOP_BIT = 1
// ********************************************************************************
void SERIAL_Initialization(int baud)
{
#if (defined(SERIAL_DEBUG) || defined(USE_UART))
	// On démarre le port série : NB_BIT = 8, PARITY NONE, NB_STOP_BIT = 1
	Serial.begin(baud);
	delay(100);  // Pour stabiliser UART
#else
	(void) baud;
#endif
#ifdef SERIAL_DEBUG
	// On attend 5 secondes pour stabiliser l'alimentation et pour lancer la console UART (debug)
	delay(WAIT_SETUP);
#endif
}

// ********************************************************************************
// Function that explain the reason of reset
// ********************************************************************************
#ifdef ESP32
String verbose_print_reset_reason(esp_reset_reason_t reason)
{
  switch (reason)
  {
    case ESP_RST_UNKNOWN     : return ("Reset reason can not be determined");
    case ESP_RST_POWERON     : return ("Reset due to power-on event");
    case ESP_RST_EXT         : return ("Reset by external pin (not applicable for ESP32)");
    case ESP_RST_SW          : return ("Software reset via esp_restart");
    case ESP_RST_PANIC       : return ("Software reset due to exception/panic");
    case ESP_RST_INT_WDT     : return ("Reset (software or hardware) due to interrupt watchdog");
    case ESP_RST_TASK_WDT    : return ("Reset due to task watchdog");
    case ESP_RST_WDT         : return ("Reset due to other watchdogs");
    case ESP_RST_DEEPSLEEP   : return ("Reset after exiting deep sleep mode");
    case ESP_RST_BROWNOUT    : return ("Brownout reset (software or hardware)");
    case ESP_RST_SDIO        : return ("Reset over SDIO");
    case ESP_RST_USB         : return ("Reset by USB peripheral");
    case ESP_RST_JTAG        : return ("Reset by JTAG");
    case ESP_RST_EFUSE       : return ("Reset due to efuse error");
    case ESP_RST_PWR_GLITCH  : return ("Reset due to power glitch detected");
    case ESP_RST_CPU_LOCKUP  : return ("Reset due to CPU lock up (double exception)");
    default : return("NO_MEAN");
  }
}
#endif

// ********************************************************************************
// Function that extract the name of the ino sketch (or a filename)
// extra : add extra info like IDF version (ESP32) and the reason of the restart (default false)
// ********************************************************************************
String getSketchName(const String the_path, bool extra)
{
	int slash_loc = the_path.lastIndexOf('/');
	String the_cpp_name = the_path.substring(slash_loc + 1);
	int dot_loc = the_cpp_name.lastIndexOf('.');
	String ino = the_cpp_name.substring(0, dot_loc) + "\r\n";
	if (extra)
	{
#ifdef ESP32
		esp_reset_reason_t reason = esp_reset_reason();
		ino += "Restart reason: " + String(reason) + "\r\n";
		ino += verbose_print_reset_reason(reason) + "\r\n";
		// Add IDF version for ESP32
		ino += "IDF version: " + String(esp_get_idf_version()) + "\r\n";
#endif
	}
	return ino;
}

// ********************************************************************************
// WatchDog initialization
// ********************************************************************************

#ifdef ESP32
void WatchDog_Init(uint32_t WDT_TIMEOUT_ms)
{
	// https://github.com/espressif/esp-idf/blob/v5.2.2/examples/system/task_watchdog/main/task_watchdog_example_main.c
  //Watchdog initialisation
  esp_task_wdt_deinit();
  // Initialisation de la structure de configuration pour la WDT
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_ms,                     // TimeOut in ms
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Bitmask of all cores
    .trigger_panic = true                             // Enable panic to restart ESP32
  };
  // Initialisation de la WDT avec la structure de configuration
  ESP_ERROR_CHECK(esp_task_wdt_init(&wdt_config));
  esp_task_wdt_add(NULL);  //add current thread to WDT watch
  esp_task_wdt_reset();
  delay(1);  //VERY VERY IMPORTANT for Watchdog Reset
}
#endif

// ********************************************************************************
// Management functions
// ********************************************************************************
#ifdef ESP32
#define LOG_BUFFER_SIZE	512
static char *log_buffer = NULL;
static vprintf_like_t Old_log_vprintf = NULL;
static bool log_keep_UART = false;

int LogMessageOutputToFile(const char *format, va_list args)
{
	int ret = 0;
#ifdef LOG_DEBUG
	if (GLOBAL_PRINT_DEBUG)
	{
		ret = vsnprintf(log_buffer, LOG_BUFFER_SIZE, format, args);
		// Pas trouvé le moyen de supprimer ces deux messages !!
		if ((strstr(log_buffer, "STA already disconnected") == NULL) &&
				(strstr(log_buffer, "Reason: 201 - NO_AP_FOUND") == NULL))
		{
			File logFile = FS_Partition->open(LOG_Filename, "a");
			if (logFile)
			{
				logFile.print(log_buffer);
				logFile.print("\r\n");
				logFile.flush();
				logFile.close();
			}
		}
	}
#endif
	if (log_keep_UART)
		return vprintf(format, args); // To keep the UART message
	else
		return ret;
}

/**
 * This function is used to redirect core debug information to log file
 * Don't forget to add the directive USE_ESP_IDF_LOG to project
 * BEWARE: not work in DEBUG and VERBOSE mode with WIFI
 */
void Core_Debug_Log_Init(bool keepUART)
{
	// Alloc memory for buffer for log message
	log_buffer = (char*) malloc((LOG_BUFFER_SIZE) * sizeof(char));
	log_keep_UART = keepUART;

	Old_log_vprintf = esp_log_set_vprintf(LogMessageOutputToFile);
	esp_log_level_set("*", ESP_LOG_VERBOSE);

	ESP_LOGI("My_Debug", "Log callback running\n");
}

/**
 * Restaure log message to uart
 */
void Core_Debug_Log_Restaure(void)
{
	if (Old_log_vprintf)
		esp_log_set_vprintf(Old_log_vprintf);
}
#endif

void delete_logFile()
{
	FS_Partition->remove(LOG_Filename);
}

// ********************************************************************************
// Print functions
// ********************************************************************************
void print_debug(String mess, bool ln)
{
#ifdef DEBUG
	if (ln)
		mess += "\r\n";

#ifdef LOG_DEBUG
	// print message to log file
	if (!Lock_File && GLOBAL_PRINT_DEBUG)
	{
		File temp = FS_Partition->open(LOG_Filename, "a");
		if (temp)
		{
			temp.print(mess);
			temp.close();
		}
	}
#endif
#ifdef SERIAL_DEBUG
	Serial.print(mess);
	Serial.flush();
#endif

#else  // DEBUG
	// Juste pour éviter le warning du compilateur
	(void) mess;
	(void) ln;
#endif
}

void print_debug(const char *mess, bool ln)
{
	print_debug(String(mess), ln);
}

void print_debug(char val, bool ln)
{
	print_debug((String) val, ln);
}

void print_debug(int val, bool ln)
{
	print_debug((String) val, ln);
}

void print_debug(float val, bool ln)
{
	print_debug((String) val, ln);
}

void print_millis(void)
{
	print_debug((String) millis());
}

// Crash functions
#ifdef USE_SAVE_CRASH
void init_and_print_crash(void)
{
#ifdef ESP32
	esp_core_dump_init();
	if (esp_core_dump_image_check() == ESP_OK)
#endif
		// Sauvegarde du log d'un éventuel crash
		print_crash();
}

#ifdef ESP8266
void print_crash(void)
{
#ifdef SERIAL_DEBUG
	SaveCrash.print();
#endif

#ifdef LOG_DEBUG
	if (!Lock_File)
	{
		File temp = FS_Partition->open(LOG_Filename, "a");
		if (temp)
		{
			SaveCrash.print(temp);
			temp.close();
		}
	}
#endif
	SaveCrash.clear();
}

#else

void print_crash(void)
{
	esp_core_dump_summary_t *summary = (esp_core_dump_summary_t*) malloc(
			sizeof(esp_core_dump_summary_t));
	if (summary)
	{
		esp_log_level_set("esp_core_dump_elf", ESP_LOG_VERBOSE); // so that func below prints stuff.. but doesn't actually work, have to set logging level globally through menuconfig
		printf("Retrieving core dump summary..\n");
		esp_err_t err = esp_core_dump_get_summary(summary);
		if (err == ESP_OK)
		{
			//get summary function already pints stuff
			printf("Getting core dump summary ok.\n");
			//todo: do something with dump summary
			readCoreDump();
		}
		else
		{
			printf("Getting core dump summary not ok. Error: %d\n", (int) err);
			printf("Probably no coredump present yet.\n");
			printf("esp_core_dump_image_check() = %d\n", esp_core_dump_image_check());
		}
		free(summary);
	}
	fflush(stdout);
}

/**
 * https://github.com/MathieuDeprez/ESP32_CoreDump_Arduino_1.0.6/blob/master/src/main.cpp
 */
void readCoreDump()
{
	size_t size = 0;
	size_t address = 0;
	if (esp_core_dump_image_get(&address, &size) == ESP_OK)
	{
		const esp_partition_t *pt = NULL;
		pt = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP,
				"coredump");

		if (pt != NULL)
		{
			uint8_t bf[256];
			char str_dst[640];
			int16_t toRead;

			for (int16_t i = 0; i < (size / 256) + 1; i++)
			{
				strcpy(str_dst, "");
				toRead = (size - i * 256) > 256 ? 256 : (size - i * 256);

				esp_err_t er = esp_partition_read(pt, i * 256, bf, toRead);
				if (er != ESP_OK)
				{
					Serial.printf("FAIL [%x]\n", er);
					//ESP_LOGE("ESP32", "FAIL [%x]", er);
					break;
				}

				for (int16_t j = 0; j < 256; j++)
				{
					char str_tmp[3];

					sprintf(str_tmp, "%02x", bf[j]);
					strcat(str_dst, str_tmp);
				}

				printf("%s", str_dst);
			}
		}
		else
		{
			Serial.println("Partition NULL");
			//ESP_LOGE("ESP32", "Partition NULL");
		}
		esp_core_dump_image_erase();
	}
	else
	{
		Serial.println("esp_core_dump_image_get() FAIL");
		//ESP_LOGI("ESP32", "esp_core_dump_image_get() FAIL");
	}
}

esp_err_t esp_core_dump_image_erase_2()
{
	/* Find the partition that could potentially contain a (previous) core dump. */
	const esp_partition_t *core_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
			ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");
	if (!core_part)
	{
		Serial.println("No core dump partition found!");
		return ESP_ERR_NOT_FOUND;
	}
	if (core_part->size < sizeof(uint32_t))
	{
		Serial.println("Too small core dump partition!");
		return ESP_ERR_INVALID_SIZE;
	}

	esp_err_t err = ESP_OK;
	err = esp_partition_erase_range(core_part, 0, core_part->size);
	if (err != ESP_OK)
	{
		Serial.printf("Failed to erase core dump partition (%d)!\n", err);
		return err;
	}

	// on encrypted flash esp_partition_erase_range will leave encrypted
	// garbage instead of 0xFFFFFFFF so overwriting again to safely signalize
	// deleted coredumps
	const uint32_t invalid_size = 0xFFFFFFFF;
	err = esp_partition_write(core_part, 0, &invalid_size, sizeof(invalid_size));
	if (err != ESP_OK)
	{
		Serial.printf("Failed to write core dump partition size (%d)!\n", err);
	}

	return err;
}
#endif // USE_SAVE_CRASH

#endif

// ********************************************************************************
// UART section
// ********************************************************************************

/**
 * Function that read incoming formated message from UART.
 * The message is stored in UART_Message_Buffer and 0 is add to the end
 * By default, use Serial for UART
 */
bool CheckUARTMessage(HardwareSerial *Serial_Message)
{
	bool StartMessage = false;
	volatile char *pMessage_Buffer;
	uint8_t data;

	while (Serial_Message->available())
	{
		data = Serial_Message->read();
		if (StartMessage)
		{
			if (data == END_DATA)
			{
				// End the string
				*pMessage_Buffer = 0;
				return true;
			}
			else
			{
				*pMessage_Buffer++ = (char) data;
				// Check the buffer size. If true, message is lost
				if (pMessage_Buffer - UART_Message_Buffer == BUFFER_SIZE)
				{
					StartMessage = false;
				}
			}
		}
		else
			if (data == BEGIN_DATA)
			{
				StartMessage = true;
				pMessage_Buffer = &UART_Message_Buffer[0];
				*pMessage_Buffer = 0;
			}
	}
	return false;
}

/**
 * A minimal auto reset function
 */
void __attribute__((weak)) Auto_Reset(void)
{
#ifdef ESP8266
	ESP.reset();
#endif
#ifdef ESP32
	ESP.restart();
#endif
}

/**
 * should be redefined elsewhere
 */
const String __attribute__((weak)) GetIPaddress(void)
{
	return "";
}

bool __attribute__((weak)) getESPMacAddress(String &mac)
{
	(void) mac;
	return false;
}

/**
 * Check the UART UART_Message_Buffer to get basic message :
 * - GET_IP: Send current IP
 * - GET_MAC: Send MAC address
 * - GET_TIME: Send Time
 * - RESET_ESP: Reset ESP
 * then send back the responce to UART
 */
bool BasicAnalyseMessage(void)
{
	char datetime[30] = {0};
	char *date = NULL;

	if (strcmp((char*) UART_Message_Buffer, "GET_IP") == 0)
	{
		printf_message_to_UART(GetIPaddress());
		return true;
	}
	else
		if (strcmp((char*) UART_Message_Buffer, "GET_MAC") == 0)
		{
			String mac;
			if (getESPMacAddress(mac))
			  printf_message_to_UART(mac.c_str());
			return true;
		}
		else
			if (strcmp((char*) UART_Message_Buffer, "GET_TIME") == 0)
			{
#ifdef USE_RTCLocal
				printf_message_to_UART("TIME=" + String(RTC_Local.getFormatedDateTime(datetime)));
#endif
				return true;
			}
			else
				if (strcmp((char*) UART_Message_Buffer, "GET_DATE=") == 0)
				{
#ifdef USE_RTCLocal
					printf_message_to_UART(String(RTC_Local.getDateTime(datetime, false, '#')) + "#END_DATE\r\n", false);
#endif
					return true;
				}
				else
					if ((date = strstr((char*) UART_Message_Buffer, "DATE=")) != NULL)
					{
#ifdef USE_RTCLocal
						// Format DD/MM/YY#hh:mm:ss
						date += 5;
						strncpy(datetime, date, 17);
						datetime[18] = 0;
						RTC_Local.setDateTime(datetime, false, false);
						// Sauvegarde de l'heure
						RTC_Local.saveDateTime();
						// Renvoie la nouvelle heure
						printf_message_to_UART(String(RTC_Local.getDateTime(datetime, false, '#')) + "#END_DATE\r\n", false);
#endif
						return true;
					}
					else
						if (strcmp((char*) UART_Message_Buffer, "RESET_ESP") == 0)
						{
							Auto_Reset();
							return true;
						}
	return false;
}

/**
 * Function that send to UART a formated message with BEGIN_DATA and END_DATA
 * All requests to UART must use this function.
 * If balise is true (default) then message is enclosed by BEGIN_DATA and END_DATA.
 * By default, use Serial for UART
 */
void printf_message_to_UART(const char *mess, bool balise, HardwareSerial *Serial_Message)
{
	if (balise)
		Serial_Message->printf("%c%s%c", BEGIN_DATA, mess, END_DATA);
	else
		Serial_Message->printf("%s", mess);
	Serial_Message->flush();
}

/**
 * Function that send to UART a formated message with BEGIN_DATA and END_DATA
 * All requests to UART must use this function.
 * If balise is true (default) then message is enclosed by BEGIN_DATA and END_DATA.
 * By default, use Serial for UART
 */
void printf_message_to_UART(const String &mess, bool balise, HardwareSerial *Serial_Message)
{
	if (balise)
		Serial_Message->printf("%c%s%c", BEGIN_DATA, mess.c_str(), END_DATA);
	else
		Serial_Message->printf("%s", mess.c_str());
	Serial_Message->flush();
}

// ********************************************************************************
// Other utilitary function
// ********************************************************************************

/**
 * Recherche de données entre deux expressions dans une chaine de caractère
 * data : la chaine de caractère
 * B_Begin : la balise de début
 * B_End : la balise de fin
 * value : renvoie la chaine entre les balises (l'espace mémoire doit exister)
 * len : longueur de la chaine trouvée
 * Le fonction retourne un pointeur sur la balise de début si elle est trouvée, NULL sinon
 */
char* Search_Balise(uint8_t *data, const char *B_Begin, const char *B_end, char *value,
		uint16_t *len)
{
	char *pos_B_Begin;
	char *pos_B_end;
	char *result = NULL;

	*len = 0;
	if ((pos_B_Begin = strstr((char*) data, B_Begin)) != NULL)
	{
		pos_B_Begin += strlen(B_Begin);

		if ((pos_B_end = strstr((char*) pos_B_Begin, B_end)) != NULL)
		{
			*len = (uint16_t) (pos_B_end - pos_B_Begin);
			if (*len != 0)
			{
				strncpy((char*) value, (char*) pos_B_Begin, *len);
				value[*len] = 0;
			}
			else
				value[0] = 0;
			result = pos_B_Begin;
		}
	}
	return result;
}

// ********************************************************************************
// Basic Task function to analyse UART message
// ********************************************************************************
/**
 * A basic Task to analyse UART message
 */
#ifdef UART_USE_TASK

/**
 * This functions should be redefined elsewhere with your analyse
 */
bool __attribute__((weak)) UserAnalyseMessage(void)
{
  return true;
}

void UART_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("UART_Task");
	for (EVER)
	{
		if (CheckUARTMessage())
		{
			if (! BasicAnalyseMessage())
				UserAnalyseMessage();
		}
		END_TASK_CODE(false);
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
