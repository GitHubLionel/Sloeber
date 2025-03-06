#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <deque>
#include "CIRRUS_define.h"

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

/**
 * For FLASH
 */
//#define CIRRUS_FLASH

// Spécifique UART
#ifdef CIRRUS_USE_UART
// define SoftwareSerial = 0 or HardwareSerial = 1 mode
#define CIRRUS_UART_HARD	1

/**
 * TimeOut definition for uart and spi communication
 */
#define TIMEOUT600	100	// Timeout for 600 baud, should always be enough
#define TIMEOUT512K	8	  // Timeout for >= 128 kbaud, 5 is good for CS5490 but depend of the insulator

// Delay for the read/write register in ms. May be increased if necessary
#define READ_REGISTER_DELAY_MS 100 // Not used
#define WRITE_REGISTER_DELAY_MS 64 // 128 ms is the max time out serial reset

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

// ******************************************************************
// Type definition
// ******************************************************************
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
			LSB = MSB = HSB = CHECK = 0x00;
		}

		Bit_List(const uint8_t _LSB, const uint8_t _MSB, const uint8_t _HSB,
				const uint8_t _CHECK = 0x00)
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
			LSB = MSB = HSB = CHECK = 0x00;
		}

		uint32_t ToInt(void)
		{
			return Bit32;
		}

		void Negate(void)
		{
//			LSB = LSB | 0x80;
//			MSB = MSB | 0x80;
//			HSB = HSB | 0x80;
			Bit32 = Bit32 | 0x800000;
			CHECK = 0x00;
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

//typedef enum
//{
//	CIRRUS_Do1_Interrupt,
//	CIRRUS_Do1_Energy,
//	CIRRUS_Do2_Energy,
//	CIRRUS_Do3_Energy,
//	CIRRUS_Do1_P1Sign,
//	CIRRUS_Do1_P2Sign,
//	CIRRUS_Do1_V1Zero,
//	CIRRUS_Do1_V2Sign,
//	CIRRUS_Do_Nothing
//} CIRRUS_Do_Action_Simple;
//
//typedef enum
//{
//	CIRRUS_DO1,
//	CIRRUS_DO2,
//	CIRRUS_DO3
//} CIRRUS_DO_Number;

/**
 * Enumération des actions sur les DO
 */
typedef enum
{
	CIRRUS_DO_EPG1 = 0,
	CIRRUS_DO_EPG2,
	CIRRUS_DO_EPG3,
	CIRRUS_DO_EPG4,
	CIRRUS_DO_P1Sign,
	CIRRUS_DO_P2Sign,
	CIRRUS_DO_PSumSign,
	CIRRUS_DO_Q1Sign,
	CIRRUS_DO_Q2Sign,
	CIRRUS_DO_QSumSign,
	CIRRUS_DO_Reserved1,
	CIRRUS_DO_VZero,
	CIRRUS_DO_IZero,
	CIRRUS_DO_Reserved2,
	CIRRUS_DO_Hi_Z,
	CIRRUS_DO_Interrupt
} CIRRUS_DO_Mode;

typedef enum
{
	CIRRUS_OFF = 0,
	CIRRUS_ON
} CIRRUS_DO_OnOff;

typedef struct
{
		union
		{
				CIRRUS_DO_OnOff EPG[4] = {CIRRUS_OFF};
				struct
				{
						CIRRUS_DO_OnOff EPG1;
						CIRRUS_DO_OnOff EPG2;
						CIRRUS_DO_OnOff EPG3;
						CIRRUS_DO_OnOff EPG4;
				};
		};
		union
		{
				CIRRUS_DO_OnOff DO[4] = {CIRRUS_OFF};
				struct
				{
						CIRRUS_DO_OnOff DO1;
						CIRRUS_DO_OnOff DO2;
						CIRRUS_DO_OnOff DO3;
						CIRRUS_DO_OnOff DO4;
				};
		};
		union
		{
				CIRRUS_DO_Mode DO_Mode[4]= {CIRRUS_DO_Hi_Z};
				struct
				{
						CIRRUS_DO_Mode DO_Mode1;
						CIRRUS_DO_Mode DO_Mode2;
						CIRRUS_DO_Mode DO_Mode3;
						CIRRUS_DO_Mode DO_Mode4;
				};
		};
} Config1_Struct_typedef;

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
	Channel_1,
	Channel_2,
	Channel_all,
	Channel_none
} CIRRUS_Channel;

typedef enum
{
	CIRRUS_SHUNT,
	CIRRUS_CT,
	CIRRUS_ROG
} CIRRUS_Current_Sensor;

