#include "Get_Data.h"
#ifdef USE_SSR
#include "SSR.h"
#endif
#include "display.h"
#include "Server_utils.h"			// Some utils functions for the server (pour Lock_File)
#include "RTCLocal.h"					// A pseudo RTC software library
#include "Partition_utils.h"	// Some utils functions for LittleFS/SPIFFS/FatFS
#include "DS18B20.h"

// Use DS18B20
extern uint8_t DS_Count;
extern DS18B20 DS;

// data au format CSV
const String CSV_Filename = "/data.csv";
const String Energy_Filename = "/energy.csv";

// Data 200 ms
volatile Data_Struct Current_Data; //{0,{0}};

extern CIRRUS_Communication CS_Com;
extern CIRRUS_CS5490 CS5490;

// Booléen indiquant une acquisition de donnée en cours
bool Data_acquisition = false;

// Tension et puissance du premier Cirrus, premier channel
volatile float Cirrus_voltage = 230.0;
volatile float Cirrus_power_signed = 0.0;

// Les données actualisées pour le SSR
#ifdef USE_SSR
extern Gestion_SSR_TypeDef Gestion_SSR_CallBack;
#endif

// Gestion énergie
float energy_day_conso = 0.0;
float energy_day_surplus = 0.0;

// Gestion log pour le graphique
volatile Graphe_Data log_cumul;
bool log_new_data = false;

// Pour enregistrer les températures DS18B20
#ifdef USE_DS
extern DS18B20 DS;
#endif

// ********************************************************************************
// Functions prototype
// ********************************************************************************

void append_data(void);
void append_energy(void);

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
	static int countmessage = 0;
	bool log = false;

#ifdef LOG_CIRRUS_CONNECT
	if (Data_acquisition || CS_Com.Is_IHM_Locked())
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

	if (CS5490.GetErrorCount() > 0)
		print_debug("*** Cirrus error : " + String(CS5490.GetErrorCount()));

	// Fill current data channel 1
	Current_Data.Cirrus_ch1.Voltage = CS5490.GetURMS();
#ifdef CIRRUS_RMS_FULL
	Current_Data.Cirrus_ch1.Current = CS5490.GetIRMS();
#endif
	Current_Data.Cirrus_ch1.Power = CS5490.GetPRMSSigned();
	Current_Data.Cirrus_PF = CS5490.GetPowerFactor();
	Current_Data.Cirrus_Temp = CS5490.GetTemperature();
	CS5490.GetEnergy(&energy_day_conso, &energy_day_surplus);

#ifdef USE_SSR
	// On choisi le premier channel qui mesure la consommation et le surplus
	Cirrus_voltage = Current_Data.Cirrus_ch1.Voltage;
	Cirrus_power_signed = Current_Data.Cirrus_ch1.Power;

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
		log_cumul.Power = data.Power;
		log_cumul.Temp = temp;

		// Pour le rafraichissement de la page Internet si connecté
		log_new_data = true;

		// Sauvegarde des données
		append_data();

//		print_debug("*** LOG " );
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
 * Fonction d'affichage simple des données de Get_Data
 * Renvoie la dernière ligne libre
 */
uint8_t Update_IHM(const char *first_text, const char *last_text, bool display)
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

		return 1;
	}
#endif

	sprintf(buffer, "Urms: %.2f", Current_Data.Cirrus_ch1.Voltage);
	IHM_Print(line++, (char*) buffer);

#ifdef CIRRUS_RMS_FULL
	sprintf(buffer, "Irms:%.2f  ", Current_Data.Cirrus_ch1.Current);
	IHM_Print(line++, (char*) buffer);
#endif

	sprintf(buffer, "Prms:%.2f   ", Current_Data.Cirrus_ch1.Power);
	IHM_Print(line++, (char*) buffer);

	sprintf(buffer, "Energie:%.2f   ", energy_day_conso);
	IHM_Print(line++, (char*) buffer);

	sprintf(buffer, "T:%.2f", Current_Data.Cirrus_Temp);
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

bool Get_Last_Data(float *Energy, float *Surplus)
{
	bool _log_new_data = log_new_data;
	log_new_data = false;
	*Energy = energy_day_conso;
	*Surplus = energy_day_surplus;
	return _log_new_data;
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
		String data = String(RTC_Local.getUNIXDateTime()) + '\t' + (String) log_cumul.Power + '\t'
				+ (String) log_cumul.Voltage + '\t' + (String) log_cumul.Temp;
		if (DS_Count > 0)
			data += '\t' + DS.get_Temperature_Str(0) + '\t' + DS.get_Temperature_Str(1) + "\r\n";
		else
			data += "\t0.0\t0.0\r\n";
		temp.print(data);
		temp.close();
	}
	Lock_File = false;
}

void append_energy(void)
{
	// On vérifie qu'on n'est pas en train de l'uploader
	if (Lock_File)
		return;

	// Ouvre le fichier en append, le crée s'il n'existe pas
	Lock_File = true;
	File temp = Data_Partition->open(Energy_Filename, "a");
	if (temp)
	{
		String data = String(RTC_Local.getUNIXDateTime()) + '\t' + (String) energy_day_conso + '\t'
				+ (String) energy_day_surplus + "\r\n";
		temp.print(data);
		temp.close();
	}
	Lock_File = false;
}

void onDaychange(uint8_t year, uint8_t month, uint8_t day)
{
	print_debug(F("Callback day change"));
	// On ajoute les énergies au fichier
	append_energy();
	energy_day_conso = 0.0;
	energy_day_surplus = 0.0;
	CS5490.RestartEnergy();

	// On archive le fichier data du jour en lui donnant le nom du jour
	if (Data_Partition->exists(CSV_Filename))
	{
		while (Lock_File)
			;
		char Day_Name[20] = {0};
		sprintf(Day_Name, "/%02d-%02d-%02d.csv", year, month, day);
		Data_Partition->rename(CSV_Filename, String(Day_Name));
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
