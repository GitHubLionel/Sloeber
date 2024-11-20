#include "Arduino.h"

// Vérifier que les modifications sont bien prises en compte dans sloeber.ino.cpp

// Pour le display, à définir comme directive dans le projet :
// OLED_SSD1306 pour le SSD1306  // Need the directive SSD1306_RAM_128 or SSD1306_RAM_132
// OLED_SSD1327 pour le SSD1327
// OLED_SH1107 pour le SH1107

#include "Server_utils.h"			  // Some utils functions for the server
#include "RTCLocal.h"					  // A pseudo RTC software library
#include "Partition_utils.h"		// Some utils functions for LittleFS/SPIFFS/FatFS
#include "display.h"				  	// Display functions
#include "CIRRUS.h"
#include "Get_Data.h"
#include "DS18B20.h"
#include "TeleInfo.h"
#include "SSR.h"
#include "Keyboard.h"
#include "Relay.h"
#include "iniFiles.h"
//#include "ADC_Utils.h"
#include "Fast_Printf.h"

/**
 * Define de debug
 * SERIAL_DEBUG	: Message de debug sur le port série (baud 115200)
 * LOG_DEBUG	: Enregistre le debug dans un fichier log
 */
//#define SERIAL_DEBUG
//#define LOG_DEBUG
//#define USE_SAVE_CRASH   // Permet de sauvegarder les données du crash
#include "Debug_utils.h"		// Some utils functions for debug

// Calibration
#ifdef CIRRUS_CALIBRATION
#include "CIRRUS_Calibration.h"
#endif

// Use DS18B20
#define USE_DS

// Use Teleinfo
//#define USE_TI

// Active le SSR
#define USE_ZC_SSR

// Active le relais
#define USE_RELAY

// Active le clavier
#define USE_KEYBOARD

// Active ADC
//#define USE_ADC

// Liste des taches
#include "Tasks_utils.h"       // Task list functions

#define USE_IDLE_TASK	false

/**
 * Ce programme executé par l'ESPxx fait le lien entre l'interface web et le hardware
 * WEB <--- Wifi ---> ESPxx <--- UART ---> Hardware (BluePill, Nucleo, ...)
 * La page web est stockée dans l'ESPxx dans sa mémoire FLASH au format LittleFS
 * L'analyse des messages web se fait via les callback "server.on()"
 * L'analyse des messages UART se fait via la fonction CheckUARTMessage()
 * Messages reconnus par défaut (UART) :
 * - ssid#password pour se connecter
 * - GET_IP pour obtenir l'adresse IP
 * - GET_TIME pour avoir l'heure de l'ESP (réponse au format TIME=dd-mm-yyHhh-nn-ssUunix_timestamp)
 * - GET_DATE= pour avoir l'heure de l'ESP (réponse au format dd-mm-yy#hh:nn:ss#END_DATE\r\n sans balise)
 * - DATE=DD/MM/YY#hh:mm:ss pour mettre à l'heure l'ESP
 * - RESET_ESP pour un soft reset de l'ESP
 * Pour définir d'autre message, utiliser la fonction UserAnalyseMessage
 * Les messages entrants et sortants doivent être encadrés par les balises BEGIN_DATA et END_DATA
 *
 * Trois façon de se connecter via SSID_CONNEXION (section DEFINE NETWORK):
 * - Automatique : ssid et password codés en dur
 * - Par programme : Envoyer le message ssid#password par UART ou fichier
 *
 * Connexion SoftAP ou réseau
 * - Access point : Directive ISSOFTAP = true. Dans ce cas l'adresse IP est 192.168.4.1
 *
 * L'utilisation de la classe ServerConnexion simplifie la gestion de la connexion :
 * - définition d'un objet ServerConnexion suivant le type de connexion, SSID, ...
 * - définition d'une callback à effectuer une fois que la connexion est établie
 *
 * La directive USE_NTP_SERVER permet de récupérer l'heure via Internet pour initialiser le RTC Local
 */

/**
 * Détail consommation
 * ESP01 : 72 mA max (250 mA ? : https://arduino-esp8266.readthedocs.io/en/3.0.2/boards.html)
 * JSN : 8 mA max
 * DS18B20 : 1.5 mA
 * Alimentation Meanwell IRM01-3.3 : 300 mA, 1 W
 * Alimentation Hi-Link HLK-PM03 : 3.3 V, 900 mA, 3 W
 */

