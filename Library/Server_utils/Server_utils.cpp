#include "server_utils.h"
#include <FS.h>               // For File
#include "Partition_utils.h"	// Some utils functions for LittleFS/SPIFFS/FatFS
#if defined(USE_RTCLocal)
#include "RTCLocal.h"		      // A pseudo RTC software library
#endif
#include "Debug_utils.h"		  // Some utils functions for debug

#ifdef KEEP_ALIVE_USE_TASK
#include "Tasks_utils.h"
#endif

/**
 * Set web server port number to PORT
 */
#ifdef ESP8266
ESP8266WebServer server(SERVER_PORT);
ESP8266WebServer *pserver = &server;
#endif
#ifdef ESP32
#ifdef USE_ASYNC_WEBSERVER
AsyncWebServer server(SERVER_PORT);
#else
WebServer server(SERVER_PORT);
WebServer *pserver = &server;
#endif
#endif

/**
 * Updater definitions
 * Note : there is 15 s for the reboot
 */
#ifdef USE_HTTPUPDATER
#ifdef ESP8266
ESP8266HTTPUpdateServer httpUpdater(UPDATER_DEBUG);
#endif

#ifdef ESP32
#ifdef USE_ASYNC_WEBSERVER
ESPAsyncHTTPUpdateServer httpUpdater;
#else
HTTPUpdateServer httpUpdater(UPDATER_DEBUG);
#endif
#endif
const char *update_path = "/firmware";
#endif

const char *update_username = "admin";
const char *update_password = "admin";

// A counter for the connexion
#define TRY_COUNT	40	// soit 10 s

// The buffer for the received message from UART
volatile char UART_Message_Buffer[BUFFER_SIZE] = {0};

// Private variables
// holds the current upload
File fsUploadFile;
// Reload the page when upload
bool ReloadPage = false;
// SSID file name
String SSID_FileName = SSID_FILE;
// DefaultAP restriction
bool DefaultAP = false;
// EEPROM Storage
bool Use_EEPROM = false;
Preferences *credential = NULL;

static const char SSIDResponse[] PROGMEM
		=
		"<META http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">SSID enregistré, redémarrez sur la nouvelle IP ...";

static const char SSIDReset[] PROGMEM
		=
		"<META http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">SSID effacé. Redémarrage sur IP locale ...";

// ********************************************************************************
// ServerConnexion class section
// ********************************************************************************

void ServerConnexion::begin(ServerSettings &settings)
{
	_IsSoftAP = settings._IsSoftAP;
	_SSID = settings._SSID;
	_PWD = settings._PWD;
	_ip = settings._ip;
	_gateway = settings._gateway;
	_onBeforeConnexion = settings._onBeforeConnexion;
	_onAfterConnexion = settings._onAfterConnexion;
}

/**
 * Get SSID and password from a file
 * String must be as : SSID#pwd
 */
bool ServerConnexion::SSIDFromFile(const String &filename)
{
	String path = filename;
	CheckBeginSlash(path);
	File ssid_file = FS_Partition->open(path, "r");
	if (ssid_file)
	{
		String str = ssid_file.readString();
		// Use the UART buffer like if we receive string from UART
		// We suppose that message is SSID#pwd
		strcpy((char*) UART_Message_Buffer, str.c_str());
		ssid_file.close();
		return ExtractSSID_Password();
	}
	return false;
}

bool ServerConnexion::SSIDFromEEPROM()
{
	if (!credential)
		return false;

	credential->begin("credentials", false);
	_SSID = credential->getString("ssid", "");
	_PWD = credential->getString("password", "");
	credential->end();
	return ((!_SSID.isEmpty()) && (!_PWD.isEmpty()));
}

/**
 * Write SSID and password to a file
 * String must be as : SSID#pwd
 */
void SSIDToFile(const String &filename, const String &ssidpwd)
{
	String path = filename;
	CheckBeginSlash(path);
	File ssid_file = FS_Partition->open(path, "w");
	if (ssid_file)
	{
#ifdef ESP8266
		ssid_file.write(ssidpwd.c_str());
#endif
#ifdef ESP32
		ssid_file.print(ssidpwd);
#endif
		ssid_file.close();
	}
}

void SSIDToEEPROM(const String &ssid, const String &pwd)
{
	if (!credential)
		return;
	credential->begin("credentials", false);
	credential->putString("ssid", ssid);
	credential->putString("password", pwd);
	credential->end();
}

void DeleteSSID(void)
{
	while (Lock_File)
		;
	FS_Partition->remove(SSID_FILE);
	if (credential)
	{
		SSIDToEEPROM("", "");
	}
	print_debug(F("SSID reset"));
}

String macToString(const unsigned char *mac)
{
	char buf[20];
	snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
			mac[4], mac[5]);
	return String(buf);
}

#ifdef ESP8266
void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected &evt)
{
	print_debug("Station disconnected: mac = " + macToString(evt.mac));
}

void onWifiDisconnected(const WiFiEventStationModeDisconnected &evt)
{
	print_debug("Station disconnected: reason = " + String(evt.reason));
	WiFi.disconnect();
}
#endif

