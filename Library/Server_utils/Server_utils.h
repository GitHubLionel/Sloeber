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
 * If you don't want gz file, uncomment USE_ZG_FILE
 *
 * Library list:
 * ESP8266:
 * - ESP8266WiFi
 * - ESP8266WebServer or ESP_Async_WebServer + ESPAsyncTCP for Async WebServer
 * - ESP8266HTTPUpdateServer or ESPAsyncHTTPUpdateServer for Async WebServer
 *
 * ESP32:
 * - WiFi
 * - WebServer or ESP_Async_WebServer + Async_TCP for Async WebServer
 * - ESPHTTPUpdateServer or ESPAsyncHTTPUpdateServer for Async WebServer
 */

// If we have an RTC_Local instance of RTCLocal
//#define USE_RTCLocal

// If we want to use gz file if available
//#define USE_GZ_FILE

// To allow HTTP updater
//#define USE_HTTPUPDATER

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"

// Include WiFi library
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else // ESP32
#include <WiFi.h>
#endif

// Include (Async)WebServer library
#ifdef USE_ASYNC_WEBSERVER
	#include <ESPAsyncWebServer.h>
  #define SERVER_CLASSNAME AsyncWebServer
	#define CB_SERVER_PARAM	AsyncWebServerRequest *pserver
	#define SERVER_PARAM	pserver
#else
	#ifdef ESP8266
		#include <ESP8266WebServer.h>
    #define SERVER_CLASSNAME ESP8266WebServer
	#else // ESP32
		#include <WebServer.h>
    #define SERVER_CLASSNAME WebServer
	#endif
	#define CB_SERVER_PARAM void
	#define SERVER_PARAM
#endif

// HTTP Updater, OTA library
#ifdef USE_HTTPUPDATER
	#ifdef USE_ELEGANT_OTA
		#warning "See corrections to apply : https://github.com/ayushsharma82/ElegantOTA/issues/254"
		#include <ElegantOTA.h>
	#else
		#ifdef USE_ASYNC_WEBSERVER
			#ifdef USE_FATFS
				#warning "HTTPUpdateServer not available"
			#else
				#ifndef USE_SPIFFS
					#ifndef ESPASYNCHTTPUPDATESERVER_LITTLEFS
					  #error "ESPASYNCHTTPUPDATESERVER_LITTLEFS must be defined."
					#endif
				#endif
      #endif
			#include <ESPAsyncHTTPUpdateServer.h>
      #define UPDATER_CLASSNAME ESPAsyncHTTPUpdateServer
		#else // not async
			#ifdef ESP8266
				#include <ESP8266HTTPUpdateServer.h>
        #define UPDATER_CLASSNAME ESP8266HTTPUpdateServer
			#else // ESP32
				#include <HTTPUpdateServer.h>
        #define UPDATER_CLASSNAME HTTPUpdateServer
			#endif
      // Set to true to send debug to Serial
			#define UPDATER_DEBUG false
		#endif
	#endif
#endif

#ifdef USE_MDNS
#include <ESPmDNS.h>
#endif

#include <Preferences.h> // For EEPROM access

// The port for the server, default 80
#ifndef SERVER_PORT
#define SERVER_PORT	80
#endif

// Default SSID file name
#ifndef SSID_FILE
#define SSID_FILE	"/SSID.txt"
#endif

// Default name for SoftAP connexion
#ifndef DEFAULT_SOFTAP
#define DEFAULT_SOFTAP	"DefaultAP"
#endif

// For StreamUARTMessage
#define STREAM_BUFFER_SIZE	1024
// Default TimeOut for the stream
#define STREAM_TIMEOUT		2000

// Extern web server access
extern SERVER_CLASSNAME server;
extern SERVER_CLASSNAME *pserver;

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
	Ev_SetSSID = 512,
	Ev_ResetSSID = 1024,
	Ev_SetDHCP = 2048,
	Ev_GetMACAddress = 4096,
	Ev_GetESPMACAddress = 8192
} Event_typedef;

// Default events
const uint32_t default_Events = Ev_LoadPage | Ev_GetFile | Ev_DeleteFile | Ev_UploadFile |
		Ev_SetSSID | Ev_ResetSSID | Ev_SetDHCP;

// Connexion Event callback
typedef void (*onConnexionEvent)();
typedef void (*onTimeOut)();

// Typical bad argument response
#ifdef USE_ASYNC_WEBSERVER
#define	RETURN_BAD_ARGUMENT()	if (pserver->args() == 0) \
		return pserver->send(500, "text/plain", "BAD ARGS");
#else
#define	RETURN_BAD_ARGUMENT()	if (server.args() == 0) \
		return server.send(500, "text/plain", "BAD ARGS");
#endif

/**
 * Task to keep alive the connexion
 * Check the connexion every minute and try to reconnect if connexion is lost
 */
#ifdef KEEP_ALIVE_USE_TASK
#define KEEP_ALIVE_DATA_TASK	{condCreate, "KEEP_ALIVE_Task", 8192, 5, 60000, CoreAny, KEEP_ALIVE_Task_code}
void KEEP_ALIVE_Task_code(void *parameter);
#else
#define KEEP_ALIVE_DATA_TASK	{}
#endif

/**
 * Server settings class
 * Manage the params of a connexion:
 * - SSID and password
 * - IP address
 * - callback after and before connexion
 */