/**
 * Si on veut que Serial soit initialisé dans le setup dans le cas où l'on veut communiquer
 * avec un autre périphérique (exemple le Cirrus).
 * ATTENTION : Si on défini en même temps SERIAL_DEBUG, on peut avoir des comportements indéfinis.
 */
//#define USE_UART

/*********************************************************************************
 *                    *****  DEFINE NETWORK  *****
 *********************************************************************************
 * Définition de où se trouve le SSID et le password :
 * SSID_CONNEXION : CONN_INLINE = 0, CONN_UART = 1, CONN_FILE = 2, CONN_EEPROM = 3.
 * Correspondant respectivement à soit en dur dans le programme, soit lecture via UART,
 * soit lecture fichier, soit lecture EEPROM
 * ISSOFTAP	  : false = mode Station connecté au réseau présent, true = Mode Access Point (192.168.4.1)
 */
#define SSID_CONNEXION	CONN_EEPROM  // CONN_FILE
#define ISSOFTAP	false

// Défini les callback events et les opérations à faire après connexion
void OnAfterConnexion(void);

#if SSID_CONNEXION == CONN_INLINE
	#if (ISSOFTAP)
	// Pas de mot de passe
	ServerConnexion myServer(ISSOFTAP, "SoftAP", "", OnAfterConnexion);
	//  ServerConnexion myServer(ISSOFTAP, "SoftAP", "", IPAddress(192, 168, 4, 2), IPAddress(192, 168, 4, 1));
		#else
	// Définir en dur le SSID et le pwd du réseau
	ServerConnexion myServer(ISSOFTAP, "TP-Link_6C90", "77718983", OnAfterConnexion);
	#endif
#else
// Connexion UART, File or EEPROM
ServerConnexion myServer(ISSOFTAP, OnAfterConnexion);
#endif

// ********************************************************************************
// Définition user
// ********************************************************************************

// Définition de l'adresse du port I2C
#ifndef I2C_ADDRESS
#define I2C_ADDRESS	0x3C // ou 0x78
#endif
String UART_Message = "";

String Temp_str = "";
String Extra_str = "";
#define EXTRA_TIMEOUT	5  // L'extra texte est affiché durant EXTRA_TIMEOUT secondes
uint8_t count_Extra_Display = 0;

// ********************************************************************************
// Définition DS18B20
// ********************************************************************************

// Address 40 : DS18B20 air
// Address 82 : DS18B20 eau

uint8_t DS_Count = 0;
DS18B20 DS(DS18B20_GPIO);

// ********************************************************************************
// Définition TeleInfo
// ********************************************************************************

bool TI_OK = false;
uint32_t TI_Counter = 0;
TeleInfo TI(TI_RX_GPIO, 5000);

// ********************************************************************************
// Définition Cirrus
// ********************************************************************************

// CS1 = CS5484
CIRRUS_Calib_typedef CS1_Calib = {242.00, 53.55, 0x3CC756, 0x40D6B0, 0x7025B9, 0x0, 0x0, 242.00,
		17.00, 0x3CC756, 0x4303EE, 0x8A6100, 0x0, 0x0};
CIRRUS_Config_typedef CS1_Config = {0xC00000, 0x44E2EB, 0x2AA, 0x731F0, 0x51D67C, 0x0}; // 0x400000

// Config1 = 0x44E2EB : version ZC sur DO0
// Config1 = 0x00E4EB : version ZC sur DO0, P1 négatif sur DO3
// Config1 = 0x40E5EB : version ZC sur DO0, P2 négatif sur DO3
// Config1 = 0x44E25B : version ZC sur DO0, P2 négatif sur DO2, EPG3 output (P1 avg) sur DO3
// Config1 = 0x22E51B : version ZC sur DO0, EPG2 output (P1 avg) sur DO2, P2 négatif sur DO3

// CS2 = 5480
CIRRUS_Calib_typedef CS2_Calib = {260.00, 16.50, 0x3CC00B, 0x41B8D9, 0x5CF11, 0x9, 0x800009, 224.50,
		60.00, 0x400000, 0x400000, 0x0, 0x0, 0x0};
CIRRUS_Config_typedef CS2_Config = {0xC02000, 0x44E2E4, 0x1002AA, 0x931F0, 0x6C77D9, 0x0};

