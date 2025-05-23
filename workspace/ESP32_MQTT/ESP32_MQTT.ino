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

#include "Tasks_utils.h"

// A cause de l'erreur : Brownout detector was triggered
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "MQTT_utils.h"

//USER CODE BEGIN Includes

//USER CODE END Includes

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
#ifndef I2C_ADDRESS
#define I2C_ADDRESS	0x3C // ou 0x78
#endif
String UART_Message = "";

// MQTT Broker
MQTTClient MQTTTest;
// NOTE : SUPPRESS THE DUPLICATED DECLARATION IN sloeber.ino.cpp !
WRAPPER_STATIC_TASK(MQTTTest)

MQTT_Credential_t mqtt = {"", "", "192.168.1.103", 1883};
String topic = "ESP32_MQTT";
bool MQTT_Connected = false;

// Utilise keep alive task for loop
#define USE_TASK_FOR_LOOP

// Ajoute la task idle et publie les mesures idle
#define RUN_TASK_IDLE	true
void Task_ShowIdle(void *parameter)
{
	BEGIN_TASK_CODE("ShowIdle");
	for (EVER)
	{
		MQTTTest.Publish("Idle_Task", TaskList.GetIdleStr().c_str());
		END_TASK_CODE(false);
	}
}

// Pour la reconnexion Wifi (keep alive task)
void Wifi_Connexion(void)
{
	myServer.Connexion(false);
}

//USER CODE BEGIN Privates

//USER CODE END Privates

/**
 * Handle the request from web for MQTT server
 */
void handleMQTTRequest(CB_SERVER_PARAM);

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
	Partition_ListDir();
#endif

	// **** 2- initialisation datetime ****
	// Essaye de récupérer la dernière heure, utile pour un reboot
	// Paramètres éventuellement à adapter : AutoSave et SaveDelay (par défaut toutes les 10 s)
	RTC_Local.setupDateTime();

	// **** 3- Initialisation du display
	if (IHM_Initialization(I2C_ADDRESS, false))
		print_debug(F("Display Ok"));
	IHM_TimeOut_Display(OLED_TIMEOUT);

	// A cause de l'erreur : Brownout detector was triggered
//	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

	//USER CODE BEGIN Initialization

	//USER CODE END Initialization

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

	// **** FIN- Attente connexion réseau
	print_debug("*** Setup time : " + String(millis() - start_time) + " ms ***\r\n");

	// IDLE Task
	TaskList.AddTask( {condNotCreate, "ShowIdle", 5000, 5, 5000, CoreAny, Task_ShowIdle});
	// MQTT Keep alive task
#ifdef USE_TASK_FOR_LOOP
	TaskList.AddTask(MQTT_STANDARD_KEEPALIVE_TASK_DATA(condNotCreate));
#endif
	TaskList.AddTask(MQTT_STANDARD_PARSEMESSAGE_TASK_DATA(condNotCreate));
	TaskList.Create(RUN_TASK_IDLE);
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
		if (! BasicAnalyseMessage())
			UserAnalyseMessage();
	}

	// Opération à faire si on est à l'heure : par exemple afficher l'heure
	if (uptodate)
	{
		IHM_Print0(RTC_Local.the_time());

		// Afficher Idle
		IHM_Print(3, TaskList.GetIdleStr().c_str(), true);

		// Test extinction de l'écran
		IHM_CheckTurnOff();

//		print_debug(Idle_str);
	}

	// Listen for HTTP requests from clients
#ifndef USE_ASYNC_WEBSERVER
	server.handleClient();
#endif

	// MQTT
#ifndef USE_TASK_FOR_LOOP
	MQTTTest.Loop();
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
	Server_CommonEvent(default_Events | Ev_ListFile | Ev_ResetESP | Ev_SetTime | Ev_GetTime);

	// Server specific events (voir le javascript)
	server.on("/getUARTData", HTTP_GET, [](CB_SERVER_PARAM)
	{
		pserver->send(200, "text/plain", UART_Message);
	});

	server.on("/getLastData", HTTP_GET, [](CB_SERVER_PARAM)
	{
		pserver->send(200, "text/plain", (String(RTC_Local.the_time()) + '#' + UART_Message));
	});

	server.on("/operation", HTTP_PUT, handleOperation);
	server.on("/operation", HTTP_GET, handleOperation);

	// MQTT
	server.on("/MQTT_Request", HTTP_POST, handleMQTTRequest);
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
	RETURN_BAD_ARGUMENT();

	print_debug("Operation: " + pserver->arg((int) 0) + "=" +  pserver->arg((int) 0));

