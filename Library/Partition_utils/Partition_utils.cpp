#include "Partition_utils.h"

#ifdef USE_FATFS
#include "vfs_api.h"
extern "C" {
#include "esp_vfs_fat.h"
#include "diskio.h"
#include "diskio_wl.h"
#include "vfs_fat_internal.h"
}
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

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(String mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

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
 * if Data_Partition (default false), return free space of data partition else free space of file system partition
 */
size_t Partition_FreeSpace(bool Data)
{
	FSInfo fsInfo;
	// Select data partition
	if (Data)
		SelectPartitionForInfo(Data_Partition);
#ifdef ESP8266
	Info_Partition->info(fsInfo);
#endif
#ifdef ESP32
	FillFSInfo(fsInfo);
#endif
	// Return filesystem partition
	if (Data)
		SelectPartitionForInfo(FS_Partition);

	return fsInfo.totalBytes - fsInfo.usedBytes;
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
		Serial_Info->printf("Max open files:\t%d\r\n", fsInfo.maxOpenFiles);

		// Taille max. d'un chemin
		Serial_Info->printf("Max path lenght:\t%d\r\n", fsInfo.maxPathLength);
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
		Serial_Info->println(" - not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
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
		file = root.openNextFile();
	}
}
#endif

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
// End of file
// ********************************************************************************
