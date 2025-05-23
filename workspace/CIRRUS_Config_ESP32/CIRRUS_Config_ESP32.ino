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
#include "Simple_Get_Data.h"
#include "DS18B20.h"
#include "Keyboard.h"
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

// Active le clavier
#define USE_KEYBOARD

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
// Définition Cirrus
// ********************************************************************************

// Pour la version tri, sinon version mono
#define TWO_CIRRUS	false

//CIRRUS_Calib_typedef CS1_Calib = CS_CALIB0;
//CIRRUS_Config_typedef CS1_Config = CS_CONFIG0;
//CIRRUS_Calib_typedef CS2_Calib = CS_CALIB0;
//CIRRUS_Config_typedef CS2_Config = CS_CONFIG0;

#if TWO_CIRRUS == true
// Cirrus1 (CS5484 pour le tri)
CIRRUS_Calib_typedef CS1_Calib = {242.00, 33.15, 0x3CC966, 0x405450, 0xAC12A1, 0x000000, 0x800000,
		242.00, 33.15, 0x3C9481, 0x3FC850, 0x247199, 0x000000, 0x800000};
#else
// Cirrus 1 : CS5480
CIRRUS_Calib_typedef CS1_Calib = {242.00, 33.15, 0x3C638E, 0x3F94EA, 0x629900, 0x000030, 0x800000,
		242.00, 33.15, 0x3C2EE9, 0x3F8CDA, 0xEC5B10, 0x000030, 0x800000};
#endif

//CIRRUS_Calib_typedef CS1_Calib = {242.00, 33.15, 0x3D4A81, 0x3FB28B, 0x7025B9, 0x000000, 0x000000,
//			242.00, 33.15, 0x3D15C6, 0x3F67DA, 0x8A6100, 0x000000, 0x000000};

//CIRRUS_Calib_typedef CS1_Calib = {242.00, 33.15, 0x3CC756, 0x40D6B0, 0x7025B9, 0x0, 0x0, 242.00,
//		33.15, 0x3CC756, 0x4303EE, 0x8A6100, 0x0, 0x0};

//CIRRUS_Calib_typedef CS1_Calib = {242.00, 33.15, 0x3C5C3B, 0x401665, 0x532144, 0x0, 0x0,
//		242.00, 33.15, 0x3C5C3B, 0x3FE51B, 0x81EF04, 0x0, 0x0};

#if TWO_CIRRUS == true
// config0 = 0x400000 pour le CS5484
CIRRUS_Config_typedef CS1_Config = {0x400000, 0x44E2EB, 0x0602AA, 0x731F0, 0x51D67C, 0x0};
#else
// config0 = 0xC02000 pour le CS5480
CIRRUS_Config_typedef CS1_Config = {0xC02000, 0x44E25B, 0x0002AA, 0x731F0, 0x51D67C, 0x0};
#endif

// Config1 = 0x44E2EB : version ZC sur DO0
// Config1 = 0x00E4EB : version ZC sur DO0, P1 négatif sur DO3
// Config1 = 0x40E5EB : version ZC sur DO0, P2 négatif sur DO3
// Config1 = 0x44E25B : version ZC sur DO0, P2 négatif sur DO2, EPG3 output (P1 avg) sur DO3
// Config1 = 0x22E51B : version ZC sur DO0, EPG2 output (P1 avg) sur DO2, P2 négatif sur DO3

// Cirrus2 (5480 pour le tri)
CIRRUS_Calib_typedef CS2_Calib = {242.00, 33.15, 0x3C7286, 0x3F193C, 0x716F10, 0x000000, 0x800000,
		242.00, 33.15, 0x3C7286, 0x3F897F, 0x5300A4, 0x000000, 0x800000};

//CIRRUS_Calib_typedef CS2_Calib = {242.00, 33.15, 0x3C65AD, 0x3F48B7, 0x5AD0F1, 0x000000, 0x800000,
//		242.00, 33.15, 0x3C65AD, 0x3EE855, 0x2605A4, 0x000000, 0x800000};

//CIRRUS_Calib_typedef CS2_Calib = {242.00, 33.15, 0x3C5375, 0x3E8464, 0x05CF11, 0x000009, 0x800009,
//		242.00, 33.15, 0x3C5375, 0x3E1C67, 0x000000, 0x000000, 0x000000};
//CIRRUS_Calib_typedef CS2_Calib = {242.00, 33.15, 0x3CC00B, 0x41B8D9, 0x5CF11, 0x9, 0x800009, 242.00,
//		33.15, 0x400000, 0x400000, 0x0, 0x0, 0x0};

