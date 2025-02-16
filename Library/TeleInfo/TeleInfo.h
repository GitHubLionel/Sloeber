#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#ifdef ESP8266
#include <SoftwareSerial.h>
#endif
#ifdef ESP32
#include <HardwareSerial.h>
#endif

// To create a basic task to check TeleInfo every 1 s
#ifdef TELEINFO_USE_TASK
#define TELEINFO_DATA_TASK(start)	{(start), "TELEINFO_Task", 1024, 5, 1000, CoreAny, TELEINFO_Task_code}
void TELEINFO_Task_code(void *parameter);
#else
#define TELEINFO_DATA_TASK(start)	{}
#endif

#define LABEL_MAX_SIZE     9  // Maximum 8 caractères pour MOTDETAT
#define DATA_MAX_SIZE     13  // Maximum 12 caractères pour ADCO
#define LINE_MAX_COUNT    30  // Avec les options tempo et en tri
#define FRAME_MAX_SIZE   350  // Buffer trame complète

// La longueur de la trame de base est 141, reçue en 1180 ms
// Exemple :
//ADCO 040622146651 <
//OPTARIF BASE 0
//ISOUSC 15 <
//BASE 029757986 @
//PTEC TH.. $
//IINST 000 W
//IMAX 003 B
//PAPP 00000 !
//MOTDETAT 000000 B

typedef struct
{
	char *label;          // Pointeur sur le label
	char checkLabel;			// Checksum du label
	char *data;						// Pointeur sur la data
	uint8_t lenData;			// Longueur de la data
	char *checksum;				// Pointeur sur le checksum complet
} TI_TypeDef;

class TeleInfo
{
	private:
#ifdef ESP8266
		SoftwareSerial *tiSerial;
#endif
#ifdef ESP32
		HardwareSerial *tiSerial;
#endif
		bool _tiRunning = false;

		bool _frameBegin = false;
		volatile bool _isAvailable = false;

		char _frame[FRAME_MAX_SIZE + 1];
		uint16_t _frameIndex = 0;

		char _label[LINE_MAX_COUNT][LABEL_MAX_SIZE];
		char _data[LINE_MAX_COUNT][DATA_MAX_SIZE];
		volatile int _labelCount = 0;

		// Structure complète de la frame
		TI_TypeDef _structure[LINE_MAX_COUNT];
		bool use_structure = false;

		char TI_PowerVA_str[6] = {0}; // 5 caractères
		char TI_IndexWh_str[10] = {0}; // 9 caractères

		uint8_t ID_PowerVA = 7;
		uint8_t ID_IndexWh = 3;

		void resetAll();
		uint16_t DataReceived(uint8_t ch);
		int decode(int beginIndex, char *field, unsigned char *sum, uint8_t max_size);
		bool decodeFrame();
		void build_Structure();
		bool get_Structure_Data(uint8_t id, char *result);

	public:
		TeleInfo(uint8_t rxPin, uint32_t refresh_ms = 10000, uint32_t baud = 1200);
		~TeleInfo();

		void Process();
		void Pause();
		void Resume();

		bool Configure(uint32_t timeout);
		bool Available();
		void ResetAvailable();

		/**
		 * return the string value of a label
		 * return NULL if this label is not found
		 */
		const char* getStringVal(const char *label);

		/**
		 * return the long value of a label
		 * return a negative value if this label is not found or value is not a long
		 */
		int32_t getLongVal(const char *label);

		uint32_t getPowerVA();
		uint32_t getIndexWh();
		char *getPowerVA_str()
		{
			return TI_PowerVA_str;
		}
		char *getIndexWh_str()
		{
			return TI_IndexWh_str;
		}

		void PrintAllToSerial();
		void Test();
};