#ifdef ESP32
void WiFiEvent(WiFiEvent_t event)
{
//	Serial.printf("[WiFi-event] event: %d\n", event);

	switch (event)
	{
		case ARDUINO_EVENT_WIFI_READY:
			print_debug("WiFi interface ready");
			break;
		case ARDUINO_EVENT_WIFI_SCAN_DONE:
			print_debug("Completed scan for access points");
			break;
		case ARDUINO_EVENT_WIFI_STA_START:
			print_debug("WiFi client started");
			break;
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
			print_debug("WiFi clients stopped");
			break;
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
//			print_debug("Disconnected from WiFi access point");
			break;
		case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
			print_debug("Authentication mode of access point has changed");
			break;
		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			print_debug("Obtained IP address: ", false);
			print_debug(WiFi.localIP().toString());
			break;
		case ARDUINO_EVENT_WIFI_STA_LOST_IP:
			print_debug("Lost IP address and IP address is reset to 0");
			break;
		case ARDUINO_EVENT_WPS_ER_SUCCESS:
			print_debug("WiFi Protected Setup (WPS): succeeded in enrollee mode");
			break;
		case ARDUINO_EVENT_WPS_ER_FAILED:
			print_debug("WiFi Protected Setup (WPS): failed in enrollee mode");
			break;
		case ARDUINO_EVENT_WPS_ER_TIMEOUT:
			print_debug("WiFi Protected Setup (WPS): timeout in enrollee mode");
			break;
		case ARDUINO_EVENT_WPS_ER_PIN:
			print_debug("WiFi Protected Setup (WPS): pin code in enrollee mode");
			break;
		case ARDUINO_EVENT_WIFI_AP_START:
			print_debug("WiFi access point started");
			break;
		case ARDUINO_EVENT_WIFI_AP_STOP:
			print_debug("WiFi access point  stopped");
			break;
		case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
			print_debug("Client connected");
			break;
		case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
			print_debug("Client disconnected");
			break;
		case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
			print_debug("Assigned IP address to client");
			break;
		case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
			print_debug("Received probe request");
			break;
		case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
			print_debug("AP IPv6 is preferred");
			break;
		case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
			print_debug("STA IPv6 is preferred");
			break;
		case ARDUINO_EVENT_ETH_GOT_IP6:
			print_debug("Ethernet IPv6 is preferred");
			break;
		case ARDUINO_EVENT_ETH_START:
			print_debug("Ethernet started");
			break;
		case ARDUINO_EVENT_ETH_STOP:
			print_debug("Ethernet stopped");
			break;
		case ARDUINO_EVENT_ETH_CONNECTED:
			print_debug("Ethernet connected");
			break;
		case ARDUINO_EVENT_ETH_DISCONNECTED:
			print_debug("Ethernet disconnected");
			break;
		case ARDUINO_EVENT_ETH_GOT_IP:
			print_debug("Obtained IP address");
			break;
		default:
			break;
	}
}
#endif

ServerConnexion::~ServerConnexion()
{
	if (credential)
	{
		delete credential;
		credential = NULL;
	}
}

/**
 * A basic function to get connexion to the network
 * toUART: if true, send the formated string "IP=x.x.x.x" to UART
 * return true if connexion success
 */
bool ServerConnexion::Connexion(bool toUART)
{
	uint32_t time = millis();
	// Connect to Wi-Fi network with _SSID and password
	print_debug(F("\r\nConnecting to "), false);
	print_debug(_SSID);
	print_debug(_PWD);
#ifdef ESP8266
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
#endif
#ifdef ESP32
	WiFi.setSleep(WIFI_PS_NONE);
#endif
	if (_IsSoftAP)
	{
		WiFi.mode(WIFI_AP);
#ifdef ESP8266
		if ((_ip.isSet()) && (_gateway.isSet()))
			WiFi.softAPConfig(_ip, _gateway, subnet);
		WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
#else
		if ((!_ip.toString().equals("0.0.0.0")) && (!_gateway.toString().equals("0.0.0.0")))
			WiFi.softAPConfig(_ip, _gateway, subnet);
#endif
		if (!WiFi.softAP(_SSID, _PWD))
			return false;
		_IPaddress = "IP=" + WiFi.softAPIP().toString();
	}
	else
	{
		uint8_t count = 0;
		WiFi.mode(WIFI_STA);
#ifdef ESP8266
		if ((_ip.isSet()) && (_gateway.isSet()))
			WiFi.config(_ip, _gateway, subnet);
		WiFi.onStationModeDisconnected(&onWifiDisconnected);
#else
		if ((!_ip.toString().equals("0.0.0.0")) && (!_gateway.toString().equals("0.0.0.0")))
			WiFi.config(_ip, _gateway, subnet);
		WiFi.onEvent(WiFiEvent);
#endif
		WiFi.begin(_SSID, _PWD);
		delay(100);  // small delay
		while ((WiFi.status() != WL_CONNECTED) && (++count < TRY_COUNT))
		{
			delay(250);  // Allow background operation
			print_debug(F("."), false);
		}
		print_debug(F(""));
		if (WiFi.status() != WL_CONNECTED)
		{
			switch (WiFi.status())
			{
				case WL_NO_SSID_AVAIL:
					print_debug("SSID is not available");
					break;
				case WL_CONNECT_FAILED:
					print_debug("The connection fails for all the attempts");
					break;
				case WL_CONNECTION_LOST:
					print_debug("The connection is lost");
					break;
				case WL_DISCONNECTED:
					print_debug("Disconnected from the network");
					break;
				default:
					break;
			}
			return false;
		}
		_IPaddress = "IP=" + WiFi.localIP().toString();
	}

	// Print local IP address
	print_debug(F("WiFi connected in "), false);
	print_debug(String(millis() - time), false);
	print_debug(F(" ms"));
	print_debug(F("IP address: "));
	print_debug(_IPaddress);

	// Signal dBm
	_Wifi_Signal = WiFi.RSSI();
	print_debug(F("WiFi signal : "), false);
	print_debug(String(_Wifi_Signal), false);
	print_debug(F(" dBm"));

#ifdef USE_MDNS
  if (MDNS.begin(_mDNSHostName.c_str()))
  {
  	print_debug(F("mDNS responder started"));
  }
#endif

	// Send IP address to UART if required
	if (toUART)
		printf_message_to_UART(_IPaddress);

	// Now, we are connected
	return true;
}

