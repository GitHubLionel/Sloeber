#include "Debug_utils.h"
#include "Partition_utils.h"		// Some utils functions for LittleFS/SPIFFS/FatFS

#ifdef USE_SAVE_CRASH
#ifdef ESP8266
#include "EspSaveCrash.h"
#else
#include "esp_core_dump.h"
#endif
#endif

// data Log dans un fichier
const String LOG_Filename = "/log.txt";

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
// Print functions
// ********************************************************************************
void print_debug(String mess, bool ln)
{
#ifdef DEBUG
	if (ln)
		mess += "\r\n";

#ifdef LOG_DEBUG
	if (!Lock_File)
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
#else
	// Juste pour éviter le warning du compilateur
	(void) mess;
	(void) ln;
#endif
}

void print_debug(const char *mess, bool ln)
{
	print_debug(String(mess), ln);
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
#endif
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
