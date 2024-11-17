//---------------------------------------------------------------------------
#include "IniFiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Partition_utils.h"

#define ALLOC_CHAR(field)	(char *) malloc((strlen(field) + 1) * sizeof(char))
#define ALLOC_CHAR_SIZE(size)	(char *) malloc((size) * sizeof(char))

/// An usefull shortcut to delete and null a pointer
/// Note the use of brace {} to enclose the operations
#define FREE_AND_NULL(p)	\
	{ if ((p) != NULL) \
		{ free(p);	\
			(p) = NULL; }}

/**
 * Les fichiers ini sont des fichiers textes contenant des informations formatées
 * Les données sont regroupées dans des sections repérées par des crochets : [section]
 * On peut mettre des commentaires en utilisant un caractére spécial
 * Exemple
 * [DATA]
 * data1=45.66 ; premiére donnée réelle
 * data2=96 ; deuxiéme donnée entiére
 * data3=un texte  ; troisiéme donnée une chaine de caractére
 * [AUTRE]
 * val1=True  ; une donnée booléenne dans une autre section
 */

//---------------------------------------------------------------------------
/* ================================================================== */
/*                          Constructor/Destructor                    */
/* ================================================================== */

/**
 * Création d'un fichier ini
 * 	aFileName : nom du fichier
 * 	aComment : caractére préfixant un commentaire. ';' par défaut
 * 	Le format des réels par défaut est %3.6f. SetFloatFormat pour le changer.
 */
IniFiles::IniFiles(const char *aFileName, char aComment)
{
	SetFileName(aFileName);

	FNbSection = 0;
	if (aComment != 0)
		FComment = aComment;
	SetFloatFormat("%3.6f");
	FSaved = true;
}

//---------------------------------------------------------------------------
/**
 * Détruit la structure IniFiles
 * 	- effectue la sauvegarde du fichier si nécessaire
 * 	- libére la mémoire
 */
IniFiles::~IniFiles(void)
{
	if (!FSaved)
		SaveFile("");
	FreeMemory();
	FREE_AND_NULL(FFileName);
}

/**
 *  Initialise en lisant le fichier ini
 * 	Renvoie true si le fichier existe
 * 	ATTENTION : la partition doit être montée avant l'appel de la fonction
 */
bool IniFiles::Begin(bool autosave)
{
	FAutoSave = autosave;
	return ReadFile();
}

void IniFiles::SetFileName(const char *aFileName)
{
	if (FFileName)
		FREE_AND_NULL(FFileName);

	// Le / de la racine doit être présent
	if (aFileName[0] != '/')
	{
		FFileName = ALLOC_CHAR_SIZE(strlen(aFileName) + 2);
		if (FFileName)
		{
			strcpy(FFileName, "/");
			strcat(FFileName, aFileName);
		}
	}
	else
	{
		FFileName = ALLOC_CHAR(aFileName);
		if (FFileName)
			strcpy(FFileName, aFileName);
	}
}

//---------------------------------------------------------------------------
/**
 * Défini le format pour les réels. Par défaut %3.6f
 */
void IniFiles::SetFloatFormat(const char *aFormat)
{
	strcpy(FFloatFormat, aFormat);
}

//---------------------------------------------------------------------------
void IniFiles::FreeMemory()
{
	uint16_t i, j;
	TRecord *lRecord;
	TSection *lSection;

	for (i = 0; i < GetNbSections(); i++)
	{
		lSection = &FSections[i];
		FREE_AND_NULL(lSection->Section);
		if (lSection->Comment != NULL)
			FREE_AND_NULL(lSection->Comment);
		for (j = 0; j < lSection->NbRecord; j++)
		{
			lRecord = &FSections[i].Record[j];
			FREE_AND_NULL(lRecord->Key);
			if (lRecord->Value != NULL)
				FREE_AND_NULL(lRecord->Value);
			if (lRecord->Comment != NULL)
				FREE_AND_NULL(lRecord->Comment);
		}
		if (lSection->Record != NULL)
			FREE_AND_NULL(lSection->Record);
	}
	FNbSection = 0;
}

//---------------------------------------------------------------------------
/**
 * Enregistrement du fichier ini.
 * Si aFileName est vide, utilise le nom utilisé lors de l'initialisation
 */
void IniFiles::SaveFile(const char *aFileName)
{
	WriteFile(aFileName);
}

