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
#include "Fast_Printf.h"
#include "Emul_PV.h"

#ifdef CIRRUS_USE_TASK
#include "Tasks_utils.h"
#endif

// Use DS18B20
extern uint8_t DS_Count;
extern DS18B20 DS;

// Use TI
extern bool TI_OK;
extern TeleInfo TI;

// Use ADC
extern bool ADC_OK;

// Data PV
extern EmulPV_Class emul_PV;

// Calcul des extra data : TI, ADC, PV
int ExtraDataCount = 1;

// data au format CSV
const String CSV_Filename = "/data.csv";
String Energy_Filename = "/energy_2025.csv";
String Energy_Heading = "Date\tE_Conso\tE_Surplus\tE_Prod\tE_Talema\r\n";

// Data 200 ms
Data_Struct Current_Data; //{0,{0}};

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
uint32_t Talema_time = millis();
Talema_Factor_Enum TalemaFactor = tfChannel1;

// Gestion log pour le graphique
volatile Graphe_Data log_cumul;
bool log_new_data = false;

// Sauvegarde du log
volatile SemaphoreHandle_t logSemaphore = NULL;

// ********************************************************************************
// Functions prototype
// ********************************************************************************

void GetExtraData(void);
double GetTalemaFactor(void);
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
	uint32_t err;

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

	log = CS5480.GetData(Channel_all); // durée : 256 ms
	taskYIELD();

	if ((err = CS5480.GetErrorCount()) > 0)
	{
		print_debug("*** Cirrus error : " + String(CS5480.Print_LastError()));
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
	Current_Data.Cirrus_ch2.PowerFactor = CS5480.GetExtraData(Channel_2, exd_PF);
#endif
	Current_Data.Cirrus_ch1.Temperature = CS5480.GetTemperature();
	CS5480.GetEnergy(Channel_1, &Current_Data.energy_day_conso, &Current_Data.energy_day_surplus);

	// A voir le signe
	CS5480.GetEnergy(Channel_2, &Current_Data.energy_day_prod, NULL);
	Current_Data.Cirrus_ch2.ActivePower = CS5480.GetPRMSSigned(Channel_2);

	// Temps d'acquisition des données
//	if (countmessage < 20)
//		print_debug("*** Data time : " + String(millis() - start_time) + " ms ***"); // 67 - 133 ms

#ifdef USE_SSR
	// On choisi le premier channel qui mesure la consommation et le surplus
	if (Gestion_SSR_CallBack != NULL)
		Gestion_SSR_CallBack(Current_Data.Cirrus_ch1.Voltage, Current_Data.Cirrus_ch1.ActivePower);
#endif

	// Talema
	if (ADC_OK)
	{
		uint32_t ref_time = millis();
		Current_Data.Talema_Current = ADC_GetTalemaCurrent();
		Current_Data.Talema_Power = Current_Data.Talema_Current * Current_Data.Cirrus_ch1.Voltage * GetTalemaFactor();
		Current_Data.Talema_Energy += Current_Data.Talema_Power * ((ref_time - Talema_time) / 1000.0) / 3600.0;
		Talema_time = ref_time;
	}

//	uint32_t err = CS5480.GetErrorCount();
//	if (err > 0)
//		print_debug("erreur = " + (String)err);

	// Get extra data
	GetExtraData();

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

		// Sauvegarde des données, à faire dans une task
//		append_data();
		if (logSemaphore != NULL)
			xSemaphoreGive(logSemaphore);
	}

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

void GetExtraData(void)
{
	if (ExtraDataCount > 0)
	{
		if (DS_Count > 0)
		{
			Current_Data.DS18B20_Int = DS.get_Temperature(0);
			Current_Data.DS18B20_Ext = DS.get_Temperature(1);
		}
		Current_Data.Prod_Th = emul_PV.Compute_Power_TH(RTC_Local.getSecondOfTheDay(), false);
		if (TI_OK)
		{
			Current_Data.TI_Energy = (TI.getIndexWh() - Current_Data.TI_Counter);
			Current_Data.TI_Power = TI.getPowerVA();
		}
	}
}
void SetTalemaFactor(Talema_Factor_Enum factor)
{
	TalemaFactor = factor;
}

