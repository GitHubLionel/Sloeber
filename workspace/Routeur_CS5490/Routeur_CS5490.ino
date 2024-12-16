#include "Arduino.h"

// Vérifier que les modifications sont bien prises en compte dans sloeber.ino.cpp

// Pour le display, à définir comme directive dans le projet :
// OLED_SSD1306 pour le SSD1306  // Need the directive SSD1306_RAM_128 or SSD1306_RAM_132
// OLED_SSD1327 pour le SSD1327
// OLED_SH1107 pour le SH1107

#include "Server_utils.h"			  // Some utils functions for the server
#include "RTCLocal.h"					  // A pseudo RTC software library
#include "Partition_utils.h"		// Some utils functions for LittleFS/SPIFFS/FatFS
#include "display.h"					  // Display functions
#include "CIRRUS.h"
#include "Get_Data.h"
#include "DS18B20.h"
#include "SSR.h"
#include "Keyboard.h"
#include "Relay.h"

/**
 * Define de debug
 * SERIAL_DEBUG	: Message de debug sur le port série (baud 115200)
 * LOG_DEBUG	: Enregistre le debug dans un fichier log
 */
//#define SERIAL_DEBUG
//#define LOG_DEBUG
//#define USE_SAVE_CRASH   // Permet de sauvegarder les données du crash
#include "Debug_utils.h"		// Some utils functions for debug

// Use DS18B20
#define USE_DS

// Use Teleinfo
//#define USE_TI
#ifdef USE_TI
#include "TeleInfo.h"
#endif

// Active le SSR
#define USE_ZC_SSR

// Active le relais
#define USE_RELAY

// Active le clavier
#define USE_KEYBOARD

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

String Temp_str = "";
String Extra_str = "";
#define EXTRA_TIMEOUT	5  // L'extra texte est affiché durant EXTRA_TIMEOUT secondes
uint8_t count_Extra_Display = 0;

// ********************************************************************************
// Définition DS18B20
// ********************************************************************************

// Address 40 : DS18B20 air
// Address 82 : DS18B20 eau

#ifdef USE_DS
DS18B20 DS(DS18B20_GPIO);  // 2
uint8_t DS_Count = 0;
#endif

// ********************************************************************************
// Définition TeleInfo
// ********************************************************************************

#ifdef USE_TI
TeleInfo TI(TI_RX_GPIO, 5000);
bool TI_OK = false;
#endif

// ********************************************************************************
// Définition Cirrus
// ********************************************************************************

//CIRRUS_Calib_typedef CS_Calib = {260.00, 16.50, 0x3BF15A, 0x43E86F, 0x000000, 0x000000, 0x000000};
//CIRRUS_Calib_typedef CS_Calib = {260.00, 16.50, 0x380B99, 0x3FE3E9, 0x084BDE, 0x000000, 0x000000};
CIRRUS_Calib_typedef CS_Calib = {260.00, 16.50, 0x380B99, 0xC8B5CA, 0x1A04A6, 0x000000, 0x000000};
CIRRUS_Config_typedef CS_Config = {0xC02000, 0x00EEEB, 0x10020A, 0x000001, 0x800000, 0x000000};

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

bool Cirrus_OK = false;

// To access cirrus data
extern volatile Data_Struct Current_Data;
extern volatile Graphe_Data log_cumul;

// ********************************************************************************
// Définition SSR - Relais
// ********************************************************************************

#ifdef USE_RELAY
// Pin commande du relais
Relay_Class Relay({D0});
#endif

// ********************************************************************************
// Définition Clavier
// ********************************************************************************

#ifdef USE_KEYBOARD
Btn_Action Button;
bool Toggle_Keyboard = true;
String Keyboard_Txt = "";
uint32_t Btn_K1_count = 0;
#endif

// ********************************************************************************
// Functions prototype
// ********************************************************************************

// UART message
bool UserAnalyseMessage(void);
void handleInitialization(void);
void handleLastData(void);
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
//  Partition_Info();
//  Partition_ListDir();
#endif

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
	}
	else
		print_debug(F("Configuration TeleInfo ERROR"));