typedef enum
{
	// Specific channel
	CIRRUS_Inst_Voltage,
	CIRRUS_Inst_Current,
	CIRRUS_Inst_Power,
	CIRRUS_Inst_React_Power,
	CIRRUS_RMS_Voltage,
	CIRRUS_RMS_Current,
	CIRRUS_Peak_Voltage,
	CIRRUS_Peak_Current,
	CIRRUS_Active_Power,
	CIRRUS_Reactive_Power,
	CIRRUS_Apparent_Power,
	CIRRUS_Power_Factor,
	// Common channel
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

typedef enum
{
	FOR_WRITE,
	FOR_READ
} CIRRUS_Reg_Operation;

typedef union
{
		float tab[3];
		struct
		{
				float V_SCALE = 0.0;
				float I_SCALE = 0.0;
				float P_SCALE = 0.0;
		};
} CIRRUS_Scale_typedef;

typedef enum
{
	gain_Full_IUScale,
	gain_Full_UScale,
	gain_IUScale
} Cirrus_DoGain_typedef;

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
		float V1_Calib = 0;
		float I1_MAX = 0;
		// Gain AC
		uint32_t V1GAIN = 0, I1GAIN = 0;
		// AC Offset
		uint32_t I1ACOFF = 0;
		// No Load Offset
		uint32_t P1OFF = 0, Q1OFF = 0;

		// Deuxième channel
		// Scale factor
		float V2_Calib = 0;
		float I2_MAX = 0;
		// Gain AC
		uint32_t V2GAIN = 0, I2GAIN = 0;
		// AC Offset
		uint32_t I2ACOFF = 0;
		// No Load Offset
		uint32_t P2OFF = 0, Q2OFF = 0;
} CIRRUS_Calib_typedef;

#define CS_CALIB0	{0.0, 0.0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0}

/**
 * Structure contenant les principales valeurs de configuration
 * Config register : config0 pour le gain, config1 pour le DO et Pulse, config2 pour les filtres
 * Pulse register : paramètres du pulse de métrologie
 */
