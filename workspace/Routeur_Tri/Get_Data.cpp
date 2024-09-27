#include "Get_Data.h"
#ifdef USE_SSR
#include "SSR.h"
#endif
#include "display.h"
#include "Server_utils.h"			// Some utils functions for the server (pour Lock_File)
#include "RTCLocal.h"					// A pseudo RTC software library
#include "Partition_utils.h"	// Some utils functions for LittleFS/SPIFFS/FatFS
#include "DS18B20.h"

#ifdef CIRRUS_USE_TASK
#include "Tasks_utils.h"
#endif

// Use DS18B20
extern uint8_t DS_Count;
extern DS18B20 DS;

// data au format CSV
const String CSV_Filename = "/data.csv";
const String Energy_Filename = "/energy.csv";

// Data 200 ms
Data_Struct Current_Data; //{0,{0}};

// Tension et puissance du premier Cirrus, premier channel
volatile float Cirrus_voltage = 230.0;
volatile float Cirrus_power_signed = 0.0;

// Les données actualisées pour le SSR
#ifdef USE_SSR
extern Gestion_SSR_TypeDef Gestion_SSR_CallBack;
#endif

// Le Cirrus CS5480
extern CIRRUS_Communication CS_Com;
extern CIRRUS_CS548x CS5480;
extern CIRRUS_CS548x CS5484;

// Booléen indiquant une acquisition de donnée en cours
bool Data_acquisition = false;

// Gestion énergie
float energy_day_conso = 0.0;
float energy_day_surplus = 0.0;
float energy_day_prod = 0.0;

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
	bool log1 = false;
	bool log2 = false;

#ifdef LOG_CIRRUS_CONNECT
	if (Data_acquisition || CS_Com.Is_IHM_Locked())
#else
  if (Data_acquisition)
#endif
	{
		return; // On est déjà dans la procédure ou il y a un message pour le Cirrus
	}

	CIRRUS_CS548x *CurrentCirrus;
	Data_acquisition = true;

	// To know the time required for the setup
	uint32_t start_time = millis();

	// Sélection du premier cirrus : CS5484, phase 1 et 2
	CurrentCirrus = (CIRRUS_CS548x*) CS_Com.SelectCirrus(0, Channel_1);
	log1 = CurrentCirrus->GetData(Channel_all); // durée : 256 ms
	taskYIELD();

	if (CurrentCirrus->GetErrorCount() > 0)
		print_debug("*** Cirrus1 error : " + String(CurrentCirrus->GetErrorCount()));

	float Esurplus;

	// Fill current data
	Current_Data.Phase1.Voltage = CurrentCirrus->GetURMS(Channel_1);
#ifdef CIRRUS_RMS_FULL
	Current_Data.Phase1.Current = CurrentCirrus->GetIRMS(Channel_1);
#endif
	Current_Data.Phase1.Power = CurrentCirrus->GetPRMSSigned(Channel_1);
	CurrentCirrus->GetEnergy(Channel_1, &Current_Data.Phase1.Energy, &Esurplus);
	energy_day_surplus = Esurplus;

	Current_Data.Phase2.Voltage = CurrentCirrus->GetURMS(Channel_2);
	Current_Data.Phase2.Power = CurrentCirrus->GetPRMSSigned(Channel_2);
	CurrentCirrus->GetEnergy(Channel_2, &Current_Data.Phase2.Energy, &Esurplus);
	energy_day_surplus += Esurplus;

	Current_Data.Cirrus1_PF = CurrentCirrus->GetPowerFactor(Channel_1);


	// Sélection du deuxième cirrus : CS5480, phase 3 et production
	CurrentCirrus = (CIRRUS_CS548x*) CS_Com.SelectCirrus(1, Channel_1);
	log2 = CurrentCirrus->GetData(Channel_all);
	taskYIELD();

	if (CurrentCirrus->GetErrorCount() > 0)
		print_debug("*** Cirrus2 error : " + String(CurrentCirrus->GetErrorCount()));

	// Fill current data
	Current_Data.Phase3.Voltage = CurrentCirrus->GetURMS(Channel_1);
	Current_Data.Phase3.Power = CurrentCirrus->GetPRMSSigned(Channel_1);
	CurrentCirrus->GetEnergy(Channel_1, &Current_Data.Phase3.Energy, &Esurplus);
	energy_day_surplus += Esurplus;

	Current_Data.Production.Voltage = CurrentCirrus->GetURMS(Channel_2);
	Current_Data.Production.Power = CurrentCirrus->GetPRMSSigned(Channel_2);
	CurrentCirrus->GetEnergy(Channel_2, &Current_Data.Production.Energy, &Esurplus);
	energy_day_surplus += Esurplus;

	Current_Data.Cirrus1_Temp = CurrentCirrus->GetTemperature();

	// On revient sur le premier cirrus
	CS_Com.SelectCirrus(0, Channel_1);

