#include "Get_Data.h"
#ifdef USE_SSR
#include "SSR.h"
#endif
#include "display.h"
#include "Server_utils.h"			// Some utils functions for the server (pour Lock_File)
#include "RTCLocal.h"					// A pseudo RTC software library
#include "Partition_utils.h"	// Some utils functions for LittleFS/SPIFFS/FatFS
#include "DS18B20.h"
#include "TeleInfo.h"
#include "ADC_Utils.h"

#ifdef CIRRUS_USE_TASK
#include "Tasks_utils.h"
#endif

// Use DS18B20
extern uint8_t DS_Count;
extern DS18B20 DS;

// Use TI
extern bool TI_OK;
extern uint32_t TI_Counter;
extern TeleInfo TI;
extern bool ADC_OK;

// data au format CSV
const String CSV_Filename = "/data.csv";
const String Energy_Filename = "/energy_2024.csv";

// Data 200 ms
volatile Data_Struct Current_Data; //{0,{0}};

// Les données actualisées pour le SSR
#ifdef USE_SSR
extern Gestion_SSR_TypeDef Gestion_SSR_CallBack;
bool CIRRUS_get_rms_data(float *uRMS, float *pRMS);
#endif

// Le Cirrus CS5480
extern CIRRUS_Communication CS_Com;
extern CIRRUS_CS548x CS5480;

// Booléen indiquant une acquisition de donnée en cours
bool Data_acquisition = false;

// Gestion énergie
float energy_day_conso = 0.0;
float energy_day_surplus = 0.0;
float energy_day_prod = 0.0;
uint32_t last_time = millis();

// Gestion log pour le graphique
volatile Graphe_Data log_cumul;
bool log_new_data = false;

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
// Acquisition des données
// ********************************************************************************

/**
 * En pratique, il convient d'appeler régulièrement cette fonction dans un timer (200 ms ou 1 s) ou une tache
 */
void Get_Data(void)
{
	static int countmessage = 0;
	bool log = false;

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
	uint32_t start_time = millis();

	log = CS5480.GetData(Channel_all); // durée : 256 ms
	taskYIELD();

	uint32_t err;
	if ((err = CS5480.GetErrorCount()) > 0)
	{
		print_debug("*** Cirrus error : " + String(err));
		Data_acquisition = false;
		return;
	}

	// Fill current data channel 1
	Current_Data.Cirrus_ch1.Voltage = CS5480.GetURMS(Channel_1);
	Current_Data.Cirrus_ch1.ActivePower = CS5480.GetPRMSSigned(Channel_1);
#ifdef CIRRUS_RMS_FULL
	Current_Data.Cirrus_ch1.Current = CS5480.GetIRMS(Channel_1);
	Current_Data.Cirrus_ch1.ApparentPower = CS5480.GetExtraData(Channel_1, exd_PApparent);
	Current_Data.Cirrus_ch1.PowerFactor = CS5480.GetExtraData(Channel_1, exd_PF);
	Current_Data.Cirrus_ch2.Current = CS5480.GetIRMS(Channel_2);
#endif
	Current_Data.Cirrus_ch1.Temperature = CS5480.GetTemperature();
	CS5480.GetEnergy(Channel_1, &energy_day_conso, &energy_day_surplus);

	// A voir le signe
	CS5480.GetEnergy(Channel_2, &energy_day_prod, NULL);
	Current_Data.Cirrus_ch2.ActivePower = CS5480.GetPRMSSigned(Channel_2);

#ifdef USE_SSR
	// On choisi le premier channel qui mesure la consommation et le surplus
	if (Gestion_SSR_CallBack != NULL)
		Gestion_SSR_CallBack(Current_Data.Cirrus_ch1.Voltage, Current_Data.Cirrus_ch1.ActivePower);
#endif

	// Talema
	if (ADC_OK)
	{
		Current_Data.Talema_Current = ADC_GetTalemaCurrent();
		Current_Data.Talema_Power = Current_Data.Talema_Current * Current_Data.Cirrus_ch1.Voltage *
				Current_Data.Cirrus_ch1.PowerFactor;
		Current_Data.Talema_Energy = Current_Data.Talema_Energy + Current_Data.Talema_Power * ((start_time - last_time) / 1000.0) / 3600.0;
		last_time = start_time;
	}

//	uint32_t err = CS5480.GetErrorCount();
//	if (err > 0)
//		print_debug("erreur = " + (String)err);

	// Log
	if (log)
	{
		double temp;
		RMS_Data data = CS5480.GetLog(Channel_1, &temp);
		log_cumul.Voltage = data.Voltage;
		log_cumul.Power_ch1 = data.ActivePower;
		log_cumul.Temp = temp;

		data = CS5480.GetLog(Channel_2, NULL);
		log_cumul.Power_ch2 = data.ActivePower;

		// Pour le rafraichissement de la page Internet si connecté
		log_new_data = true;

		// Sauvegarde des données
		append_data();
	}

//	if (countmessage < 20)
//		print_debug("*** Data time : " + String(millis() - start_time) + " ms ***"); // 67 - 133 ms

	countmessage++;

	Data_acquisition = false;
}

