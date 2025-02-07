#include "Simple_Get_Data.h"
#ifdef USE_SSR
#include "SSR.h"
#endif
#include "display.h"
#include "RTCLocal.h"					// A pseudo RTC software library
#include "Partition_utils.h"	// Some utils functions for LittleFS/SPIFFS/FatFS

// data au format CSV
const String CSV_Filename = "/simple_data.csv";

// Data 200 ms
volatile Simple_Data_Struct Simple_Current_Data; //{0,{0}};

// Up to date data for SSR
#ifdef USE_SSR
extern Gestion_SSR_TypeDef Gestion_SSR_CallBack;
bool CIRRUS_get_rms_data(float *uRMS, float *pRMS);
#endif

// Extern definitions
extern CIRRUS_Communication CS_Com;

// Current cirrus initialized in Simple_Set_Cirrus() function
#if defined(CIRRUS_SIMPLE_IS_CS5490)
	#if (CIRRUS_SIMPLE_IS_CS5490 == true)
	CIRRUS_CS5490 *Current_Cirrus = NULL;
  #define CHANNEL_ALL
	#define CHANNEL
  #define CHANNEL_EX
	#define CHANNEL2
	#else
		#if (CIRRUS_SIMPLE_IS_CS5490 == false)
		CIRRUS_CS548x *Current_Cirrus = NULL;
    #define CHANNEL_ALL	Channel_all
		#define CHANNEL	Channel_1
		#define CHANNEL_EX	Channel_1,
    #define CHANNEL2	Channel_2
		#else
			#error "You must define CIRRUS_SIMPLE_IS_CS5490 to true or false"
		#endif
	#endif
#else
	#error "You must define CIRRUS_SIMPLE_IS_CS5490 to true or false, or exclude this file to build"
#endif

// Booléen indiquant une acquisition de donnée en cours
bool Data_acquisition = false;
bool IsTwoChannel = false;

// Gestion énergie
float energy_day_conso = 0.0;
float energy_day_surplus = 0.0;

// Gestion log pour le graphique
volatile Graphe_Data log_cumul;
bool log_new_data = false;

// ********************************************************************************
// Functions prototype
// ********************************************************************************