#ifdef CIRRUS_USE_UART
#if CIRRUS_UART_HARD == 1
//#undef SERIAL_DEBUG	// Ne peut pas debugger en Cirrus hard ou rediriger vers Serial1
CIRRUS_SERIAL_MODE *csSerial = &Serial2;
#else
CIRRUS_SERIAL_MODE *csSerial = new CIRRUS_SERIAL_MODE(D7, D8); // D7=RX, D8=TX  SoftwareSerial(int8_t rxPin, int8_t txPin)
#endif
#endif

CIRRUS_Communication CS_Com = CIRRUS_Communication(csSerial, CIRRUS_RESET_GPIO);
CIRRUS_CS548x CS5480 = CIRRUS_CS548x(CS_Com);
CIRRUS_CS548x CS5484 = CIRRUS_CS548x(CS_Com, true);
#ifdef CIRRUS_CALIBRATION
CIRRUS_Calibration CS_Calibration = CIRRUS_Calibration(CS5480);
extern bool Calibration;
#endif

bool Cirrus_OK = false;
bool Mess_For_Cirrus_Connect = false;

// To access cirrus data
extern volatile Data_Struct Current_Data;
extern volatile Graphe_Data log_cumul;

#ifndef USE_ZC_SSR
extern volatile SemaphoreHandle_t topZC_Semaphore;
extern void onCirrusZC(void);
#endif

// ********************************************************************************
// Définition Relais, Clavier, ADC
// ********************************************************************************

#ifdef USE_RELAY
// Pin commande du relais
Relay_Class Relay( {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33}); // @suppress("Invalid arguments")
#define  GPIO_RELAY_FACADE	GPIO_NUM_15

// callback to update relay every minutes
void onRelayMinuteChange_cb(uint16_t minuteOfTheDay)
{
	Relay.updateTime(minuteOfTheDay);
}
#endif

#ifdef USE_KEYBOARD
bool Toggle_Keyboard = true;
uint16_t interval[] = {3600, 2500, 1140, 240};
#endif

bool ADC_OK = false;

// Pour SSR_Compute_Dump_power
bool CIRRUS_get_rms_data(float *uRMS, float *pRMS)
{
	return CS_Com.Get_rms_data(uRMS, pRMS);
}

// ********************************************************************************
// Définition Fichier ini
// ********************************************************************************

IniFiles init_routeur = IniFiles("Conf.ini");

// ********************************************************************************
// Functions prototype
// ********************************************************************************

// UART message
bool UserAnalyseMessage(void);
void handleInitialization(CB_SERVER_PARAM);
void handleLastData(CB_SERVER_PARAM);
void handleOperation(CB_SERVER_PARAM);
extern void handleCirrus(CB_SERVER_PARAM);

// ********************************************************************************
// Task functions
// ********************************************************************************

void Display_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("Display_Task");
	uint8_t line = 0;
	for (EVER)
	{
#ifdef CIRRUS_CALIBRATION
		if (Calibration)
		{
			END_TASK_CODE();
			continue;
		}
#endif

		Temp_str = "";
		// Cirrus message
		if (Cirrus_OK)
		{
			line = Update_IHM(RTC_Local.the_time, "", false);
		}
		else
		{
			line = 0;
			IHM_Clear();
			IHM_Print(line++, RTC_Local.the_time);
//			UART_Message = "Cirrus failled";
		}

		// DS18B20 message
		if (DS_Count > 0)
		{
			Temp_str = "DS: " + DS.get_Temperature_Str(0) + "` " + DS.get_Temperature_Str(1) + "` ";
			IHM_Print(line++, (const char*) Temp_str.c_str());
		}

		// TI message
		if (TI_OK)
		{
			Temp_str = "TI: " + String(TI.getIndexWh());
			IHM_Print(line++, (const char*) Temp_str.c_str());
		}

		// Keyboard info message
		if (count_Extra_Display != 0)
		{
			IHM_Print(line++, Extra_str.c_str());
			count_Extra_Display--;
			if (count_Extra_Display == 0)
				Extra_str = "";
		}

		// Idle counter
#if USE_IDLE_TASK == true
		IHM_Print(line++, (const char*) TaskList.GetIdleStr().c_str());
#endif

		// Refresh display
		IHM_Display();

		// Test extinction de l'écran
		IHM_CheckTurnOff();
		END_TASK_CODE();
	}
}
#define DISPLAY_DATA_TASK	{true, "Display_Task", 4096, 4, 1000, CoreAny, Display_Task_code}

