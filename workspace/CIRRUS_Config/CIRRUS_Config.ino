#include "Arduino.h"

// Vérifier que les modifications sont bien prises en compte dans sloeber.ino.cpp

// Pour le display, à définir comme directive dans le projet :
// OLED_SSD1306 pour le SSD1306  // Need the directive SSD1306_RAM_128 or SSD1306_RAM_132
// OLED_SSD1327 pour le SSD1327
// OLED_SH1107 pour le SH1107

#include "Server_utils.h"			  // Some utils functions for the server
#include "RTCLocal.h"					  // A pseudo RTC software library
#include "Partition_utils.h"		// Some utils functions for LittleFS/SPIFFS/FatFS
#include "display.h"					// Display functions
#include "CIRRUS.h"
#include "Simple_Get_Data.h"

// Calibration
#ifdef CIRRUS_CALIBRATION
#include "CIRRUS_Calibration.h"
#endif

/**
 * Define de debug
 * SERIAL_DEBUG	: Message de debug sur le port série (baud 115200)
 * LOG_DEBUG	: Enregistre le debug dans un fichier log
 */
//#define SERIAL_DEBUG
//#define LOG_DEBUG
//#define USE_SAVE_CRASH   // Permet de sauvegarder les données du crash
#include "Debug_utils.h"		// Some utils functions for debug

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
#define SSID_CONNEXION	CONN_INLINE
#define ISSOFTAP	false

// Défini les callback events et les opérations à faire après connexion
void OnAfterConnexion(void);

#if SSID_CONNEXION == CONN_INLINE
#if (ISSOFTAP)
			// Pas de mot de passe
			//  ServerConnexion myServer(ISSOFTAP, "SoftAP", "", OnAfterConnexion);
			//  ServerConnexion myServer(ISSOFTAP, "SoftAP", "", IPAddress(192, 168, 4, 2), IPAddress(192, 168, 4, 1));
		#else
// Définir en dur le SSID et le pwd du réseau
ServerConnexion myServer(ISSOFTAP, "TP-Link_8584", "88222462", OnAfterConnexion);
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

// ********************************************************************************
// Définition Cirrus
// ********************************************************************************

#ifdef CIRRUS_USE_UART
#define CS_BAUD		512000	// 115200	512000
#endif

#ifndef CIRRUS_CS5480
//CIRRUS_Calib_typedef CS_Calib = {0};
//CIRRUS_Config_typedef CS_Config = {0};

// Config du mini
//CIRRUS_Calib_typedef CS_Calib = {260.00, 16.50, 0x3BF336, 0x40FA21, 0x000000, 0x000000, 0x000000};
//CIRRUS_Config_typedef CS_Config = {0xC02000, 0x00EEEB, 0x10020A, 0x000001, 0x800000, 0x000000};
// Config du proto6 maison

//CIRRUS_Calib_typedef CS_Calib = {260.00, 16.50, 0x3BF15A, 0x43E86F, 0x000000, 0x000000, 0x000000};
//CIRRUS_Calib_typedef CS_Calib = {260.00, 16.50, 0x380B99, 0x3FE3E9, 0x084BDE, 0x000000, 0x000000};

CIRRUS_Calib_typedef CS_Calib = {260.00, 16.50, 0x380B99, 0xC8B5CA, 0x1A04A6, 0x000000, 0x000000};
CIRRUS_Config_typedef CS_Config = {0xC02000, 0x00EEEB, 0x10020A, 0x000001, 0x800000, 0x000000};

#else
CIRRUS_Calib_typedef CS_Calib = { 242.00, 53.55, 0x3CC756, 0x40D6B0, 0x7025B9, 0x0, 0x0, 242.00,
		17.00, 0x3CC756, 0x4303EE, 0x8A6100, 0x0, 0x0 };
CIRRUS_Config_typedef CS_Config = { 0xC02000, 0x44E2EB, 0x2AA, 0x731F0, 0x51D67C, 0x0 };

CIRRUS_Calib_typedef CS2_Calib = { 260.00, 16.50, 0x3CC00B, 0x41B8D9, 0x5CF11, 0x9, 0x800009,
		224.50, 60.00, 0x400000, 0x400000, 0x0, 0x0, 0x0 };
CIRRUS_Config_typedef CS2_Config = { 0xC02000, 0x44E2E4, 0x1002AA, 0x931F0, 0x6C77D9, 0x0 };
#endif