//CIRRUS_Calib_typedef CS2_Calib = {242.00, 33.15, 0x3BEEE2, 0x3FE47F, 0x8FC5F9, 0x0, 0x0,
//		242.00, 33.15, 0x3BEEE2, 0x3FB374, 0x278251, 0x0, 0x0};
CIRRUS_Config_typedef CS2_Config = {0xC02000, 0x44E2E4, 0x0002AA, 0x931F0, 0x6C77D9, 0x0};

#ifdef CIRRUS_USE_UART
#if CIRRUS_UART_HARD == 1
//#undef SERIAL_DEBUG	// Ne peut pas debugger en Cirrus hard ou rediriger vers Serial1
CIRRUS_SERIAL_MODE *csCom = &Serial2;
#else
CIRRUS_SERIAL_MODE *csCom = new CIRRUS_SERIAL_MODE(D7, D8); // D7=RX, D8=TX  SoftwareSerial(int8_t rxPin, int8_t txPin)
#endif
#define CS_BEGIN_PARAM(init_param)	CIRRUS_UART_BAUD, init_param
#else
SPIClass *csCom = &SPI;
#define CS_BEGIN_PARAM(init_param)
#endif

CIRRUS_Communication CS_Com = CIRRUS_Communication(csCom, CIRRUS_RESET_GPIO);
#if TWO_CIRRUS == true
CIRRUS_CS548x Cirrus1 = CIRRUS_CS548x(CS_Com, true); // CS5484
CIRRUS_CS548x Cirrus2 = CIRRUS_CS548x(CS_Com); // CS5480
#else
CIRRUS_CS548x Cirrus1 = CIRRUS_CS548x(CS_Com); // CS5480 (routeur version mono)
#endif
#ifdef CIRRUS_CALIBRATION
CIRRUS_Calibration CS_Calibration = CIRRUS_Calibration(Cirrus1);
extern bool Calibration;
#endif

bool Cirrus_OK = false;
bool Mess_For_Cirrus_Connect = false;

// ********************************************************************************
// Définition Clavier
// ********************************************************************************

#ifdef USE_KEYBOARD
bool Toggle_Keyboard = true;
uint16_t interval[] = {1250, 950, 650, 250};
#endif

#define  GPIO_RELAY_FACADE	GPIO_NUM_15

// ********************************************************************************
// Functions prototype
// ********************************************************************************

// UART message
bool UserAnalyseMessage(void);
void handleLastData(CB_SERVER_PARAM);
void handleInitialization(CB_SERVER_PARAM);
void handleOperation(CB_SERVER_PARAM);
void handleCirrus(CB_SERVER_PARAM);
void Selectchange_cb(CIRRUS_Base &cirrus);
void SecondeChange_cb(void)
{
	if (Cirrus_OK && !Calibration)
		Simple_Get_Data();
}

// ********************************************************************************
// Task functions
// ********************************************************************************

void Display_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("DISPLAY_Task");
	uint8_t line = 0;
	for (EVER)
	{
		if (!Calibration)
		{
			// Cirrus message
			if (Cirrus_OK)
			{
				line = Simple_Update_IHM(RTC_Local.the_time(), "", false);
			}
			else
			{
				line = 0;
				IHM_Clear();
				IHM_Print(line++, RTC_Local.the_time());
				UART_Message = "Cirrus failled";
			}

			// DS18B20 message
			if (DS_Count > 0)
			{
				Temp_str = "DS: " + DS.get_Temperature_Str(0) + "` " + DS.get_Temperature_Str(1) + "` ";
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
		}

		// Test extinction de l'écran
		IHM_CheckTurnOff();
		END_TASK_CODE(IHM_IsDisplayOff());
	}
}
#define DISPLAY_DATA_TASK	{condCreate, "DISPLAY_Task", 4096, 4, 1000, Core0, Display_Task_code}