void UserKeyboardAction(Btn_Action Btn_Clicked)
{
	static uint32_t Btn_K1_count = 0;
	if (Btn_Clicked != Btn_NOP)
		Serial.println(Btn_Texte[Btn_Clicked]);

	switch (Btn_Clicked)
	{
		case Btn_K1: // Bouton du bas : Affiche IP et reset
		{
			IHM_DisplayOn();
			if (Extra_str.isEmpty())
				Extra_str = myServer.IPaddress();
			count_Extra_Display = EXTRA_TIMEOUT;
			Btn_K1_count++;
			// On a appuyé plus de 2 secondes sur le bouton
			if (Btn_K1_count > (DEBOUNCING_MS * 10) / 1000)
			{
				Extra_str = "SSID reset";
				// Delete SSID file
//				DeleteSSID();
				Btn_K1_count = 0;
			}
			break;
		}

		case Btn_K2: // Bouton du milieu : toggle relais
		{
			IHM_DisplayOn();
			Relay.setState(0, !Relay.getState(0));
			if (Relay.getState(0))
			{
				digitalWrite(GPIO_RELAY_FACADE, HIGH);
				Extra_str = "Relais ON";
			}
			else
			{
				digitalWrite(GPIO_RELAY_FACADE, LOW);
				Extra_str = "Relais OFF";
			}
			count_Extra_Display = EXTRA_TIMEOUT;
			break;
		}

		case Btn_K3: // Bouton du haut : toggle display
		{
			IHM_ToggleDisplay();
			break;
		}

		case Btn_NOP:
		{
			Btn_K1_count = 0;
			break;
		}

		default:
			;
	}
}

// ********************************************************************************
// Initialization
// ********************************************************************************

/**
 * The setup function is called once at startup of the sketch
 * Initialisation de :
 * 0- serial
 * 1- littleFS
 * 2- datetime
 * 3- ...
 */