/**
 * Check if we have client connected
 */
bool ServerConnexion::WifiConnected(void)
{
//	return ((WiFi.status() == WL_CONNECTED) && server.client().connected());
	return ((WiFi.status() == WL_CONNECTED));
}

/**
 * Keep alive connexion
 * This function should be called regularly in a loop or a task
 */
void ServerConnexion::KeepAlive(void)
{
	if (!WifiConnected())
	{
		Connexion(false);
	}
}

/**
 * Wait for the connexion
 * If toUART = true then send IP to UART after connexion (default false)
 * return true while connexion is not established
 */
bool ServerConnexion::WaitForConnexion(Conn_typedef connexion, bool toUART)
{
	uint32_t tryMax, trycount = 0;

	// Evènement à faire avant d'essayer la connexion
	if (_onBeforeConnexion != NULL)
		_onBeforeConnexion();

	switch (connexion)
	{
		case Conn_Inline:
			print_debug(F("Inline connexion mode."));
			tryMax = 3;
			break;  // 30 secondes
		case Conn_UART:
			print_debug(F("UART connexion mode."));
			tryMax = 100;
			break;  // 25 secondes max
		case Conn_File:
			print_debug(F("File connexion mode."));
			tryMax = 1;
			break;
		case Conn_EEPROM:
			print_debug(F("EEPROM connexion mode."));
			tryMax = 1;
			if (!credential)
				credential = new Preferences();
			Use_EEPROM = true;
			break;
		default:
			tryMax = 1;
	}

	while (WaitForNetWork && (trycount++ < tryMax))
	{
		switch (connexion)
		{
			case Conn_Inline:
			{
				WaitForNetWork = !Connexion(toUART);
				break;
			}
			case Conn_UART:
			{
				// On attend la transmission du SSID et password au format SSID#pwd
				if (CheckUARTMessage())
				{
					if (ExtractSSID_Password())
						WaitForNetWork = !Connexion(toUART);
				}
				// Temporisation
				delay(250);
				break;
			}
			case Conn_File:
			{
				// Si le fichier contenait le SSID et le password au format SSID#pwd
				if (SSIDFromFile(SSID_File))
				{
					WaitForNetWork = !Connexion(toUART);
				}
				else
				{
					print_debug(F("File not found or invalid SSID/password"));
					WaitForNetWork = !DefaultAPConnexion();
				}
				break;
			}
			case Conn_EEPROM:
			{
				// Si le fichier contenait le SSID et le password au format SSID#pwd
				if (SSIDFromEEPROM())
				{
					WaitForNetWork = !Connexion(toUART);
				}
				else
				{
					print_debug(F("EEPROM empty or invalid SSID/password"));
					WaitForNetWork = !DefaultAPConnexion();
				}
				break;
			}
			default:
				print_debug(F("Unknown connexion method"));
		}

		// HTTP updater setup
#ifdef USE_HTTPUPDATER
		httpUpdater.setup(&server, update_path, update_username, update_password);
#endif

		// We are connected
		if (!WaitForNetWork)
		{
			// Callback that define server events
			if (_onAfterConnexion != NULL)
				_onAfterConnexion();

			// Start the server
			server.begin();
#ifdef USE_MDNS
			MDNS.addService("http", "tcp", SERVER_PORT);
#endif

			// Get Epoch time via NTP
#if defined(USE_NTP_SERVER) && defined(USE_RTCLocal)
			if (!_IsSoftAP)
			{
				print_debug(F("Get EpochTime"));
				if (RTC_Local.setEpochTime(USE_NTP_SERVER))
					print_debug(F("Get EpochTime ended"));
				else
				{
					print_debug(F("Get EpochTime, second try"));
					if (RTC_Local.setEpochTime(USE_NTP_SERVER))
						print_debug(F("Get EpochTime ended"));
					else
						print_debug(F("Get EpochTime failed"));
				}
			}
#endif
		}
	}
	return WaitForNetWork;
}

/**
 * The SSID file is not found or identifiants are incorrect
 * We reboot in Soft AP mode
 */
bool ServerConnexion::DefaultAPConnexion()
{
	_IsSoftAP = true;
	_SSID = DEFAULT_SOFTAP;
	DefaultAP = true;
	return Connexion();
}

String ServerConnexion::getGateway() const
{
	return WiFi.gatewayIP().toString();
}

String ServerConnexion::getMAC() const
{
	return WiFi.BSSIDstr();
}

void ServerConnexion::setSSID_FileName(const String &name)
{
	SSID_File = name;
	SSID_FileName = name;
}

/**
 * Check the UART UART_Message_Buffer and try to extract SSID and password
 * Message should be like : SSID#pwd
 * pwd may be empty
 */
bool ServerConnexion::ExtractSSID_Password(void)
{
	char *pBuffer;

	if (strchr((char*) UART_Message_Buffer, '#') != NULL)
	{
		// Extrait SSID et password. Format SSID#pwd
		_SSID = String(strtok((char*) UART_Message_Buffer, "#"));
		pBuffer = strtok(NULL, "#");
		if (pBuffer != NULL)
			_PWD = String(pBuffer);
		else
			_PWD = "";
		return true;
	}

	return false;
}

