#pragma once

/**
 * This is a global config file that can be included in each library
 * if define USE_CONFIG_LIB_FILE is used.
 * This is an alternative for the define
 */

/**********************************************************
 * Debug and UART define : Debug_utils library
 **********************************************************/
#define SERIAL_DEBUG
#define LOG_DEBUG
//#define USE_UART // Beware if SERIAL_DEBUG is also defined since all two use Serial0
#define UART_BAUD	115200

//#define USE_SAVE_CRASH

//#define UART_USE_TASK

/**********************************************************
 * Partition define : Partition_utils library
 **********************************************************/
// Select only one. By default it is LittleFS
//#define USE_SPIFFS
//#define USE_FATFS

/**********************************************************
 * Server define : Server_utils library
 **********************************************************/
#define USE_RTCLocal // Default
#define USE_GZ_FILE // Default
#define SERVER_PORT	80
#define SSID_FILE	"/SSID.txt"
#define DEFAULT_SOFTAP	"DefaultAP"
#define USE_HTTPUPDATER

//#define USE_ASYNC_WEBSERVER
//#define ESPASYNCHTTPUPDATESERVER_LITTLEFS // Necessary for HTTPUPDATER with LITTLEFS in async server

/**********************************************************
 * MQTT define : MQTT_utils library
 **********************************************************/
#define MQTT_PORT	1883
#define TOPIC_MAXSIZE	100
#define PAYLOAD_MAXSIZE	250

/**********************************************************
 * Task define : Task_utils library
 **********************************************************/
#define RUN_TASK_MEMORY	false // true or false

/**********************************************************
 * RTC define : RTCLocal library
 **********************************************************/
#define USE_NTP_SERVER	2 // 1 or 2 for summer time
//#define RTC_USE_CORRECTION
//#define RTC_USE_TASK

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
 * Cirrus define
 **********************************************************/
//#define CIRRUS_CS5480
//#define CIRRUS_USE_UART
//#define CIRRUS_FLASH
//#define LOG_CIRRUS_CONNECT
//#define DEBUG_CIRRUS
//#define DEBUG_CIRRUS_BAUD