void setup()
{
	// To know the time required for the setup
	uint32_t start_time = millis();

	// **** 0- initialisation Serial, debug ou pas ****
	SERIAL_Initialization();

	// **** 1- initialisation LittleFS ****
	// Start the SPI Flash Files System, abort if not success
	if (!FS_Partition->begin())
	{
		print_debug(F("LittleFS/SPIFFS/FatFS mount failed"));
		// On abandonne l'initialisation
		return;
	}

	// Message de démarrage à mettre APRES SERIAL et LittleFS !
	print_debug(F("\r\n\r\n****** Starting : "), false);
	print_debug(getSketchName(__FILE__, true));

	// Création d'heure pseudo RTC. Utilise RTC_Local.
#ifdef ESP8266
	FS_Partition->setTimeCallback(RTC_Local_Callback);
#endif

#ifdef USE_SAVE_CRASH
	init_and_print_crash();
#endif

#ifdef SERIAL_DEBUG
	// Des infos sur la flash
	ESPinformations();
	Partition_Info();
//  Partition_ListDir();
#endif

	// Création de la partition data
	CreateOpenDataPartition(false, true);

	// Initialisation fichier ini
	init_routeur.Begin(true);

	// **** 2- initialisation datetime ****
	// Essaye de récupérer la dernière heure, utile pour un reboot
	// Paramètres éventuellement à adapter : AutoSave et SaveDelay (par défaut toutes les 10 s)
	RTC_Local.setupDateTime();
	RTC_Local.setEndDayCallBack(onDaychange); // Action à faire juste avant minuit
	RTC_Local.setDayChangeCallback(onDaychange); // Action à faire si on change de jour (mise à jour)

	// **** 3- Initialisation du display
	if (IHM_Initialization(I2C_ADDRESS, false))
		print_debug(F("Display Ok"));
	IHM_TimeOut_Display(OLED_TIMEOUT);
	Display_offset_line = 0;

	// **** 4- initialisation DS18B20 ****
#ifdef USE_DS
	DS_Count = DS.Initialize(DS18B20_PRECISION);
#endif

	// **** 5- Initialisation TéléInfo
#ifdef USE_TI
	IHM_Print0("Test TI");

	if (TI.Configure(10000))
	{
		print_debug(F("Configuration TeleInfo OK"));
		TI.PrintAllToSerial();
		TI_OK = true;
		TI_Counter = TI.getIndexWh();
	}
	else
		print_debug(F("Configuration TeleInfo ERROR"));
#endif

	// **** 6- initialisation Cirrus ****
	// Initialisation du Cirrus
	// Start the communication
	CS_Com.begin();

	// Add Cirrus
	// CS1 = CS5484, CS2 = CS5480
	CS_Com.AddCirrus(&CS5484, CIRRUS_CS1_GPIO);
	CS_Com.AddCirrus(&CS5480, CIRRUS_CS2_GPIO);

	// Start the Cirrus CS5480 and CS5484
	if ((CS_Com.SelectCirrus(0))->begin(CIRRUS_UART_BAUD, false))
	{
		Cirrus_OK = (CS_Com.SelectCirrus(1))->begin(CIRRUS_UART_BAUD, true);
	}

	if (Cirrus_OK)
		print_debug(F("CS begin OK"));
	else
		print_debug(F("CS begin ERROR"));

	// Configure first Cirrus : CS5484
	CS_Com.SelectCirrus(0);

	// Initialisation CS5484 : phase 1, phase 2
	if (Cirrus_OK)
		Cirrus_OK = CIRRUS_Generic_Initialization(CS5484, &CS1_Calib, &CS1_Config, false, false, '1');

	// Configure second Cirrus : CS5480
	CS_Com.SelectCirrus(1);

	// Initialisation CS5484 : phase 3, production
	if (Cirrus_OK)
		Cirrus_OK = CIRRUS_Generic_Initialization(CS5480, &CS2_Calib, &CS2_Config, false, false, '2');

	// On revient sur le premier Cirrus : CS5484
	CS_Com.SelectCirrus(0);

	// **** 7- Initialisation SSR avec Zero-Cross ****
#ifdef USE_ZC_SSR
	SSR_Initialize(ZERO_CROSS_GPIO, SSR_COMMAND_GPIO, SSR_LED_GPIO);

//	SSR_Compute_Dump_power(1000);
	SSR_Set_Dump_Power(1000);  // Par défaut, voir page web
//	SSR_Set_Dimme_Target(50);
//  SSR_Action(SSR_Action_Dimme);

	SSR_Action(SSR_Action_Percent);  // Par défaut à 10%, voir page web
//	SSR_Set_Percent(20);
			// NOTE : le SSR est éteint, on le démarre dans la page web
#else
	topZC_Semaphore = xSemaphoreCreateBinary();
	CS5480.ZC_Initialize(ZERO_CROSS_GPIO, onCirrusZC);
#endif

	// **** 8- Initialisation Clavier, relais, ADC
#ifdef USE_ADC
	if ((ADC_OK = ADC_Initialize_OneShot({KEYBOARD_ADC_GPIO, GPIO_NUM_39})) == true) // @suppress("Invalid arguments")
	{
		print_debug(F("ADC OK"));
//		ADC_Begin();
	}
	else
		print_debug(F("ADC Failed"));
#endif

#ifdef USE_KEYBOARD
	Keyboard_Initialize(3, interval);
//	Keyboard_Init(3, ADC_12bits); // pas bon
//	IHM_Print0("Test Btn");
//  Btn_Check_Config();
#endif

#ifdef USE_RELAY
	pinMode(GPIO_RELAY_FACADE, OUTPUT);
	digitalWrite(GPIO_RELAY_FACADE, LOW);
	for (int i = 0; i < Relay.size(); i++)
	{
		Relay.addAlarm(i, Alarm1, init_routeur.ReadIntegerIndex(i, "Relais", "Alarm1_start", -1),
				init_routeur.ReadIntegerIndex(i, "Relais", "Alarm1_end", -1));
		Relay.addAlarm(i, Alarm2, init_routeur.ReadIntegerIndex(i, "Relais", "Alarm2_start", -1),
				init_routeur.ReadIntegerIndex(i, "Relais", "Alarm2_end", -1));
		Relay.setState(i, init_routeur.ReadBoolIndex(i, "Relais", "State", false));
	}
	RTC_Local.setMinuteChangeCallback(onRelayMinuteChange_cb);
#endif

	// **** FIN- Attente connexion réseau
	IHM_Print0("Connexion .....");
	print_debug(F("==> Wait for network <=="));
	if (!myServer.WaitForConnexion((Conn_typedef) SSID_CONNEXION))
	{
		print_debug(F("==> Connected to network <=="));

		// Affichage ip adresse
		IHM_IPAddress(myServer.IPaddress().c_str());
	}
	else
	{
		IHM_Print0("Failed !   ");
		print_debug(F("==> Connexion to network failed <=="));
	}

	print_debug("*** Setup time : " + String(millis() - start_time) + " ms ***\r\n");
	delay(2000);

	// Initialisation des taches
	TaskList.AddTask(RTC_DATA_TASK); // RTC Task
	TaskList.AddTask(UART_DATA_TASK); // UART Task
#ifdef USE_DS
	TaskList.AddTask(DS18B20_DATA_TASK(DS_Count > 0)); // DS18B20 Task
#endif
#ifdef USE_TI
	TaskList.AddTask(TELEINFO_DATA_TASK(TI_OK)); // TeleInfo Task
#endif
	TaskList.AddTask(CIRRUS_DATA_TASK(Cirrus_OK)); // Cirrus get data Task
	TaskList.AddTask(DISPLAY_DATA_TASK);
#ifdef USE_KEYBOARD
	TaskList.AddTask(KEYBOARD_DATA_TASK(true));
#endif
	TaskList.Create(USE_IDLE_TASK);
#ifdef USE_ADC
	if (ADC_OK)
	{
		ADC_Begin();
	}
#endif
}