#if defined(OLED_DEFINED)
	if (pserver->hasArg("Toggle_Oled"))
	{
		IHM_ToggleDisplay();
	}
#endif

	pserver->send(204, "text/plain", "");
}

void handleMQTTRequest(CB_SERVER_PARAM)
{
	RETURN_BAD_ARGUMENT();

	// Return si Request n'est pas présent
	if (!pserver->hasArg("Request"))
		return;

	String request = pserver->arg("Request");

	// Eviter de spammer le log
	if (!(request.equals("Data")))
		print_debug("---> Request for MQTT broker: ");

	// Connexion request
	if (request.equals("Connexion"))
	{
		print_debug("Connect MQTT IP: " + pserver->arg("IP"));
		mqtt.broker = pserver->arg("IP");
		if (MQTT_Connected)
		{
			// On est déjà connecté
			pserver->send(200, "text/plain", "Connected");
		}
		else
		{
			MQTT_Connected = MQTTTest.begin(mqtt);
			if (MQTT_Connected)
			{
				TaskList.CreateTask(MQTTTest.GetParseMessageTaskName());
				MQTTTest.Publish(topic, "Hi, I'm ESP32", true);
				// Exemple, on crée une task
#if RUN_TASK_IDLE == true
				TaskList.CreateTask("ShowIdle");
#endif
#ifdef USE_TASK_FOR_LOOP
				TaskList.CreateTask(MQTTTest.GetKeepaliveTaskName());
#endif
				pserver->send(200, "text/plain", "Connected");
			}
			else
				pserver->send(200, "text/plain", "Erreur");
		}
		return;
	}

	// Publish request
	if (request.equals("Publish"))
	{
		if (pserver->hasArg("Topic"))
			topic = pserver->arg("Topic");
		String Message = "";
		if (pserver->hasArg("Message"))
			Message = pserver->arg("Message");
		bool subscribe = false;
		if (pserver->hasArg("Subscribe"))
			subscribe = (pserver->arg("Subscribe")).equals("true");
		print_debug("Topic: " + topic + " Message: " + Message);

		if (MQTT_Connected)
		{
			MQTTTest.Publish(topic, Message, subscribe);
		}

		pserver->send(204, "text/plain", "");
		return;
	}

	// Subscribe request
	if (request.equals("Subscribe"))
	{
		String newtopic = "";
		print_debug("Subsribe to MQTT: ", false);
		if (pserver->hasArg("Topic"))
			newtopic = pserver->arg("Topic");

		print_debug("Topic: " + newtopic);

		if (MQTT_Connected && !newtopic.isEmpty())
		{
			MQTTTest.Subscribe(newtopic, true);
		}

		pserver->send(204, "text/plain", "");
		return;
	}

	// UnSubscribe request
	if (request.equals("UnSubscribe"))
	{
		String newtopic = "";
		print_debug("UnSubsribe to MQTT: ", false);
		if (pserver->hasArg("Topic"))
			newtopic = pserver->arg("Topic");

		print_debug("Topic: " + newtopic);

		if (MQTT_Connected && !newtopic.isEmpty())
		{
			MQTTTest.Subscribe(newtopic, false);
		}

		pserver->send(204, "text/plain", "");
		return;
	}

	// Data request
	if (request.equals("Data"))
	{
		if (MQTT_Connected)
		{
			String data = String(MQTTTest.GetMessage().topic) + ": " + String(MQTTTest.GetMessage().payload);
			pserver->send(200, "text/plain", data.c_str());
		}
		else
			pserver->send(200, "text/plain", "Not connected");
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
