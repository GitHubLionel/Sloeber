#pragma once

//---------------------------------------------------------------------------
#ifndef IniFilesH
#define IniFilesH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <cstddef>

#define MAX_LINESIZE  1024    // Longueur max d'une ligne lue dans le fichier
#define MAX_SECTION   50      // Nombre de section max
#define CAPACITY_STEP 10      // Taux d'accroissement pour le nombre d'enregistrement par section

typedef struct
{
		char *Key;
		char *Value;
		char *Comment;
} TRecord;

typedef struct
{
		char *Section;
		char *Comment;
		uint16_t NbRecord;
		uint16_t Capacity;
		TRecord *Record;
} TSection;

//---------------------------------------------------------------------------

class IniFiles
{
	public:
		IniFiles()
		{
			;
		}
		IniFiles(const char *aFileName, char aComment = ';');
		~IniFiles();

		bool Begin(void);

		void SetFileName(const char *aFileName);
		void SetComment(char lComment)
		{
			FComment = lComment;
		}
		void SetFloatFormat(const char *aFormat);

		uint16_t GetNbSections()
		{
			return FNbSection;
		}
		TSection* GetSection_ByID(uint16_t i);
		void SaveFile(const char *aFileName = "");
		bool Saved(void)
		{
			return FSaved;
		}

		char* ReadString(const char *Section, const char *Name, const char *Default);
		void ReadString_Dest(const char *Section, const char *Name, const char *Default, char *Dest);
		void WriteString(const char *Section, const char *Name, const char *Value, const char *Comment = "");
		int ReadInteger(const char *Section, const char *Name, int Default);
		void WriteInteger(const char *Section, const char *Name, int Value, const char *Comment = "");
		bool ReadBool(const char *Section, const char *Name, bool Default);
		void WriteBool(const char *Section, const char *Name, bool Value, const char *Comment = "");
		double ReadFloat(const char *Section, const char *Name, double Default);
		void WriteFloat(const char *Section, const char *Name, double Value, const char *Comment = "");
		int ReadIntegerIndex(uint16_t id, const char *Section, const char *Name, int Default);
		void WriteIntegerIndex(uint16_t id, const char *Section, const char *Name, int Value,
				const char *Comment = "");
		double ReadFloatIndex(uint16_t id, const char *Section, const char *Name, double Default);
		void WriteFloatIndex(uint16_t id, const char *Section, const char *Name, double Value,
				const char *Comment = "");

	protected:
		TSection* GetSection(const char *aSection, bool aCreate);
		TRecord* GetRecord(const char *aSection, const char *aKey, bool aCreate);
		void SetValue(const char *aSection, const char *aKey, const char *aValue, const char *aComment);
		char* GetValue(const char *aSection, const char *aKey);
		char* GetValue_Default(const char *aSection, const char *aKey, const char *aDefault);

	private:
		char FComment = ';';
		char FFloatFormat[10];
		char *FFileName = NULL;
		uint16_t FNbSection = 0;
		TSection FSections[MAX_SECTION];
		bool FSaved = true;

		bool ReadFile(void);
		bool WriteFile(const char *aFileName);
		char* GetComment(char *str);
		void FreeMemory();
		void AllocateRecord_ByID(int aSection);
		void AllocateRecord(TSection *aSection);
};

//---------------------------------------------------------------------------
#endif
