#pragma once

/**
 * This is a global config file that can be included in each library
 * if define USE_CONFIG_LIB_FILE is used.
 * This is an alternative for the define
 * See doc for more informations
 */

// This define is used in Debug_utils and Server_utils library
// Require RTC_Local global instance
#define USE_RTCLocal // Default

/**********************************************************
 * Debug and UART define : Debug_utils library
 **********************************************************/
#define SERIAL_DEBUG
#define LOG_DEBUG
//#define USE_UART // Beware if SERIAL_DEBUG is also defined since all two use Serial0
#define UART_BAUD	115200

//#define USE_SAVE_CRASH

/**********************************************************
 * Partition define : Partition_utils library
 **********************************************************/
// Select only one. By default it is LittleFS
//#define USE_SPIFFS
//#define USE_FATFS

/**********************************************************
 * Server define : Server_utils library
 **********************************************************/
#define USE_GZ_FILE // Default
#define SERVER_PORT	80
#define SSID_FILE	"/SSID.txt"
#define DEFAULT_SOFTAP	"DefaultAP"
#define USE_HTTPUPDATER
//#define USE_MDNS

//#define USE_ASYNC_WEBSERVER
// Necessary for HTTPUPDATER with LITTLEFS in async server
// MUST BE PUT IN DEFINE OF THE PROJECT
//#define ESPASYNCHTTPUPDATESERVER_LITTLEFS

/**********************************************************
 * MQTT define : MQTT_utils library
 **********************************************************/
#define MQTT_PORT	1883
#define TOPIC_MAXSIZE	100
#define PAYLOAD_MAXSIZE	250

/**********************************************************
 * Task define : Task_utils library
 * By default, global instance TaskList is created
 * See end of file for task define used in several libraries
 **********************************************************/
#define RUN_TASK_MEMORY	false // true or false

/**********************************************************
 * RTC define : RTCLocal library
 * By default, global instance RTC_Local is created
 **********************************************************/
#define USE_NTP_SERVER	2 // 1 or 2 for summer time
//#define RTC_USE_CORRECTION

/**********************************************************
 * LCD, Oled, TFT define
 **********************************************************/
#define I2C_ADDRESS	0x3C // or 0x78
#define OLED_TIMEOUT	600	// 10 minutes

//#define USE_LCD

//#define OLED_SSD1306 // SSD1306
//#define SSD1306_RAM_128
//#define SSD1306_RAM_132

#define OLED_SSD1327 // SSD1327
//#define OLED_SH1107 // SH1107
//#define OLED_TOP_DOWN
//#define OLED_LEFT_RIGHT
//#define OLED_DOWN_TOP
//#define OLED_RIGHT_LEFT

/**********************************************************
 * Dallas DS18B20 define
 **********************************************************/

/**********************************************************
 * TeleInfo define
 **********************************************************/

/**********************************************************
 * Keyboard define
 **********************************************************/
#define DEBOUNCING_MS	200
#define DEBOUNCING_US	200000

/**********************************************************
 * Cirrus define
 **********************************************************/
//#define CIRRUS_CS5480
//#define CIRRUS_USE_UART
//#define CIRRUS_FLASH
//#define LOG_CIRRUS_CONNECT
//#define DEBUG_CIRRUS
//#define DEBUG_CIRRUS_BAUD

/**********************************************************
 * SSR define
 **********************************************************/
//#define USE_SSR
//#define SIMPLE_ZC_TEST

/**********************************************************
 * Task define used in several libraries
 * Need Task_utils library
 **********************************************************/
//#define UART_USE_TASK        // A basic task to analyse UART message
//#define RTC_USE_TASK         // To run RTCLocal in a task
//#define DS18B20_USE_TASK     // A basic task to check DS18B20 temperature every 2 s
//#define TELEINFO_USE_TASK    // A basic task to check TeleInfo every 1 s
//#define CIRRUS_USE_TASK      // A basic task to check Cirrus data every 200 ms
//#define KEYBOARD_USE_TASK    // A basic task to check keyboard every 10 ms