// The loop function is called in an endless loop
// If empty, about 50000 loops by second
// Les mesures ne sont faites que si au moins une seconde est passée
void loop()
{
	// Listen for HTTP requests from clients
#ifndef USE_ASYNC_WEBSERVER
	server.handleClient();
#else
	// Temporisation à adapter
	vTaskDelay(1);
#endif
}

// ********************************************************************************
// OnAfterConnexion
// ********************************************************************************
/**
 * Définition des évènements après la connexion au serveur
 * Définition des opérations à faire juste après la connexion
 * Cette fonction ne sera appelée qu'une seule fois.
 * Cette fonction répond aux messages de la fonction ESP_Request du fichier JavaScript ESPTemplate.js
 */
void OnAfterConnexion(void)
{
	// Server default events
	Server_CommonEvent(
			Ev_LoadPage | Ev_GetFile | Ev_DeleteFile | Ev_UploadFile | Ev_ListFile | Ev_ResetESP
					| Ev_SetTime | Ev_GetTime | Ev_SetDHCPIP | Ev_ResetDHCPIP);

	// Server specific events (voir le javascript)
	server.on("/getUARTData", HTTP_GET, [](CB_SERVER_PARAM)
	{
		pserver->send(200, "text/plain", UART_Message);
	});

	server.on("/initialisation", HTTP_GET, handleInitialization);

	server.on("/getLastData", HTTP_GET, handleLastData);

	server.on("/operation", HTTP_PUT, handleOperation);

	server.on("/getCirrus", HTTP_PUT, handleCirrus);
}

// ********************************************************************************
// Print functions
// ********************************************************************************

void PrintTerminal(const char *text)
{
////  UART_Message = String(text);
////	pserver->send(200, "text/plain", (String(text)));
//	print_debug(String(text), false);
////	(void) text;

	if (Mess_For_Cirrus_Connect)
		printf_message_to_UART(text, false);
	else
		print_debug(String(text));
}

// ********************************************************************************
// Web/UART functions
// ********************************************************************************

bool UserAnalyseMessage(void)
{
	// Pour cirrus_Connect faut rajouter la balise de fin.
	uint8_t answer = 0;

#ifdef LOG_CIRRUS_CONNECT
	char buffer[BUFFER_SIZE];

	strcpy(buffer, (const char*) UART_Message_Buffer);
	strcat(buffer, "\03");
	Mess_For_Cirrus_Connect = true;
	answer = CS_Com.UART_Message_Cirrus((unsigned char*) buffer);
	Mess_For_Cirrus_Connect = false;
#endif
	if (answer == 0)
	{
		// Default
		printf_message_to_UART("Unknown message : " + String((const char*) UART_Message_Buffer));
		// Message pour la page web
		UART_Message = String((const char*) UART_Message_Buffer);
	}
	return false;
}

