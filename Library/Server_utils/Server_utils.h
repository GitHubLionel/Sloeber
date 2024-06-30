#pragma once

/**
 * A list of basic functions used with web server
 * This unit create the ESP8266 or ESP32 WebServer : server
 * An exported variable Lock_File is defined. Put this variable to true when you access a file.
 * The function CheckUARTMessage() can be used to get message from UART. The message is stored in UART_Message_Buffer
 * CheckUARTMessage() need to be placed in the main loop
 * Define USE_RTCLocal for function with time
 * Define USE_HTTPUPDATER to add firmware upload function
 * Define USE_ASYNC_WEBSERVER for an asynchrone WebServer
 *
 * If yon don't want gz file, uncomment USE_ZG_FILE
 */

// To allow HTTP updater
//#define USE_HTTPUPDATER

#include "Arduino.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>	// Include WiFi library
#include <ESP8266WebServer.h>	// Include WebServer library
#ifdef USE_HTTPUPDATER
#include <ESP8266HTTPUpdateServer.h>
#endif
#endif

#ifdef ESP32
#include <WiFi.h>	// Include WiFi library
#ifdef USE_ASYNC_WEBSERVER
	#include <ESPAsyncWebServer.h>
	#define CB_SERVER_PARAM	AsyncWebServerRequest *pserver
	#define SERVER_PARAM	pserver
#else
	#include <WebServer.h>	// Include WebServer library
	#define CB_SERVER_PARAM void
	#define SERVER_PARAM
#endif

#ifdef USE_HTTPUPDATER
	#ifdef USE_ASYNC_WEBSERVER
	#include <ESPAsyncHTTPUpdateServer.h>
	#else
	#include <HTTPUpdateServer.h>
	#endif
#endif
#endif // ESP32
#include <Preferences.h> // For EEPROM access

// If we have an RTC_Local instance of RTCLocal
#define USE_RTCLocal

// If we want to use gz file if available
#define USE_ZG_FILE

// The port for the server
#define SERVER_PORT	80

// The web server
#ifdef ESP8266
extern ESP8266WebServer server;
#ifdef USE_HTTPUPDATER
// Set to true to send debug to Serial
#define UPDATER_DEBUG	false
extern ESP8266HTTPUpdateServer httpUpdater;
#endif
#endif

#ifdef ESP32
#ifdef USE_ASYNC_WEBSERVER
extern AsyncWebServer server;
#else
extern WebServer server;
extern WebServer *pserver;
#endif
#ifdef USE_HTTPUPDATER
// Set to true to send debug to Serial
#define UPDATER_DEBUG	false
//extern HTTPUpdateServer httpUpdater;
#endif
#endif

// Default SSID file name
#define SSID_FILE	"/SSID.txt"

// Default name for SoftAP connexion
#define DEFAULT_SOFTAP	"DefaultAP"

// For CheckUARTMessage
// The char that begin a message
#define BEGIN_DATA	0x02
// The char that end a message
#define END_DATA	0x03
// The size of the buffer for the message received
#define BUFFER_SIZE	1024
// The buffer that contain the message received
extern volatile char UART_Message_Buffer[];
// End Text string
#define LOG_ETX_STR  	"\03"

// For StreamUARTMessage
#define STREAM_BUFFER_SIZE	1024
// Default TimeOut for the stream
#define STREAM_TIMEOUT		2000

/**
 * List of connexion
 */
#define CONN_INLINE	0
#define CONN_UART		1
#define CONN_FILE		2
#define CONN_EEPROM		3
typedef enum
{
	Conn_Inline = CONN_INLINE,
	Conn_UART = CONN_UART,
	Conn_File = CONN_FILE,
	Conn_EEPROM = CONN_EEPROM
} Conn_typedef;

/**
 * The list of events available
 */
typedef enum
{
	Ev_Null = 0,
	Ev_LoadPage = 1,
	Ev_GetFile = 2,
	Ev_DeleteFile = 4,
	Ev_UploadFile = 8,
	Ev_ListFile = 16,
	Ev_CreateFile = 32,
	Ev_ResetESP = 64,
	Ev_SetTime = 128,
	Ev_GetTime = 256,
	Ev_SetDHCPIP = 512,
	Ev_ResetDHCPIP = 1024
} Event_typedef;

// Connexion Event callback
typedef void (*onConnexionEvent)();
typedef void (*onTimeOut)();

class ServerSettings
{
	public:
		ServerSettings() :
				_IsSoftAP(true), _SSID(""), _PWD(""), _onAfterConnexion(NULL)
		{
		}
		ServerSettings(bool IsSoftAP, const String ssid, const String pwd,
				const onConnexionEvent &afterConnexion_cb = NULL) :
				_IsSoftAP(IsSoftAP), _SSID(ssid), _PWD(pwd), _onAfterConnexion(afterConnexion_cb)
		{
		}

