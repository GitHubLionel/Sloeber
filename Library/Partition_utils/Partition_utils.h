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
#include <list>

// Typedef for a list of String
typedef std::list<String> StringList_td;

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
void UnmountPartition(void);

void CheckBeginSlash(String &path);
void CheckEndSlash(String &path);

void ESPinformations(void);
void Partition_Info(void);
void Partition_ListDir(void);
size_t Partition_FreeSpace(bool data_partition = false);
size_t Partition_FileSize(const String &file, bool data_partition = false);
String formatBytes(float bytes, int id_multi);

void ListDirToUART(const String &dirname, bool data_partition = false);
void SendFileToUART(const String &filename, bool data_partition = false);
bool DeleteFile(const String &filename, bool data_partition = false);

/**
 * Get the list of the name of the files in a directory
 * @param: data_partition : true for data partition else filesystem partition
 * @param: dir : the name of the directory
 * @return: list : the list of the file in the directory
 * @param: filter : the list of the extension for the file allowed
 * @param: skipfile : the list of files to exclude
 * @return: true if the list is filled
 */
bool FillListFile(bool data_partition, const String &dir, StringList_td &list,
		const StringList_td &filter = {}, const StringList_td &skipfile = {});
void PrintListFile(StringList_td &list);
String ListFileToString(StringList_td &list);

#ifdef USE_TARGZ_LIB
void GZFile(const String &file, bool data_partition);
#endif