#ifdef CIRRUS_USE_UART
#if CIRRUS_UART_HARD == 1
#ifdef SERIAL_DEBUG
#error "SERIAL_DEBUG must be undefined !"
#undef SERIAL_DEBUG	// Ne peut pas debugger en Cirrus hard ou rediriger vers Serial1
#endif
CIRRUS_SERIAL_MODE *csSerial = &Serial;
#else
CIRRUS_SERIAL_MODE *csSerial = new CIRRUS_SERIAL_MODE(D7, D8); // D7=RX, D8=TX  SoftwareSerial(int8_t rxPin, int8_t txPin)
#endif
#endif

CIRRUS_Communication CS_Com = CIRRUS_Communication(csSerial, CIRRUS_RESET_GPIO);
CIRRUS_CS5490 CS5490 = CIRRUS_CS5490(CS_Com);
#ifdef CIRRUS_CALIBRATION
CIRRUS_Calibration CS_Calibration = CIRRUS_Calibration(CS5490);
#endif

bool Cirrus_OK = false;

// ********************************************************************************
// Functions prototype
// ********************************************************************************

// UART message
bool UserAnalyseMessage(void);
void handleOperation(void);
extern void handleCirrus(void);

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
	// Ici, ne fait rien : USE_UART et SERIAL_DEBUG ne sont pas défini
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
	print_debug(getSketchName(__FILE__));

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
//	Partition_Info();
//	Partition_ListDir();
#endif

	// **** 2- initialisation datetime ****
	// Essaye de récupérer la dernière heure, utile pour un reboot
	// Paramètres éventuellement à adapter : AutoSave et SaveDelay (par défaut toutes les 10 s)
	RTC_Local.setupDateTime();

	// **** 3- Initialisation du display
	if (IHM_Initialization(I2C_ADDRESS, false))
		print_debug(F("Display Ok"));
	IHM_TimeOut_Display(OLED_TIMEOUT);
	Display_offset_line = 1;

	// **** 4- initialisation Cirrus ****
	// Initialisation du Cirrus CS5490
	// Start the communication
	CS_Com.begin();

	// Start Cirrus
	Cirrus_OK = CS5490.begin(CS_BAUD, true);

	// Initialisation
	if (Cirrus_OK)
		Cirrus_OK = CIRRUS_Generic_Initialization(CS5490, &CS_Calib, &CS_Config, true, true, '1');

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
}

// The loop function is called in an endless loop
// If empty, about 50000 loops by second
// Les mesures ne sont faites que si au moins une seconde est passée
void loop()
{
	bool uptodate = RTC_Local.update();

	// Toutes les secondes
	if (uptodate)
	{
		if (Cirrus_OK)
		{
			// Toutes les secondes on récupère les données du Cirrus
			Simple_Get_Data();

			Simple_Update_IHM(RTC_Local.the_time, "");
		}
		else
			UART_Message = "Cirrus failled";

		// Test extinction de l'écran
		IHM_CheckTurnOff();
	} // end uptodate

	// Listen for HTTP requests from clients
#ifndef USE_ASYNC_WEBSERVER
	server.handleClient();
#else
	// Temporisation à adapter
	delay(10);
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
	server.on("/getUARTData", HTTP_GET, []()
	{
		server.send(200, "text/plain", UART_Message);
	});

	server.on("/getLastData", HTTP_GET, []()
	{
		server.send(200, "text/plain", (String(RTC_Local.the_time) + '#' + UART_Message));
	});

	server.on("/operation", HTTP_PUT, handleOperation);

	server.on("/getCirrus", HTTP_PUT, handleCirrus);
}

// ********************************************************************************
// Print functions
// ********************************************************************************
void PrintTerminal(const char *text)
{
//  UART_Message = String(text);
//	server.send(200, "text/plain", (String(text)));
	print_debug(String(text));
//	(void) text;
}

// ********************************************************************************
// Web/UART functions
// ********************************************************************************

bool UserAnalyseMessage(void)
{
	// Default
	// Retour à UART
	printf_message_to_UART("Unknown message : " + String((const char*) UART_Message_Buffer));
	// Message pour la page web
	UART_Message = String((const char*) UART_Message_Buffer);
	return false;
}

void handleOperation(void)
{
	// Default
	RETURN_BAD_ARGUMENT();

	print_debug("Operation: " + server.argName(0) + " = " + server.arg(0));

#if defined(OLED_DEFINED)
	if (server.hasArg("Toggle_Oled"))
	{
		IHM_ToggleDisplay();
		IHM_TimeOut_Display(OLED_TIMEOUT);
	}
#endif

	server.send(204, "text/plain", "");
}

String Handle_Wifi_Request(CS_Common_Request Wifi_Request, char *Request)
{
	return CS_Com.Handle_Common_Request(Wifi_Request, Request, &CS_Calib, &CS_Config);
}

// ********************************************************************************
// End of file
// ********************************************************************************
