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

// Print SSID/pwd in debug log file
//#define SSID_PRINT_DEBUG

// For debug, load only default page
//#define USE_DEFAULT_HTML_PAGE

/**
 * Set web server port number to PORT
 */
SERVER_CLASSNAME server(SERVER_PORT);
SERVER_CLASSNAME *pserver = &server;

/**
 * Updater definitions
 * Note : there is 15 s for the reboot
 */
#ifdef USE_HTTPUPDATER
	#ifndef USE_ELEGANT_OTA
		#ifdef USE_ASYNC_WEBSERVER
      UPDATER_CLASSNAME httpUpdater;
      void onBeforeUpdate(UpdateType updateType, int &resultCode)
      {
      #ifdef TASKLIST_DEFINED
      	print_debug(F("Suspend all tasks"));
      	TaskList.SuspendAllTask();
      #endif
      //	resultCode = 1; // To abort update
      }
      void onAfterUpdate(UpdateType updateType, int &resultCode)
      {
      #ifdef TASKLIST_DEFINED
      	if (resultCode != 0)
      	{
      	  TaskList.ResumeAllTask();
      	  print_debug(F("Resume all tasks because error"));
      	}
      #endif
      }
		#else
      UPDATER_CLASSNAME httpUpdater(UPDATER_DEBUG);
		#endif
  #endif
#endif

const char *update_path = "/firmware";
const char *update_username = "admin";
const char *update_password = "admin";

// A counter for the connexion in station mode
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
static Preferences credential;

static const char SSIDResponse[] PROGMEM
		= "<META http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">SSID enregistré, redémarrez sur la nouvelle IP ...";

static const char SSIDReset[] PROGMEM
		= "<META http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">SSID effacé. Redémarrage sur IP locale ...";

#ifdef USE_ELEGANT_OTA
unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
	print_debug("OTA update started!");
  // <Add your own code here>
#ifdef TASKLIST_DEFINED
	print_debug(F("Suspend all task"));
	TaskList.SuspendAllTask();
#endif
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
		String logmessage = "OTA Progress Current: " + String(current) + " bytes, Final: " + String(final) + " bytes";
		print_debug(logmessage);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
  	print_debug("OTA update finished successfully!");
  } else {
  	print_debug("There was an error during OTA update!");
  }
  // <Add your own code here>
}
#endif

void ElegantOTAloop(void)
{
#ifdef USE_ELEGANT_OTA
  ElegantOTA.loop();
#endif
}

// ********************************************************************************
// ServerConnexion class section
// ********************************************************************************

void ServerConnexion::begin(const ServerSettings &settings)
{
	_IsSoftAP = settings.IsSoftAP;
	setSSID_PWD(settings.SSID, settings.PWD);
	setIPAddress(settings.ip, settings.gateway);
	setOnBeforeConnexion(settings.getOnBeforeConnexion());
	setOnAfterConnexion(settings.getOnAfterConnexion());
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
	credential.begin("credentials", true);
	_SSID = credential.getString("ssid", "");
	_PWD = credential.getString("password", "");
	credential.end();
	return ((!_SSID.isEmpty()) && (!_PWD.isEmpty()));
}

bool ServerConnexion::DHCPFromEEPROM()
{
	credential.begin("credentials", true);
	_useDHCP = credential.getBool("useDHCP", true);
	if (!_useDHCP)
	{
#ifdef ESP8266
		_ip = IPAddress((const uint8_t *)(credential.getString("ip", "").c_str()));
		_gateway = IPAddress((const uint8_t *)(credential.getString("gateway", "").c_str()));
		_subnet = IPAddress((const uint8_t *)(credential.getString("subnet", "255.255.255.0").c_str()));
		_dns1 = IPAddress((const uint8_t *)(credential.getString("dns1", _gateway.toString()).c_str()));
		_dns2 = IPAddress((const uint8_t *)(credential.getString("dns2", "255.255.255.255").c_str()));
#else
		_ip = IPAddress(credential.getString("ip", "").c_str());
		_gateway = IPAddress(credential.getString("gateway", "").c_str());
		_subnet = IPAddress(credential.getString("subnet", "255.255.255.0").c_str());
		_dns1 = IPAddress(credential.getString("dns1", _gateway.toString()).c_str());
		_dns2 = IPAddress(credential.getString("dns2", "255.255.255.255").c_str());
#endif
	}
	credential.end();
	return true;
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
	credential.begin("credentials", false);
	credential.putString("ssid", ssid);
	credential.putString("password", pwd);
	credential.end();
}

