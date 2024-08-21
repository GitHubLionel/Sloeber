#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "CIRRUS_define.h"

#include "Arduino.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * For debug purpose
 */
//#define DEBUG_CIRRUS
//#define DEBUG_CIRRUS_BAUD
//#define DEBUG_CIRRUS_WAIT_MESSAGE

/**
 * If we use Cirrus_Connect software to communicate with the Cirrus in UART.
 * In this case, only on SPI or if we have two UART (one for Cirrus, one for Cirrus_Connect)
 * Or
 * if we communicate by Wifi
 */
//#define LOG_CIRRUS_CONNECT

/** The two defines to be present in the configuration :
 * Define CIRRUS_CS5480 for cs5480 else it is cs5490
 * If CIRRUS_CS5480 and CIRRUS_USE_UART then cs5480 is used with UART else SPI
 * For cs5490, CIRRUS_USE_UART is automaticaly define
 * When we have several CS5480, the logic is to put LOW the CS pin to select a chip
 * and put HIGH the others chips
 */
//#define CIRRUS_CS5480
//#define CIRRUS_USE_UART

// If not cs5480 then only uart is possible for cs5490
#ifndef CIRRUS_CS5480
	#ifndef CIRRUS_USE_UART
	#define CIRRUS_USE_UART
	#endif
#endif

// Spécifique UART
#ifdef CIRRUS_USE_UART
// define SoftwareSerial = 0 or HardwareSerial = 1 mode
#define CIRRUS_UART_HARD	1

/**
 * TimeOut definition for uart and spi communication
 */
#define TIMEOUT600	100	// Timeout for 600 baud, should always be enough
#define TIMEOUT512K	8	// Timeout for >= 128 kbaud, 5 is good for CS5490 but depend of the insulator

// Delay for the read/write register in ms. May be increased if necessary
#define READ_REGISTER_DELAY_MS 100
#define WRITE_REGISTER_DELAY_MS 128

/**
 * For FLASH
 */
//#define CIRRUS_FLASH

// ******************************************************************
// Ne pas modifier ci-dessous
	#if CIRRUS_UART_HARD == 1
	#define CIRRUS_SERIAL_MODE	HardwareSerial
	#else
	#include <SoftwareSerial.h>
	#define CIRRUS_SERIAL_MODE	SoftwareSerial
	#endif

#else // CIRRUS_USE_UART
#include "SPI.h"
#endif

typedef union Bit_List
{
	public:
		uint8_t tab[4];
		struct
		{
			uint8_t LSB;   // Low
			uint8_t MSB;   // Middle
			uint8_t HSB;   // High
			uint8_t CHECK; // Checksum
		};
		uint32_t Bit32;

	public:
		Bit_List()
		{
			LSB = MSB = HSB = CHECK = 0;
		}

		Bit_List(const uint8_t _LSB, const uint8_t _MSB, const uint8_t _HSB, const uint8_t _CHECK = 0)
		{
			LSB = _LSB;
			MSB = _MSB;
			HSB = _HSB;
			CHECK = _CHECK;
		}

		Bit_List(const uint32_t val)
		{
			Bit32 = val;
		}

		void Clear(void)
		{
			LSB = MSB = HSB = CHECK = 0;
		}

		uint32_t ToInt(void)
		{
			return Bit32;
		}

} Bit_List;

/**
 * Structure masquant un bit dans un registre
 * PAGE: PAGE0, PAGE16, PAGE17, PAGE18
 * REGISTER: the register in the page
 * BIT: the bit in the register [0..23]
 */
typedef struct
{
	uint8_t PAGE;     // The page
	uint8_t REGISTER; // The register on the page
	uint8_t BIT;      // The numero of the bit in the register
} Reg_Mask;

typedef enum
{
	CIRRUS_RegBit_UnSet,
	CIRRUS_RegBit_Set,
	CIRRUS_RegBit_Error
} CIRRUS_RegBit;

typedef enum
{
	CIRRUS_Do1_Interrupt,
	CIRRUS_Do1_Energy,
	CIRRUS_Do2_Energy,
	CIRRUS_Do3_Energy,
	CIRRUS_Do1_P1Sign,
	CIRRUS_Do1_P2Sign,
	CIRRUS_Do1_V1Zero,
	CIRRUS_Do1_V2Sign,
	CIRRUS_Do_Nothing
} CIRRUS_Do_Action_Simple;

