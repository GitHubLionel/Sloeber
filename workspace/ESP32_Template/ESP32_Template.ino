#include "Arduino.h"

// Vérifier que les modifications sont bien prises en compte dans sloeber.ino.cpp

// Pour le display, à définir comme directive dans le projet :
// OLED_SSD1306 pour le SSD1306  // Need the directive SSD1306_RAM_128 or SSD1306_RAM_132
// OLED_SSD1327 pour le SSD1327
// OLED_SH1107 pour le SH1107

#include "Server_utils.h"			  // Some utils functions for the server
#include "RTCLocal.h"					  // A pseudo RTC software library
#include "Partition_utils.h"		// Some utils functions for LittleFS/SPIFFS/FatFS
#include "Display.h"            // Display function

/**
 * Define de debug
 * SERIAL_DEBUG	: Message de debug sur le port série (baud 115200)
 * LOG_DEBUG	: Enregistre le debug dans un fichier log
 */
//#define SERIAL_DEBUG
//#define LOG_DEBUG
//#define USE_SAVE_CRASH   // Permet de sauvegarder les données du crash
#include "Debug_utils.h"		// Some utils functions for debug

// A cause de l'erreur : Brownout detector was triggered
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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
#define UART_BAUD	115200

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
	ServerConnexion myServer(ISSOFTAP, "SoftAP", "", OnAfterConnexion);
//	ServerConnexion myServer(ISSOFTAP, "SoftAP", "", IPAddress(192, 168, 4, 2), IPAddress(192, 168, 4, 1));
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
#define I2C_ADDRESS	0x3C // ou 0x78
String UART_Message = "";
#if defined(OLED_DEFINED)
// Delay d'extinction du Oled en seconde
#define OLED_TIMEOUT	600	// 10 minutes
#endif

// ********************************************************************************
// Functions prototype
// ********************************************************************************

// UART message
bool UserAnalyseMessage(void);
void handleOperation(CB_SERVER_PARAM);

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

	// Small delay to stabilise
	delay(1000);

	// **** 0- initialisation Serial, debug ou pas ****
#if (defined(SERIAL_DEBUG) || defined(USE_UART))
	// On démarre le port série : NB_BIT = 8, PARITY NONE, NB_STOP_BIT = 1
	Serial.begin(UART_BAUD);
	delay(100);  // Pour stabiliser UART
#endif
#ifdef SERIAL_DEBUG
	// On attend 5 secondes pour stabiliser l'alimentation et pour lancer la console UART (debug)
	delay(WAIT_SETUP);
#endif

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
	Partition_ListDir();
#endif

	// **** 2- initialisation datetime ****
	// Essaye de récupérer la dernière heure, utile pour un reboot
	// Paramètres éventuellement à adapter : AutoSave et SaveDelay (par défaut toutes les 10 s)
	RTC_Local.setupDateTime();

	// **** 3- Initialisation du display
	if (IHM_Initialization(I2C_ADDRESS, true))
		print_debug(F("Display Ok"));

	// **** FIN- Attente connexion réseau

	// A cause de l'erreur : Brownout detector was triggered
//	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

	IHM_Print0("Connexion .....");
	print_debug(F("==> Wait for network <=="));
	if (!myServer.WaitForConnexion((Conn_typedef)SSID_CONNEXION))
	  print_debug(F("==> Connected to network <=="));
	else
	{
		IHM_Print0("Failed !   ");
		print_debug(F("==> Connexion to network failed <=="));
	}

	print_debug("*** Setup time : " + String(millis() - start_time) + " ms ***\r\n");
}

// The loop function is called in an endless loop
// If empty, about 50000 loops by second
// Les mesures ne sont faites que si au moins une seconde est passée
void loop()
{
	bool uptodate = RTC_Local.update();

	// Exemple analyse d'un message UART
	if (CheckUARTMessage())
	{
		if (! myServer.BasicAnalyseMessage())
			UserAnalyseMessage();
	}

	// Opération à faire si on est à l'heure : par exemple afficher l'heure
	if (uptodate)
	{
		IHM_Print0(RTC_Local.the_time);
		// Test extinction de l'écran
		IHM_TurnOff();
	}

	// Listen for HTTP requests from clients
#ifndef USE_ASYNC_WEBSERVER
	server.handleClient();
#endif

	// Temporisation à adapter
	delay(100);
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
	Server_CommonEvent(Ev_LoadPage | Ev_GetFile | Ev_DeleteFile | Ev_UploadFile |
			Ev_ListFile | Ev_ResetESP | Ev_SetTime | Ev_GetTime | Ev_SetDHCPIP | Ev_ResetDHCPIP);

	// Server specific events (voir le javascript)
	server.on("/getUARTData", HTTP_GET, [](CB_SERVER_PARAM)
	{
		pserver->send(200, "text/plain", UART_Message);
	});

	server.on("/getLastData", HTTP_GET, [](CB_SERVER_PARAM)
	{
		pserver->send(200, "text/plain", (String(RTC_Local.the_time) + '#' + UART_Message));
	});

	server.on("/operation", HTTP_PUT, handleOperation);
	server.on("/operation", HTTP_GET, handleOperation);

	// Begin the server
	server.begin();

	// Affichage ip adresse
	IHM_IPAddress(myServer.IPaddress().c_str());
}

// ********************************************************************************
// Web/UART functions
// ********************************************************************************

bool UserAnalyseMessage(void)
{
	// Default
	// Retour à UART
	printf_message_to_UART("Unknown message : " + String((const char *)UART_Message_Buffer));
	// Message pour la page web
	UART_Message = String((const char *)UART_Message_Buffer);
	return false;
}

void handleOperation(CB_SERVER_PARAM)
{
	// Default
	if (pserver->args() == 0)
		return pserver->send(500, "text/plain", "BAD ARGS");

	print_debug("Operation: " + pserver->arg((int) 0) + "=" +  pserver->arg((int) 0));

#if defined(OLED_DEFINED)
	if (pserver->hasArg("Toggle_Oled"))
	{
		IHM_ToggleDisplay();
		IHM_TimeOut_Display(OLED_TIMEOUT);
	}
#endif

	pserver->send(204, "text/plain", "");
}

// ********************************************************************************
// End of file
// ********************************************************************************