typedef struct CIRRUS_Config_typedef
{
		// Config register
		uint32_t config0 = 0, config1 = 0, config2 = 0;
		// Pulse register
		uint32_t P_width = 0, P_rate = 0, P_control = 0;
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

/**
 * Common request enumeration
 */
typedef enum
{
	csw_REG,
	csw_SCALE,
	csw_REG_MULTI,
	csw_BAUD,
	csw_CS,
	csw_LOCK,
	csw_FLASH,
	csw_GAIN,
	csw_IACOFF,
	csw_PQOFF,
	csw_NONE
} CS_Common_Request;

/**
 * RMS data struct.
 * Contain Voltage, Current, Power and Energy by day
 */
#ifdef CIRRUS_RMS_FULL

typedef enum
{
	dat_Voltage,
	dat_Current,
	dat_ActivePower,
	dat_ApparentPower,
	dat_ReactivePower,
	dat_PowerFactor,
	dat_Frequency,
	dat_Temperature,
	dat_Energy
} Data_List;

typedef struct RMS_Data
{
		union
		{
				float tab[dat_Energy] = {0};
				struct
				{
						float Voltage;
						float Current;
						float ActivePower;
						float ApparentPower;
						float ReactivePower;
						float PowerFactor;
						float Frequency;
						float Temperature;
						float Energy;
				};
		};

	public:
		RMS_Data()
		{
		}
		/// assignment operator. Return this if we chain affectation
//		RMS_Data& operator =(const RMS_Data &data)
//		{
//			for (int i = 0; i <= dat_Energy; i++)
//				tab[i] = data.tab[i];
//			return *this;
//		}

		inline RMS_Data& operator +=(const RMS_Data &data)
		{
			for (int i = 0; i < dat_Energy; i++)
				tab[i] += data.tab[i];
			return *this;
		}

		inline RMS_Data& operator /=(const float val)
		{
			float inv = 1.0 / val;
			for (int i = 0; i < dat_Energy; i++)
				tab[i] *= inv;
			return *this;
		}

		inline RMS_Data operator /(float const val) const
		{
//			float inv = 1.0 / val;
			RMS_Data dat = *this;
			dat /= val;
			return dat;
//			return RMS_Data(Voltage * inv, Current * inv, Power * inv);
		}

		inline RMS_Data& Zero(void)
		{
			for (int i = 0; i <= dat_Energy; i++)
				tab[i] = 0;
			return *this;
		}
} RMS_Data_type;
#else
typedef struct RMS_Data
{
		float Voltage;
		float ActivePower;
		float Energy;
		float Temperature;

	public:
		RMS_Data(float u = 0.0, float e = 0.0) :
				Voltage(u), ActivePower(e)
		{
			Energy = 0.0;
			Temperature = 0.0;
		}

		inline RMS_Data& operator +=(const RMS_Data &data)
		{
			Voltage += data.Voltage;
			ActivePower += data.ActivePower;
			return *this;
		}

		inline RMS_Data& operator /=(const float val)
		{
			float inv = 1.0 / val;
			Voltage *= inv;
			ActivePower *= inv;
			return *this;
		}

		inline RMS_Data operator +(RMS_Data const &data) const
		{
			return RMS_Data(Voltage + data.Voltage, ActivePower + data.ActivePower);
		}

		inline RMS_Data operator /(float const val) const
		{
			float inv = 1.0 / val;
			return RMS_Data(Voltage * inv, ActivePower * inv);
		}

		inline RMS_Data& Zero(void)
		{
			Voltage = ActivePower = Energy = Temperature = 0.0;
			return *this;
		}
} RMS_Data_type;
#endif

//// Un compteur des passages du zéro
//extern volatile uint32_t Zero_Cirrus;
//// Un top à chaque période (2 zéro)
//extern volatile uint8_t CIRRUS_Top_Period;

void CIRRUS_Interrupt_DO_Action_Tore();
void CIRRUS_Interrupt_DO_Action_SSR();

#ifdef CIRRUS_FLASH
void CIRRUS_Load_From_FLASH(char id_cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config);
void CIRRUS_Save_To_FLASH(char id_cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config);
#endif

#define CSDelay(delay_ms)	delay(delay_ms);

// Forward class definition
class CIRRUS_Base;

/**
 * Define of a list of Cirrus.
 * Usefull when we have several Cirrus
 */
typedef std::deque<CIRRUS_Base*> CirrusList;

// Callback pour le changement de jour : pour minuit et la mise à jour date
typedef void (*CIRRUS_selectchange_cb)(CIRRUS_Base &cirrus);

// Callback for Zero cross
typedef void (*onZCcallback)(void);

/**
 * Cirrus Communication class
 * The purpose of this class is to manage the communication with the Cirrus in UART or SPI (with the define)
 */
class CIRRUS_Communication
{
	public:
		CIRRUS_Communication()
		{
		}
		CIRRUS_Communication(void *com, uint8_t RESET_Pin);

		// Initialization
		void Initialize(void *com, uint8_t RESET_Pin);
		void begin(void);
		bool IsStarted(void) const
		{
			return Comm_started;
		}

#ifdef CIRRUS_USE_UART
		// Specific to UART
		void UART_Change_Baud(uint32_t baud);
		void ClearBuffer(void);
#endif

		// Transmit and receive data
		CIRRUS_State_typedef Transmit(uint8_t *pData, uint16_t Size);
		CIRRUS_State_typedef Receive(Bit_List *pResult, uint8_t Size = 3);

		// Reset
		void Chip_Reset(GPIO_PinState PinState);
		void Hard_Reset(void);

		// Current Cirrus used
		void SetCurrentCirrus(CIRRUS_Base *cirrus)
		{
			CurrentCirrus = cirrus;
		}
		CIRRUS_Base* GetCurrentCirrus()
		{
			return CurrentCirrus;
		}

		// Return the number of Cirrus present
		uint8_t GetNumberCirrus(void);

		// Ensure the managment of several Cirrus
		void AddCirrus(CIRRUS_Base *cirrus, uint8_t select_Pin = 0);
		void DisableCirrus(uint8_t select_Pin);
		CIRRUS_Base* SelectCirrus(uint8_t position, CIRRUS_Channel channel = Channel_none);
		CIRRUS_Base* GetCirrus(int position);
		int8_t GetSelectedID(void)
		{
			return Selected;
		}

		String Handle_Common_Request(CS_Common_Request Common_Request, char *Request,
				CIRRUS_Calib_typedef *CS_Calib, CIRRUS_Config_typedef *CS_Config);

#ifdef CIRRUS_FLASH
		void Register_To_FLASH(char id_cirrus);
#endif

		/**
		 * Set the callback when we change the Cirrus
		 * Should be used only if we have several Cirrus
		 */
		void setCirrusChangeCallback(const CIRRUS_selectchange_cb &callback)
		{
			SelectChange_cb = callback;
		}

		void COM_ChangeCirrus(uint8_t *Request);

		// Communication UART with Cirrus_Connect or Wifi with CIRRUS_Config
#ifdef LOG_CIRRUS_CONNECT
		void Do_Lock_IHM(bool op);
		bool Is_IHM_Locked(void)
		{
			return (CIRRUS_Lock_IHM > 0);
		}

		uint8_t UART_Message_Cirrus(uint8_t *RxBuffer);
		char* COM_Scale(uint8_t *Request, float *scale, char *response);
		char* COM_Register(uint8_t *Request, char *response);
		char* COM_Register_Multi(uint8_t *Request, char *response);
		bool COM_ChangeBaud(uint8_t *Baud, char *response);
#ifdef CIRRUS_FLASH
		char* COM_Flash(char id_cirrus, char *response);
#endif
#endif

		bool Get_rms_data(float *uRMS, float *pRMS);

	private:
#ifdef CIRRUS_USE_UART
		// UART handle initialization
		CIRRUS_SERIAL_MODE *Cirrus_UART = NULL;

		// Le timeout pour les opérations uart
		uint32_t Cirrus_TimeOut = TIMEOUT600;
#else
		// SPI handle initialization
		SPIClass *Cirrus_SPI = NULL;
		// 2 MHz, MSB first, clock polarity high, clock phase 2 edge
		SPISettings spisettings = SPISettings(2000000, MSBFIRST, SPI_MODE3);  // A priori mode 1 ou 3 SPI_MODE3
#endif

		// The current Cirrus
		CIRRUS_Base *CurrentCirrus = NULL;

		// Cirrus Reset pin
		uint8_t Cirrus_RESET_Pin = 0;

		// The list of the Cirrus in case we have several Cirrus
		CirrusList m_Cirrus;
		// The number of the current Cirrus in the Cirrus list
		int8_t Selected = -1;
		// The GPIO pin of the current Cirrus
		uint8_t Selected_Pin = 0;

		// Flag to know if we have started the communication
		bool Comm_started = false;

		// Callback when we change Cirrus in case we have several Cirrus
		CIRRUS_selectchange_cb SelectChange_cb = NULL;

#ifdef LOG_CIRRUS_CONNECT
		// Une commande pour le Cirrus est en attente de traitement
		bool CIRRUS_Command = false;

		// To stop data acquisition for log and IHM
		int CIRRUS_Lock_IHM = 0;
#endif
};

/**
 * Config1 register class
 */
class Config1_Register
{
	public:
		Config1_Register(void);
		Config1_Register(uint32_t config1_hex);

		void SetConfig1(uint32_t config1_hex);
		uint32_t GetConfig1(void)
		{
			return config1;
		}

		void SetEPG(uint8_t id, CIRRUS_DO_OnOff state);
		void SetDO(uint8_t id, CIRRUS_DO_OnOff state);
		void SetDO_Mode(uint8_t id, CIRRUS_DO_Mode mode);

		char *Print_Config1(char *mess);

	private:
		uint32_t config1;
		Config1_Struct_typedef config1_struct;
		void DO_Config1(Config1_Struct_typedef DO_struct);
		void Create_Config1_Struct(void);
};

/**
 * Cirrus Base class
 */
class CIRRUS_Base
{
	public:
		CIRRUS_Base(bool _twochannel)
		{
			twochannel = _twochannel;
			// We select the first channel who always exist
			SelectChannel(Channel_1);
		}
		CIRRUS_Base(CIRRUS_Communication &com, bool _twochannel) :
				CIRRUS_Base(_twochannel)
		{
			SetCommunication(com);
		}
		virtual ~CIRRUS_Base()
		{
		}

		void SetCommunication(CIRRUS_Communication &com);

#ifdef CIRRUS_USE_UART
		bool begin(uint32_t baud, bool change_UART);
#else // SPI
		bool begin();
#endif

		void Calibration(CIRRUS_Calib_typedef *calib);
		void Configuration(uint32_t sample_count_ms, CIRRUS_Config_typedef *config, bool start);
		void Get_Parameters(CIRRUS_Calib_typedef *calib, CIRRUS_Config_typedef *config);
		void GetScale(float *scale);
		void SetScale(float *scale);
		bool TryConnexion();
#ifdef CIRRUS_USE_UART
		bool SetUARTBaud(uint32_t baud, bool change_UART);
		bool set_uart_baudrate(uint32_t baud);
#endif
		virtual const String GetName(void);
		virtual bool IsTwoChannel(void)
		{
			return false;
		}

		void Load_From_FLASH(CIRRUS_Calib_typedef *calib, CIRRUS_Config_typedef *config);
		void Save_To_FLASH(CIRRUS_Calib_typedef *calib, CIRRUS_Config_typedef *config);
		void Register_To_FLASH(void);

		void Soft_reset(void);

		void SelectChannel(CIRRUS_Channel channel = Channel_1);

		bool Is_Data_Ready(void);
		const char* Print_LastError();

		void Print_DataChannel();
		void Print_FullData();
		void Print_Calib(CIRRUS_Calib_typedef *calib);
		void Print_Config(CIRRUS_Config_typedef *config);

		void start_conversion(void);
		void stop_conversion(void);
		void single_conversion(void);
		void softsoft_reset(void);

		// Get data functions
		bool wait_for_data_ready(bool clear_ready);
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
		bool get_system_time(uint32_t *cstime);
		float get_temperature(void);
		float get_frequency(void);
		bool get_rms_data(volatile float *uRMS, volatile float *pRMS);
		bool get_rms_data(float *uRMS, float *iRMS, float *pRMS, float *CosPhi);
		float get_data(CIRRUS_Data _data, float *result);

		bool Check_Positive_Power(CIRRUS_Channel channel);

		// Set settings
		void set_settle_time_ms(float owr_samples);
		void set_sample_count(uint32_t N);
		void set_calibration_scale(float scale);

		// print prototype
		void print_str(const char *str);
		void print_int(const char *str, uint32_t integer);
		void print_float(const char *str, float val);
		void print_blist(const char *str, Bit_List *list, uint8_t size);
		void print_bin(uint8_t val);

		// Basic functions
		bool read_register(uint8_t register_no, uint8_t page_no, Bit_List *result);
		void write_register(uint8_t register_no, uint8_t page_no, Bit_List *thedata);
		void send_instruction(uint8_t instruction);

		// Calibration set
		void set_gain_calibrations(Bit_List *v_gain, Bit_List *i_gain, CIRRUS_Channel channel);
		void set_dc_offset_calibrations(Bit_List *v_off, Bit_List *i_off, CIRRUS_Channel channel);
		void set_Iac_offset_calibrations(Bit_List *i_acoff, CIRRUS_Channel channel);
		void set_power_offset_calibrations(Bit_List *p_off, Bit_List *q_off, CIRRUS_Channel channel);
		void set_phase_compensations(void);
		void set_temp_calibrations(void);

		// Calibration do
		void do_gain_calibration(Cirrus_DoGain_typedef dogain, float param1 = -1, float param2 = -1);
		void do_Iac_offset_calibration(void);
		void do_dc_offset_calibration(bool only_I);
		void do_power_offset_calibration();

		// The pin to select the Cirrus in case we have several Cirrus
		uint8_t Cirrus_Pin = 0;

		// Zero cross initialization
		// ZC callback must be declared with IRAM_ATTR directive
		void ZC_Initialize(uint8_t ZC_Pin, onZCcallback onZC);

	protected:

// ********************************************************************************
// Utilitary functions
// ********************************************************************************

		// blist operation
		Bit_List* create_blist(uint8_t LSB, uint8_t MSB, uint8_t HSB, Bit_List *list);
		Bit_List* clear_blist(Bit_List *list);
		Bit_List* copy_blist(Bit_List *src, Bit_List *dest, bool full);

		Bit_List* int_to_blist(uint32_t integer, Bit_List *list);
		uint32_t blist_to_int(const Bit_List *bt);
		float twoscompl_to_real(const Bit_List *twoscompl);
		Bit_List* real_to_twoscompl(float num, Bit_List *list);

		// Checksum
		void set_checksum_validation(void);
		void reset_checksum_validation(void);
		bool detect_comm_csum_mode(bool *error);
		void add_comm_checksum(Bit_List *thedata);
		Bit_List* sub_comm_checksum(Bit_List *thedata);

		// Register access and communication
		void send(Bit_List *msg, uint8_t size);
		void select_page(uint8_t page_no);
		void select_register(uint8_t register_no, CIRRUS_Reg_Operation operation);

		CIRRUS_RegBit temp_updated(void);
		CIRRUS_RegBit rx_timeout();
		CIRRUS_RegBit rx_checksum_err(void);
		CIRRUS_RegBit invalid_cmnd();
		bool data_ready(void);
		void clear_data_ready(void);

		void CorrectBug(void);
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

		Bit_List config0_default = 0xC02000; // For CS5490 and CS5480, 0x400000 for CS5484

	private:
		// Communication
		CIRRUS_Communication *Com = NULL;

		//Page 0 registers.
		uint8_t P0_V_PEAK;
		uint8_t P0_I_PEAK;

		//Page 16 registers.
		uint8_t P16_I;
		uint8_t P16_V;
		uint8_t P16_P;
		uint8_t P16_P_AVG;
		uint8_t P16_I_RMS;
		uint8_t P16_V_RMS;
		uint8_t P16_Q_AVG;
		uint8_t P16_Q;
		uint8_t P16_S;
		uint8_t P16_PF;
		// Config parameters
//		uint8_t P16_I_DCOFF;
//		uint8_t P16_I_GAIN;
//		uint8_t P16_V_DCOFF;
//		uint8_t P16_V_GAIN;
//		uint8_t P16_P_OFF;
//		uint8_t P16_I_ACOFF;
//		uint8_t P16_Q_OFF;

		//Page 17 registers.
		uint8_t P17_VSag_DUR;
		uint8_t P17_VSag_LEVEL;
		uint8_t P17_IOver_DUR;
		uint8_t P17_IOver_LEVEL;

		//Page 18 registers.
		uint8_t P18_VSweil_DUR;
		uint8_t P18_VSweil_LEVEL;

		// Scale factors
		// Pointer to the current scale : Scale_ch1 or Scale_ch2
		CIRRUS_Scale_typedef *Scale = NULL;
		// Scale for channel 1 and channel 2
		CIRRUS_Scale_typedef Scale_ch1;
		CIRRUS_Scale_typedef Scale_ch2;

		// Channel
		bool twochannel = false;
		CIRRUS_Channel currentchannel = Channel_none;

		bool comm_checksum = false;    // Booléen Vrai/Faux. Define USE_CHECKSUN must be defined
		bool csResponse = false;
		uint8_t current_selected_page = 20; // La page en cours sélectionnée, évite de la redemander au Cirrus
		bool Conversion_Running = false; // La conversion est-elle en cours
		uint8_t read_data_nb; // Nombre d'octet lu (utile pour la méthode detect_comm_csum_mode)
		bool IS_READ_REG = false;
		CIRRUS_State_typedef CIRRUS_Last_Error = CIRRUS_OK;

		// Le taux d'échantillonnage. Permet de déterminer le temps maximum à
		// attendre pour que le Cirrus soit ready : Ready_TimeOut
		uint32_t Sample_Count_ms = 25;
		uint32_t Ready_TimeOut = 25 + 10;

		// Old values
		float old_temp = 0.0;
		float old_freq = 0.0;
		float old_psumreact = 0.0;
		float old_psumapp = 0.0;
		float old_psumact = 0.0;

		void data_reset(void);

		// Calibration
		typedef enum
		{
			Base_real,
			Base_22,
			Base_23,
			Base_24,
			Base_d24
		} Print_Base_def;
		void print_calibration(const char *message, Print_Base_def base, const Bit_List &val);
		bool CheckPivor(void);
};

// ********************************************************************************
// Basic initialization
// Just the minimum operation, no IHM
// ********************************************************************************
bool CIRRUS_Basic_Initialization(CIRRUS_Base &Cirrus, CIRRUS_Calib_typedef *CS_Calib,
		CIRRUS_Config_typedef *CS_Config, bool print_data);

// ********************************************************************************
// RMSData class
// ********************************************************************************

/**
 * The list of extra data
 */
typedef enum
{
	exd_Null = 0,
	exd_PApparent = 1,
	exd_PReactive = 2,
	exd_PF = 4,
	exd_Frequency = 8
} ExtraData_typedef;

/**
 * RMSData class
 * Get U, I and P RMS data
 */
class CIRRUS_RMSData
{
	public:
		CIRRUS_RMSData(bool temperature = true)
		{
			_temperature = temperature;
			_log_time = _last_time = _ref_time = millis();
		}
		CIRRUS_RMSData(CIRRUS_Base *parent, bool temperature = true) :
				CIRRUS_RMSData(temperature)
		{
			Cirrus = parent;
		}
		~CIRRUS_RMSData()
		{
		}

		void SetParent(CIRRUS_Base *parent)
		{
			Cirrus = parent;
		}

		void SetTemperature(bool temp)
		{
			_temperature = temp;
		}

		void SetLogTime(uint32_t second)
		{
			_log_time_ms = second * 1000;
		}

		/**
		 * Extra data to calculate
		 * Param extra is a combinaison of ExtraData_typedef
		 * For example : extra = exd_PF | exd_Frequency to have power factor and frequency
		 * By default, no extra data is calculated
		 */
		void SetWantData(uint16_t extra)
		{
			_ExtraData = extra;
		}

		void RestartEnergy(float conso = 0.0, float surplus = 0.0)
		{
			energy_day_conso = conso;
			energy_day_surplus = surplus;
		}

		/**
		 * Main function that get RMS data and others parameters.
		 * Compute the energy and the mean for the log.
		 * Return true and set flag log available if mean is complete.
		 */
		bool GetData(bool reset_ready = true);

		float GetURMS() const
		{
			return _inst_data.Voltage;
		}

		float GetIRMS() const
		{
#ifdef CIRRUS_RMS_FULL
			return _inst_data.Current;
#else
			return 0;
#endif
		}

		float GetPRMSSigned() const
		{
			return _inst_data.ActivePower;
		}

		/**
		 * Return the last mean of P RMS
		 * This is usefull when you have several Cirrus and need to compute energy
		 */
		float GetlastMeanPRMSSigned() const
		{
			return _last_mean_active_power;
		}

		float GetTemperature() const
		{
			return _inst_data.Temperature;
		}

		/**
		 * Return extra data
		 */
#ifdef CIRRUS_RMS_FULL
		float GetExtraData(ExtraData_typedef extra) const
		{
			double result = 0;
			switch (extra)
			{
				case exd_PApparent: result = _inst_data.ApparentPower; break;
				case exd_PReactive: result = _inst_data.ReactivePower; break;
				case exd_PF: result = _inst_data.PowerFactor; break;
				case exd_Frequency: result = _inst_data.Frequency; break;
				default: ;
			}
			return result;
		}
#endif

		float GetEnergyConso() const
		{
			return energy_day_conso;
		}

		float GetEnergySurplus() const
		{
			return energy_day_surplus;
		}

		/**
		 * Get log data. Flag log available is reseted.
		 * Should be called if GetData() return true
		 */
		RMS_Data GetLog(double *temp)
		{
			_logAvailable = false;
			if (temp != NULL)
				*temp = _log_data.Temperature;
			return _log_data;
		}

		/**
		 * Return error count and reset it
		 */
		uint32_t GetErrorCount(void)
		{
			uint32_t error = _error_count;
			_error_count = 0;
			return error;
		}

	private:
		CIRRUS_Base *Cirrus = NULL;

		// instantaneous values
		RMS_Data _inst_data;

		// log values (mean over _log_time_ms milliseconds)
		RMS_Data _log_data;
		uint32_t _log_time_ms = 120000; // 2 minutes
		bool _logAvailable = false;

		bool _temperature = true;

		// Extra data flag
		uint16_t _ExtraData = exd_Null;

		// la dernière puissance moyenne
		float _last_mean_active_power = 0.0;

		float energy_day_conso = 0.0;
		float energy_day_surplus = 0.0;

		uint32_t _error_count = 0;

		// Variables for GetData (Do not make them static in GetData() function !!)
		RMS_Data _inst_data_cumul;
		RMS_Data _log_cumul_data;

		uint32_t _inst_count = 0;
		uint32_t _log_count = 0;

		unsigned long _ref_time = 0;
		unsigned long _last_time = 0;
		unsigned long _log_time = 0;
};

// ********************************************************************************
// Definition of CS5490, CS548x class
// ********************************************************************************
/**
 * Cirrus CIRRUS_CS5490 class for CS5490 with RMS data
 */

class CIRRUS_CS5490: public CIRRUS_Base
{
	public:
		CIRRUS_CS5490() :
				CIRRUS_Base(false)
		{
			Initialize();
		}
		CIRRUS_CS5490(CIRRUS_Communication &com) :
				CIRRUS_Base(com, false)
		{
			Initialize();
		}
		~CIRRUS_CS5490();
		const String GetName(void);

		/**
		 * Get direct access to RMS data class
		 */
		CIRRUS_RMSData* GetRMSData(void)
		{
			return RMSData;
		}

		void SetLogTime(uint32_t second)
		{
			RMSData->SetLogTime(second);
		}

		void RestartEnergy(float conso = 0.0, float surplus = 0.0)
		{
			RMSData->RestartEnergy(conso, surplus);
		}

		/**
		 * Main function that get the RMS data and others from Cirrus
		 * Get RMS Data and temperature
		 */
		bool GetData(void);

		/**
		 * Return U RMS
		 */
		float GetURMS(void) const
		{
			return RMSData->GetURMS();
		}

		/**
		 * Return I RMS
		 */

		float GetIRMS(void) const
		{
#ifdef CIRRUS_RMS_FULL
			return RMSData->GetIRMS();
#else
			return 0;
#endif
		}

		/**
		 * Return P RMS
		 */
		float GetPRMSSigned(void) const
		{
			return RMSData->GetPRMSSigned();
		}

		/**
		 * Return last mean P RMS
		 */
		float GetLastMeanPRMSSigned(void) const
		{
			return RMSData->GetlastMeanPRMSSigned();
		}

		/**
		 * Return temperature
		 */
		float GetTemperature(void) const
		{
			return RMSData->GetTemperature();
		}

		/**
		 * Return extra data
		 */
#ifdef CIRRUS_RMS_FULL
		float GetExtraData(ExtraData_typedef extra) const
		{
			return RMSData->GetExtraData(extra);
		}
#endif

		/**
		 * Return energies of the day
		 */
		void GetEnergy(float *conso, float *surplus)
		{
			*conso = RMSData->GetEnergyConso();
			*surplus = RMSData->GetEnergySurplus();
		}

		/**
		 * Return log (mean data over 2 minutes). Flag log availble is reseted.
		 */
		RMS_Data GetLog(double *temp) const
		{
			return RMSData->GetLog(temp);
		}

		/**
		 * Return error count
		 */
		uint32_t GetErrorCount(void) const
		{
			return RMSData->GetErrorCount();
		}

	protected:
		void Initialize();

	private:
		// RMS Data
		CIRRUS_RMSData *RMSData = NULL;
};
#ifndef CIRRUS_USE_UART
#warning "CIRRUS_CS5490 not available with SPI"
#endif

/**
 * Cirrus CIRRUS_CS548x class for CS5480 and CS5484 with RMS data
 */
class CIRRUS_CS548x: public CIRRUS_Base
{
	public:
		CIRRUS_CS548x(bool _isCS5484 = false) :
				CIRRUS_Base(true)
		{
			isCS5484 = _isCS5484;
			Initialize();
		}
		CIRRUS_CS548x(CIRRUS_Communication &com, bool _isCS5484 = false) :
				CIRRUS_Base(com, true)
		{
			isCS5484 = _isCS5484;
			Initialize();
		}
		~CIRRUS_CS548x();
		const String GetName(void);
		virtual bool IsTwoChannel(void)
		{
			return true;
		}

		void SetLogTime(uint32_t second);
		void RestartEnergy(void);
		void RestartEnergy(CIRRUS_Channel channel, float conso = 0.0, float surplus = 0.0);
		bool GetData(CIRRUS_Channel channel);
		float GetURMS(CIRRUS_Channel channel) const;
		float GetIRMS(CIRRUS_Channel channel) const;
		float GetPRMSSigned(CIRRUS_Channel channel) const;
		float GetLastMeanPRMSSigned(CIRRUS_Channel channel) const;
		float GetTemperature(void) const;
#ifdef CIRRUS_RMS_FULL
		float GetExtraData(CIRRUS_Channel channel, ExtraData_typedef extra) const;
		float GetFrequency(void) const;
#endif
		void GetEnergy(CIRRUS_Channel channel, float *conso, float *surplus);
		RMS_Data GetLog(CIRRUS_Channel channel, double *temp);
		uint32_t GetErrorCount(void) const;

		CIRRUS_RMSData* GetRMSData(CIRRUS_Channel channel)
		{
			if (channel == Channel_1)
				return RMSData_ch1;
			else
				if (channel == Channel_2)
					return RMSData_ch2;
				else
					return NULL;
		}

	protected:
		bool isCS5484;
		void Initialize();

	private:
		CIRRUS_RMSData *RMSData_ch1 = NULL;
		CIRRUS_RMSData *RMSData_ch2 = NULL;
};

// ********************************************************************************
// Some basic functions used with IHM
// ********************************************************************************

/**
 * Generic initialization of a cirrus after begin
 */
#ifdef CIRRUS_FLASH
bool CIRRUS_Generic_Initialization(CIRRUS_Base &Cirrus, CIRRUS_Calib_typedef *CS_Calib,
		CIRRUS_Config_typedef *CS_Config, bool print_data, bool Flash_op_load, char Flash_id);
#else
bool CIRRUS_Generic_Initialization(CIRRUS_Base &Cirrus, CIRRUS_Calib_typedef *CS_Calib,
		CIRRUS_Config_typedef *CS_Config, bool print_data, bool Flash_op_load = false, char Flash_id = '1');
#endif
void CIRRUS_Restart(CIRRUS_Base &Cirrus, CIRRUS_Calib_typedef *CS_Calib,
		CIRRUS_Config_typedef *CS_Config);

/**
 * For the general response for a Wifi "/getCirrus" request
 */
String Handle_Cirrus_Wifi_Request(CS_Common_Request Wifi_Request, char *Request);

// To create a basic task to check Cirrus data every 100 ms
#ifdef CIRRUS_USE_TASK
#define CIRRUS_DATA_TASK(start)	{(Task_Condition)(start), "CIRRUS_Task", 6144, 8, CIRRUS_TASK_DELAY, CoreAny, CIRRUS_Task_code}
void CIRRUS_Task_code(void *parameter);
#else
#define CIRRUS_DATA_TASK(start)	{}
#endif