//---------------------------------------------------------------------------
void IniFiles::AllocateRecord(TSection *aSection)
{
	TRecord *lRecord;

	if ((aSection->Capacity - aSection->NbRecord) <= 0)
	{
		aSection->Capacity += CAPACITY_STEP;
		lRecord = (TRecord*) malloc(aSection->Capacity * sizeof(TRecord));
		// Transfert des records existant
		if (aSection->Record != NULL)
		{
			for (int i = 0; i < aSection->NbRecord; i++)
				lRecord[i] = aSection->Record[i];
			FREE_AND_NULL(aSection->Record);
		}
		aSection->Record = lRecord;
	}
}

//---------------------------------------------------------------------------
void IniFiles::AllocateRecord_ByID(int aSection)
{
	AllocateRecord(&FSections[aSection]);
}

//---------------------------------------------------------------------------
char* IniFiles::GetComment(char *str)
{
	char *strDeb = str; // Conserve le début

	// On cherche l'indice d'un commentaire
	while ((strlen(str) != 0) && ((*str != '#') && (*str != FComment)))
		str++;

	if (strlen(str) != 0)
	{
		*str = '\0';
		str++;
		// Supprime les blancs devant
		while ((strlen(str) != 0) && (*str == ' '))
			str++;
		// Supprime les blancs à la fin
		while (str[strlen(str) - 1] == ' ')
			str[strlen(str) - 1] = '\0';
	}

	// On supprime les blancs pouvant se trouver avant le commentaire
	while (strDeb[strlen(strDeb) - 1] == ' ')
		strDeb[strlen(strDeb) - 1] = '\0';

	if (strlen(str) != 0)
		return str;
	else
		return NULL;
}

//---------------------------------------------------------------------------
bool IniFiles::ReadFile(void)
{
	char lCarry[] = "\n\r";
	char *ptrDeb, *ptrFin, *ptrCom;
	uint16_t lRecordID = 0;
	bool lSectionOpen = false;
	TRecord *lRecord;
	char line[MAX_LINESIZE];

	if (FFileName == NULL)
		return false;

	File stream = FS_Partition->open(FFileName, "r");

	// Sortie directe si on n'a pu ouvrir le fichier
	if (!stream)
		return false;

	while (stream.available())
	{
		stream.readStringUntil('\n').toCharArray(line, MAX_LINESIZE);
		while (line[strlen(line) - 1] == lCarry[0])
			line[strlen(line) - 1] = '\0';
#ifdef LINUX
    while (line[strlen(line)-1] == lCarry[1]) line[strlen(line)-1] = '\0';
#endif
		if (strlen(line) == 0)
			continue;                      // Saute les lignes vides
		if ((line[0] == '#') || (line[0] == ';') || (line[0] == FComment))
			continue;   // Saute les lignes de commentaires
		if (line[0] == '[')                                        // Début de section
		{
			lRecordID = 0;
			FSections[FNbSection].NbRecord = 0;
			FSections[FNbSection].Capacity = 0;
			FSections[FNbSection].Record = NULL;
			lSectionOpen = true;
			ptrDeb = &line[0];
			ptrFin = ptrDeb++;
			while (*ptrFin != ']')
				ptrFin++;   // Cherche la fin de la section
			*ptrFin = '\0';
			FSections[FNbSection].Section = ALLOC_CHAR(ptrDeb);
			strcpy(FSections[FNbSection].Section, ptrDeb);
			// Cherche si on a des commentaires
			if ((ptrFin = GetComment(++ptrFin)) != NULL)
			{
				FSections[FNbSection].Comment = ALLOC_CHAR(ptrFin);
				strcpy(FSections[FNbSection].Comment, ptrFin);
			}
			else
				FSections[FNbSection].Comment = NULL;
			FNbSection++;
			continue;
		}
		// Reste maintenant uniquement les couples clé=valeur
		if (!lSectionOpen)
			continue;  // On ne fait rien si aucune section n'est ouverte
		AllocateRecord_ByID(FNbSection - 1);
		FSections[FNbSection - 1].NbRecord++;
		lRecord = &FSections[FNbSection - 1].Record[lRecordID];
		lRecordID++;
		ptrDeb = &line[0];
		ptrFin = ptrDeb;
		while (*ptrFin != '=')
			ptrFin++; // Cherche la fin de la clé
		*ptrFin = '\0';
		ptrCom = ++ptrFin;
		lRecord->Key = ALLOC_CHAR(ptrDeb);
		strcpy(lRecord->Key, ptrDeb);
		if ((ptrCom = GetComment(ptrCom)) != NULL)
		{
			lRecord->Comment = ALLOC_CHAR(ptrCom);
			strcpy(lRecord->Comment, ptrCom);
		}
		else
			lRecord->Comment = NULL;
		if (strlen(ptrFin) != 0)
		{
			lRecord->Value = ALLOC_CHAR(ptrFin);
			strcpy(lRecord->Value, ptrFin);
		}
		else
			lRecord->Value = NULL;

	}

	stream.close();

	return true;
}