class ServerSettings
{
	public:
		ServerSettings() :
				IsSoftAP(true), SSID(""), PWD(""), _onAfterConnexion(NULL)
		{
		}
		ServerSettings(bool IsSoftAP, const String ssid, const String pwd, const onConnexionEvent &afterConnexion_cb = NULL) :
				IsSoftAP(IsSoftAP), SSID(ssid), PWD(pwd), _onAfterConnexion(afterConnexion_cb)
		{
		}

		bool IsSoftAP;
		String SSID;
		String PWD;
		IPAddress ip = INADDR_NONE;
		IPAddress gateway = INADDR_NONE;

		void setOnBeforeConnexion(const onConnexionEvent &callback)
		{
			_onBeforeConnexion = callback;
		}

		onConnexionEvent getOnBeforeConnexion(void) const
		{
			return _onBeforeConnexion;
		}

		void setOnAfterConnexion(const onConnexionEvent &callback)
		{
			_onAfterConnexion = callback;
		}

		onConnexionEvent getOnAfterConnexion(void) const
		{
			return _onAfterConnexion;
		}

	private:
		onConnexionEvent _onBeforeConnexion = NULL;
		onConnexionEvent _onAfterConnexion;
};

class ServerConnexion
{
	private:
		bool _IsSoftAP = true;
		bool _useDHCP = true;
		String _SSID = "";
		String _PWD = "";
		String _mDNSHostName = "ESP_DNS";
		IPAddress _ip = INADDR_NONE;
		IPAddress _gateway = INADDR_NONE;
		// subnet mask
		IPAddress _subnet = IPAddress(255, 255, 255, 0);
		// dns1. By default is equal to gateway
		IPAddress _dns1 = INADDR_NONE;
		IPAddress _dns2 = IPAddress(255, 255, 255, 255);
		String _IPaddress = "";
		int8_t _Wifi_Signal = 0;
		// Boolean to wait for network
		bool WaitForNetWork = true;
		bool FirstConnexion = true;
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
		ServerConnexion(bool IsSoftAP, const String &ssid, const String &pwd, const onConnexionEvent &afterConnexion_cb =
				NULL) :
				ServerConnexion(IsSoftAP, afterConnexion_cb)
		{
			setSSID_PWD(ssid, pwd);
		}
		ServerConnexion(bool IsSoftAP, const String &ssid, const String &pwd, const IPAddress &ip, const IPAddress &gateway,
				const onConnexionEvent &afterConnexion_cb = NULL) :
				ServerConnexion(IsSoftAP, ssid, pwd, afterConnexion_cb)
		{
			setIPAddress(ip, gateway);
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

		void begin(const ServerSettings &settings);

		bool WaitForConnexion(Conn_typedef connexion, bool toUART = false);

		void setIPAddress(const IPAddress &ip)
		{
			_ip = ip;
			// Create gateway from ip address
			_gateway = IPAddress(_ip[0], _ip[1], _ip[2], 1);
			_dns1 = _gateway;
			_useDHCP = (_ip == INADDR_NONE);
		}

		void setIPAddress(const IPAddress &ip, const IPAddress &gateway)
		{
			_ip = ip;
			_gateway = gateway;
			_dns1 = _gateway;
			_useDHCP = (_ip == INADDR_NONE);
		}

		/**
		 * Set subnet mask. Default is 255.255.255.0
		 */
		void setSubnet(const IPAddress &subnet)
		{
			_subnet = subnet;
		}

		void setSSID_PWD(const String &ssid, const String &pwd)
		{
			_SSID = ssid;
			_PWD = pwd;
		}

		void setMDNSHostName(const String &name)
		{
			_mDNSHostName = name;
		}

		bool SSIDFromFile(const String &filename);
		bool SSIDFromEEPROM();
		bool DHCPFromEEPROM();

		bool Connexion(bool toUART = false);
		bool IsConnected(void) const
		{
			return (!WaitForNetWork);
		}
		bool WifiConnected(void);
		void KeepAlive(void);

		bool ExtractSSID_Password(void);
		String IPaddress() const
		{
			return _IPaddress;
		}
		bool ISSoftAP() const
		{
			return _IsSoftAP;
		}
		long Wifi_Signal() const
		{
			return _Wifi_Signal;
		}
		String getCurrentRSSI(void) const;

		String getDHCP() const;
		String getGateway() const;
		String getMAC() const;

		String getSSID_FileName(void) const
		{
			return SSID_File;
		}
		void setSSID_FileName(const String &name);

// UART functions
		void StreamUARTMessage(const String &ContentType, const onTimeOut &TimeOut_cb = NULL, uint32_t timeout =
				STREAM_TIMEOUT, HardwareSerial *Serial_Message = &Serial);

#ifdef USE_ASYNC_WEBSERVER
		void SendDefaultXML(AsyncWebServerRequest *request);
#else
		void SendDefaultXML(void);
#endif
};

const String GetIPaddress(void);
const String GetMACaddress(void);
bool getESPMacAddress(String &mac);

void Server_CommonEvent(uint32_t event = default_Events);
void SSIDToFile(const String &filename, const String &ssidpwd);
void SSIDToEEPROM(const String &ssid, const String &pwd);
void DeleteSSID(void);
const String getContentType(const String &filename);

// Only for ElegantOTA
void ElegantOTAloop(void);

