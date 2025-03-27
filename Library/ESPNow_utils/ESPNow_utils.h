#pragma once

/**
 * ESP Now Master and Slave class
 * From ESP-NOW Broadcast Master example
 */

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include "ESP32_NOW.h"
#include "esp32-hal-log.h"
#include "WiFi.h"

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

typedef void (*esp_now_peer_message_cb_t)(const uint8_t*, int);

/**
 * Master ESP Now class
 */
class ESP_NOW_Master_Peer: public ESP_NOW_Peer
{
	public:
		// Constructor of the class using the broadcast address
		ESP_NOW_Master_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) :
				ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk)
		{
		}

		// Destructor of the class
		~ESP_NOW_Master_Peer()
		{
			remove();
		}

		// Function to properly initialize the ESP-NOW and register the broadcast peer
		bool begin(void)
		{
			if (!ESP_NOW.begin() || !add())
			{
				log_e("Failed to initialize ESP-NOW or register the broadcast peer");
				return false;
			}
			return true;
		}

		// Function to send a message to all devices within the network
		bool send_message(const uint8_t *data, size_t len)
		{
			if (!send(data, len))
			{
				log_e("Failed to broadcast message");
				return false;
			}
			return true;
		}

		// Set call back to manage message receive from slave
		void setOnReceive_cb(esp_now_peer_message_cb_t cb)
		{
			Receive_master_cb = cb;
		}

	protected:
		virtual void onReceive(const uint8_t *data, size_t len, bool broadcast) {
			if (Receive_master_cb)
				Receive_master_cb(data, len);
		}

		virtual void onSent(bool success) {
			// nothing to do here
		}

	private:
		esp_now_peer_message_cb_t Receive_master_cb = nullptr;
};

/**
 * Slave ESP Now class
 */
class ESP_NOW_Slave_Peer
{
	public:
		ESP_NOW_Slave_Peer()
		{
		}
		bool begin(void);

		// Set call back to manage message receive from master
		void setOnReceive_cb(esp_now_peer_message_cb_t cb);

		// Send message to the first master
		bool send_message(const uint8_t *data, size_t len);
};

