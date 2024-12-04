#pragma once

/**
 * Quelques fonctions utiles pour l'analyse du filesystem
 * Ne pas oublier d'ajouter la librairie adéquate au projet
 */

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"

#ifdef ESP8266
	#ifdef USE_SPIFFS  // SPIFFS partition
	#include "spiffs_api.h"
  #else
		#ifdef USE_FATFS  // FatFS partition
		#error "No fatfs partition for ESP8266"
		#else
		#include <LittleFS.h>
		#define USE_LITTLEFS  // LITTLEFS partition
		#endif
  #endif
	#define PART_TYPE	FS
#endif

#ifdef ESP32
	#ifdef USE_SPIFFS  // SPIFFS partition
	#include <SPIFFS.h>
	#define PART_TYPE	fs::SPIFFSFS
  #else
		#ifdef USE_FATFS  // FatFS partition
		#include <FFat.h>
		#define PART_TYPE	fs::F_Fat
		#else
		#include <LittleFS.h>
		#define USE_LITTLEFS  // LITTLEFS partition
		#define PART_TYPE	fs::LittleFSFS
		#endif
	#endif
#endif

// The pre-defined filesystem partition
extern PART_TYPE *FS_Partition;

/**
 * A data partition. By default it is the same as FileSystem partition
 * To create it, call CreateOpenDataPartition() function.
 * The partition "/data" must exist in the partitions.csv file
 */
extern PART_TYPE *Data_Partition;

// Lock when use file
extern volatile bool Lock_File;

PART_TYPE* CreatePartition(void);
void SelectPartitionForInfo(PART_TYPE *part);
bool CreateOpenDataPartition(bool ForceFormat, bool ShowInfo = false);

void CheckBeginSlash(String &path);

void ESPinformations(void);
void Partition_Info(void);
void Partition_ListDir(void);
size_t Partition_FreeSpace(void);
String formatBytes(float bytes, int id_multi);