/**
 * Return string IP=ipaddress
 */
String GetIPaddress(void)
{
#ifdef ESP8266
	if (WiFi.getMode() == WIFI_AP)
	  return "IP=" + WiFi.softAPIP().toString();
	else
		return "IP=" + WiFi.localIP().toString();
#else
	if (WiFi.getMode() == WIFI_MODE_AP)
	  return "IP=" + WiFi.softAPIP().toString();
	else
		return "IP=" + WiFi.localIP().toString();
#endif
}

// ********************************************************************************
// UART section
// ********************************************************************************

/**
 * Function that receive a stream from UART and directly send it
 * to the server.
 * By default, use Serial for UART
 */
void ServerConnexion::StreamUARTMessage(const String &ContentType, const onTimeOut &TimeOut_cb, uint32_t timeout,
		HardwareSerial *Serial_Message)
{
#ifndef USE_ASYNC_WEBSERVER
	static char Stream_Buffer1[STREAM_BUFFER_SIZE] = {0};
	static char Stream_Buffer2[STREAM_BUFFER_SIZE] = {0};
	unsigned long currentTime = 0;
	uint32_t count = 0;
	uint8_t buffer_index = 1;
	char *pXML_Buffer = &Stream_Buffer1[0];
	int data = 0;
	bool IsTimeOut = true;

	// Récupération des données. La fin est marquée par le caractère END_DATA.
	// On a un timeout ms pour recevoir les données
	// Les données sont renvoyées par paquet de STREAM_BUFFER_SIZE octets
	// Je sais pas si c'est utile mais on utilise deux buffers en alternance, un réception, un envoie
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, ContentType, "");
	currentTime = millis();

	while (millis() - currentTime < timeout)
	{
		if (Serial_Message->available())
		{
			data = Serial_Message->read();
			if (data != END_DATA)
			{
				*pXML_Buffer++ = (char) data;
				count++;
				// Buffer is full : send it
				if (count == STREAM_BUFFER_SIZE)
				{
					if (buffer_index == 1)
					{
						server.sendContent(Stream_Buffer1, STREAM_BUFFER_SIZE);
						count = 0;
						pXML_Buffer = &Stream_Buffer2[0];
						// On passe sur le deuxième buffer
						buffer_index = 2;
					}
					else
					{
						// Deuxième buffer
						server.sendContent(Stream_Buffer2, STREAM_BUFFER_SIZE);
						count = 0;
						pXML_Buffer = &Stream_Buffer1[0];
						// On repasse sur le premier buffer
						buffer_index = 1;
					}
				}
			}
			else
			{
				*pXML_Buffer = 0;
				IsTimeOut = false;
				// break while timeout loop
				break;
			}
		}
		yield();  // Background operation
	}
	// S'il reste des données à envoyer
	if (count > 0)
	{
		if (buffer_index == 1)
			server.sendContent(Stream_Buffer1, count);
		else
			server.sendContent(Stream_Buffer2, count);
	}

	if (IsTimeOut && (TimeOut_cb != NULL))
		TimeOut_cb();
	else
	{
		server.sendContent("");
		server.client().stop();
	}

	//  print_debug("Count : " +  String(count) + "  Time : " + String(millis() - currentTime) + " ms");
#endif
}

#ifdef USE_ASYNC_WEBSERVER
void ServerConnexion::SendDefaultXML(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = request->beginResponseStream("text/plain; charset=iso-8859-1");

	response->print(F("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\r\n<empty></empty>\r\n"));
	response->print("");
	request->send(response);
}
#else
void ServerConnexion::SendDefaultXML(void)
{
	server.sendContent(F("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\r\n<empty></empty>\r\n"));
	server.sendContent("");
	server.client().stop();
}
#endif

// ********************************************************************************
// Miscellaneous section
// ********************************************************************************

/**
 * Self reset ESP
 * You will receive a message like this :
 *  ets Jan  8 2013,rst cause:2, boot mode:(3,6)
 */
void Auto_Reset(void)
{
	print_debug(F("\r\nAuto reset : "));
#ifdef USE_ASYNC_WEBSERVER
	server.end();
#else
	server.client().stop();
#endif
	delay(100);
	// End cleanly FileSystem partition and Data partition if exist
	FS_Partition->end();
	Data_Partition->end();
#ifdef ESP8266
	ESP.reset();
#endif
#ifdef ESP32
	ESP.restart();
#endif
}

// ********************************************************************************
// Basic Task function to keep alive the connexion
// ********************************************************************************

#ifdef KEEP_ALIVE_USE_TASK

// We assume that ServerConnexion instance is myServer
extern ServerConnexion myServer;

void KEEP_ALIVE_Task_code(void *parameter)
{
	BEGIN_TASK_CODE_UNTIL("KEEP_ALIVE_Task");
	for (EVER)
	{
		myServer.KeepAlive();
		END_TASK_CODE_UNTIL();
	}
}
#endif

// ********************************************************************************
// server events section
// ********************************************************************************