typedef enum
{
	CIRRUS_DO1,
	CIRRUS_DO2,
	CIRRUS_DO3
} CIRRUS_DO_Number;

/**
 * Enumération des actions sur les DO
 */
typedef enum
{
	CIRRUS_DO_Energy,
	CIRRUS_DO_P1Sign,
	CIRRUS_DO_P2Sign,
	CIRRUS_DO_VZero,
	CIRRUS_DO_IZero,
	CIRRUS_DO_Interrupt,
	CIRRUS_DO_Nothing
} CIRRUS_DO_Action;

typedef struct
{
	union
	{
		CIRRUS_DO_Action DO[3];
		struct
		{
				CIRRUS_DO_Action DO1;
				CIRRUS_DO_Action DO2;
				CIRRUS_DO_Action DO3;
		};
	};
} CIRRUS_DO_Struct;

/**
 * Enumération des filtres possibles sur U et I
 */
typedef enum
{
	CIRRUS_NO_Filter = 0,  // Default : 0b00
	CIRRUS_HP_Filter = 1,  // High Pass Filter : 0b01
	CIRRUS_PM_Filter = 2,  // Phase Matching Filter : 0b10
	CIRRUS_ROG_Filter = 3  // Rogowsky Coil Integrator (INT) : 0b11 (uniquement sur I)
} CIRRUS_Filter;

typedef enum
{
	Cirrus_None,
	Cirrus_1,
	Cirrus_2
} CIRRUS_Cirrus;

typedef enum
{
	Channel_1,
	Channel_2
} CIRRUS_Channel;

typedef enum
{
	CIRRUS_SHUNT,
	CIRRUS_CT,
	CIRRUS_ROG
} CIRRUS_Current_Sensor;

typedef enum
{
	CIRRUS_Inst_Voltage1,
	CIRRUS_Inst_Current1,
	CIRRUS_Inst_Power1,
	CIRRUS_Inst_React_Power1,
	CIRRUS_RMS_Voltage1,
	CIRRUS_RMS_Current1,
	CIRRUS_Peak_Voltage1,
	CIRRUS_Peak_Current1,
	CIRRUS_Active_Power1,
	CIRRUS_Reactive_Power1,
	CIRRUS_Apparent_Power1,
	CIRRUS_Power_Factor1,
	// Deuxième channel
#ifdef CIRRUS_CS5480
	CIRRUS_Inst_Voltage2,
	CIRRUS_Inst_Current2,
	CIRRUS_Inst_Power2,
	CIRRUS_Inst_React_Power2,
	CIRRUS_RMS_Voltage2,
	CIRRUS_RMS_Current2,
	CIRRUS_Peak_Voltage2,
	CIRRUS_Peak_Current2,
	CIRRUS_Active_Power2,
	CIRRUS_Reactive_Power2,
	CIRRUS_Apparent_Power2,
	CIRRUS_Power_Factor2,
#endif
	CIRRUS_Total_Active_Power,
	CIRRUS_Total_Reactive_Power,
	CIRRUS_Total_Apparent_Power,
	CIRRUS_Frequency
} CIRRUS_Data;

typedef enum
{
	CIRRUS_OK = 0x00U,
	CIRRUS_ERROR = 0x01U,
	CIRRUS_BUSY = 0x02U,
	CIRRUS_TIMEOUT = 0x03U,
	CIRRUS_READY_TIMEOUT = 0x04U
} CIRRUS_State_typedef;

/**
 * Structure contenant les valeurs de calibration
 * Scale factor : U et I
 * Gain : U et I
 * AC Offset : U
 * No Load Offset : P et Q
 */
typedef struct CIRRUS_Calib_typedef
{
	// Scale factor
	float V1_Calib;
	float I1_MAX;
	// Gain AC
	uint32_t V1GAIN, I1GAIN;
	// AC Offset
	uint32_t I1ACOFF;
	// No Load Offset
	uint32_t P1OFF, Q1OFF;

	// Deuxième channel
#ifdef CIRRUS_CS5480
  // Scale factor
  float V2_Calib;
  float I2_MAX;
  // Gain AC
  uint32_t V2GAIN, I2GAIN;
  // AC Offset
  uint32_t I2ACOFF;
  // No Load Offset
  uint32_t P2OFF, Q2OFF;
#endif
} CIRRUS_Calib_typedef;

