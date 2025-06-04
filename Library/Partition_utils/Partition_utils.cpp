#include "Partition_utils.h"
#include "Debug_utils.h"

#ifdef USE_FATFS
#include "vfs_api.h"
extern "C" {
#include "esp_vfs_fat.h"
#include "diskio.h"
#include "diskio_wl.h"
#include "vfs_fat_internal.h"
}
#endif

#ifdef USE_TARGZ_LIB
#ifndef USE_SPIFFS
#define DEST_FS_USES_LITTLEFS
#endif
#include <ESP32-targz.h>
#endif

/**
 * Define shortcut to pre-defined filesystem partition
 */
#ifdef USE_SPIFFS
PART_TYPE *FS_Partition = &SPIFFS;
#endif
#ifdef USE_FATFS
PART_TYPE *FS_Partition = &FFat;
#else
PART_TYPE *FS_Partition = &LittleFS;
#endif

// A boolean to securise the file access
volatile bool Lock_File = false;

/**
 * Some functions about ESP and LittleFS or SPIFFS
 * Must define the serial port Serial_Info in main ino file
 * Choice are : Serial or Serial1
 */
extern HardwareSerial *Serial_Info;

/**
 * The selected partition to get information
 * At first time, it is the filesystem partition
 */
PART_TYPE *Info_Partition = FS_Partition;

/**
 * Create a partition of type PART_TYPE
 */
PART_TYPE *CreatePartition(void)
{
#ifdef ESP8266
// toDo définir FS_PHYS_ADDR, FS_PHYS_SIZE, ...
#ifdef USE_SPIFFS
	return new PART_TYPE(FSImplPtr(new spiffs_impl::SPIFFSImpl(FS_PHYS_ADDR, FS_PHYS_SIZE, FS_PHYS_PAGE, FS_PHYS_BLOCK, 5)));
#endif
#ifdef USE_FATFS
	return NULL;
#else
	return new PART_TYPE(FSImplPtr(new littlefs_impl::LittleFSImpl(FS_PHYS_ADDR, FS_PHYS_SIZE, FS_PHYS_PAGE, FS_PHYS_BLOCK, 5)));
#endif

#else // ESP32
#ifdef USE_FATFS
  return new PART_TYPE(FSImplPtr(new VFSImpl()));
#else
  return new PART_TYPE();
#endif
#endif
}

/**
 * The selected partition to get information
 * Must be called before getting data if we change partition
 * Default choice is FS_Partition
 */
void SelectPartitionForInfo(PART_TYPE *part)
{
	Info_Partition = part;
}

// ********************************************************************************
// Function to create and open a data partition
// ********************************************************************************
// Par défaut, la partition data est la même que la partition filesystem
PART_TYPE *Data_Partition = FS_Partition;

/**
 * Create and open a data partition
 * The partition "/data" must exist in the partitions.csv file
 * ShowInfo : print information of data partition. Only in debug mode.
 */
bool CreateOpenDataPartition(bool ForceFormat, bool ShowInfo)
{
	Data_Partition = CreatePartition();

#ifdef ESP8266
	(void) ForceFormat;
	if (Data_Partition->begin())
#else
	if (ForceFormat)
#ifdef USE_FATFS
		Data_Partition->format(0, (char *)"data");
#else
		Data_Partition->format();
#endif

	if (Data_Partition->begin(true, "/data", 10, "data"))
#endif
	{
		print_debug(F("** Data partition open **"));
		if (ShowInfo)
		{
#ifdef SERIAL_DEBUG
			SelectPartitionForInfo(Data_Partition);
			Partition_Info();
			Partition_ListDir();
			SelectPartitionForInfo(FS_Partition);
#endif
		}
		return true;
	}
	else
	{
		delete Data_Partition;
		// On revient sur la partition filesystem
		Data_Partition = FS_Partition;
		print_debug(F("** Data partition open failed **"));
		return false;
	}
}

/**
 * Unmount all partitions : filesystem and data if exist
 */
void UnmountPartition(void)
{
#ifdef ESP32
	Core_Debug_Log_Restaure();
#endif
	if (Data_Partition != FS_Partition)
		Data_Partition->end();
	FS_Partition->end();
}

// ********************************************************************************
// Check if path begin with a / and add it if not present
// ********************************************************************************
void CheckBeginSlash(String &path)
{
	// Add slash if necessary
	if (!path.startsWith("/"))
		path = "/" + path;
}