// Some handle common functions
String GetURI(CB_SERVER_PARAM);
void handleDefaultAP(CB_SERVER_PARAM);
void handleDefaultFile(CB_SERVER_PARAM);
bool handleReadFile(CB_SERVER_PARAM);
void handleNotFound(CB_SERVER_PARAM);
void handleGetFile(CB_SERVER_PARAM);
void handleDeleteFile(CB_SERVER_PARAM);
#ifdef USE_ASYNC_WEBSERVER
void handleUploadFile(AsyncWebServerRequest *request, String thefile, size_t index, uint8_t *data, size_t len, bool final);
#else
void handleUploadFile();
#endif
void handleListFile(CB_SERVER_PARAM);
void handleCreateFile(CB_SERVER_PARAM);
void handleGetTime(CB_SERVER_PARAM);
void handleSetTime(CB_SERVER_PARAM);
void handleReset(CB_SERVER_PARAM);
void handleSetDHCPIP(CB_SERVER_PARAM);

/**
 * Just the basic events for the server :
 * - load the html page : all files (html, js, ico, ...)
 * - download a specific file
 * - delete a file
 * - upload a file
 * - list a directory
 * - create a file
 * - soft reset ESP
 * - set time : update RTCLocal
 * - set network with DHCPIP
 * - reset DHCPIP
 * @param event : a combinaison of Event_typedef
 */
void Server_CommonEvent(uint16_t event)
{
	if (DefaultAP)
	{
		// Toutes les requêtes redirigées sur une seule réponse
		server.onNotFound(handleDefaultAP);

		// set DHCPIP
		if ((event & Ev_SetDHCPIP) == Ev_SetDHCPIP)
			server.on("/setDHCPIP", HTTP_POST, handleSetDHCPIP);

		return;
	}

	// default html file
	if ((event & Ev_LoadPage) == Ev_LoadPage)
		server.onNotFound(handleDefaultFile);

	// download file
	if ((event & Ev_GetFile) == Ev_GetFile)
	{
		server.on("/getfile", HTTP_GET, handleGetFile);
		server.on("/getfile", HTTP_POST, handleGetFile);
	}

	// delete file
	if ((event & Ev_DeleteFile) == Ev_DeleteFile)
	{
		server.on("/delfile", HTTP_GET, handleDeleteFile);
		server.on("/delfile", HTTP_POST, handleDeleteFile);
	}

	// upload file
	// reload if param is present
	if ((event & Ev_UploadFile) == Ev_UploadFile)
		server.on("/upload", HTTP_POST, [](CB_SERVER_PARAM)
		{
			pserver->send(200, "text/plain", "");}, handleUploadFile);

	// list files in a directory
	if ((event & Ev_ListFile) == Ev_ListFile)
		server.on("/listfile", HTTP_GET, handleListFile);

	// create file
	if ((event & Ev_CreateFile) == Ev_CreateFile)
		server.on("/createfile", HTTP_GET, handleCreateFile);

	// reset ESP
	// Server side :
	// xmlHttp.open("PUT","/resetESP",true);
	// xmlHttp.send(null);
	if ((event & Ev_ResetESP) == Ev_ResetESP)
		server.on("/resetESP", HTTP_PUT, handleReset);

	// set time
	if ((event & Ev_SetTime) == Ev_SetTime)
		server.on("/setTime", HTTP_PUT, handleSetTime);

	// get time
	// Server side :
	// xmlHttp.open("GET","/getTime",true);
	// xmlHttp.send(null);
	if ((event & Ev_GetTime) == Ev_GetTime)
		server.on("/getTime", HTTP_GET, handleGetTime);

	// set DHCPIP
	if ((event & Ev_SetDHCPIP) == Ev_SetDHCPIP)
		server.on("/setDHCPIP", HTTP_POST, handleSetDHCPIP);

	// reset DHCPIP
	if ((event & Ev_ResetDHCPIP) == Ev_ResetDHCPIP)
	{
		server.on("/resetDHCPIP", HTTP_GET, [&](CB_SERVER_PARAM)
		{
			if ( !pserver->authenticate(update_username, update_password))
			return pserver->requestAuthentication();
#ifndef USE_ASYNC_WEBSERVER
				pserver->client().setNoDelay(true);
#endif
				pserver->send_P(200, PSTR("text/html"), SSIDReset);
				FS_Partition->remove("/SSID.txt");
				Auto_Reset();
			});
	}
}
/**
 * Check if path=SSID filename
 * If true then send 403 message
 */
bool CheckSSIDFileName(const String &path)
{
	if (path == "/" + SSID_FileName)
	{
		return true;
	}
	return false;
}

/**
 *  return the url asked by the server
 */
String GetURI(CB_SERVER_PARAM)
{
#ifdef USE_ASYNC_WEBSERVER
//	int start = pserver->url().lastIndexOf('/');
//	return pserver->url().substring(start);
	return pserver->url();
#else
	return server.uri();
#endif
}

/**
 * Send the file requested
 */
#ifdef USE_ASYNC_WEBSERVER
void send_html(AsyncWebServerRequest *request, String filefs, String format, bool zipped, PART_TYPE &partition)
{
	AsyncWebServerResponse *response = request->beginResponse(partition, filefs, format);
	if (zipped)
	{
		response->addHeader("Content-Encoding", "gzip");
		response->addHeader("Cache-Control", "max-age=604800");
	}
	request->send(response);
}
#else
void send_html(String filefs, String format, PART_TYPE &partition)
{
	File file = partition.open(filefs, "r");	// Open the file
	server.streamFile(file, format); 	        // then send it to the client
	file.close();                             // then close the file
}
#endif

/**
 * handle the default AP configuration
 * When the SSID is stored in file and when the file is not found or
 * SSID is incorrect, we load a default page to get new SSID and password
 */
