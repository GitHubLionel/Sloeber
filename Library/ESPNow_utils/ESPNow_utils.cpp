#include "ESPNow_utils.h"
#include <vector>

esp_now_peer_message_cb_t Receive_slave_cb = nullptr;

class ESP_NOW_Peer_Class: public ESP_NOW_Peer
{
	public:
		// Constructor of the class
		ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) :
				ESP_NOW_Peer(mac_addr, channel, iface, lmk)
		{
		}

		// Destructor of the class
		~ESP_NOW_Peer_Class()
		{
		}

		// Function to register the master peer
		bool add_peer()
		{
			if (!add())
			{
				log_e("Failed to register the broadcast peer");
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

		// Function to print the received messages from the master
		void onReceive(const uint8_t *data, size_t len, bool broadcast)
		{
//			log_i("Received a message from master " MACSTR " (%s)\n", MAC2STR(addr()), broadcast ? "broadcast" : "unicast");
//			log_i("  Message: %s\n", (char* )data);
			if (Receive_slave_cb)
				Receive_slave_cb(data, len);
		}

		virtual void onSent(bool success)
		{
			// Nothing to do here
		}
};

/* Global Variables */

// List of all the masters. It will be populated when a new master is registered
std::vector<ESP_NOW_Peer_Class> masters;

/* Callbacks */

// Callback called when an unknown peer sends a message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg)
{
	if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0)
	{
		log_i("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
		log_i("Registering the peer as a master");

		ESP_NOW_Peer_Class new_master(info->src_addr, WiFi.channel(), WIFI_IF_STA, NULL);

		masters.push_back(new_master);
		if (!masters.back().add_peer())
		{
			log_e("Failed to register the new master");
			return;
		}
	}
	else
	{
		// The slave will only receive broadcast messages
		log_i("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
	}
}

bool ESP_NOW_Slave_Peer::begin(void)
{
	// Initialize the ESP-NOW protocol
	if (!ESP_NOW.begin())
	{
		log_e("Failed to initialize ESP-NOW");
		return false;
	}

	// Register the new peer callback
	ESP_NOW.onNewPeer(register_new_master, NULL);
	return true;
}

void ESP_NOW_Slave_Peer::setOnReceive_cb(esp_now_peer_message_cb_t cb)
{
	Receive_slave_cb = cb;
}

bool ESP_NOW_Slave_Peer::send_message(const uint8_t *data, size_t len)
{
	if (masters.size() != 0)
	{
		// Send message to the first master
		return masters.front().send_message(data, len);
	}
	else
	{
		log_i("No master to send message");
		return false;
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