void DHCPToEEPROM(bool useDHCP, const String &ip, const String &gateway, const String &subnet,
		const String &dns1, const String &dns2)
{
	credential.begin("credentials", false);
	credential.putBool("useDHCP", useDHCP);
	if (useDHCP)
	{
		credential.remove("ip");
		credential.remove("gateway");
		credential.remove("subnet");
		credential.remove("dns1");
		credential.remove("dns2");
	}
	else
	{
		credential.putString("ip", ip);
		credential.putString("gateway", gateway);
		credential.putString("subnet", subnet);
		if (dns1.isEmpty())
			credential.remove("dns1");
		else
		  credential.putString("dns1", dns1);
		if (dns2.isEmpty())
			credential.remove("dns2");
		else
			credential.putString("dns2", dns2);
	}
	credential.end();
}

void DeleteSSID(void)
{
	while (Lock_File)
		;
	FS_Partition->remove(SSID_FILE);
	SSIDToEEPROM("", "");
	DHCPToEEPROM(true, "", "", "", "", "");
	print_debug(F("SSID reset"));
}

String macToString(const unsigned char *mac)
{
	char buf[20];
	snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
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
//			print_debug("Lost IP address and IP address is reset to 0");
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
}

/**
 * A basic function to get connexion to the network
 * toUART: if true, send the formated string "IP=x.x.x.x" to UART
 * return true if connexion success
 */
bool ServerConnexion::Connexion(bool toUART)
{
	uint32_t time = millis();

	// To avoid to many debug messages in log file
	if (!FirstConnexion)
		GLOBAL_PRINT_DEBUG = false;

	// Connect to Wi-Fi network with _SSID and password
	print_debug(F("\r\nConnecting to "), false);
#ifdef SSID_PRINT_DEBUG
	print_debug(_SSID);
	print_debug(_PWD);
#else
	print_debug(F("****"));
#endif

	if (_IsSoftAP)
	{
		WiFi.mode(WIFI_AP); // SoftAP mode
#ifdef ESP8266
		if (!_useDHCP)
			WiFi.softAPConfig(_ip, _gateway, _subnet);
		WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
#else
		if (!_useDHCP)
			WiFi.softAPConfig(_ip, _gateway, _subnet, _dns1);
#endif
		if (!WiFi.softAP(_SSID, _PWD))
		{
			GLOBAL_PRINT_DEBUG = true;
			return false;
		}
		_IPaddress = "IP=" + WiFi.softAPIP().toString();
	}
	else
	{
		uint8_t count = 0;
		WiFi.mode(WIFI_STA); // Station mode
#ifdef ESP8266
		if (!_useDHCP)
			WiFi.config(_ip, _gateway, _subnet, _dns1);
		WiFi.onStationModeDisconnected(&onWifiDisconnected);
#else
		if (!_useDHCP)
			WiFi.config(_ip, _gateway, _subnet, _dns1);
#endif
		WiFi.begin(_SSID, _PWD);
		WiFi.setSleep(false);
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
			GLOBAL_PRINT_DEBUG = true;
			return false;
		}
#ifdef ESP32
		// If we want message
		WiFi.onEvent(WiFiEvent);
#endif
#ifdef ESP8266
		WiFi.setSleepMode(WIFI_NONE_SLEEP);
#else
		WiFi.setSleep(WIFI_PS_NONE);
#endif
		_IPaddress = "IP=" + WiFi.localIP().toString();
	}

	// Now, we are connected
	FirstConnexion = false;
	GLOBAL_PRINT_DEBUG = true;

#ifdef USE_ASYNC_WEBSERVER
	print_debug(F("Use Async Webserver"));
#endif
	// Print local IP address
	print_debug(F("WiFi connected in "), false);
	print_debug(String(millis() - time), false);
	print_debug(F(" ms"));
	print_debug(F("IP address: "));
	print_debug(_IPaddress);

	// Channel
	print_debug(F("WiFi channel : "), false);
	print_debug(String(WiFi.channel()));

	// MAC address
	print_debug(F("MAC address : "), false);
	if (_IsSoftAP)
		print_debug(WiFi.softAPmacAddress());
	else
		print_debug(WiFi.macAddress());

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
	int8_t gmt = GLOBAL_NTP_SUMMER_HOUR;
	if (gmt == -1)
		gmt = USE_NTP_SERVER;

	if (!_IsSoftAP)
	{
		print_debug(F("Get EpochTime"));
		if (RTC_Local.setEpochTime(gmt))
			print_debug(F("Get EpochTime ended"));
		else
		{
			print_debug(F("Get EpochTime, second try"));
			if (RTC_Local.setEpochTime(gmt))
				print_debug(F("Get EpochTime ended"));
			else
				print_debug(F("Get EpochTime failed"));
		}
	}