// ********************************************************************************
// Check if path end with a / and add it if not present
// ********************************************************************************
void CheckEndSlash(String &path)
{
	// Add slash if necessary
	if (!path.endsWith("/"))
		path = path + "/" ;
}

// ********************************************************************************
// Functions prototype
// ********************************************************************************
String formatBytes(float bytes, int id_multi);

#ifdef ESP32
struct FSInfo
{
		size_t totalBytes = 0;
		size_t usedBytes = 0;
		size_t blockSize = 0;
		size_t pageSize = 0;
		size_t maxOpenFiles = 0;
		size_t maxPathLength = 0;
};

bool FillFSInfo(FSInfo &info)
{
	info.totalBytes = Info_Partition->totalBytes();
	info.usedBytes = Info_Partition->usedBytes();
	info.blockSize = 4096;
	return true;
}
#endif

void ESPinformations(void)
{
	uint32_t realSize = 0;
#ifdef ESP8266
	realSize = ESP.getFlashChipRealSize();
#endif
#ifdef ESP32
	realSize = ESP.getFlashChipSize();
#endif
	uint32_t ideSize = ESP.getFlashChipSize();
	FlashMode_t ideMode = ESP.getFlashChipMode();

	Serial_Info->println("------------------------------");
	Serial_Info->println("ESP Flash and Sketch info");
	Serial_Info->println("------------------------------");

#ifdef ESP8266
	Serial_Info->printf("Flash chip Id:   %08X\r\n", ESP.getFlashChipId());
#endif
#ifdef ESP32
	Serial_Info->printf("Flash chip Mode:   %08X\r\n", ESP.getFlashChipMode());
#endif
	Serial_Info->printf("Flash real size: %u bytes (%s)\r\n\r\n", (unsigned int)realSize,
			formatBytes(realSize, 0).c_str());

	Serial_Info->printf("Flash ide  size: %u bytes (%s)\r\n", (unsigned int)ideSize,
			formatBytes(ideSize, 0).c_str());
	Serial_Info->printf("Flash ide speed: %u MHz\r\n", (unsigned int)(ESP.getFlashChipSpeed() / 1000000));
	Serial_Info->printf("Flash ide mode:  %s\r\n",
			(ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" :
				ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

	if (ideSize != realSize)
	{
		Serial_Info->println("Flash Chip configuration wrong!\r\n");
	}
	else
	{
		Serial_Info->println("Flash Chip configuration ok.\r\n");
	}
	Serial_Info->printf("CPU Frequency: %ld MHz\r\n", F_CPU / 1000000L);
#ifdef ESP8266
	Serial_Info->printf("Chip Id: %u\r\n", ESP.getChipId());
#endif
#ifdef ESP32
	Serial_Info->printf("Chip Model: %s\r\n", ESP.getChipModel());
#endif

	// Sketch
	realSize = ESP.getSketchSize();
	Serial_Info->printf("Sketch size: %u bytes (%s)\r\n", (unsigned int)realSize, formatBytes(realSize, 0).c_str());
	realSize = ESP.getFreeSketchSpace();
	Serial_Info->printf("Sketch free space: %u bytes (%s)\r\n\r\n", (unsigned int)realSize,
			formatBytes(realSize, 0).c_str());

	// IDF version
#ifdef ESP32
	Serial_Info->printf("IDF version: %s\r\n\r\n", esp_get_idf_version());
#endif
}

/**
 * Return free space in byte
 * @param data_partition: true use Data_Partition else use FS_Partition. Default false = use FS_Partition
 */
size_t Partition_FreeSpace(bool data_partition)
{
	FSInfo fsInfo;
	// Select data partition
	if (data_partition)
		SelectPartitionForInfo(Data_Partition);
#ifdef ESP8266
	Info_Partition->info(fsInfo);
#endif
#ifdef ESP32
	FillFSInfo(fsInfo);
#endif
	// Return filesystem partition
	if (data_partition)
		SelectPartitionForInfo(FS_Partition);

	return fsInfo.totalBytes - fsInfo.usedBytes;
}

/**
 * Return the size in byte of the specified file
 * @param data_partition: true use Data_Partition else use FS_Partition. Default false = use FS_Partition
 */
size_t Partition_FileSize(const String &file, bool data_partition)
{
	File f;
	String str_file = file;
	CheckBeginSlash(str_file);
#ifdef ESP8266
	if (data_partition)
	  f = Data_Partition->open(str_file.c_str(), "r");
	else
		f = FS_Partition->open(str_file.c_str(), "r");
#else
	if (data_partition)
	  f = Data_Partition->open(str_file.c_str());
	else
		f = FS_Partition->open(str_file.c_str());
#endif

	if ((!f) || (f.isDirectory()))
	{
		print_debug(F("Failed to open file"));
		if (f)
			f.close();
		return 0;
	}
	size_t size = f.size();
	f.close();
	return size;
}

// Get all informations about LittleFS, SPIFFS or FatFS partition
void Partition_Info(void)
{
	size_t s;
	FSInfo fsInfo;
#ifdef ESP8266
	Info_Partition->info(fsInfo);
#endif
#ifdef ESP32
	FillFSInfo(fsInfo);
#endif

	Serial_Info->println("------------------------------");
	Serial_Info->println("Partition system info");
	Serial_Info->println("------------------------------");

	// Taille de la zone de fichier
	Serial_Info->printf("Total space:\t%d bytes (%s)\r\n", fsInfo.totalBytes,
			formatBytes(fsInfo.totalBytes, 0).c_str());

	// Espace total utilisé
	Serial_Info->printf("Total space used:\t%d bytes (%s)\r\n", fsInfo.usedBytes,
			formatBytes(fsInfo.usedBytes, 0).c_str());

	// Espace libre
	s = fsInfo.totalBytes - fsInfo.usedBytes;
	Serial_Info->printf("Free space:\t%d bytes (%s)\r\n", s, formatBytes(s, 0).c_str());

	// Taille d'un bloc et page
	Serial_Info->printf("Block size:\t%d bytes (%s)\r\n", fsInfo.blockSize,
			formatBytes(fsInfo.blockSize, 0).c_str());

	// Nombre de bloc
	if (fsInfo.blockSize != 0)
	{
		s = fsInfo.totalBytes / fsInfo.blockSize;
		Serial_Info->printf("Block number:\t%d\r\n", s);
		Serial_Info->printf("Page size:\t%d bytes (%s)\r\n", fsInfo.totalBytes,
				formatBytes(fsInfo.totalBytes, 0).c_str());
#ifdef ESP8266
		Serial_Info->printf("Max open files:\t%d\r\n", fsInfo.maxOpenFiles);

		// Taille max. d'un chemin
		Serial_Info->printf("Max path lenght:\t%d\r\n", fsInfo.maxPathLength);
#endif
	}
	Serial_Info->println();
}

// ********************************************************************************
// Directory operation
// ********************************************************************************

#ifdef ESP8266
void Partition_RecurseDir(Dir dir)
{
	while (dir.next())
	{
		String fileName = dir.fileName();
		size_t fileSize = dir.fileSize();
		if (dir.isFile())
		{
			time_t time = dir.fileTime();
#if defined(_USE_LONG_TIME_T)
			Serial_Info->printf("\tFile: %s, size: %s, time: %ld\r\n", fileName.c_str(),
					formatBytes(fileSize, 0).c_str(), time);
#else
			Serial_Info->printf("\tFile: %s, size: %s, time: %lld\r\n", fileName.c_str(),
					formatBytes(fileSize, 0).c_str(), time);
#endif
		}
		else
		{
			if (dir.isDirectory())
			{
				Serial_Info->printf("\t>> Enter Dir: %s\r\n", fileName.c_str());
				Partition_RecurseDir(Info_Partition->openDir(fileName));
				Serial_Info->printf("\t<< Exit Dir: %s\r\n", fileName.c_str());
			}
		}
	}
}

void ListDirToUART(const String &dirname, bool data_partition)
{
	String path = dirname;
	CheckBeginSlash(path);
	CheckEndSlash(path);

	Dir dir;
	if (data_partition)
		dir = Data_Partition->openDir(path);
	else
		dir = FS_Partition->openDir(path);

	uint8_t buffer[300] = {0};

	while (dir.next())
	{
		String fileName = dir.fileName();
		size_t fileSize = dir.fileSize();
		if (dir.isFile())
		{
			if (path.length() == 1) // root
				sprintf((char*) buffer, "#/%s (%u octets)\r\n", fileName.c_str(), (uint32_t) fileSize);
			else
				sprintf((char*) buffer, "#%s%s (%d octets)\r\n", path.c_str(), fileName.c_str(), (uint32_t) fileSize);
			printf_message_to_UART((const char*) buffer, false);
		}
		else
		{
			if (dir.isDirectory())
			{
				ListDirToUART(fileName, data_partition);
			}
		}
	}
}

#endif
#ifdef ESP32
// From https://wokwi.com/projects/383917656227391489
void scanDir(fs::FS &fs, const char *dirname)
{
	File root = fs.open(dirname);
	if (!root)
	{
		Serial_Info->println("- failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		root.close();
		Serial_Info->println(" - not a directory");
		return;
	}

	while (true)
	{
		File file = root.openNextFile();
		if (!file)
			break;

		if (file.isDirectory())
		{
			Serial_Info->printf("\t>> Enter Dir: %s\r\n", file.name());
			scanDir(fs, file.path());
			Serial_Info->printf("\t<< Exit Dir: %s\r\n", file.name());
		}
		else
		{
			time_t time = file.getLastWrite();
#if defined(_USE_LONG_TIME_T)
			Serial_Info->printf("\tFile: %s, size: %s, time: %ld\r\n", file.name(),
					formatBytes(file.size(), 0).c_str(), time);
#else
			Serial_Info->printf("\tFile: %s, size: %s, time: %lld\r\n", file.name(),
					formatBytes(file.size(), 0).c_str(), time);
#endif
		}
		file.close();
	}

	root.close();
}

void ListDirToUART(const String &dirname, bool data_partition)
{
	String path = dirname;
	CheckBeginSlash(path);
	CheckEndSlash(path);

	File root;
	if (data_partition)
		root = Data_Partition->open(path);
	else
		root = FS_Partition->open(path);

	if (!root)
	{
		printf_message_to_UART("#Partition not mounted\r\n", false);
		return;
	}
	if (!root.isDirectory())
	{
		root.close();
		printf_message_to_UART("#Not a directory\r\n", false);
		return;
	}

  uint8_t buffer[300] = {0};

	while (true)
	{
		File file = root.openNextFile();
		if (!file)
			break;

		if (file.isDirectory())
		{
			ListDirToUART(file.path(), data_partition);
		}
		else
		{
		  if (strlen(path.c_str()) == 1) // root
		    sprintf((char *)buffer,"#/%s (%ld octets)\r\n", file.name(), (uint32_t)file.size());
		  else
		    sprintf((char *)buffer,"#%s%s (%ld octets)\r\n", path.c_str(), file.name(), (uint32_t)file.size());
		  printf_message_to_UART((const char *)buffer, false);
		}
		file.close();
	}

	root.close();
}
#endif

void SendFileToUART(const String &filename, bool data_partition)
{
	// On vérifie qu'on n'est pas en train de l'uploader
	if (Lock_File)
		return;

	PART_TYPE *partition = FS_Partition;
	if (data_partition)
		partition = Data_Partition;

	String path = filename;
	CheckBeginSlash(path);

	// Ouvre le fichier en read
	Lock_File = true;
	if (partition->exists(path))
	{
		File temp = partition->open(path, "r");
		if (temp)
		{
			uint8_t *buffer = new uint8_t[1024];
			// Read until end
			temp.seek(0); // Be sure to be at beginning of file
			while (temp.available())
			{
				size_t len = temp.read(buffer, 1024);
				printf_message_to_UART(buffer, len, false);
				yield();
			}
			delete[] buffer;
			temp.close();
		}
	}

	Lock_File = false;
}


bool DeleteFile(const String &filename, bool data_partition)
{
	PART_TYPE *partition = FS_Partition;
	if (data_partition)
		partition = Data_Partition;

	String path = filename;
	CheckBeginSlash(path);

	// Can not delete root !
	if (path.equals("/"))
		return false;

	// Delete file, wait if Lock_File
	while (Lock_File)
		delay(10);
	if (partition->exists(path))
	{
		return partition->remove(path);
	}
	return false;
}

void Partition_ListDir(void)
{
	Serial_Info->println("------------------------------");
	Serial_Info->println("List files");
	Serial_Info->println("------------------------------");
	// Ouvre le dossier racine | Open folder
#ifdef ESP8266
	Dir dir = Info_Partition->openDir("/");
	// Affiche le contenu du dossier racine | Print dir the content
	Serial_Info->println("\tEnter Dir: /");
	Partition_RecurseDir(dir);
#endif
#ifdef ESP32
	Serial_Info->println("\tEnter Dir: /");
	scanDir(*Info_Partition, "/");
#endif
}

String formatBytes(float bytes, int id_multi)
{ // convert sizes in bytes to kB, MB, GB
	const String multi[] = {" ", " k", " M", " G"};
	if (bytes < 1024)
		return String(bytes) + multi[id_multi] + "o";
	else
		return formatBytes(bytes / 1024, id_multi + 1);
}

// ********************************************************************************
// List of filename of a directory
// ********************************************************************************

/**
 * Check if the filename is valid
 * @Param file: the filename
 * @Param filter: list of the extension allowed with the point (ex: .csv). Leave empty if no extension
 * @Param skipfile: list of filename to exclude
 */
inline bool checkFile(const String &file, const StringList_td &filter, const StringList_td &skipfile)
{
	bool ret = true;
	// Check file to exclude
	for (auto const &skip : skipfile)
	{
		if ((strstr(file.c_str(), skip.c_str()) != NULL))
		{
			ret = false;
			break;
		}
	}
	// Check extension to allow
	if (ret && (filter.size() > 0))
	{
		ret = false;
		for (auto const &ext : filter)
		{
			if (file.endsWith(ext))
			{
				ret = true;
				break;
			}
		}
	}
	return ret;
}

#ifdef ESP8266
bool FillListFile(bool data_partition, const String &dir, StringList_td &list,
		const StringList_td &filter, const StringList_td &skipfile)
{
	Dir root;
	if (data_partition)
		root = Data_Partition->openDir(dir);
	else
		root = FS_Partition->openDir(dir);

	if (!root.isDirectory())
	{
		print_debug(F("Failed to open data partition"));
		return false;
	}

	while (root.next())
	{
		if (!root.isDirectory())
		{
			const String filename = root.fileName();
			if (checkFile(filename, filter, skipfile))
			{
				list.push_back(filename);
			}
		}
	}
	return true;
}
#endif

#ifdef ESP32
bool FillListFile(bool data_partition, const String &dir, StringList_td &list,
		const StringList_td &filter, const StringList_td &skipfile)
{
	File root;
	if (data_partition)
		root = Data_Partition->open(dir);
	else
		root = FS_Partition->open(dir);

	if (!root)
	{
		print_debug(F("Failed to open data partition"));
		return false;
	}
	if (!root.isDirectory())
	{
		root.close();
		print_debug(F("Failed to open data partition"));
		return false;
	}

	list.clear();
	while (true)
	{
		File file = root.openNextFile();
		if (!file)
			break;

		if (!file.isDirectory())
		{
			const char *filename = file.name();
			if (checkFile(filename, filter, skipfile))
			{
				list.push_back("/" + (String) filename);
			}
		}
		file.close();
	}
	root.close();
	return true;
}
#endif

/**
 * Print the list of file
 */
void PrintListFile(StringList_td &list)
{
	for (auto const &l : list)
	{
		print_debug(l);
	}
}

/**
 * Return the list of file in a string
 */
String ListFileToString(StringList_td &list)
{
	String result = "";
	for (auto const &l : list)
	{
		result += l + "\r\n";
	}
	return result;
}

// ********************************************************************************
// GZ file
// ********************************************************************************

#ifdef USE_TARGZ_LIB

/**
 * Compress the specified file
 * @Param filename: the file to be compressed
 * @Param data_partition: true use Data_Partition else use FS_Partition
 * @Param delete_file: delete the original file after gzip. Default false
 * The name of the compressed file is the name of the file + ".gz"
 */
void GZFile(const String &filename, bool data_partition, bool delete_file)
{
	File src, dst;
	String file = filename;
	PART_TYPE *part = FS_Partition;

	if (data_partition)
		part = Data_Partition;

	print_debug("### GZ File ###");
	CheckBeginSlash(file);

	// open the uncompressed text file for streaming
	src = part->open(file.c_str(), "r");
	if (!src)
	{
		print_debug("[GZFile] Unable to read input file, halting");
		return;
	}

	// open the gz file for writing, delete it before if exists
	String file_gz = file + ".gz";
	if (part->exists(file_gz))
		part->remove(file_gz);
	dst = part->open(file_gz, "w");
	if (!dst)
	{
		print_debug("[GZFile] Unable to create output file, halting");
		src.close();
		return;
	}

//	LZPacker::setProgressCallBack(LZPacker::defaultProgressCallback);
	size_t dstLen = LZPacker::compress(&src, src.size(), &dst);
	size_t srcLen = src.size();

	float done = float(dstLen) / float(srcLen);
	float left = -(1.0f - done);

	char message[120] = {0};
	sprintf(message, "[GZFile] Deflated %d bytes to %d bytes (%s%.1f%s)", srcLen, dstLen, left > 0 ? "+" : "",
			left * 100.0, "%");
	print_debug(message);

	src.close();
	dst.close();

	if (delete_file)
		part->remove(file);
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