void UserKeyboardAction(Btn_Action Btn_Clicked, uint32_t count)
{
//	if (Btn_Clicked != Btn_NOP)
//		Serial.println(Btn_Texte[Btn_Clicked]);

	switch (Btn_Clicked)
	{
		case Btn_K1: // Bouton du bas : Affiche IP et reset
		{
			if (count == 1)
			{
				IHM_DisplayOn();
				TaskList.ResumeTask("DISPLAY_Task");
				Extra_str = myServer.IPaddress();
				count_Extra_Display = EXTRA_TIMEOUT;
			}

			// On a appuyé plus de 5 secondes sur le bouton
			if (count == SECOND_TO_DEBOUNCING(5))
			{
				Extra_str = "SSID reset";
				count_Extra_Display = EXTRA_TIMEOUT;
				// Delete SSID file
				DeleteSSID();
			}
			break;
		}

		case Btn_K2: // Bouton du milieu : toggle relais
		{
			if (count == 1)
			{
				IHM_DisplayOn();
				TaskList.ResumeTask("DISPLAY_Task");
//				Relay.setState(0, !Relay.getState(0));
//				if (Relay.getState(0))
//				{
//					digitalWrite(GPIO_RELAY_FACADE, HIGH);
//					Extra_str = "Relais ON";
//				}
//				else
//				{
//					digitalWrite(GPIO_RELAY_FACADE, LOW);
//					Extra_str = "Relais OFF";
//				}
//				count_Extra_Display = EXTRA_TIMEOUT;
			}
			break;
		}

		case Btn_K3: // Bouton du haut : toggle display
		{
			if (count == 1)
			{
				if (IHM_ToggleDisplay())
					TaskList.ResumeTask("DISPLAY_Task");
				else
					TaskList.SuspendTask("DISPLAY_Task");
			}
			break;
		}

		case Btn_NOP:
		{
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
//  ESPinformations();
	Partition_Info();
	Partition_ListDir();
#endif

	// **** 2- initialisation datetime ****
	// Essaye de récupérer la dernière heure, utile pour un reboot
	// Paramètres éventuellement à adapter : AutoSave et SaveDelay (par défaut toutes les 10 s)
	RTC_Local.setupDateTime();
	RTC_Local.setSecondeChangeCallback(SecondeChange_cb); // Cirrus get data toutes les secondes

	// **** 3- Initialisation du display
	if (IHM_Initialization(I2C_ADDRESS, false))
		print_debug(F("Display Ok"));
	IHM_TimeOut_Display(OLED_TIMEOUT);

	// **** 4- initialisation DS18B20 ****
#ifdef USE_DS
	DS_Count = DS.Initialize(DS18B20_PRECISION);
#endif

	// **** 6- initialisation Cirrus ****

	// Initialisation du Cirrus
	// Start the communication
	CS_Com.begin();
	CS_Com.setCirrusChangeCallback(Selectchange_cb);

#if TWO_CIRRUS == true
	// Add Cirrus
	// Cirrus1 = CS5484, Cirrus2 = CS5480
	CS_Com.AddCirrus(&Cirrus1, CIRRUS_CS1_GPIO);
	CS_Com.AddCirrus(&Cirrus2, CIRRUS_CS2_GPIO);

	// Start the Cirrus : Cirrus1 and Cirrus2
	if ((CS_Com.SelectCirrus(0))->begin(CS_BEGIN_PARAM(false)))
	{
		Cirrus_OK = (CS_Com.SelectCirrus(1))->begin(CS_BEGIN_PARAM(true));
	}

	if (Cirrus_OK)
		print_debug(F("CS begin OK"));
	else
		print_debug(F("CS begin ERROR"));

	// Configure first Cirrus : Cirrus1
	CS_Com.SelectCirrus(0);

	// Initialisation Cirrus1 : phase 1, phase 2
	if (Cirrus_OK)
		Cirrus_OK = CIRRUS_Generic_Initialization(Cirrus1, &CS1_Calib, &CS1_Config, false, false, '1');

	// Configure second Cirrus : Cirrus2
	CS_Com.SelectCirrus(1);

	// Initialisation Cirrus2 : phase 3, production
	if (Cirrus_OK)
		Cirrus_OK = CIRRUS_Generic_Initialization(Cirrus2, &CS2_Calib, &CS2_Config, false, false, '2');

	// On revient sur le premier Cirrus : Cirrus1
	CS_Com.SelectCirrus(0);
#else
	// Si on ne sélectionne pas le deuxième (cas ou on a deux Cirrus)
	CS_Com.DisableCirrus(CIRRUS_CS2_GPIO);
	CS_Com.AddCirrus(&Cirrus1, CIRRUS_CS1_GPIO);
	CS_Com.SelectCirrus(0);

	// Start the Cirrus
	Cirrus_OK = Cirrus1.begin(CS_BEGIN_PARAM(true));

	// Initialisation
	if (Cirrus_OK)
		Cirrus_OK = CIRRUS_Generic_Initialization(Cirrus1, &CS1_Calib, &CS1_Config, false, false, '1');

	if (Cirrus_OK)
	{
		// Pour le channel 1, on veut le PF et la puissance apparente
		Cirrus1.GetRMSData(Channel_1)->SetWantData(exd_PF | exd_PApparent);
		// Pour le channel 2, on ne veut pas la temprérature, ni le PF ni la fréquence
		Cirrus1.GetRMSData(Channel_2)->SetTemperature(false); // Normalement déjà  false
	}
#endif

	// Get data sur Cirrus1
	Simple_Set_Cirrus(Cirrus1);

	// **** 8- Initialisation Clavier, relais
#ifdef USE_KEYBOARD
//	Keyboard_Initialize(3, ADC_12bits);
	Keyboard_Initialize(3, interval);
//	IHM_Print0("Test Btn 10s");
//	Btn_Check_Config();
#endif

	// Juste pour éteindre la led orange
	pinMode(GPIO_RELAY_FACADE, OUTPUT);
	digitalWrite(GPIO_RELAY_FACADE, LOW);

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
	TaskList.AddTask(DISPLAY_DATA_TASK);
	TaskList.AddTask(KEYBOARD_DATA_TASK(true));
	TaskList.Create(USE_IDLE_TASK);
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
	vTaskDelay(10);
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
	Server_CommonEvent(default_Events | Ev_ListFile | Ev_ResetESP | Ev_SetTime | Ev_GetTime);

	// Server specific events (voir le javascript)
	server.on("/getUARTData", HTTP_GET, [](CB_SERVER_PARAM)
	{
		pserver->send(200, "text/plain", UART_Message);
	});

	server.on("/initialisation", HTTP_GET, handleInitialization);

//	server.on("/getLastData", HTTP_GET, [](CB_SERVER_PARAM)
//	{
//		pserver->send(200, "text/plain", (String(RTC_Local.the_time()) + '#' + UART_Message));
//	});
	server.on("/getLastData", HTTP_GET, handleLastData);

	server.on("/operation", HTTP_PUT, handleOperation);

	server.on("/getCirrus", HTTP_PUT, handleCirrus);

	server.on("/error", HTTP_GET, [](CB_SERVER_PARAM)
	{
		//pserver->send(200, "text/plain", (""));
		pserver->send(200, "text/plain", (String(RTC_Local.the_time()) + '#' + UART_Message));
	});
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

void handleLastData(CB_SERVER_PARAM)
{
	char buffer[255] = {0};
	strcpy(buffer, RTC_Local.the_time()); // Copie la date
	pserver->send(200, "text/plain", buffer);
}

void handleInitialization(CB_SERVER_PARAM)
{
	String message = "";

	pserver->send(200, "text/plain", message);
}

void handleOperation(CB_SERVER_PARAM)
{
	// Default
	RETURN_BAD_ARGUMENT();

	print_debug("Operation: " + pserver->argName((int) 0) + "=" + pserver->arg((int) 0));

#if defined(OLED_DEFINED)
	if (pserver->hasArg("Toggle_Oled"))
	{
		if (IHM_ToggleDisplay())
			TaskList.ResumeTask("DISPLAY_Task");
		else
			TaskList.SuspendTask("DISPLAY_Task");
	}
#endif

#ifdef USE_KEYBOARD
	if (pserver->hasArg("Toggle_Keyboard"))
	{
		Toggle_Keyboard = !Toggle_Keyboard;
//		Keyboard_Txt = "";
	}
#endif

	pserver->send(204, "text/plain", "");
}

String Handle_Cirrus_Wifi_Request(CS_Common_Request Wifi_Request, char *Request)
{
	if (CS_Com.GetNumberCirrus() == 2)
	{
		if (CS_Com.GetSelectedID() == 0)
			return CS_Com.Handle_Common_Request(Wifi_Request, Request, &CS1_Calib, &CS1_Config);
		else
			return CS_Com.Handle_Common_Request(Wifi_Request, Request, &CS2_Calib, &CS2_Config);
	}
	else
		return CS_Com.Handle_Common_Request(Wifi_Request, Request, &CS1_Calib, &CS1_Config);
}

void Selectchange_cb(CIRRUS_Base &cirrus)
{
	Simple_Set_Cirrus(cirrus);
#ifdef CIRRUS_CALIBRATION
	CS_Calibration.SetCirrus(cirrus);
#endif
}

// ********************************************************************************
// End of file
// ********************************************************************************