//---------------------------------------------------------------------------
bool IniFiles::WriteFile(const char *aFileName)
{
	uint16_t i, j;
	TRecord *lRecord;
	char line[MAX_LINESIZE];

	if (strlen(aFileName) != 0)
	{
		SetFileName(aFileName);
	}

	if (FFileName == NULL)
		return false;

	File stream = FS_Partition->open(FFileName, "w");

	if (!stream)
		return false;

	for (i = 0; i < GetNbSections(); i++)
	{
		if (GetSection_ByID(i)->Comment != NULL)
			sprintf(line, "\n[%s] %c %s\n", FSections[i].Section, FComment, FSections[i].Comment);
		else
			sprintf(line, "\n[%s]\n", FSections[i].Section);
		stream.print(line);

		for (j = 0; j < FSections[i].NbRecord; j++)
		{
			lRecord = &FSections[i].Record[j];
			if (lRecord->Comment != NULL)
				sprintf(line, "%s=%s %c %s\n", lRecord->Key, lRecord->Value, FComment, lRecord->Comment);
			else
				sprintf(line, "%s=%s\n", lRecord->Key, lRecord->Value);
			stream.print(line);
		}
	}
	stream.close();

	FSaved = true;

	return true;
}

//---------------------------------------------------------------------------
TSection* IniFiles::GetSection_ByID(uint16_t i)
{
	if (i < FNbSection)
		return &FSections[i];
	else
		return NULL;
}

//---------------------------------------------------------------------------
TSection* IniFiles::GetSection(const char *aSection, bool aCreate)
{
	uint16_t i;
	TSection *lSection;

	i = 0;
	while ((i < FNbSection) && (strcmp(FSections[i].Section, aSection) != 0))
		i++;

	if (i == FNbSection)
	{
		if (aCreate)
		{
			lSection = &FSections[FNbSection];
			lSection->Section = ALLOC_CHAR(aSection);
			strcpy(lSection->Section, aSection);
			lSection->NbRecord = 0;
			lSection->Capacity = 0;
			lSection->Record = NULL;
			lSection->Comment = NULL;
			FNbSection++;
			return lSection;
		}
		else
			return NULL;
	}
	else
		return &FSections[i];
}

//---------------------------------------------------------------------------
TRecord* IniFiles::GetRecord(const char *aSection, const char *aKey, bool aCreate)
{
	uint16_t i;
	TSection *lSection;
	TRecord *lRecord;

	if ((lSection = GetSection(aSection, aCreate)) != NULL)
	{
		i = 0;
		while ((i < lSection->NbRecord) && (strcmp(lSection->Record[i].Key, aKey) != 0))
			i++;

		if (i == lSection->NbRecord)
		{
			if (aCreate)
			{
				AllocateRecord(lSection);
				lRecord = &lSection->Record[lSection->NbRecord];
				lRecord->Key = ALLOC_CHAR(aKey);
				strcpy(lRecord->Key, aKey);
				lRecord->Value = NULL;
				lRecord->Comment = NULL;
				lSection->NbRecord++;
				return lRecord;
			}
			else
				return NULL;
		}
		else
			return &lSection->Record[i];
	}
	else
		return NULL;
}