void handleDefaultAP(CB_SERVER_PARAM)
{
	String path = GetURI(SERVER_PARAM);
	// Refuse la transmission du SSID
	if (CheckSSIDFileName(path))
		return pserver->send(403, "text/plain", "403: Access not allowed.");

	// Dans tous les cas, on envoie la page d'initialisation SSID
#ifdef USE_ASYNC_WEBSERVER
	send_html(SERVER_PARAM, "/config_SSID.html", "text/html", false, *FS_Partition);
#else
	send_html("/config_SSID.html", "text/html", *FS_Partition);
#endif
}

/**
 * handle default file
 */
void handleDefaultFile(CB_SERVER_PARAM)
{
#ifdef USE_ASYNC_WEBSERVER
	if (!handleReadFile(pserver))
		handleNotFound(pserver);
#else
	if (!handleReadFile())
		handleNotFound();
#endif
}

/**
 * Not found file message from the server
 */
void handleNotFound(CB_SERVER_PARAM)
{
	String path = GetURI(SERVER_PARAM);
	// Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
	pserver->send(404, "text/plain", "404: Not found for " + path);
}

/**
 * Handle the basic read file with HTTP_GET method :
 * JavaScript side :
 * 	xmlHttp.open("GET",url,true);
 * 	xmlHttp.send(null);
 * Server side :
 * 	server.onNotFound([]() {
 if (!handleReadFile(server.uri()))
 handleNotFound(server.uri());
 });
 */
bool handleReadFile(CB_SERVER_PARAM)
{
	String path = GetURI(SERVER_PARAM);
	File op_file;
	String _path = path;

	print_debug("handleReadFile: " + path);

	// Refuse la transmission du SSID
	if (CheckSSIDFileName(path))
	{
		pserver->send(403, "text/plain", "403: Access not allowed.");
		return false;
	}

	// Les pages web
	if (_path.endsWith("/"))
		_path += "index.html";         		// If a folder is requested, send the index file
	if (_path.equals("/home"))
		_path = "/index.html";
	String contentType;
	if (pserver->hasArg("download"))
		contentType = "application/octet-stream";
	else
		contentType = getContentType(_path);	// Get the MIME type
	bool pathWithGz = false;
#ifdef USE_GZ_FILE
	if (FS_Partition->exists(_path + ".gz")) // If there's a compressed version available, use it
	{
		_path += ".gz";
		pathWithGz = true;
	}
#endif
	if (pathWithGz || FS_Partition->exists(_path))
	{
#ifdef USE_ASYNC_WEBSERVER
		send_html(SERVER_PARAM, _path, contentType, pathWithGz, *FS_Partition);
#else
		send_html(_path, contentType, *FS_Partition);
#endif
		return true;
	}

	print_debug(F("\tFile Not Found"));
	return false;			// If the file doesn't exist, return false
}

/**
 * Handle the basic delete file with GET or POST method :
 * With xmlHttp :
 * JavaScript side :
 * 	xmlHttp.open("POST","/delfile",true);
 * 	xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
 * 	xmlHttp.send("FILE=" + filename);
 * Server side :
 * 	server.on("/delfile", HTTP_POST, handleDeleteFile);
 * With form :
 * HTML side :
 <form method="GET" action="/delfile">
 <label for="delfile">File to delete : </label>
 <input type="text" name="delfile">
 <input type="submit" value="Delete">
 </form>
 * Server side :
 * 	server.on("/delfile", HTTP_GET, handleDeleteFile);
 */
void handleDeleteFile(CB_SERVER_PARAM)
{
	// No file to delete
	RETURN_BAD_ARGUMENT();

	// The name of the file to delete is the first argument
	String path = pserver->arg((int) 0);

	// Can not delete root !
	if (path == "/")
		return pserver->send(500, "text/plain", "BAD PATH");

	// Add slash if necessary
	CheckBeginSlash(path);

	// Refuse la suppression du SSID
	if (CheckSSIDFileName(path))
		return pserver->send(403, "text/plain", "403: Access not allowed.");

	// File does not exist !
	if (!FS_Partition->exists(path))
		return pserver->send(404, "text/plain", "404: Not found for " + path);

	// Delete file, wait if Lock_File
	while (Lock_File)
		delay(10);
	FS_Partition->remove(path);

	print_debug("handleDeleteFile: " + path);
	pserver->send(204, "text/plain", "");
}

/**
 * Handle the basic get file with GET or POST method :
 * JavaScript side :
 * 	xmlHttp.open("POST","/getfile",true);
 * 	xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
 * 	xmlHttp.send("FILE=" + filename); //  + "&DATA_PART=1" to get file from data partition
 * Server side :
 * 	server.on("/getfile", HTTP_POST, handleGetFile);
 */
void handleGetFile(CB_SERVER_PARAM)
{
	// No file to get
	RETURN_BAD_ARGUMENT();

	// The name of the file to get is the first argument
	String path = pserver->arg((int) 0);
	print_debug("handleGetFile: " + path);

	// Refuse la transmission du SSID
	if (CheckSSIDFileName(path))
		return pserver->send(403, "text/plain", "403: Access not allowed.");

	PART_TYPE *partition = FS_Partition;
	// In case of requested file is on the data partition
	if ((pserver->args() == 2) && (pserver->hasArg("DATA_PART")))
		partition = Data_Partition;

	if (partition->exists(path))
	{
		Lock_File = true;
#ifdef USE_ASYNC_WEBSERVER
		send_html(SERVER_PARAM, path, "text/plain", false, *partition);
#else
		send_html(path, "text/plain", *partition);
#endif
		Lock_File = false;
	}
	else
		return pserver->send(404, "text/plain", "404: Not found for " + path);
}