void append_data(void);

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(String mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

// ********************************************************************************
// Exemple d'acquisition de données
// ********************************************************************************

/**
 * Set the Cirrus
 */
void Simple_Set_Cirrus(const CIRRUS_Base &cirrus)
{
#if (CIRRUS_SIMPLE_IS_CS5490 == true)
	Current_Cirrus = (CIRRUS_CS5490 *) (&cirrus);
	IsTwoChannel = false;
#else
	Current_Cirrus = (CIRRUS_CS548x*) (&cirrus);
	IsTwoChannel = true;
#endif
}

/**
 * Exemple de fonction d'acquisition des données du Cirrus
 * En pratique, il convient d'appeler régulièrement cette fonction dans un timer (200 ms ou 1 s)
 */
void Simple_Get_Data(void)
{
	static int countmessage = 0;

	if (Current_Cirrus == NULL)
	{
		print_debug("ERROR: Current_Cirrus is NULL in Simple_Get_Data\n");
		return;
	}

#ifdef LOG_CIRRUS_CONNECT
	if (Data_acquisition || CS_Com.Is_IHM_Locked())
#else
  if (Data_acquisition)
#endif
	{
		return; // On est déjà dans la procédure ou il y a un message pour le Cirrus
	}

	Data_acquisition = true;

	// To know the time required for data acquisition
//	uint32_t start_time = millis();

	bool log = Current_Cirrus->GetData(CHANNEL_ALL); // durée : 256 ms
#ifdef ESP32
	vTaskDelay(1);
#endif

	uint32_t err;
	if ((err = Current_Cirrus->GetErrorCount()) > 0)
	{
		print_debug("*** Cirrus error : " + String(err));
		Data_acquisition = false;
		return;
	}

	// Fill current data channel 1
	Simple_Current_Data.Cirrus_ch1.Voltage = Current_Cirrus->GetURMS(CHANNEL);
#ifdef CIRRUS_RMS_FULL
	Simple_Current_Data.Cirrus_ch1.Current = Current_Cirrus->GetIRMS(CHANNEL);
#endif
	Simple_Current_Data.Cirrus_ch1.ActivePower = Current_Cirrus->GetPRMSSigned(CHANNEL);

	// Second channel if present
	if (Current_Cirrus->IsTwoChannel())
	{
		Simple_Current_Data.Cirrus_ch2.Voltage = Current_Cirrus->GetURMS(CHANNEL2);
#ifdef CIRRUS_RMS_FULL
		Simple_Current_Data.Cirrus_ch2.Current = Current_Cirrus->GetIRMS(CHANNEL2);
#endif
		Simple_Current_Data.Cirrus_ch2.ActivePower = Current_Cirrus->GetPRMSSigned(CHANNEL2);
	}

	Simple_Current_Data.Cirrus_PF = Current_Cirrus->GetExtraData(CHANNEL_EX exd_PF);
	Simple_Current_Data.Cirrus_Temp = Current_Cirrus->GetTemperature();
	Current_Cirrus->GetEnergy(CHANNEL_EX &energy_day_conso, &energy_day_surplus);

#ifdef USE_SSR
	// On choisi le premier channel qui mesure la consommation et le surplus
	if (Gestion_SSR_CallBack != NULL)
		Gestion_SSR_CallBack(Simple_Current_Data.Cirrus_ch1.Voltage, Simple_Current_Data.Cirrus_ch1.ActivePower);
#endif

//	uint32_t err = Current_Cirrus.GetErrorCount();
//	if (err > 0)
//		print_debug("erreur = " + (String)err);

	// Log
	if (log)
	{
		double temp;
		RMS_Data data = Current_Cirrus->GetLog(CHANNEL_EX &temp);
		log_cumul.Voltage = data.Voltage;
		log_cumul.Power_ch1 = data.ActivePower;
		log_cumul.Temp = temp;

		// Pour le rafraichissement de la page Internet si connecté
		log_new_data = true;

		// Sauvegarde des données
		append_data();
	}

//	if (countmessage < 20)
//		print_debug("*** Data time : " + String(millis() - start_time) + " ms ***"); // 130 - 260 ms

	countmessage++;

	Data_acquisition = false;
}

#ifdef USE_SSR
bool CIRRUS_get_rms_data(float *uRMS, float *pRMS)
{
	Current_Cirrus->SelectChannel(CHANNEL);
	return Current_Cirrus->get_rms_data(uRMS, pRMS);
}
#endif

// ********************************************************************************
// Affichage des données
// ********************************************************************************

/**
 * Fonction d'affichage simple des données de Simple_Get_Data
 */
uint8_t Simple_Update_IHM(const char *first_text, const char *last_text, bool display)
{
	char buffer[30];  // Normalement 20 caractères + eol mais on sait jamais
	uint8_t line = 0;

	if (IHM_IsDisplayOff())
		return line;

	// Efface la mémoire de l'écran si nécessaire
	IHM_Clear();

	if (strlen(first_text) > 0)
	{
		IHM_Print(line++, first_text);
	}

#ifdef LOG_CIRRUS_CONNECT
	if (CS_Com.Is_IHM_Locked())
	{
		IHM_Print(line++, "Cirrus Lock");

		// Actualise l'écran si nécessaire
		if (display)
			IHM_Display();

		return line;
	}
#endif

	sprintf(buffer, "Urms: %.2f", Simple_Current_Data.Cirrus_ch1.Voltage);
	IHM_Print(line++, (char*) buffer);

#ifdef CIRRUS_RMS_FULL
	sprintf(buffer, "Irms:%.2f  ", Simple_Current_Data.Cirrus_ch1.Current);
	IHM_Print(line++, (char*) buffer);
#endif

	sprintf(buffer, "Prms:%.2f   ", Simple_Current_Data.Cirrus_ch1.ActivePower);
	IHM_Print(line++, (char*) buffer);

	if (IsTwoChannel)
	{
		sprintf(buffer, "Urms2: %.2f", Simple_Current_Data.Cirrus_ch2.Voltage);
		IHM_Print(line++, (char*) buffer);

#ifdef CIRRUS_RMS_FULL
		sprintf(buffer, "Irms2:%.2f  ", Simple_Current_Data.Cirrus_ch2.Current);
		IHM_Print(line++, (char*) buffer);
#endif

		sprintf(buffer, "Prms2:%.2f   ", Simple_Current_Data.Cirrus_ch2.ActivePower);
		IHM_Print(line++, (char*) buffer);
	}

	sprintf(buffer, "Energie:%.2f   ", energy_day_conso);
	IHM_Print(line++, (char*) buffer);

	sprintf(buffer, "T:%.2f", Simple_Current_Data.Cirrus_Temp);
	IHM_Print(line++, (char*) buffer);

	if (strlen(last_text) > 0)
	{
		IHM_Print(line++, last_text);
	}

	// Actualise l'écran si nécessaire
	if (display)
		IHM_Display();
	return line;
}

// ********************************************************************************
// Graphe functions, sauvegarde des données
// ********************************************************************************

void append_data(void)
{
	// On vérifie qu'on n'est pas en train de l'uploader
	if (Lock_File)
		return;

	// Ouvre le fichier en append, le crée s'il n'existe pas
	Lock_File = true;
	File temp = Data_Partition->open(CSV_Filename, "a");
	if (temp)
	{
		// Time, Pch1_rms, Uch1_rms, Tcs
		String data = String(RTC_Local.getUNIXDateTime()) + '\t' + (String) log_cumul.Power_ch1 + '\t'
				+ (String) log_cumul.Voltage + '\t' + (String) log_cumul.Temp + "\r\n";
		temp.print(data);
		temp.close();
	}
	Lock_File = false;
}

// ********************************************************************************
// End of file
// ********************************************************************************