//---------------------------------------------------------------------------
void IniFiles::SetValue(const char *aSection, const char *aKey, const char *aValue,
		const char *aComment)
{
	TRecord *lRecord;

	if ((lRecord = GetRecord(aSection, aKey, true)) != NULL)
	{
		if (lRecord->Value != NULL)
			FREE_AND_NULL(lRecord->Value);
		lRecord->Value = ALLOC_CHAR(aValue);
		strcpy(lRecord->Value, aValue);
		if (lRecord->Comment != NULL)
			FREE_AND_NULL(lRecord->Comment);
		if (strlen(aComment) != 0)
		{
			lRecord->Comment = ALLOC_CHAR(aComment);
			strcpy(lRecord->Comment, aComment);
		}
		else
			lRecord->Comment = NULL;
	}
	FSaved = false;
	if (FAutoSave)
		WriteFile("");
}

//---------------------------------------------------------------------------
char* IniFiles::GetValue(const char *aSection, const char *aKey)
{
	TRecord *lRecord;

	if ((lRecord = GetRecord(aSection, aKey, true)) != NULL)
	{
		return lRecord->Value;
	}
	else
		return NULL;
}

//---------------------------------------------------------------------------
char* IniFiles::GetValue_Default(const char *aSection, const char *aKey, const char *aDefault)
{
	TRecord *lRecord = GetRecord(aSection, aKey, true);

	if (lRecord->Value == NULL)
	{
		lRecord->Value = ALLOC_CHAR(aDefault);
		strcpy(lRecord->Value, aDefault);
	}
	return lRecord->Value;
}

//---------------------------------------------------------------------------
/**
 * Lecture d'une chaine Name dans la Section
 * Renvoie un pointeur sur la chaine. L'espace mémoire est alloué.
 * ATTENTION la fonction appelante doit gérer la mémoire (faire un free à la fin du traitement)
 */
char* IniFiles::ReadString(const char *Section, const char *Name, const char *Default)
{
	char *lvalue = GetValue_Default(Section, Name, Default);

	char *lresult = ALLOC_CHAR(lvalue);
	strcpy(lresult, lvalue);
	return lresult;
}

//---------------------------------------------------------------------------
/**
 * Lecture d'une chaine Name dans la Section
 * La chaine est copiée dans Dest.
 * ATTENTION : L'espace mémoire Dest doit être suffisant pour contenir la chaine.
 */
void IniFiles::ReadString_Dest(const char *Section, const char *Name, const char *Default,
		char *Dest)
{
	char *str = ReadString(Section, Name, Default);
	strcpy(Dest, str);
	FREE_AND_NULL(str);
}

//---------------------------------------------------------------------------
void IniFiles::WriteString(const char *Section, const char *Name, const char *Value,
		const char *Comment)
{
	if (Value)
		SetValue(Section, Name, Value, Comment);
	else
		SetValue(Section, Name, "", Comment);
}

//---------------------------------------------------------------------------
/**
 * Lecture d'un entier Name dans la Section
 * Si l'entier n'existe pas, il est crée avec la valeur par défaut
 */
int IniFiles::ReadInteger(const char *Section, const char *Name, int Default)
{
	char *lvalue = GetValue(Section, Name);

	if (lvalue == NULL)
	{
		TRecord *lRecord = GetRecord(Section, Name, true);
		char Temp[20];
		sprintf(Temp, "%d", Default);
		lRecord->Value = ALLOC_CHAR(Temp);
		strcpy(lRecord->Value, Temp);
		return Default;
	}
	else
		return strtol(lvalue, NULL, 10);
}

//---------------------------------------------------------------------------
void IniFiles::WriteInteger(const char *Section, const char *Name, int Value, const char *Comment)
{
	char lInt[20];
	sprintf(lInt, "%d", Value);

	SetValue(Section, Name, lInt, Comment);
}

//---------------------------------------------------------------------------
/**
 * Lecture d'un booléen Name dans la Section
 * Si le booléen n'existe pas, il est crée avec la valeur par défaut
 * True, true ou 1 renvoie Vrai
 */
bool IniFiles::ReadBool(const char *Section, const char *Name, bool Default)
{
	char *lvalue = GetValue(Section, Name);

	if (lvalue == NULL)
	{
		TRecord *lRecord = GetRecord(Section, Name, true);
		lRecord->Value = ALLOC_CHAR_SIZE(6);
		if (Default)
			strcpy(lRecord->Value, "True");
		else
			strcpy(lRecord->Value, "False");
		return Default;
	}
	else
		return ((strcmp(lvalue, "True") == 0) || (strcmp(lvalue, "true") == 0)
				|| (strcmp(lvalue, "1") == 0));
}