/**
 * Handle the upload file with POST method :
 * The file is copied in the root folder
 * HTML side :
 <form method="POST" enctype="multipart/form-data"  action="/upload">
 <input type="checkbox" name="reload" checked>
 <label for="reload">Reload page</label><br>
 <label for="dir">Dir : </label>
 <input type="text" name="dir"><br>
 <input type="file" name="the_filename">
 <input type="submit" value="Télécharger">
 </form>
 * Server side :
 * 	server.on("/", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleUploadFile);
 */
#ifdef USE_ASYNC_WEBSERVER
void handleUploadFile(AsyncWebServerRequest *request, String thefile, size_t index, uint8_t *data,
		size_t len, bool final)
{
	String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
	print_debug(logmessage);
	String filename = thefile;
	PART_TYPE *Part = FS_Partition;

	if (!index)
	{
		CheckBeginSlash(filename);

		if (request->hasArg("partition"))
			Part = Data_Partition;

		if (request->hasArg("dir") && (request->arg("dir") != ""))
		{
			String dir = request->arg("dir");
			CheckBeginSlash(dir);
			if (!Part->exists(dir))
				Part->mkdir(dir);
			filename = dir + filename;
		}

		logmessage = "Upload Start: " + String(filename);
		print_debug(logmessage);
		// open the file on first call and store the file handle in the request object
		request->_tempFile = Part->open(filename, "w");
	}

	if (len)
	{
		// stream the incoming chunk to the opened file
		request->_tempFile.write(data, len);
		logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
		print_debug(logmessage);
	}

	if (final)
	{
		// close the file handle as the upload is now done
		request->_tempFile.close();
		logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
		print_debug(logmessage);
		if (request->hasArg("reload"))
			request->redirect("/");
	}
}
#else
void handleUploadFile(CB_SERVER_PARAM)
{
	HTTPUpload &upload = server.upload();
	PART_TYPE *Part = FS_Partition;

	switch (upload.status)
	{
		case UPLOAD_FILE_START:
		{
			ReloadPage = (server.hasArg("reload") && server.arg("reload").equals("on"));

			if (server.hasArg("partition"))
				Part = Data_Partition;

			String filename = upload.filename;
			CheckBeginSlash(filename);

			if (server.hasArg("dir") && (!server.arg("dir").isEmpty()))
			{
				String dir = server.arg("dir");
				CheckBeginSlash(dir);
				if (!Part->exists(dir))
					Part->mkdir(dir);
				filename = dir + filename;
			}

			print_debug("handleUploadFile Name: " + filename);

			// Open the file for writing in LittleFS (create if it doesn't exist)
			fsUploadFile = Part->open(filename, "w");
			if (fsUploadFile)
				print_debug("FS open : " + filename);
			else
				print_debug("LittleFS failed to open : " + filename);
			break;
		}
		case UPLOAD_FILE_WRITE:
		{
			if (fsUploadFile)
				fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
			break;
		}
		case UPLOAD_FILE_END:
		{
			if (fsUploadFile)
			{
				fsUploadFile.close();              // Close the file
				print_debug("handleFileUpload Size: " + String(upload.totalSize));
				if (ReloadPage)
				{
					delay(500);  // Small delay before reload page
					server.sendHeader("Location", "/");      // Redirect the client to the main page
					server.send(303);
					ReloadPage = false;
				}
				else
					server.send(204, "text/plain", "");
			}
			else
			{
				print_debug(F("File upload failed"));
				server.send(500, "text/plain", "500: couldn't create file");
				ReloadPage = false;
			}
			break;
		}
		default:
		{
			print_debug(F("File upload failed"));
			server.send(500, "text/plain", "500: file upload failed");
			ReloadPage = false;
		}
	}
}
#endif

/**
 * Handle the list directory with GET method :
 * The parameter DIR is the name of the directory to read
 * Responce is in JSON format
 * JavaScript side :
 * 	xmlHttp.open("GET","/listfile?DIR=" + the_dir,true);
 * 	xmlHttp.send(null);
 * Server side :
 * 	server.on("/listfile", HTTP_GET, handleListFile);
 */
void handleListFile(CB_SERVER_PARAM)
{
	// DIR arg not found
	if (!pserver->hasArg("DIR"))
		return pserver->send(500, "text/plain", "BAD ARGS");

	String path = pserver->arg("DIR");
	print_debug("handleListFile: " + path);
	CheckBeginSlash(path);

#ifdef USE_LITTLEFS
	if (!FS_Partition->exists(path))
		return pserver->send(404, "text/plain", "404: Not found for " + path);
#endif

	String output = "[";
#ifdef ESP8266
	Dir dir = FS_Partition->openDir(path);
	while (dir.next())
	{
		String fileName = dir.fileName();
		size_t fileSize = dir.fileSize();
		if (output != "[")
			output += ",\r\n";
		if (dir.isFile())
		{
			time_t time = dir.fileTime();
			output += "{\"type\":\"file\"";
			output += ", \"name\":\"";
			output += fileName;
			output += "\", \"size\":\"";
			output += formatBytes(fileSize, 0);
			output += "\", \"time\":";
			output += time;
			output += "}";
		}
		else
		{
			if (dir.isDirectory())
			{
				output += "{\"type\":\"dir\"";
				output += ", \"name\":\"";
				output += fileName;
				output += "\"}";
			}
		}
	}
#endif
#ifdef ESP32
	File root = FS_Partition->open(path);
	if (root.isDirectory())
	{
		File file = root.openNextFile();
		while (file)
		{
			if (file.isDirectory())
			{
				output += "{\"type\":\"dir\"";
				output += ", \"name\":\"";
				output += file.name();
				output += "\"}";
			}
			else
			{
				time_t time = file.getLastWrite();
				output += "{\"type\":\"file\"";
				output += ", \"name\":\"";
				output += file.name();
				output += "\", \"size\":\"";
				output += formatBytes(file.size(), 0);
				output += "\", \"time\":";
				output += time;
				output += "}";
			}
			file = root.openNextFile();
			if (file)
				output += ",\r\n";
		}
	}
#endif
	output += "]";
	print_debug(output);
	pserver->send(200, "text/json", output);
}