#ifdef CIRRUS_CS5480
#define CS_CALIB0	{0.0, 0.0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0}
#else
#define CS_CALIB0	{0.0, 0.0, 0, 0, 0, 0, 0}
#endif

/**
 * Structure contenant les principales valeurs de configuration
 * Config register : config0 pour le gain, config1 pour le DO et Pulse, config2 pour les filtres
 * Pulse register : paramètres du pulse de métrologie
 */
typedef struct CIRRUS_Config_typedef
{
	// Config register
	uint32_t config0, config1, config2;
	// Pulse register
	uint32_t P_width, P_rate, P_control;
} CIRRUS_Config_typedef;

#define CS_CONFIG0	{0, 0, 0, 0, 0, 0}

/**
 * @brief  GPIO Bit SET and Bit RESET enumeration
 */
typedef enum
{
	GPIO_PIN_RESET = LOW,
	GPIO_PIN_SET = HIGH
} GPIO_PinState;

/**
 * @brief  HAL Status structures definition
 */
typedef enum
{
	HAL_OK = 0x00U,
	HAL_ERROR = 0x01U,
	HAL_BUSY = 0x02U,
	HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

//// Un compteur des passages du zéro
//extern volatile uint32_t Zero_Cirrus;
//// Un top à chaque période (2 zéro)
//extern volatile uint8_t CIRRUS_Top_Period;

// Indique l'état du Cirrus
extern volatile CIRRUS_State_typedef CIRRUS_Last_Error;

// Le cirrus actif
extern volatile CIRRUS_Cirrus CIRRUS_Selected;

void CIRRUS_Interrupt_DO_Action_Tore();
void CIRRUS_Interrupt_DO_Action_SSR();

void CIRRUS_UART_Init(void *puart, uint8_t RESET_Pin, uint32_t baud);
void CIRRUS_SPI_Init(void *pspi, uint8_t RESET_Pin);
void CIRRUS_Select_Init(uint8_t CS1_Pin, uint8_t CS2_Pin = 0);
void CIRRUS_Calibration(CIRRUS_Calib_typedef *calib);
void CIRRUS_Configuration(uint32_t sample_count_ms, CIRRUS_Config_typedef *config, bool start);
void CIRRUS_Get_Parameters(CIRRUS_Calib_typedef *calib, CIRRUS_Config_typedef *config);
void CIRRUS_GetScale(float *scale);
void CIRRUS_SetScale(float *scale);
bool CIRRUS_TryConnexion(void);

void CIRRUS_Load_From_FLASH(CIRRUS_Cirrus cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config);
void CIRRUS_Save_To_FLASH(CIRRUS_Cirrus cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config);
void CIRRUS_Register_To_FLASH(void);

void CIRRUS_DO_Configuration(CIRRUS_DO_Struct DO_struct);
//void CIRRUS_UpdateTime(void);
void CIRRUS_UART_Change_Baud(uint32_t baud);
void CIRRUS_Select(CIRRUS_Cirrus cirrus, bool with_scale = true);
void CIRRUS_UnSelect();

void CIRRUS_Hard_Reset(void);
void CIRRUS_Soft_reset(void);
void CIRRUS_Chip_Reset(GPIO_PinState PinState);

bool CIRRUS_Is_Data_Ready(void);
const char* CIRRUS_Print_LastError();

void CIRRUS_Print_FullData();
void CIRRUS_Print_Calib(CIRRUS_Calib_typedef *calib);
void CIRRUS_Print_Config(CIRRUS_Config_typedef *config);

// ********************************************************************************
// Utilitary functions
// ********************************************************************************

// print prototype
void CIRRUS_print_str(const char *str);
void CIRRUS_print_int(const char *str, uint32_t integer);
void CIRRUS_print_float(const char *str, float val);
void CIRRUS_print_blist(const char *str, Bit_List *list, uint8_t size);
void CIRRUS_print_bin(uint8_t val);

// blist operation
Bit_List* create_blist(uint8_t LSB, uint8_t MSB, uint8_t HSB, Bit_List *list);
Bit_List* clear_blist(Bit_List *list);
Bit_List* copy_blist(Bit_List *src, Bit_List *dest, bool full);
void print_blist(char *str, Bit_List *list, uint8_t size);

Bit_List* int_to_blist(uint32_t integer, Bit_List *list);
uint32_t blist_to_int(Bit_List *bt);
float twoscompl_to_real(Bit_List *twoscompl);
Bit_List* real_to_twoscompl(float num, Bit_List *list);

// Checksum
void set_checksum_validation(void);
void reset_checksum_validation(void);
bool detect_comm_csum_mode(bool *error);
void add_comm_checksum(Bit_List *thedata);
Bit_List* sub_comm_checksum(Bit_List *thedata);

// Basic functions
bool read_register(uint8_t register_no, uint8_t page_no, Bit_List *result);
void write_register(uint8_t register_no, uint8_t page_no, Bit_List *thedata);
void send_instruction(uint8_t instruction);

void start_conversion(void);
void stop_conversion(void);
void single_conversion(void);
void softsoft_reset(void);

CIRRUS_RegBit temp_updated(void);
CIRRUS_RegBit rx_timeout();
CIRRUS_RegBit rx_checksum_err(void);
CIRRUS_RegBit invalid_cmnd();
bool CIRRUS_data_ready(void);
void CIRRUS_clear_data_ready(void);
bool CIRRUS_wait_for_ready(bool clear_ready);
bool CIRRUS_Check_Positive_Power(CIRRUS_Channel channel);
uint8_t pivor(void);
CIRRUS_RegBit ioc(void);
CIRRUS_RegBit tod(void);
void set_ipga(CIRRUS_Current_Sensor sensor);
void set_highpass_filter(bool vhpf, bool ihpf);
void set_apcm(bool apcm);
void set_mcfg(bool mcfg);
void set_filter(CIRRUS_Filter V1, CIRRUS_Filter I1, CIRRUS_Filter V2, CIRRUS_Filter I2);

CIRRUS_RegBit get_bitmask(Reg_Mask b_mask);
void clear_bitmask(Reg_Mask b_mask);
void interrupt_bitmask(uint8_t bit);

// Get data functions
float get_instantaneous_voltage(void);
float get_instantaneous_current(void);
float get_instantaneous_power(void);
float get_instantaneous_quadrature_power(void);
float get_rms_voltage(void);
float get_rms_current(void);
float get_average_power(void);
float get_average_reactive_power(void);
float get_peak_voltage(void);
float get_peak_current(void);
float get_apparent_power(void);
float get_power_factor(void);
float get_sum_active_power(void);
float get_sum_apparent_power(void);
float get_sum_reactive_power(void);
bool CIRRUS_get_system_time(uint32_t *cstime);
float CIRRUS_get_temperature(void);
float CIRRUS_get_frequency(void);
bool CIRRUS_get_rms_data(volatile float *uRMS, volatile float *pRMS);
bool CIRRUS_get_rms_data(float *uRMS, float *iRMS, float *pRMS, float *CosPhi);
float CIRRUS_get_data(CIRRUS_Data _data, volatile float *result);

// Set settings
void set_settle_time(uint32_t owr_samples);
void set_sample_count(uint32_t N);
void set_calibration_scale(float scale);

// Calibration do
void do_dc_offset_calibration(bool only_I);
void do_ac_offset_calibration(void);
void do_gain_calibration(float calib_vac, float calib_r);
void set_phase_compensations(void);
void do_noload_power_calibration();
void set_temp_calibrations(void);

// Communication UART/Wifi
#ifdef LOG_CIRRUS_CONNECT
uint8_t UART_Message_Cirrus(uint8_t *RxBuffer);
char *CIRRUS_COM_Scale(uint8_t *Request, float *scale, char *response);
char *CIRRUS_COM_Register(uint8_t *Request, char *response);
char *CIRRUS_COM_Register_Multi(uint8_t *Request, char *response);
bool CIRRUS_COM_ChangeBaud(uint8_t *Baud, char *response);
char *CIRRUS_COM_Flash(char *response);
#endif

