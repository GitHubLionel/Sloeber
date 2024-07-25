/**
 * MQTT utils
 * Definition of a class to encapsulate MQTT operations
 *
 * USAGE :
 * Just create an MQTTClient instance in your main program
 * You must have somewhere a Wifi_Connexion() function that provide a wifi connexion
 */

#pragma once

#include "Arduino.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>	// Include WiFi library
#endif
#ifdef ESP32
#include <WiFi.h>	// Include WiFi library
#endif
#include <PubSubClient.h>

/**
 * The max length of the payload message in the MQTT callback
 */
#define PAYLOAD_MAXSIZE	250

/**
 * Structure for the MQTT callback
 */
typedef struct
{
		char payload[PAYLOAD_MAXSIZE] = {'\0'};
		String topic;
} MQTT_Message_cb_t;

/**
 * The credential for MQTT
 */
typedef struct
{
		String username;		// Username
		String password;		// Password of the username
		String broker;			// Machine IP on which the broker is
		int port = 1883;		// Port of the broker (don't forget to open this port in your firewall !)
} MQTT_Credential_t;

/**
 * An extern function that assure the Wifi connexion
 */
extern void Wifi_Connexion(void);

/**
 * Define a static function for xTaskCreate for mqttclient instance
 * called : StaticKeepalive_Task()
 */
#define WRAPPER_STATIC_TASK(mqttclient) \
void StaticKeepalive_Task(void *parameter) \
{ \
	mqttclient.Keepalive_Task(parameter); \
}

/**
 * A name for the keep alive task
 */
#define MQTT_KEEPALIVE_TASK_NAME	"MQTT_Keepalive"

/**
 * A standard definition of a keep alive task
 * Set create to true to create the task at beginning of task list
 */
#define MQTT_STANDARD_KEEPALIVE_TASK_DATA(create)	{create, MQTT_KEEPALIVE_TASK_NAME, 5000, 6, 10, CoreAny, StaticKeepalive_Task}

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

		void Publish(const String topic, const String text, bool subscribe = false);
		void Subscribe(const String topic, bool subscribe);
		bool Loop(void);

		bool Connected()
		{
			return wifiClient.connected();
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

	protected:
		void mqttCallback(char *topic, uint8_t *payload, unsigned int length);

	private:
		WiFiClient wifiClient;
		PubSubClient *MQTT_client;
		SemaphoreHandle_t sema_MQTT_KeepAlive;
		String client_id = "";

		MQTT_Credential_t Credential;
		MQTT_Message_cb_t Message;

		bool MQTT_Connected = false;

		bool Connexion(int trycount = 10);
};