/**
 * Handle the creation of a file with GET method :
 * JavaScript side :
 * 	xmlHttp.open("GET","/createfile?FILE=" + the_filename,true);
 * 	xmlHttp.send(null);
 * Server side :
 * 	server.on("/createfile", HTTP_GET, handleCreateFile);
 */
void handleCreateFile(CB_SERVER_PARAM)
{
	RETURN_BAD_ARGUMENT();

	String path = pserver->arg((int) 0);
	print_debug("handleCreateFile: " + path);

	if (path == "/")
		return pserver->send(500, "text/plain", "BAD PATH");

	CheckBeginSlash(path);
	if (FS_Partition->exists(path))
		return pserver->send(500, "text/plain", "FILE EXISTS");

	File file = FS_Partition->open(path, "w");
	if (file)
		file.close();
	else
		return pserver->send(500, "text/plain", "CREATE FAILED");

	pserver->send(200, "text/plain", "OK");
}

/**
 * Handle Get time
 */
void handleGetTime(CB_SERVER_PARAM)
{
#if defined(USE_RTCLocal)
	pserver->send(200, "text/plain", (RTC_Local.the_time()));
#endif
}

/**
 * Handle time with PUT method :
 * First argument is : TIME=the_time
 * Second argument optionnel is : UART=1 if we want to forward the time to UART
 * JavaScript side :
 * 	xmlHttp.open("PUT","/setTime",true);
 * 	xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
 * 	xmlHttp.send(arg);
 * Server side :
 * 	server.on("/setTime", HTTP_PUT, handleSetTime);
 */
void handleSetTime(CB_SERVER_PARAM)
{
	RETURN_BAD_ARGUMENT();

	if (pserver->hasArg("TIME"))
	{
		// Mise à l'heure
		String time = pserver->arg("TIME");
		print_debug("handleSetTime: " + time);
#if defined(USE_RTCLocal)
		RTC_Local.setDateTime(time.c_str());
		// Sauvegarde de l'heure
		RTC_Local.saveDateTime(time.c_str());
#endif
		if (pserver->hasArg("UART"))
		{
			printf_message_to_UART("TIME=" + time);
		}
		pserver->send(200, "text/plain", "");
	}
	else
		return pserver->send(500, "text/plain", "TIME FAILED");
}

/**
 * Handle reset ESP
 */
void handleReset(CB_SERVER_PARAM)
{
	pserver->send(204, "text/plain", "");
	Auto_Reset();
}

/**
 * Handle the set DHCPIP with POST method :
 * The file is copied in the root folder
 * HTML side :
 <form method="POST"  action="/setDHCPIP">
 <input type="text" name="networkNameDHCP">
 <input type="text" name="networkPasswordDHCP">
 <input type="submit" value="Enregistrer">
 </form>
 * Server side :
 * 	server.on("/setDHCPIP", HTTP_POST, handleSetDHCPIP);
 */
void handleSetDHCPIP(CB_SERVER_PARAM)
{
	RETURN_BAD_ARGUMENT();

	if (SSID_FileName != "")
	{
		if (Use_EEPROM)
			SSIDToEEPROM(pserver->arg((int) 0), pserver->arg((int) 1));
		else
			SSIDToFile(SSID_FileName, pserver->arg((int) 0) + "#" + pserver->arg((int) 1));
	}

#ifndef USE_ASYNC_WEBSERVER
	pserver->client().setNoDelay(true);
#endif
	pserver->send_P(200, PSTR("text/html"), SSIDResponse);

	if (DefaultAP)
		Auto_Reset();
}

/**
 * Get the content type of the requested document
 */
const String getContentType(const String &filename)
{
	if (filename.endsWith(".html") || filename.endsWith(".htm"))
		return "text/html";
	else
		if (filename.endsWith(".gz"))
			return "application/x-gzip";
		else
			if (filename.endsWith(".css"))
				return "text/css";
			else
				if (filename.endsWith(".js"))
					return "application/javascript";
				else
					if (filename.endsWith(".png"))
						return "image/png";
					else
						if (filename.endsWith(".gif"))
							return "image/gif";
						else
							if (filename.endsWith(".jpg"))
								return "image/jpeg";
							else
								if (filename.endsWith(".ico"))
									return "image/x-icon";
								else
									if (filename.endsWith(".xml"))
										return "text/xml";
									else
										if (filename.endsWith(".pdf"))
											return "application/x-pdf";
										else
											if (filename.endsWith(".zip"))
												return "application/x-zip";
	return "text/plain";
}

// ********************************************************************************
// End of file
// ********************************************************************************