void handleInitialization(CB_SERVER_PARAM)
{
	String message = "";
	// SSR Part
	if (SSR_Get_Action() == SSR_Action_Percent)
		message = "1#";
	else
		if (SSR_Get_Action() == SSR_Action_Surplus)
			message = "2#";
		else
			message = "3#";
	message += (String) SSR_Get_Dump_Power() + '#';
	message += (String) SSR_Get_Target() + '#';
	message += (String) SSR_Get_Percent() + '#';
	if (SSR_Get_State() == SSR_OFF)
		message += "OFF#";
	else
		message += "ON#";

	// Relay part
	String alarm = "";
	String start = "", end = "";
	for (int i = 0; i < Relay.size(); i++)
	{
		(Relay.getState(i)) ? alarm += "ON," : alarm += "OFF,";
		Relay.getAlarm(i, Alarm1, start, end);
		alarm += start + "," + end + ",";
		Relay.getAlarm(i, Alarm2, start, end);
		alarm += start + "," + end + ",";
	}
	message += alarm;

	pserver->send(200, "text/plain", message);
}

/**
 * Inst data
 * Time#Ph1_data#Ph2_data#Ph3_data#Prod_data#Temp_data#SSR_state#Log_data
 */
void handleLastData(CB_SERVER_PARAM)
{
	char buffer[255] = {0};
	char *pbuffer = &buffer[0];
	uint16_t len;
	float Energy, Surplus, Prod;

	uint32_t TI_Energy = 0;
	bool graphe = Get_Last_Data(&Energy, &Surplus, &Prod);

	if (TI_OK)
		TI_Energy = (TI.getIndexWh() - TI_Counter);

	strcpy(buffer, RTC_Local.the_time); // Copie la date
	pbuffer = Fast_Pos_Buffer(buffer, "#", Buffer_End, &len); // On se positionne en fin de chaine
	pbuffer = Fast_Printf(pbuffer, 2, "#", Buffer_End,
			{Current_Data.Phase1.Voltage, Current_Data.Phase1.ActivePower, Current_Data.Phase1.Energy,
					Current_Data.Phase2.Voltage, Current_Data.Phase2.ActivePower, Current_Data.Phase2.Energy,
					Current_Data.Phase3.Voltage, Current_Data.Phase3.ActivePower, Current_Data.Phase3.Energy,
					Current_Data.Production.ActivePower, Current_Data.Production.Energy, Surplus});

//	pbuffer = Fast_Printf(pbuffer, TI_Energy, 0, "#", "#", Buffer_End, &len);

	// Températures
	pbuffer = Fast_Printf(pbuffer, Current_Data.Cirrus1_Temp, 2, "#", "#", Buffer_End, &len);
	if (DS_Count > 0)
	{
		pbuffer = Fast_Printf(pbuffer, DS.get_Temperature(0), 2, "", "#", Buffer_End, &len);
		pbuffer = Fast_Printf(pbuffer, DS.get_Temperature(1), 2, "", "#", Buffer_End, &len);
	}
	else
	{
		pbuffer = Fast_Printf(pbuffer, 0.0, 2, "", "#", Buffer_End, &len);
		pbuffer = Fast_Printf(pbuffer, 0.0, 2, "", "#", Buffer_End, &len);
	}

	// Talema
//	pbuffer = Fast_Printf(pbuffer, Current_Data.Talema_Power, 2, "", "#", Buffer_End, &len);
//	pbuffer = Fast_Printf(pbuffer, Current_Data.Talema_Energy, 2, "", "#", Buffer_End, &len);

	// Etat du SSR
	pbuffer = Fast_Printf(pbuffer, (int) SSR_Get_State(), 0, "SSR=", "", Buffer_End, &len);

	// On a de nouvelles données pour le graphe
	if (graphe)
	{
		Fast_Set_Decimal_Separator('.');
		pbuffer = Fast_Pos_Buffer(pbuffer, "#", Buffer_End, &len); // On se positionne en fin de chaine
		pbuffer = Fast_Printf(pbuffer, 2, "#", Buffer_End, {log_cumul.Voltage_ph1, log_cumul.Power_ph1,
				log_cumul.Voltage_ph2, log_cumul.Power_ph2, log_cumul.Voltage_ph3, log_cumul.Power_ph3,
				log_cumul.Temp});
		Fast_Set_Decimal_Separator(',');
	}
	pserver->send(200, "text/plain", buffer);
}

