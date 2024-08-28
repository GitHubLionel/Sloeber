#include "Simple_Get_Data.h"
#ifdef USE_SSR
#include "SSR.h"
#endif
#include "display.h"
#include "RTCLocal.h"					// A pseudo RTC software library
#include "Partition_utils.h"	// Some utils functions for LittleFS/SPIFFS/FatFS

#ifdef CIRRUS_USE_TASK
#include "Tasks_utils.h"
#endif

// data au format CSV
const String CSV_Filename = "/data.csv";

// Data 200 ms
volatile Simple_Data_Struct Simple_Current_Data; //{0,{0}};

// Tension et puissance du premier Cirrus, premier channel
volatile float Cirrus_voltage = 230.0;
volatile float Cirrus_power_signed = 0.0;

// Les données actualisées pour le SSR
#ifdef USE_SSR
extern Gestion_SSR_TypeDef Gestion_SSR_CallBack;
#endif

// Le Cirrus CS5490, défini ailleurs
extern CIRRUS_CS5490 CS5490;

volatile bool Simple_Data_acquisition = false;

// UART message
#ifdef LOG_CIRRUS_CONNECT
extern volatile bool CIRRUS_Command;
extern volatile bool CIRRUS_Lock_IHM;
#endif

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
 * Exemple de fonction d'acquisition des données du Cirrus
 * En pratique, il convient d'appeler régulièrement cette fonction dans un timer (200 ms ou 1 s)
 */
void Get_Data(void)
{
	static bool Data_acquisition = false;
	static int countmessage = 0;
	bool log = false;

#ifdef LOG_CIRRUS_CONNECT
	if (Data_acquisition || CIRRUS_Command || CIRRUS_Lock_IHM)
#else
  if (Simple_Data_acquisition)
#endif
	{
		return; // On est déjà dans la procédure ou il y a un message pour le Cirrus
	}

	Data_acquisition = true;

	// To know the time required for data acquisition
//	uint32_t start_time = millis();

	log = CS5490.GetData(); // durée : 256 ms
	vTaskDelay(1);

	if (CS5490.GetErrorCount() > 0)
		print_debug("*** Cirrus error : " + String(CS5490.GetErrorCount()));

	// Fill current data channel 1
	Simple_Current_Data.Cirrus_ch1.Voltage = CS5490.GetURMS();
#ifdef CIRRUS_RMS_FULL
	Simple_Current_Data.Cirrus_ch1.Current = CS5490.GetIRMS();
#endif
	Simple_Current_Data.Cirrus_ch1.Power = CS5490.GetPRMSSigned();
	Simple_Current_Data.Cirrus_PF = CS5490.GetPowerFactor();
	Simple_Current_Data.Cirrus_Temp = CS5490.GetTemperature();
	CS5490.GetEnergy(&energy_day_conso, &energy_day_surplus);

#ifdef USE_SSR
	// On choisi le premier channel qui mesure la consommation et le surplus
	Cirrus_voltage = Simple_Current_Data.Cirrus_ch1.Voltage;
	Cirrus_power_signed = Simple_Current_Data.Cirrus_ch1.Power;

	if (Gestion_SSR_CallBack != NULL)
		Gestion_SSR_CallBack();
#endif

//	uint32_t err = CS5490.GetErrorCount();
//	if (err > 0)
//		print_debug("erreur = " + (String)err);

	// Log
	if (log)
	{
		double temp;
		RMS_Data data = CS5490.GetLog(&temp);
		log_cumul.Voltage = data.Voltage;
		log_cumul.Power_ch1 = data.Power;
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

// ********************************************************************************
// Affichage des données
// ********************************************************************************

/**
 * Fonction d'affichage simple des données de Simple_Get_Data
 */
uint8_t Update_IHM(const char *first_text, const char *last_text, bool display)
{
  char buffer[30];  // Normalement 20 caractères + eol mais on sait jamais
  uint8_t line = 0;

  // Efface la mémoire de l'écran si nécessaire
  IHM_Clear();

#ifdef LOG_CIRRUS_CONNECT
	if (CIRRUS_Lock_IHM)
	{
		IHM_Print(0, "Cirrus Lock");

		// Actualise l'écran si nécessaire
		IHM_Display();

		return 1;
	}
#endif

	if (strlen(first_text) > 0)
	{
		IHM_Print(line++, first_text);
	}

	sprintf(buffer, "Urms: %.2f", Simple_Current_Data.Cirrus_ch1.Voltage);
	IHM_Print(line++, (char*) buffer);

#ifdef CIRRUS_RMS_FULL
	sprintf(buffer, "Irms:%.2f  ", Simple_Current_Data.Cirrus_ch1.Current);
	IHM_Print(line++, (char*) buffer);
#endif

	sprintf(buffer, "Prms:%.2f   ", Simple_Current_Data.Cirrus_ch1.Power);
	IHM_Print(line++, (char*) buffer);

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
		// Time, Pconso_rms, Pprod_rms, U_rms, Tcs, TDS1, TDS2
		String data = String(RTC_Local.getUNIXDateTime()) + '\t' + (String) log_cumul.Power_ch1 + '\t'
				 + (String) log_cumul.Voltage + '\t' + (String) log_cumul.Temp;
		temp.print(data);
		temp.close();
	}
	Lock_File = false;
}

// ********************************************************************************
// End of file
// ********************************************************************************
