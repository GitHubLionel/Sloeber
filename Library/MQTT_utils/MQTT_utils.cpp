#include "MQTT_utils.h"

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const String mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

/**
 * Default constructor
 * You then must provide Credential before call begin
 */
MQTTClient::MQTTClient()
{
	MQTT_client = new PubSubClient(wifiClient);

	// auto stand for std::function<void (char *, unsigned char *, unsigned int)>
	// Declaration with bind method
//	using namespace std::placeholders; // for the tree parameters topic, payload and length
//	auto callback = std::bind(&MQTTClient::mqttCallback, this, _1, _2, _3);

	// Declaration with Lambda function
	auto callback = [&](char *topic, unsigned char *payload, unsigned int length) {
	  this->mqttCallback(topic, payload, length); // Pass arg from the function object arguments
	};

	MQTT_client->setCallback(callback);

	// Create a semaphore to secure MQTT transaction
	sema_MQTT_KeepAlive = xSemaphoreCreateBinary();
	xSemaphoreGive(sema_MQTT_KeepAlive);

	// Create the queue message
	QMessage = xQueueCreate(1, sizeof(MQTT_Message_cb_t));
}

MQTTClient::MQTTClient(const MQTT_Credential_t mqtt) :
		MQTTClient()
{
	SetCredential(mqtt);
}

/**
 * The MQTT callback
 * The payload is stored in the Message structure
 */
void MQTTClient::mqttCallback(char *topic, unsigned char *payload, unsigned int length)
{
	memset(Message.topic, '\0', TOPIC_MAXSIZE); // clear topic char buffer
	memset(Message.payload, '\0', PAYLOAD_MAXSIZE); // clear payload char buffer
	strcpy(Message.topic, topic);

	// extract payload. Note : string is ended since we have initalized to 0
	for (int i = 0; i < length; i++)
	{
		Message.payload[i] = ((char) payload[i]);
	}

	if (AddToQueue)
		xQueueOverwrite(QMessage, (void* ) &Message);

	// print_debug
//	print_debug("Message arrived in topic: ", false);
//	print_debug(topic);
//	print_debug("Message: ", false);
//	print_debug(String(Message.payload));
//	print_debug("-----------------------");
}

/**
 * The credential for the MQTT connexion
 */
void MQTTClient::SetCredential(const MQTT_Credential_t mqtt)
{
	Credential.username = mqtt.username;
	Credential.password = mqtt.password;
	Credential.broker = mqtt.broker;
	Credential.port = mqtt.port;
}

/**
 * Establish the connexion with the MQTT broker
 */
bool MQTTClient::begin()
{
	MQTT_client->setServer(Credential.broker.c_str(), Credential.port);

	client_id = "esp32-client-";
	client_id += String(WiFi.macAddress());
	print_debug("The client " + client_id + " connects to the public MQTT brokern");

	return Connexion();
}

bool MQTTClient::begin(const MQTT_Credential_t mqtt)
{
	SetCredential(mqtt);
	return begin();
}

/**
 * Connexion to the broker
 * Try to connect to the broker. Max try = 10 by default
 */
bool MQTTClient::Connexion(int trycount)
{
	// We must be connected to Wifi !
	if (!(WiFi.status() == WL_CONNECTED))
		return false;

	int count = 0;
	while (!MQTT_client->connected())
	{
		if (MQTT_client->connect(client_id.c_str(), Credential.username.c_str(),
				Credential.password.c_str()))
		{
			print_debug("Public MQTT broker connected");
			return true;
		}
		else
		{
			print_debug("failed with state: " + String(MQTT_client->state()));
			count++;
			if (count == trycount)
				break;
		}
	}
	return false;
}

/**
 * Publish text in the topic
 */
void MQTTClient::Publish(const String &topic, const String &text, bool subscribe)
{
	// Publish and subscribe
	if (MQTT_client->connected())
	{
		xSemaphoreTake(sema_MQTT_KeepAlive, portMAX_DELAY);
		MQTT_client->publish(topic.c_str(), text.c_str());
		if (subscribe)
			MQTT_client->subscribe(topic.c_str());
		xSemaphoreGive(sema_MQTT_KeepAlive);
		LastTopic = topic;
	}
}

/**
 * Publish text in the last topic used
 */
void MQTTClient::Publish(const String &text)
{
	// Publish
	if (MQTT_client->connected())
	{
		xSemaphoreTake(sema_MQTT_KeepAlive, portMAX_DELAY);
		MQTT_client->publish(LastTopic.c_str(), text.c_str());
		xSemaphoreGive(sema_MQTT_KeepAlive);
	}
}

/**
 * Subscribe to topic if subscribe == true else unsubscribe
 */
void MQTTClient::Subscribe(const String &topic, bool subscribe)
{
	if (MQTT_client->connected())
	{
		if (subscribe)
			MQTT_client->subscribe(topic.c_str());
		else
			MQTT_client->unsubscribe(topic.c_str());
	}
}

/**
 * Client loop. Should be call at the end of the main loop
 * Another solution is to use a the Keepalive_Task to do that
 */
bool MQTTClient::Loop(void)
{
	if (WifiConnected())
	{
		// whiles MQTTlient.loop() is running no other mqtt operations should be in process
		xSemaphoreTake(sema_MQTT_KeepAlive, portMAX_DELAY);
		MQTT_client->loop();
		xSemaphoreGive(sema_MQTT_KeepAlive);
		return true;
	}
	return false;
}

/**
 * Return true if Wifi is connected
 */
bool MQTTClient::WifiConnected(void)
{
//	print_debug("MQTT keep alive found MQTT status: " + String(wifiClient.connected()) +
//			", WiFi status: " + String(WiFi.status()));
	return (wifiClient.connected() && (WiFi.status() == WL_CONNECTED));
}

/**
 * Return true if a MQTT client is connected
 */
bool MQTTClient::MQTTConnected(void)
{
	return MQTT_client->connected();
}

/**
 * The keep alive task
 * Do the MQTT loop operation and make reconnexion if necessary
 */
void MQTTClient::Keepalive_Task(void *parameter)
{
	// setting keep alive to 90 seconds makes for a very reliable connection, must be set before the 1st connection is made.
//	MQTT_client->setKeepAlive(90);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = 250; //delay for ms
	for (;;)
	{
		if (!Loop())
		{
			if (!WifiConnected())
			{
				Wifi_Connexion();
			}
			Connexion();
		}
//		print_debug(" high watermark: " +  String(uxTaskGetStackHighWaterMark(NULL)) );
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
	vTaskDelete( NULL);
}

void MQTTClient::ParseMessage_Task(void *pvParameters)
{
	MQTT_Message_cb_t pMessage;
	for (;;)
	{
		// Read the queue and clean it
		if (xQueueReceive(QMessage, &pMessage, portMAX_DELAY) == pdTRUE)
		{
			print_debug("Message arrived in topic: ", false);
			print_debug(String(pMessage.topic));
			print_debug("Message: ", false);
			print_debug(String(pMessage.payload));
			print_debug("-----------------------");
		}
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