#endif

	// **** 6- initialisation Cirrus ****
	// Initialisation du Cirrus CS5490
	// Start the communication
	CS_Com.begin();

	// Start Cirrus
	Cirrus_OK = CS5490.begin(CIRRUS_UART_BAUD, true);

	// Initialisation
	if (Cirrus_OK)
		Cirrus_OK = CIRRUS_Generic_Initialization(CS5490, &CS_Calib, &CS_Config, false, false, '1');

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
#endif

	// **** 8- Initialisation Clavier, relais
#ifdef USE_KEYBOARD
	Keyboard_Initialize(3);
//  Btn_Check_Config();
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
}

// The loop function is called in an endless loop
// If empty, about 50000 loops by second
// Les mesures ne sont faites que si au moins une seconde est passée
void loop()
{
	bool uptodate = RTC_Local.update();
	uint8_t line = 1;

	// Toutes les 200 ms on récupère les données du Cirrus
	// Met à jour en même temps le délais SSR
#ifdef USE_ZC_SSR
	if (ZC_Top_Xms())
	{
		if (Cirrus_OK)
			Get_Data();
	}
#else
	if (uptodate)
		Simple_Get_Data(); // Utilise Simple_Get_Data.cpp
#endif

	// TeleInfo process à faire régulièrement
#ifdef USE_TI
	if (TI_OK)
	TI.Process();
#endif

	// Clavier à faire régulièrement
#ifdef USE_KEYBOARD
	if (Toggle_Keyboard)
		Keyboard_UpdateTime();
#endif

	// Toutes les secondes
	if (uptodate)
	{
#ifdef USE_KEYBOARD
		// Check du clavier
		if (Toggle_Keyboard)
		{
			if (Check_Keyboard(&Button))
			{
//		  print_debug(Btn_Texte[Button]);
				Keyboard_Txt = Btn_Texte[Button];
				if (Button == Btn_K3) // Bouton du haut
				{
					IHM_ToggleDisplay();
				}

				if (Button == Btn_K2) // Bouton du milieu
				{
					IHM_DisplayOn();
					Relay.setState(0, !Relay.getState(0));
					if (Relay.getState(0))
						Extra_str = "Relais ON";
					else
						Extra_str = "Relais OFF";
					count_Extra_Display = EXTRA_TIMEOUT;
				}

				if (Button == Btn_K1) // Bouton du bas
				{
					IHM_DisplayOn();
					if (Extra_str.isEmpty())
						Extra_str = myServer.IPaddress();
					count_Extra_Display = EXTRA_TIMEOUT;
					Btn_K1_count++;
					// On a appuyé plus de 5 secondes sur le bouton
					if (Btn_K1_count > 4)
					{
						Extra_str = "SSID reset";
						// Delete SSID file
						while (Lock_File)
							;
						FS_Partition->remove(SSID_FILE);
						Btn_K1_count = 0;
					}
				}

				//	  SSD1327_String({0, 20}, Btn_Texte[Button], &Font12, FONT_BACKGROUND, SSD1327_WHITE);
			}
			else
			{
				Keyboard_Txt = "";
				Extra_str = "";
				Btn_K1_count = 0;
			}
		}
#endif

		// Lecture température
#ifdef USE_DS
		if (DS_Count > 0)
			DS.check_dallas();
#endif

		if (Cirrus_OK)
			line = Update_IHM(RTC_Local.the_time(), "", false);
		else
		{
			IHM_Print(line++, RTC_Local.the_time());
			UART_Message = "Cirrus failled";
		}

		Temp_str = "";
#ifdef USE_DS
		if (DS_Count > 0)
		{
			Temp_str = "DS: " + DS.get_Temperature_Str(0) + "` " + DS.get_Temperature_Str(1) + "` ";
			IHM_Print(line++, (const char*) Temp_str.c_str());
		}
#endif
#ifdef USE_TI
		if (TI_OK)
		{
			Temp_str = "TI: " + String(TI.getIndexWh());
			IHM_Print(line++, (const char*) Temp_str.c_str());
		}
#endif

		if (count_Extra_Display != 0)
		{
			IHM_Print(line, Extra_str.c_str());
			count_Extra_Display--;
			if (count_Extra_Display == 0)
				Extra_str = "";
		}
		IHM_Display();

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

void handleInitialization(void)
{
	String message = "";
	if (SSR_Get_Action() == SSR_Action_Percent)
		message = "true#";
	else
		message = "false#";
	message += (String) SSR_Get_Dump_Power() + '#';
	message += (String) SSR_Get_Percent() + '#';
	if (SSR_Get_State())
		message += "ON#";
	else
		message += "OFF#";
	if (Relay.getState(0))
		message += "ON#";
	else
		message += "OFF#";
#ifdef USE_KEYBOARD
	if (Toggle_Keyboard)
		message += "ON";
	else
#endif
		message += "OFF";

	server.send(200, "text/plain", message);
}

void handleLastData(void)
{
	float Energy, Surplus;
	bool graphe;
	String message = String(RTC_Local.the_time());
	message += '#';
	graphe = Get_Last_Data(&Energy, &Surplus);
	message += (String) Current_Data.Cirrus_ch1.ActivePower + '#';
	message += (String) Current_Data.Cirrus_ch1.Voltage + '#';
	message += (String) Energy + '#';
	message += (String) Surplus + '#';
	message += (String) Current_Data.Cirrus_PF + '#';
	message += (String) Current_Data.Cirrus_Temp;
#ifdef USE_DS
	if (DS_Count > 0)
	{
		message += '#' + DS.get_Temperature_Str(0) + '#';
		message += DS.get_Temperature_Str(1);
	}
	else
		message += "#0.0#0.0";
#else
	message += "#0.0#0.0";
#endif

	// On a de nouvelles données pour le graphe
	if (graphe)
	{
		message += '#' + (String) log_cumul.Power;
		message += '#' + (String) log_cumul.Voltage;
		message += '#' + (String) log_cumul.Temp;
	}

	server.send(200, "text/plain", message);
}

void handleOperation(void)
{
	// Default
	RETURN_BAD_ARGUMENT();

	print_debug("Operation: " + server.argName(0) + "=" + server.arg(0));

#if defined(OLED_DEFINED)
	if (server.hasArg("Toggle_Oled"))
	{
		IHM_ToggleDisplay();
	}
#endif

	// Togggle relay
#ifdef USE_RELAY
	if (server.hasArg("Toggle_Relay"))
	{
		Relay.setState(0, !Relay.getState(0));
	}
#endif

#ifdef USE_KEYBOARD
	if (server.hasArg("Toggle_Keyboard"))
	{
		Toggle_Keyboard = !Toggle_Keyboard;
		Keyboard_Txt = "";
	}
#endif

#ifdef USE_ZC_SSR
	// La puissance du CE pour le mode zéro
	if (server.hasArg("CEPower"))
	{
		float power = server.arg("CEPower").toFloat();
		SSR_Set_Dump_Power(power);
	}

	// Change le mode d'action Pourcent/Zéro
	// Ne pas oublier de redémarrer le SSR après
	if (server.hasArg("SSRType"))
	{
		if (server.arg("SSRType") == "true")
			SSR_Action(SSR_Action_Percent);
		else
			SSR_Action(SSR_Action_Surplus);
	}

	// Gestion dimmer en pourcentage
	if (server.hasArg("Dimmer") && (SSR_Get_Action() == SSR_Action_Percent))
	{
		float percent = server.arg("Dimmer").toFloat();
		SSR_Set_Percent(percent);
//		SSR_Set_Dimme_Target(percent * 10);
	}

	// Allume ou éteint le SSR
	if (server.hasArg("Toggle_SSR"))
	{
		if (SSR_Get_State())
		{
			SSR_Disable();
		}
		else
			SSR_Enable();
	}
#endif

	server.send(204, "text/plain", "");
}

String Handle_Cirrus_Wifi_Request(CS_Common_Request Wifi_Request, char *Request)
{
	return CS_Com.Handle_Common_Request(Wifi_Request, Request, &CS_Calib, &CS_Config);
}

// ********************************************************************************
// End of file
// ********************************************************************************