#ifdef USE_SSR
bool CIRRUS_get_rms_data(float *uRMS, float *pRMS)
{
	CS5480.SelectChannel(Channel_1);
	return CS5480.get_rms_data(uRMS, pRMS);
}
#endif

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

	sprintf(buffer, "Prms:%.2f   ", Current_Data.Cirrus_ch1.ActivePower);
	IHM_Print(line++, (char*) buffer);

#ifdef CIRRUS_RMS_FULL
	sprintf(buffer, "Papp:%.2f   ", Current_Data.Cirrus_ch1.ApparentPower);
	IHM_Print(line++, (char*) buffer);
#endif

	sprintf(buffer, "Prms2:%.2f   ", Current_Data.Cirrus_ch2.ActivePower);
	IHM_Print(line++, (char*) buffer);

	sprintf(buffer, "Energie:%.2f   ", energy_day_conso);
	IHM_Print(line++, (char*) buffer);

//	sprintf(buffer, "T:%.2f", Current_Data.Cirrus_ch1.Temperature);
	sprintf(buffer, "Irms2:%.2f  ", Current_Data.Cirrus_ch2.Current);
	IHM_Print(line++, (char*) buffer);

	if (ADC_OK)
	{
		sprintf(buffer, "Talema:%.2f   ", Current_Data.Talema_Current);
		IHM_Print(line++, (char*) buffer);
	}

	if (strlen(last_text) > 0)
	{
		IHM_Print(line++, last_text);
	}

	// Actualise l'écran si nécessaire
	if (display)
		IHM_Display();
	return line;
}

bool Get_Last_Data(float *Energy, float *Surplus, float *Prod)
{
	bool _log_new_data = log_new_data;
	log_new_data = false;
	*Energy = energy_day_conso;
	*Surplus = energy_day_surplus;
	*Prod = energy_day_prod;
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
		// Time, Pconso_rms, Pprod_rms, U_rms, Tcs
		String data = String(RTC_Local.getUNIXDateTime()) + '\t' + (String) log_cumul.Power_ch1 + '\t'
				+ (String) log_cumul.Power_ch2 + '\t' + (String) log_cumul.Voltage + '\t' + (String) log_cumul.Temp;
		// DS Temp
		if (DS_Count > 0)
			data += '\t' + DS.get_Temperature_Str(0) + '\t' + DS.get_Temperature_Str(1);
		else
			data += "\t0.0\t0.0";
		// Energy
		data += '\t' + (String) energy_day_conso + '\t' + (String) energy_day_surplus + '\t' + (String) energy_day_prod;
		// End line
		data += "\r\n";
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
		char date[7] = {0};
		// Time, Econso, Esurplus, Eprod
		String data = String(RTC_Local.getShortDate(date)) + '\t' + (String) energy_day_conso + '\t'
				+ (String) energy_day_surplus + '\t' + (String) energy_day_prod + "\r\n";
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
	energy_day_prod = 0.0;
	Current_Data.Talema_Energy = 0.0;
	CS5480.RestartEnergy();
	if (TI_OK)
		TI_Counter = TI.getIndexWh();

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