#ifdef USE_SSR
	if (Gestion_SSR_CallBack != NULL)
	{
		// On choisi le deuxième cirrus channel 1 qui correspond à la troisième phase
		Cirrus_voltage = Current_Data.Phase3.Voltage;
		Cirrus_power_signed = Current_Data.Phase3.Power; // normalement somme des 3 puissances
		Gestion_SSR_CallBack();
	}
#endif

	// Log
	if (log1 && log2)
	{
		double temp;
		RMS_Data data = CS5484.GetLog(Channel_1, NULL);
		log_cumul.Voltage_ph1 = data.Voltage;
		log_cumul.Power_ph1 = data.Power;
		data = CS5484.GetLog(Channel_2, NULL);
		log_cumul.Voltage_ph2 = data.Voltage;
		log_cumul.Power_ph2 = data.Power;
		data = CS5480.GetLog(Channel_1, &temp);
		log_cumul.Voltage_ph3 = data.Voltage;
		log_cumul.Power_ph3 = data.Power;

		log_cumul.Temp = temp;

		// Pour le rafraichissement de la page Internet si connecté
		log_new_data = true;

		// Sauvegarde des données
		append_data();
	}

//	if (countmessage < 20)
//		print_debug("*** Data time : " + String(millis() - start_time) + " ms ***"); // ~390 ms

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

	sprintf(buffer, "Urms: %.2f", Current_Data.Phase1.Voltage);
	IHM_Print(line++, (char*) buffer);

#ifdef CIRRUS_RMS_FULL
	sprintf(buffer, "Irms:%.2f  ", Current_Data.Phase1.Current);
	IHM_Print(line++, (char*) buffer);
#endif

	sprintf(buffer, "Prms:%.2f   ", Current_Data.Phase1.Power);
	IHM_Print(line++, (char*) buffer);

	float conso, surplus;
	CS5484.GetEnergy(Channel_1, &conso, &surplus);
	sprintf(buffer, "Energie:%.2f   ", conso); // energy_day_conso
	IHM_Print(line++, (char*) buffer);

	sprintf(buffer, "T:%.2f", CS5480.GetTemperature()); // Current_Data.Cirrus1_Temp
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
		// Time, Pconso_rms, Pprod_rms, U_rms, Tcs, TDS1, TDS2
		String data = String(RTC_Local.getUNIXDateTime()) + '\t'
				+ (String) log_cumul.Voltage_ph1 + '\t'
				+ (String) log_cumul.Power_ph1 + '\t'
				+ (String) log_cumul.Voltage_ph2 + '\t'
				+ (String) log_cumul.Power_ph2 + '\t'
				+ (String) log_cumul.Voltage_ph3 + '\t'
				+ (String) log_cumul.Power_ph3 + '\t'
				+ (String) log_cumul.Temp;
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
		// Time, Econso, Esurplus, Eprod
		String data = String(RTC_Local.getUNIXDateTime()) + '\t' + (String) energy_day_conso + '\t'
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
	CS5480.RestartEnergy();
	CS5484.RestartEnergy();

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