void handleOperation(CB_SERVER_PARAM)
{
	// Default
	RETURN_BAD_ARGUMENT();

	print_debug("Operation: " + pserver->argName((int) 0) + "=" + pserver->arg((int) 0));

#if defined(OLED_DEFINED)
	if (pserver->hasArg("Toggle_Oled"))
	{
		IHM_ToggleDisplay();
	}
#endif

	// Togggle relay
#ifdef USE_RELAY
	if (pserver->hasArg("Toggle_Relay"))
	{
		Relay.setState(0, !Relay.getState(0));
		if (Relay.getState(0))
			digitalWrite(GPIO_RELAY_FACADE, HIGH);
		else
			digitalWrite(GPIO_RELAY_FACADE, LOW);
		int id = pserver->arg("Toggle_Relay").toInt();
		bool state = (pserver->arg("State").toInt() == 1);
		Relay.setState(id, state);
		init_routeur.WriteIntegerIndex(id, "Relais", "State", state);
	}
	if (pserver->hasArg("AlarmID"))
	{
		int id = pserver->arg("AlarmID").toInt();
		int start = pserver->arg("Alarm1D").toInt();
		int end = pserver->arg("Alarm1F").toInt();
		Relay.addAlarm(id, Alarm1, start, end);
		init_routeur.WriteIntegerIndex(id, "Relais", "Alarm1_start", start);
		init_routeur.WriteIntegerIndex(id, "Relais", "Alarm1_end", end);

		start = pserver->arg("Alarm2D").toInt();
		end = pserver->arg("Alarm2F").toInt();
		Relay.addAlarm(id, Alarm2, start, end);
		init_routeur.WriteIntegerIndex(id, "Relais", "Alarm2_start", start);
		init_routeur.WriteIntegerIndex(id, "Relais", "Alarm2_end", end);
	}
#endif

#ifdef USE_ZC_SSR
	// Change le mode d'action Pourcent/Zéro
	// Ne pas oublier de redémarrer le SSR après
	if (pserver->hasArg("SSRAction"))
	{
		if (pserver->arg("SSRAction") == "percent")
			SSR_Action(SSR_Action_Percent);
		else
			if (pserver->arg("SSRAction") == "zero")
				SSR_Action(SSR_Action_Surplus);
			else
				SSR_Action(SSR_Action_FULL);

		init_routeur.WriteInteger("SSR", "Action", SSR_Get_Action(), "Full=1, Percent=2, Zero=3");
	}

	// La puissance du CE pour le mode zéro
	if (pserver->hasArg("CEPower"))
	{
		float power = pserver->arg("CEPower").toFloat();
		SSR_Set_Dump_Power(power);
		init_routeur.WriteFloat("SSR", "P_CE", power);

		print_debug("Operation: " + pserver->argName((int) 1) + "=" + pserver->arg((int) 1));
		float target = pserver->arg("SSRTarget").toFloat();
		SSR_Set_Target(target);
		init_routeur.WriteFloat("SSR", "Target", target);
	}

	if (pserver->hasArg("CheckPower"))
	{
		TaskList.SuspendTask("CIRRUS_Task");
		delay(200);
		double power = SSR_Compute_Dump_power();
		TaskList.ResumeTask("CIRRUS_Task");
		init_routeur.WriteFloat("SSR", "P_CE", power);
	}

	// Gestion dimmer en pourcentage
	if (pserver->hasArg("Pourcent") && (SSR_Get_Action() == SSR_Action_Percent))
	{
		float percent = pserver->arg("Pourcent").toFloat();
		SSR_Set_Percent(percent);
		init_routeur.WriteFloat("SSR", "Pourcent", percent);
	}

	// Allume ou éteint le SSR
	if (pserver->hasArg("Toggle_SSR"))
	{
		if (SSR_Get_State() == SSR_OFF)
			SSR_Enable();
		else
			SSR_Disable();
	}
#endif

	pserver->send(204, "text/plain", "");
}

String Handle_Wifi_Request(CS_Common_Request Wifi_Request, char *Request)
{
	return CS_Com.Handle_Common_Request(Wifi_Request, Request, &CS1_Calib, &CS1_Config);
}

// ********************************************************************************
// End of file
// ********************************************************************************