#endif

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
 * We stop the debug message in log file to avoid filling it completely
 */
void ServerConnexion::KeepAlive(void)
{
	// Try to reconnect if we are disconnected
	if (!WifiConnected())
	{
#ifdef ESP32
		WiFi.removeEvent(WiFiEvent);
#endif
		Connexion(false);
	}
}

/**
 * Wait for the connexion
 * Initialize the parameters (SSID, pwd) and try to connect.
 * If toUART = true then send IP to UART after connexion (default false)
 * return true while connexion is not established
 */
bool ServerConnexion::WaitForConnexion(Conn_typedef connexion, bool toUART)
{
	uint32_t trycount = 0;

	// Evènement à faire avant d'essayer la connexion
	if (_onBeforeConnexion != NULL)
		_onBeforeConnexion();

	switch (connexion)
	{
		case Conn_Inline:
			print_debug(F("Inline connexion mode."));
			while (WaitForNetWork && (trycount++ < 3)) // 3 try
			{
				WaitForNetWork = !Connexion(toUART);
			}
			break;  // 30 secondes
		case Conn_UART:
			print_debug(F("UART connexion mode."));
			while (WaitForNetWork && (trycount++ < 100)) // 100 try
			{
				// Wait for the transmission of SSID and password in format SSID#pwd
				if (CheckUARTMessage())
				{
					if (ExtractSSID_Password())
						WaitForNetWork = !Connexion(toUART);
				}
				// Temporisation
				delay(250);
			}
			break;  // 25 secondes max
		case Conn_File:
			print_debug(F("File connexion mode."));
			// If file contain SSID and password in format SSID#pwd
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
		case Conn_EEPROM:
			print_debug(F("EEPROM connexion mode."));
			Use_EEPROM = true;
			// If SSID and password is in EEPROM
			if (SSIDFromEEPROM())
			{
				DHCPFromEEPROM();
				WaitForNetWork = !Connexion(toUART);
			}
			else
			{
				print_debug(F("EEPROM empty or invalid SSID/password"));
				WaitForNetWork = !DefaultAPConnexion();
			}
			break;
		default:
			WaitForNetWork = true;
			print_debug(F("Unknown connexion method"));
	}

	// HTTP updater setup
#ifdef USE_HTTPUPDATER
	if (!WaitForNetWork)
	{
#ifdef USE_ELEGANT_OTA
	  ElegantOTA.begin(&server, update_path, update_username, update_password);    // Start ElegantOTA
	  ElegantOTA.setAutoReboot(true);
	  // ElegantOTA callbacks
	  ElegantOTA.onStart(onOTAStart);
	  ElegantOTA.onProgress(onOTAProgress);
	  ElegantOTA.onEnd(onOTAEnd);
#else
		httpUpdater.setup(&server, update_path, update_username, update_password);
#ifdef USE_ASYNC_WEBSERVER
		httpUpdater.onUpdateBegin = onBeforeUpdate;
		httpUpdater.onUpdateEnd = onAfterUpdate;
#endif
#endif
	}
#endif
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

String ServerConnexion::getCurrentRSSI(void) const
{
	return "RSSI: " + (String) WiFi.RSSI() + " dBm";
}

String ServerConnexion::getDHCP() const
{
	IPAddress ip;
#ifdef ESP8266
	if (WiFi.getMode() == WIFI_AP)
	  ip = WiFi.softAPIP();
	else
		ip = WiFi.localIP();
#else
	if (WiFi.getMode() == WIFI_MODE_AP)
		ip = WiFi.softAPIP();
	else
		ip = WiFi.localIP();
#endif
	String dhcp = (_useDHCP) ? "1," : "0,";
	IPAddress gateway = _gateway;
	// Create a gateway from ip
	if (_useDHCP)
		gateway = IPAddress(ip[0], ip[1], ip[2], 1);
	return dhcp + ip.toString() + "," + _subnet.toString() + "," + gateway.toString();
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
const String GetIPaddress(void)
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

/**
 * Return MAC address
 */
const String GetMACaddress(void)
{
	return WiFi.BSSIDstr();
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
bool Auto_Reset(void)
{
	print_debug(F("\r\nAuto reset : "));
#ifdef TASKLIST_DEFINED
	print_debug(F("Suspend all task"));
	TaskList.SuspendAllTask();
#endif
#ifdef USE_ASYNC_WEBSERVER
	print_debug(F("End server"));
	server.end();
#else
	server.client().stop();
#endif
	print_debug(F("Unmount partition"));
	delay(100);
	// End cleanly FileSystem partition and Data partition if exist
	UnmountPartition();
	ESP.restart();
	return true;
}

/**
 * Get MAC address of the ESP
 * Should be called AFTER Wifi connexion for ESP32
 */
bool getESPMacAddress(String &mac)
{
#ifdef ESP8266
	mac = WiFi.macAddress();
	return true;
#else
	if (WiFi.getMode() == WIFI_MODE_AP)
		mac = WiFi.softAPmacAddress();
	else
		mac = WiFi.macAddress();

//	uint8_t baseMac[6];
//	char buf[20] = {0};
//	esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
//	if (ret == ESP_OK)
//	{
//		sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4],
//				baseMac[5]);
//		mac = String(buf);
//	}
//	return (ret == ESP_OK);
	return true;
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
		END_TASK_CODE_UNTIL(false);
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
void handleUploadFile(AsyncWebServerRequest *request, String thefile, size_t index, uint8_t *data, size_t len,
		bool final);
#else
void handleUploadFile();
#endif
void handleListFile(CB_SERVER_PARAM);
void handleCreateFile(CB_SERVER_PARAM);
void handleGetTime(CB_SERVER_PARAM);
void handleSetTime(CB_SERVER_PARAM);
void handleReset(CB_SERVER_PARAM);
void handleSetSSID(CB_SERVER_PARAM);
void handleSetDHCP(CB_SERVER_PARAM);
void handleGetMACAddress(CB_SERVER_PARAM);
void handleGetESPMACAddress(CB_SERVER_PARAM);

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
 * - set network with SSID
 * - reset SSID
 * @param event : a combinaison of Event_typedef
 */
void Server_CommonEvent(uint32_t event)
{
	if (DefaultAP)
	{
		// Toutes les requêtes redirigées sur une seule réponse
		server.onNotFound(handleDefaultAP);

		// set SSID
		if ((event & Ev_SetSSID) == Ev_SetSSID)
			server.on("/setSSID", HTTP_POST, handleSetSSID);

		return;
	}

	// default html file
	if ((event & Ev_LoadPage) == Ev_LoadPage)
		server.onNotFound(handleDefaultFile);

	// download file
	if ((event & Ev_GetFile) == Ev_GetFile)
	{
		server.on("/getFile", HTTP_GET, handleGetFile);
		server.on("/getFile", HTTP_POST, handleGetFile);
	}

	// delete file
	if ((event & Ev_DeleteFile) == Ev_DeleteFile)
	{
		server.on("/delFile", HTTP_GET, handleDeleteFile);
		server.on("/delFile", HTTP_POST, handleDeleteFile);
	}

	// upload file
	// reload if param is present
	if ((event & Ev_UploadFile) == Ev_UploadFile)
#ifndef USE_ASYNC_WEBSERVER
		server.on("/upload", HTTP_POST, []()
		{ pserver->send(200, "text/plain", "");}, handleUploadFile);
#else
		server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)
		{}, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
		{
			handleUploadFile(request, filename, index, data, len, final);
		});
#endif

	// list files in a directory
	if ((event & Ev_ListFile) == Ev_ListFile)
		server.on("/listFile", HTTP_GET, handleListFile);

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

	// set SSID
	if ((event & Ev_SetSSID) == Ev_SetSSID)
		server.on("/setSSID", HTTP_POST, handleSetSSID);

	// reset SSID
	if ((event & Ev_ResetSSID) == Ev_ResetSSID)
	{
		server.on("/resetSSID", HTTP_GET, [&](CB_SERVER_PARAM)
		{
			if ( !pserver->authenticate(update_username, update_password))
			return pserver->requestAuthentication();
#ifndef USE_ASYNC_WEBSERVER
				pserver->client().setNoDelay(true);
#endif
				pserver->send(200, "text/html", SSIDReset);
				FS_Partition->remove("/SSID.txt");
				Auto_Reset();
			});
	}

	// set DHCP
	if ((event & Ev_SetDHCP) == Ev_SetDHCP)
		server.on("/setDHCP", HTTP_POST, handleSetDHCP);

	// get MAC address
	// Server side :
	// xmlHttp.open("GET","/getMACAddress",true);
	// xmlHttp.send(null);
	if ((event & Ev_GetMACAddress) == Ev_GetMACAddress)
		server.on("/getMACAddress", HTTP_GET, handleGetMACAddress);

	// get ESP MAC address
	// Server side :
	// xmlHttp.open("GET","/getESPMACAddress",true);
	// xmlHttp.send(null);
	if ((event & Ev_GetESPMACAddress) == Ev_GetESPMACAddress)
		server.on("/getESPMACAddress", HTTP_GET, handleGetESPMACAddress);
}

/**
 * Check if path=SSID filename
 * If true then send 403 message
 */
bool CheckSSIDFileName(const String &path)
{
	bool equal = (path == "/" + SSID_FileName) ? true : false;
	return equal;
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

	// Get all web page
	if (_path.endsWith("/"))
		_path += "index.html";         		// If a folder is requested, send the index file
	if (_path.equals("/home"))
		_path = "/index.html";
#ifdef USE_DEFAULT_HTML_PAGE
	// for debug, load only the default page
	if (_path.equals("/index.html"))
	_path = "/default.html";
#endif
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
 * 	xmlHttp.open("POST","/delFile",true);
 * 	xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
 * 	xmlHttp.send("FILE=" + filename);
 * 	Options is:
 * 	//  + "&DATA_PART=1" to delete file from data partition
 * Server side :
 * 	server.on("/delFile", HTTP_POST, handleDeleteFile);
 * With form :
 * HTML side :
 <form method="GET" action="/delFile">
 <label for="delFile">File to delete : </label>
 <input type="text" name="delFile">
 <input type="submit" value="Delete">
 </form>
 * Server side :
 * 	server.on("/delFile", HTTP_GET, handleDeleteFile);
 */
void handleDeleteFile(CB_SERVER_PARAM)
{
	// No file to delete
	RETURN_BAD_ARGUMENT();

	// The name of the file to delete is the first argument
	String path = pserver->arg((int) 0);

	// Add slash if necessary
	CheckBeginSlash(path);

	// Can not delete root !
	if (path.equals("/"))
		return pserver->send(500, "text/plain", "BAD PATH");

	// Refuse la suppression du SSID
	if (CheckSSIDFileName(path))
		return pserver->send(403, "text/plain", "403: Access not allowed.");

	if (!DeleteFile(path, pserver->hasArg("DATA_PART")))
		return pserver->send(404, "text/plain", "404: Not found for " + path);

	print_debug("handleDeleteFile: " + path);
	pserver->send(204);
}

/**
 * Handle the basic get file with GET or POST method :
 * JavaScript side :
 * 	xmlHttp.open("POST","/getFile",true);
 * 	xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
 * 	xmlHttp.send("FILE=" + filename);
 * 	or
 * 	xmlHttp.open("GET","/getFile?FILE=" + filename,true);
 * 	xmlHttp.send(null);
 * 	Options is:
 * 	//  + "&DATA_PART=1" to get file from data partition
 * 	//  + "&USE_GZ=1" to get gz file
 * 	IMPORTANT:
 * 	  with USE_GZ, you must add: xmlHttp.responseType = 'arraybuffer';
 * 	  and use xmlHttp.response in place of xmlHttp.responseText
 * Server side :
 * 	server.on("/getFile", HTTP_POST, handleGetFile);
 * 	or
 * 	server.on("/getFile", HTTP_GET, handleGetFile);
 */
void handleGetFile(CB_SERVER_PARAM)
{
	// No file to get
	RETURN_BAD_ARGUMENT();

	// The name of the file to get is the first argument
	String path = pserver->arg((int) 0);
	CheckBeginSlash(path);
	print_debug("handleGetFile: " + path);

	// Refuse la transmission du SSID
	if (CheckSSIDFileName(path))
		return pserver->send(403, "text/plain", "403: Access not allowed.");

	PART_TYPE *partition = FS_Partition;
	// In case of requested file is on the data partition
	if (pserver->hasArg("DATA_PART"))
		partition = Data_Partition;

	String _path = path;
	bool pathWithGz = false;
	// If there's a compressed version available, use it
	if (pserver->hasArg("USE_GZ") && (partition->exists(_path + ".gz")))
	{
		_path += ".gz";
		pathWithGz = true;
	}

	if (pathWithGz || partition->exists(_path))
	{
		Lock_File = true;
#ifdef USE_ASYNC_WEBSERVER
		send_html(SERVER_PARAM, _path, "text/plain", false, *partition);
#else
		send_html(_path, "text/plain", *partition);
#endif
		Lock_File = false;
	}
	else
		return pserver->send(404, "text/plain", "404: Not found for " + _path);
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
void handleUploadFile(AsyncWebServerRequest *request, String thefile, size_t index, uint8_t *data, size_t len,
		bool final)
{
	String logmessage = "";
	String filename = thefile;
	PART_TYPE *Part = FS_Partition;

	if (!index)
	{
		ReloadPage = (request->hasArg("reload") && request->arg("reload").equals("on"));

		logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
		print_debug(logmessage);

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

	if (request->_tempFile)
	{
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
			logmessage = "Upload Complete: " + String(filename) + ", size: " + String(index + len);
			print_debug(logmessage);
			delay(10);
			if (ReloadPage)
			{
				print_debug("Reload /");
				ReloadPage = false;
				request->redirect("/index.html");
			}
			else
				request->send(204);
		}
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
					print_debug("Reload /");
					delay(500);  // Small delay before reload page
#ifdef ESP8266
					server.sendHeader("Location", "/");      // Redirect the client to the main page
					server.send(303);
#else
// I don't know why this doesn't work for version > 3.0.3
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION <= ESP_IDF_VERSION_VAL(5, 1, 4))
					server.sendHeader("Location", "/");      // Redirect the client to the main page
					server.send(303);
#endif
#endif
					ReloadPage = false;
				}
#ifdef ESP8266
				else
					server.send(204, "text/plain", "");
#else
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION <= ESP_IDF_VERSION_VAL(5, 1, 4))
				else
					server.send(204, "text/plain", "");
#endif
#endif
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
 * 	xmlHttp.open("GET","/listFile?DIR=" + the_dir,true);
 * 	xmlHttp.send(null);
 * Server side :
 * 	server.on("/listFile", HTTP_GET, handleListFile);
 */
void handleListFile(CB_SERVER_PARAM)
{
	// DIR arg not found
	if (!pserver->hasArg("DIR"))
		return pserver->send(500, "text/plain", "BAD ARGS");

	String path = pserver->arg("DIR");
	CheckBeginSlash(path);
	print_debug("handleListFile: " + path);

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
 * Handle the set SSID with POST method :
 * The file is copied in the root folder
 * HTML side :
 <form method="POST"  action="/setSSID">
 <input type="text" name="networkNameSSID">
 <input type="text" name="networkPasswordSSID">
 <input type="submit" value="Enregistrer">
 </form>
 * Server side :
 * 	server.on("/setSSID", HTTP_POST, handleSetSSID);
 */
void handleSetSSID(CB_SERVER_PARAM)
{
	RETURN_BAD_ARGUMENT();

	String ssid = pserver->arg("networkNameSSID");
	String pwd = pserver->arg("networkPasswordSSID");

	if (Use_EEPROM)
		SSIDToEEPROM(ssid, pwd);
	else
		if (SSID_FileName != "")
		{
			SSIDToFile(SSID_FileName, ssid + "#" + pwd);
		}

#ifndef USE_ASYNC_WEBSERVER
	pserver->client().setNoDelay(true);
#endif
	pserver->send(200, "text/html", SSIDResponse);

	if (DefaultAP)
		Auto_Reset();
}

/**
 * Handle set DHCP params
 */
void handleSetDHCP(CB_SERVER_PARAM)
{
	RETURN_BAD_ARGUMENT();

	bool useDHCP = pserver->arg("DHCP").toInt() == 1;
	String ip = pserver->arg("IP");
	String gateway = pserver->arg("GATEWAY");
	String mask = pserver->arg("MASK");
	String dns1 = pserver->arg("DNS1");
	String dns2 = pserver->arg("DNS2");
	DHCPToEEPROM(useDHCP, ip, gateway, mask, dns1, dns2);

	pserver->send(200, "text/html", "");
}

/**
 * Handle Get MAC Address
 */
void handleGetMACAddress(CB_SERVER_PARAM)
{
	pserver->send(200, "text/plain", GetMACaddress());
}

/**
 * Handle Get MAC Address
 */
void handleGetESPMACAddress(CB_SERVER_PARAM)
{
	String mac;
	if (getESPMacAddress(mac))
		pserver->send(200, "text/plain", mac);
	else
		pserver->send(200, "text/plain", F("Fail to get ESP MAC address"));
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