double GetTalemaFactor(void)
{
	double factor = 0.0;
	switch (TalemaFactor)
	{
		case tfSSR:
			factor = SSR_Get_Current_Percent() / 100;
			break;
		case tfChannel1:
			factor = Current_Data.Cirrus_ch1.PowerFactor;
			break;
		case tfChannel2:
			factor = Current_Data.Cirrus_ch2.PowerFactor;
			break;
	}
	return factor;
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

	sprintf(buffer, "Prms:%.2f   ", Current_Data.Cirrus_ch1.ActivePower);
	IHM_Print(line++, (char*) buffer);

#ifdef CIRRUS_RMS_FULL
	sprintf(buffer, "Papp:%.2f   ", Current_Data.Cirrus_ch1.ApparentPower);
	IHM_Print(line++, (char*) buffer);
#endif

	sprintf(buffer, "Prms2:%.2f   ", Current_Data.Cirrus_ch2.ActivePower);
	IHM_Print(line++, (char*) buffer);

	sprintf(buffer, "Energie:%.2f   ", Current_Data.energy_day_conso);
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
	*Energy = Current_Data.energy_day_conso;
	*Surplus = Current_Data.energy_day_surplus;
	*Prod = Current_Data.energy_day_prod;
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

	char buffer[255] = {0};
	char *pbuffer = &buffer[0];
	uint16_t len;

	// Ouvre le fichier en append, le crée s'il n'existe pas
	Lock_File = true;
	File temp = Data_Partition->open(CSV_Filename, "a");
	if (temp)
	{
		// Time, Pconso_rms, Pprod_rms, PTalema, Pth, U_rms, Tcs
		sprintf(buffer, "%ld", RTC_Local.getUNIXDateTime()); // Copie la date
		Fast_Set_Decimal_Separator('.');
		pbuffer = Fast_Pos_Buffer(buffer, "\t", Buffer_End, &len); // On se positionne en fin de chaine
		pbuffer = Fast_Printf(pbuffer, 2, "\t", Buffer_End, true,
				{log_cumul.Power_ch1, log_cumul.Power_ch2, Current_Data.Talema_Power, log_cumul.Voltage});

		// Temperatures, Energy
		pbuffer = Fast_Printf(pbuffer, 2, "\t", Buffer_End, false,
				{log_cumul.Temp, Current_Data.DS18B20_Int, Current_Data.DS18B20_Ext, Current_Data.energy_day_conso,
						Current_Data.energy_day_surplus, Current_Data.energy_day_prod, Current_Data.Talema_Energy});

		// End line
		Fast_Add_EndLine(pbuffer, Buffer_End);
		Fast_Set_Decimal_Separator(',');
		temp.print(buffer);
		temp.close();
	}
	Lock_File = false;
}

void reboot_energy(void)
{
	const int MAX_LINESIZE = 255;

	// On vérifie qu'on n'est pas en train de l'uploader
	if (Lock_File)
		return;

	// Ouvre le fichier en read
	Lock_File = true;
	if (Data_Partition->exists(CSV_Filename))
	{
		File temp = Data_Partition->open(CSV_Filename, "r");
		if (temp)
		{
			String strLine = "";
			char line[MAX_LINESIZE] = {0};
			char *pbuffer;
			float data[20]; // to be large
			int i = 0;
			// Read until last line
			while (temp.available())
			{
				strLine = temp.readStringUntil('\n');
			}
			temp.close();

//			print_debug(strLine);

			// split last line
			if (!strLine.isEmpty())
			{
				strLine.toCharArray(line, MAX_LINESIZE);
				pbuffer = strtok((char*) line, "\t");
				while (pbuffer != NULL)
				{
					data[i] = strtof(pbuffer, NULL);
//					print_debug(String(data[i]));
					pbuffer = strtok(NULL, "\t");
					i++;
				}
			}
			// The last 4 data is energy
			if (i > 3)
			{
				// energy_day_conso, energy_day_surplus
				CS5480.RestartEnergy(Channel_1, data[i - 4], data[i - 3]);
				// energy_day_prod
				CS5480.RestartEnergy(Channel_2, data[i - 2]);
				Current_Data.Talema_Energy = data[i - 1];
			}
		}
	}

	Lock_File = false;
}

void append_energy(void)
{
	// On vérifie qu'on n'est pas en train de l'uploader
	if (Lock_File)
		return;

	char buffer[255] = {0};
	char *pbuffer = &buffer[0];
	uint16_t len;

	// Ouvre le fichier en append, le crée s'il n'existe pas
	Lock_File = true;
	bool Exist = Data_Partition->exists(Energy_Filename);
	File temp = Data_Partition->open(Energy_Filename, "a");
	if (temp)
	{
		// Le fichier n'existait pas (nouvelle année), on ajoute la première ligne
		if (!Exist)
			temp.print(Energy_Heading);

		// Time, Econso, Esurplus, Eprod, ETalema
		RTC_Local.getShortDate(buffer); // Copie la date
		Fast_Set_Decimal_Separator('.');
		pbuffer = Fast_Pos_Buffer(buffer, "\t", Buffer_End, &len); // On se positionne en fin de chaine
		pbuffer = Fast_Printf(pbuffer, 2, "\t", Buffer_End, false,
				{Current_Data.energy_day_conso, Current_Data.energy_day_surplus, Current_Data.energy_day_prod,
						Current_Data.Talema_Energy});

		// End line
		Fast_Add_EndLine(pbuffer, Buffer_End);
		Fast_Set_Decimal_Separator(',');

		temp.print(buffer);
		temp.close();
	}
	Lock_File = false;
}

void onDaychange(uint8_t year, uint8_t month, uint8_t day)
{
	print_debug(F("Callback day change"));
	// On ajoute les énergies au fichier
	Energy_Filename = "/energy_20" + (String) year + ".csv";
	append_energy();
	Current_Data.energy_day_conso = 0.0;
	Current_Data.energy_day_surplus = 0.0;
	Current_Data.energy_day_prod = 0.0;
	Current_Data.Talema_Energy = 0.0;
	CS5480.RestartEnergy();
	if (TI_OK)
		Current_Data.TI_Counter = TI.getIndexWh();

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
// Task function to save the log
// ********************************************************************************

void Log_Data_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("LOG_DATA_Task");

	// Create the semaphore
	logSemaphore = xSemaphoreCreateBinary();

	for (EVER)
	{
		if (xSemaphoreTake(logSemaphore, 0) == pdTRUE)
			append_data();
		END_TASK_CODE(false);
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