//---------------------------------------------------------------------------
void IniFiles::WriteBool(const char *Section, const char *Name, bool Value, const char *Comment)
{
	if (Value)
		SetValue(Section, Name, "True", Comment);
	else
		SetValue(Section, Name, "False", Comment);
}

//---------------------------------------------------------------------------
/**
 * Lecture d'un réel Name dans la Section
 * Si le réel n'existe pas, il est crée avec la valeur par défaut
 */
double IniFiles::ReadFloat(const char *Section, const char *Name, double Default)
{
	char *lvalue = GetValue(Section, Name);

	if (lvalue == NULL)
	{
		TRecord *lRecord = GetRecord(Section, Name, true);
		char Temp[20];
		sprintf(Temp, FFloatFormat, Default);
		lRecord->Value = ALLOC_CHAR(Temp);
		strcpy(lRecord->Value, Temp);
		return Default;
	}
	else
		return strtod(lvalue, NULL);
}

//---------------------------------------------------------------------------
void IniFiles::WriteFloat(const char *Section, const char *Name, double Value, const char *Comment)
{
	char lFloat[20];
	sprintf(lFloat, FFloatFormat, Value);

	SetValue(Section, Name, lFloat, Comment);
}

//---------------------------------------------------------------------------
/**
 * Lecture d'un booléen indexé Nameid dans la Section
 * Si le booléen n'existe pas, il est crée avec la valeur par défaut
 */
bool IniFiles::ReadBoolIndex(uint16_t id, const char *Section, const char *Name, bool Default)
{
	char *IdentId = ALLOC_CHAR_SIZE(strlen(Name) + 3);
	sprintf(IdentId, "%s%d", Name, id);

	bool result = ReadBool(Section, IdentId, Default);
	FREE_AND_NULL(IdentId);
	return result;
}

//---------------------------------------------------------------------------
void IniFiles::WriteBoolIndex(uint16_t id, const char *Section, const char *Name, bool Value,
		const char *Comment)
{
	char *IdentId = ALLOC_CHAR_SIZE(strlen(Name) + 3);
	sprintf(IdentId, "%s%d", Name, id);
	WriteBool(Section, IdentId, Value, Comment);
	FREE_AND_NULL(IdentId);
}

//---------------------------------------------------------------------------
/**
 * Lecture d'un entier indexé Nameid dans la Section
 * Si l'entier n'existe pas, il est crée avec la valeur par défaut
 */
int IniFiles::ReadIntegerIndex(uint16_t id, const char *Section, const char *Name, int Default)
{
	char *IdentId = ALLOC_CHAR_SIZE(strlen(Name) + 3);
	sprintf(IdentId, "%s%d", Name, id);

	int result = ReadInteger(Section, IdentId, Default);
	FREE_AND_NULL(IdentId);
	return result;
}

//---------------------------------------------------------------------------
void IniFiles::WriteIntegerIndex(uint16_t id, const char *Section, const char *Name, int Value,
		const char *Comment)
{
	char *IdentId = ALLOC_CHAR_SIZE(strlen(Name) + 3);
	sprintf(IdentId, "%s%d", Name, id);
	WriteInteger(Section, IdentId, Value, Comment);
	FREE_AND_NULL(IdentId);
}

//---------------------------------------------------------------------------
/**
 * Lecture d'un réel indexé Nameid dans la Section
 * Si le réel n'existe pas, il est crée avec la valeur par défaut
 */
double IniFiles::ReadFloatIndex(uint16_t id, const char *Section, const char *Name, double Default)
{
	char *IdentId = ALLOC_CHAR_SIZE(strlen(Name) + 3);
	sprintf(IdentId, "%s%d", Name, id);
	double result = ReadFloat(Section, IdentId, Default);
	FREE_AND_NULL(IdentId);
	return result;
}

//---------------------------------------------------------------------------
void IniFiles::WriteFloatIndex(uint16_t id, const char *Section, const char *Name, double Value,
		const char *Comment)
{
	char *IdentId = ALLOC_CHAR_SIZE(strlen(Name) + 3);
	sprintf(IdentId, "%s%d", Name, id);
	WriteFloat(Section, IdentId, Value, Comment);
	FREE_AND_NULL(IdentId);
}

//---------------------------------------------------------------------------

// ********************************************************************************
// End of file
// ********************************************************************************