		bool _IsSoftAP;
		String _SSID;
		String _PWD;
		IPAddress _ip;
		IPAddress _gateway;
		onConnexionEvent _onBeforeConnexion = NULL;
		onConnexionEvent _onAfterConnexion;

		void setOnBeforeConnexion(const onConnexionEvent &callback)
		{
			_onBeforeConnexion = callback;
		}

		void setOnAfterConnexion(const onConnexionEvent &callback)
		{
			_onAfterConnexion = callback;
		}
};

class ServerConnexion
{
	private:
		bool _IsSoftAP = true;
		String _SSID = "";
		String _PWD = "";
		IPAddress _ip;
		IPAddress _gateway;
		IPAddress subnet = IPAddress(255, 255, 255, 0);
		String _IPaddress = "";
		int8_t _Wifi_Signal = 0;
		// Boolean to wait for network
		bool WaitForNetWork = true;
		String SSID_File = SSID_FILE;
		onConnexionEvent _onBeforeConnexion = NULL;
		onConnexionEvent _onAfterConnexion = NULL;
		bool DefaultAPConnexion();
	public:
		// Constructeur
		ServerConnexion()
		{
		}
		ServerConnexion(bool IsSoftAP, const onConnexionEvent &afterConnexion_cb = NULL)
		{
			_IsSoftAP = IsSoftAP;
			_onAfterConnexion = afterConnexion_cb;
		}
		ServerConnexion(bool IsSoftAP, const String &ssid, const String &pwd,
				const onConnexionEvent &afterConnexion_cb = NULL)
		{
			_IsSoftAP = IsSoftAP;
			_onAfterConnexion = afterConnexion_cb;
			setSSID_PWD(ssid, pwd);
		}
		ServerConnexion(bool IsSoftAP, const String &ssid, const String &pwd, const IPAddress &ip,
				const IPAddress &gateway, const onConnexionEvent &afterConnexion_cb = NULL)
		{
			_IsSoftAP = IsSoftAP;
			_onAfterConnexion = afterConnexion_cb;
			setSSID_PWD(ssid, pwd);
			_ip = ip;
			_gateway = gateway;
		}

		~ServerConnexion();

		void setOnBeforeConnexion(const onConnexionEvent &callback)
		{
			_onBeforeConnexion = callback;
		}

		void setOnAfterConnexion(const onConnexionEvent &callback)
		{
			_onAfterConnexion = callback;
		}

		void begin(ServerSettings &settings);

		bool WaitForConnexion(Conn_typedef connexion, bool toUART = false);

		void setSubnet(const IPAddress &_subnet)
		{
			subnet = _subnet;
		}

		void setSSID_PWD(const String &ssid, const String &pwd)
		{
			_SSID = ssid;
			_PWD = pwd;
		}

		bool SSIDFromFile(const String &filename);
		bool SSIDFromEEPROM();

		bool Connexion(bool toUART = false);
		bool IsConnected(void) const
		{
			return (!WaitForNetWork);
		}
		bool ExtractSSID_Password(void);
		bool BasicAnalyseMessage(void);
		String IPaddress() const
		{
			return _IPaddress;
		}
		bool ISSoftAP()
		{
			return _IsSoftAP;
		}
		long Wifi_Signal() const
		{
			return _Wifi_Signal;
		}

		String getGateway() const;
		String getMAC() const;

		String getSSID_FileName(void) const
		{
			return SSID_File;
		}
		void setSSID_FileName(const String &name);
};

void Server_CommonEvent(uint16_t event);
void Auto_Reset(void);
void SSIDToFile(const String &filename, const String &ssidpwd);
void SSIDToEEPROM(const String &ssid, const String &pwd);
void DeleteSSID(void);
const String getContentType(const String &filename);

// UART functions
bool CheckUARTMessage(HardwareSerial *Serial_Message = &Serial);
void StreamUARTMessage(const String &ContentType, const onTimeOut &TimeOut_cb = NULL,
		uint32_t timeout = STREAM_TIMEOUT, HardwareSerial *Serial_Message = &Serial);
void printf_message_to_UART(const char *mess, bool balise = true, HardwareSerial *Serial_Message = &Serial);
void printf_message_to_UART(const String &mess, bool balise = true, HardwareSerial *Serial_Message = &Serial);
void SendDefaultXML(void);

char* Search_Balise(uint8_t *data, const char *B_Begin, const char *B_end, char *value,
		uint16_t *len);

