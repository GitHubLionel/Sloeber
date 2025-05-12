/**
 * MQTT utils
 * Definition of a class to encapsulate MQTT operations
 *
 * USAGE :
 * Just create an MQTTClient instance in your main program
 * You must have somewhere a Wifi_Connexion() function that provide a wifi connexion for keep alive task
 *
 * If you use task for keep alive, use the define WRAPPER_STATIC_TASK(your_MQTT) to have the
 * static code of the task
 */
#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>	// Include WiFi library
#endif
#ifdef ESP32
#include <WiFi.h>	// Include WiFi library
#endif
#include <PubSubClient.h>

/**
 * The max length of the payload message and topic in the MQTT callback
 */
#ifndef TOPIC_MAXSIZE
#define TOPIC_MAXSIZE	100
#endif
#ifndef PAYLOAD_MAXSIZE
#define PAYLOAD_MAXSIZE	250
#endif

// Default MQTT port
#ifndef MQTT_PORT
#define MQTT_PORT	1883
#endif

/**
 * Structure for the message of the MQTT callback
 */
typedef struct
{
		char payload[PAYLOAD_MAXSIZE] = {'\0'};
		char topic[TOPIC_MAXSIZE] = {'\0'};
} MQTT_Message_cb_t;

/**
 * The credential for MQTT
 */
typedef struct
{
		String username;		// Username
		String password;		// Password of the username
		String broker;			// Machine IP on which the broker is
		int port = MQTT_PORT;		// Port of the broker (don't forget to open this port in your firewall !)
} MQTT_Credential_t;

/**
 * An extern function that assure the Wifi connexion
 */
extern void Wifi_Connexion(void);

/**
 * Define a static function for xTaskCreate for mqttclient instance
 * called : StaticKeepalive_Task() for keep alive task
 * called : StaticParseMessage_Task() for parse message in the queue
 */
#define WRAPPER_STATIC_TASK(mqttclient) \
	void StaticKeepalive_Task(void *parameter) { (mqttclient).Keepalive_Task(parameter); } \
	void StaticParseMessage_Task(void *parameter) { (mqttclient).ParseMessage_Task(parameter); }

/**
 * A name for the keep alive and parse message task
 */
#define MQTT_KEEPALIVE_TASK_NAME	"MQTT_Keepalive"
#define MQTT_PARSEMESSAGE_TASK_NAME	"MQTT_ParseMess"

/**
 * A standard definition of a keep alive task
 * Set create to true to create the task at beginning of task list
 */
#define MQTT_STANDARD_KEEPALIVE_TASK_DATA(create)	{create, MQTT_KEEPALIVE_TASK_NAME, 5000, 6, 10, CoreAny, StaticKeepalive_Task}
#define MQTT_STANDARD_PARSEMESSAGE_TASK_DATA(create)	{create, MQTT_PARSEMESSAGE_TASK_NAME, 10000, 6, 10, CoreAny, StaticParseMessage_Task}

/**
 * class MQTTClient
 */
class MQTTClient
{
	public:
		MQTTClient();
		MQTTClient(const MQTT_Credential_t mqtt);

		bool begin();
		bool begin(const MQTT_Credential_t mqtt);

		void SetCredential(const MQTT_Credential_t mqtt);

		void Publish(const String &topic, const String &text, bool subscribe = false);
		void Publish(const String &text);
		void Subscribe(const String &topic, bool subscribe);
		bool Loop(void);

		bool WifiConnected(void);
		bool MQTTConnected(void);

		String GetLastTopic(void) const
		{
			return LastTopic;
		}

		void SetLastTopic(const String &topic)
		{
			LastTopic = topic;
		}

		MQTT_Message_cb_t GetMessage()
		{
			return Message;
		}

		void Keepalive_Task(void *parameter);
		const String GetKeepaliveTaskName(void)
		{
			return MQTT_KEEPALIVE_TASK_NAME;
		}

		void ParseMessage_Task(void *pvParameters);
		const String GetParseMessageTaskName(void)
		{
			return MQTT_PARSEMESSAGE_TASK_NAME;
		}

	protected:
		void IRAM_ATTR mqttCallback(char *topic, unsigned char *payload, unsigned int length);

	private:
		WiFiClient wifiClient; // @suppress("Type cannot be resolved") // @suppress("Abstract class cannot be instantiated")
		PubSubClient *MQTT_client; // @suppress("Type cannot be resolved")
		SemaphoreHandle_t sema_MQTT_KeepAlive;
		QueueHandle_t QMessage;
		String client_id = "";

		MQTT_Credential_t Credential;
		MQTT_Message_cb_t Message;

		bool AddToQueue = true;
		String LastTopic = "";

		bool Connexion(int trycount = 10);
};

