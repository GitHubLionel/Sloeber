/*
 CIRRUS Energy meter
 Code adapted from the peerpressure project of bastiaan
 Initial file : CS5490.py
 Translated in C for STM32 by L. Reynaud
 Adapted for ESP8266 by L. Reynaud
 */

#include "CIRRUS.h"
#include "CIRRUS_RES_EN.h"

#include <math.h>

typedef enum
{
	FOR_WRITE,
	FOR_READ
} CIRRUS_Reg_Operation;

// Hardware configuration
// SCT013 : turn ration 100A:50mA  50mV
#define SHUNT_R  0.05 // Ohm
#define TORE_RATIO  2000  // 100/0.05
#define VDIV_R1  1.5e6 // Ohm
#define VDIV_R2  1.8e3 // Ohm
#define MCLK  4096000  // Hz clock rate: 4.096 MHz
#define MCLK100  40960 // Hz clock rate: 4.096 MHz div 100
#define sqrt2  1.414213562

//#define V_MAX  (0.25 * (VDIV_R1+VDIV_R2)/VDIV_R2 / sqrt2) // V_RMS (147.5 volt RMS)

// Valeurs mesurées sur proto6-Bis :
//V DC OFFSET is: -0.011920, (104, 121, 254)
//I DC OFFSET is: 0.001029, (180, 33, 0)
//I AC OFFSET is: 0.498169, (249, 135, 127)
Bit_List CALIB_V_DC_OFFSET = {104, 121, 254};
Bit_List CALIB_I_DC_OFFSET = {180, 33, 0};
Bit_List CALIB_I_AC_OFFSET = {249, 135, 127};

// Test radiateur 1000 W, R = 48.4  sur Proto6-Bis :
// V_RMS = 229.00 (V), I_RMS = 4.73 (A), R = 48.40 (Ohm), P_RMS = 1083.49 (W)
// INIT_SCALE_SETTING:  0.0401
// V GAIN is: 0.871775, (40, 203, 55)
// I GAIN is: 0.235367, (65, 16, 15)
Bit_List CALIB_V_GAIN = {40, 203, 55};
Bit_List CALIB_I_GAIN = {65, 16, 15};

// Calibrage
// Input : U_Calib et I_MAX
// V_SCALE = U_Calib/0.6    // V_Inst, V_RMS, V_Peak à corriger
// I_SCALE = I_MAX/0.6      // I_Inst, I_RMS, I_Peak à corriger
// P_SCALE = V_SCALE*I_SCALE

// Tableau des scales V, I et P
// Contient le premier Cirrus channel 1 et 2 puis idem second cirrus
// Pour simplifier, on défini ensuite V, I et P_Scale comme des pointeurs sur le tableau.
float UI_Scale[12] = { 0 };

// Channel 1
#define U1_Calib_default  260.0 // A CORRIGER, valeur par défaut en fonction de la calibration
#define I1_MAX_default  16.5   // A CORRIGER, valeur par défaut en fonction du tore
float *V1_SCALE = NULL;
float *I1_SCALE = NULL;
float *P1_SCALE = NULL;

// Channel 2
#define U2_Calib_default  260.0 // A CORRIGER, valeur par défaut en fonction de la calibration
#define I2_MAX_default  16.5    // A CORRIGER, valeur par défaut en fonction du tore
float *V2_SCALE = NULL;
float *I2_SCALE = NULL;
float *P2_SCALE = NULL;

// Bit_List initialisé à 0
#define Bit_List_Zero  ((Bit_List){0x00, 0x00, 0x00, 0x00})

// Fonction externe qui imprimera les textes là où on veut
extern void PrintTerminal(const char *text);
static char buffer[1500]; // Buffer pour les textes, ATTENTION aux chaines ressources

//#define USE_CHECKSUN
static volatile bool comm_checksum = false;    // Booléen Vrai/Faux
static volatile uint8_t read_data_nb; // Nombre d'octet lu (utile pour la méthode detect_comm_csum_mode)
static volatile uint8_t current_selected_page = 20; // La page en cours sélectionnée, évite de la redemander au Cirrus
static volatile bool Conversion_Running = false; // La conversion est-elle en cours

// Le nombre de Cirrus présent
extern uint8_t CIRRUS_Number;

//// Référence de temps interne
//static volatile uint32_t csReferenceTime = 0;
//
//// Delay function
//static volatile uint16_t Delay_Counter = 0;
//static volatile bool Delay_Wait = false;

static volatile bool csResponse = false;
static volatile bool IS_READ_REG = false;

// Cirrus status
volatile CIRRUS_State_typedef CIRRUS_Last_Error = CIRRUS_OK;

// Le taux d'échantillonnage. Permet de déterminer le temps maximum à
// attendre pour que le Cirrus soit ready : Ready_TimeOut
uint32_t Sample_Count_ms;
uint32_t Ready_TimeOut;

// Pour les conversions
const float over_pow2_23 = 1.0 / (8388608.0 - 1.0);
const float over_pow2_24 = 1.0 / (16777216.0 - 1.0);
const uint32_t pow2_23 = 8388608;
const uint32_t pow2_24 = 16777216;

// ********************************************************************************
// Local functions
// ********************************************************************************
// Communication
extern CIRRUS_State_typedef Cirrus_Transmit(uint8_t *pData, uint16_t Size);
extern CIRRUS_State_typedef Cirrus_Receive(Bit_List *pResult);
#ifdef CIRRUS_USE_UART
extern void Cirrus_ClearBuffer(void);
#endif

void Cirrus_Delay(uint16_t delay_ms);
void data_reset(void);
bool CIRRUS_set_uart_baudrate(uint32_t baud);

// Sélection des scale
void CIRRUS_Select_Scale(CIRRUS_Cirrus cirrus);

// Calibration
void set_gain_calibrations(Bit_List *v_gain, Bit_List *i_gain, uint8_t channel);
void set_dc_offset_calibrations(Bit_List *v_off, Bit_List *i_off, uint8_t channel);
void set_ac_offset_calibrations(Bit_List *i_acoff, uint8_t channel);
void set_no_load_calibrations(Bit_List *p_off, Bit_List *q_off, uint8_t channel);

// Register access and communication
void CIRRUS_send(Bit_List *msg, uint8_t size);
void select_page(uint8_t page_no);
void select_register(uint8_t register_no, CIRRUS_Reg_Operation operation);

// ********************************************************************************
// Initialization
// ********************************************************************************

/**
 * Reference time for TimeOut and Delay function
 * A mettre dans la callback HAL_SYSTICK_Callback()
 */
//void CIRRUS_UpdateTime(void)
//{
//	csReferenceTime++;
//	if (Delay_Wait)
//	{
//		Delay_Wait = (bool) ((--Delay_Counter) != 0);
//	}
//}

void Cirrus_Delay(uint16_t delay_ms)
{
//  Delay_Counter = delay_ms;
//  Delay_Wait = true;
//  while (Delay_Wait) ;
	delay(delay_ms);
}

bool CIRRUS_Start(uint32_t baud)
{
	bool success = true;

	// Pour repartir sur la config de base à 600 bauds
	CIRRUS_Hard_Reset();

	// Sélection du premier Cirrus
	CIRRUS_Select(Cirrus_1, false);

	if (baud != 600)
	{
		// Changement de vitesse
		success = CIRRUS_set_uart_baudrate(baud);

		// Sélection du second Cirrus si présent
		if (success && (CIRRUS_Number == 2))
		{
			CIRRUS_Select(Cirrus_2, false);
			success = CIRRUS_set_uart_baudrate(baud);
		}
	}

	// Aucun Cirrus sélectionné
	CIRRUS_UnSelect();

	return success;
}

/**
 set the baud rate to a specific baud value with the formula :
 br[15:0] = baud * 524288 / MCLK;
 Initial value is 600 baud = 76.8 => 0x4D (registre : 0x02004D)
 */
bool CIRRUS_set_uart_baudrate(uint32_t baud)
{
	Bit_List reg, brb;
	bool success;
	unsigned long br_int;

	success = read_register(P0_SerialCtrl, PAGE0, &reg);
	if (success)
	{
#ifdef DEBUG_CIRRUS_BAUD
    CIRRUS_print_blist(CS_RES_BAUD_INIT, &reg, 3);
    CIRRUS_print_int("baud = ",baud);
#endif
		br_int = ((baud / 100) * 524288) / MCLK100 - 1; // Max 65535 = 0xFFFF at 512 kb
		int_to_blist((uint32_t) br_int, &brb);
		brb.HSB = reg.HSB & 0b00000110;  // Keep RX_PU_OFF et RX_CSUM_OFF
#ifdef DEBUG_CIRRUS_BAUD
    CIRRUS_print_int("br_int = ",br_int);
    CIRRUS_print_blist(CS_RES_BAUD_NEW, &brb, 3);
#endif
		write_register(P0_SerialCtrl, PAGE0, &brb);
#ifdef DEBUG_CIRRUS_BAUD
    CIRRUS_print_str(CS_RES_BAUD_NEW2);
#endif
	}
	else
	{
#ifdef DEBUG_CIRRUS_BAUD
    CIRRUS_print_str(CS_RES_BAUD_NO);
#endif
	}

	if (success)
		data_reset();

	return success;
}

/**
 * Calibration du Cirrus sélectionné en cours (par défaut Cirrus_1)
 * calib: structure contenant les coefficients de calibration
 * Si tableau null, utilise les valeurs par défaut
 */
void CIRRUS_Calibration(CIRRUS_Calib_typedef *calib)
{
	Bit_List c1, c2;

	// Si aucun Cirrus n'est sélectionné, on suppose que c'est le premier
	if (CIRRUS_Selected == Cirrus_None)
	{
		CIRRUS_Selected = Cirrus_1;
		CIRRUS_Select_Scale(CIRRUS_Selected);
	}

	// Définition des facteurs d'échelle
	// Channel 1
	if ((calib == NULL) || (calib->V1_Calib == 0))
	{
		*V1_SCALE = (U1_Calib_default / 0.6);    // U1_Calib
		*I1_SCALE = (I1_MAX_default / 0.6);      // I1_MAX
		*P1_SCALE = ((*V1_SCALE) * (*I1_SCALE));
	}
	else
	{
		*V1_SCALE = (calib->V1_Calib / 0.6);    // U1_Calib
		*I1_SCALE = (calib->I1_MAX / 0.6);      // I1_MAX
		*P1_SCALE = ((*V1_SCALE) * (*I1_SCALE));
	}

	// Channel 2
#ifdef CIRRUS_CS5480
  if ((calib == NULL) || (calib->V2_Calib == 0))
  {
    *V2_SCALE = (U2_Calib_default/0.6);    // U2_Calib
    *I2_SCALE = (I2_MAX_default/0.6);      // I2_MAX
    *P2_SCALE = ((*V2_SCALE) * (*I2_SCALE));
  }
  else
  {
    *V2_SCALE = (calib->V2_Calib/0.6);    // U2_Calib
    *I2_SCALE = (calib->I2_MAX/0.6);      // I2_MAX
    *P2_SCALE = ((*V2_SCALE) * (*I2_SCALE));
  }
#endif

	if ((calib == NULL) || (calib->V1_Calib == 0))
	{
		// Premier channel
		set_gain_calibrations(&CALIB_V_GAIN, &CALIB_I_GAIN, 1);
//      set_dc_offset_calibrations(&CALIB_V_DC_OFFSET, &CALIB_I_DC_OFFSET, 1);
		set_ac_offset_calibrations(&CALIB_I_AC_OFFSET, 1);

		// Deuxième channel
#ifdef CIRRUS_CS5480
    set_gain_calibrations(&CALIB_V_GAIN, &CALIB_I_GAIN, 2);
//	set_dc_offset_calibrations(&CALIB_V_DC_OFFSET, &CALIB_I_DC_OFFSET, 2);
    set_ac_offset_calibrations(&CALIB_I_AC_OFFSET, 2);
#endif
	}
	else
	{
		// Premier channel
		c1.Bit32 = calib->V1GAIN;
		c2.Bit32 = calib->I1GAIN;
		set_gain_calibrations(&c1, &c2, 1);

		c1.Bit32 = calib->I1ACOFF;
		set_ac_offset_calibrations(&c1, 1);

		c1.Bit32 = calib->P1OFF;
		c2.Bit32 = calib->Q1OFF;
		set_no_load_calibrations(&c1, &c2, 1);

		// Deuxième channel
#ifdef CIRRUS_CS5480
    c1.Bit32 = calib->V2GAIN;
    c2.Bit32 = calib->I2GAIN;
    set_gain_calibrations(&c1, &c2, 2);

    c1.Bit32 = calib->I2ACOFF;
    set_ac_offset_calibrations(&c1, 2);

    c1.Bit32 = calib->P2OFF;
    c2.Bit32 = calib->Q2OFF;
    set_no_load_calibrations(&c1, &c2, 2);
#endif
	}
}

/**
 * Sélection des facteurs de scale quand on change de Cirrus
 */
void CIRRUS_Select_Scale(CIRRUS_Cirrus cirrus)
{
	// Pointeur sur la calibration en fonction du Cirrus
	float *pUI_Scale;

	if (cirrus == Cirrus_1)
		pUI_Scale = &UI_Scale[0];
	else
		pUI_Scale = &UI_Scale[6];
	V1_SCALE = pUI_Scale++;
	I1_SCALE = pUI_Scale++;
	P1_SCALE = pUI_Scale++;
	V2_SCALE = pUI_Scale++;
	I2_SCALE = pUI_Scale++;
	P2_SCALE = pUI_Scale;
}

void CIRRUS_GetScale(float *scale)
{
	scale[0] = (*V1_SCALE) * 0.6;
	scale[1] = (*I1_SCALE) * 0.6;
#ifdef CIRRUS_CS5480
  scale[2] = (*V2_SCALE) * 0.6;
  scale[3] = (*I2_SCALE) * 0.6;
#endif
}

void CIRRUS_SetScale(float *scale)
{
	*V1_SCALE = scale[0] / 0.6;
	*I1_SCALE = scale[1] / 0.6;
#ifdef CIRRUS_CS5480
  *V2_SCALE = scale[2] / 0.6;
  *I2_SCALE = scale[3] / 0.6;
#endif
}

/**
 * Configuration du Cirrus sélectionné en cours
 * sample_count_ms: Echantillonnage en ms, 25 est la valeur minimale (soit 1 période). Par défaut vaut 1 s.
 * config: structure contenant les configurations. Peut être nul, dans ce cas les registres restent inchangés.
 * start: start continuous conversion
 * IMPORTANT : sample_count_ms détermine le temps pour avoir Data Ready. Il est inutile de faire
 * des acquisitions données sur un intervale de temps inférieur à sample_count_ms.
 */
void CIRRUS_Configuration(uint32_t sample_count_ms, CIRRUS_Config_typedef *config, bool start)
{
	Bit_List reg;

	if ((config != NULL) && (config->config0 != 0))
	{
		// Config register
		reg.Bit32 = config->config0;
		write_register(P0_Config0, PAGE0, &reg);
		reg.Bit32 = config->config1;
		write_register(P0_Config1, PAGE0, &reg);
		reg.Bit32 = config->config2;
		write_register(P16_Config2, PAGE16, &reg);

		// Pulse register
		reg.Bit32 = config->P_width;
		write_register(P0_PulseWidth, PAGE0, &reg);
		reg.Bit32 = config->P_rate;
		write_register(P18_PulseRate, PAGE18, &reg);
		reg.Bit32 = config->P_control;
		write_register(P0_PulseCtrl, PAGE0, &reg);
	}

	Sample_Count_ms = sample_count_ms;
	if (Sample_Count_ms < 25)
		Sample_Count_ms = 25;
	// By default Samplecount = 4000. With MCLK = 4.096 MHz, the averaging
	// period is fixed at N/4000 = 1 second, regardless the line frequency.
	set_sample_count(4 * Sample_Count_ms);
	Ready_TimeOut = Sample_Count_ms + 10;

	if (start)
	{
		start_conversion(); // Continuous conversion.

#ifdef DEBUG_CIRRUS
    CIRRUS_print_str("Start continuous conversion\r\n");
#endif

		Cirrus_Delay(1000);
		Conversion_Running = true;
	}
}

void CIRRUS_Get_Parameters(CIRRUS_Calib_typedef *calib, CIRRUS_Config_typedef *config)
{
	Bit_List reg;

	if ((V1_SCALE == NULL) || (I1_SCALE == NULL))
		return;

	calib->V1_Calib = *V1_SCALE * 0.6;
	calib->I1_MAX = *I1_SCALE * 0.6;

	read_register(P16_V1_GAIN, PAGE16, &reg);
	calib->V1GAIN = reg.Bit32;
	read_register(P16_I1_GAIN, PAGE16, &reg);
	calib->I1GAIN = reg.Bit32;
	read_register(P16_I1_ACOFF, PAGE16, &reg);
	calib->I1ACOFF = reg.Bit32;
	read_register(P16_P1_OFF, PAGE16, &reg);
	calib->P1OFF = reg.Bit32;
	read_register(P16_Q1_OFF, PAGE16, &reg);
	calib->Q1OFF = reg.Bit32;

	// Channel 2
#ifdef CIRRUS_CS5480
	if ((V2_SCALE == NULL) || (I2_SCALE == NULL))
		return;

  calib->V2_Calib = *V2_SCALE * 0.6;
  calib->I2_MAX = *I2_SCALE * 0.6;

  read_register(P16_V2_GAIN, PAGE16, &reg);
  calib->V2GAIN = reg.Bit32;
  read_register(P16_I2_GAIN, PAGE16, &reg);
  calib->I2GAIN = reg.Bit32;
  read_register(P16_I2_ACOFF, PAGE16, &reg);
  calib->I2ACOFF = reg.Bit32;
  read_register(P16_P2_OFF, PAGE16, &reg);
  calib->P2OFF = reg.Bit32;
  read_register(P16_Q2_OFF, PAGE16, &reg);
  calib->Q2OFF = reg.Bit32;
#endif

	// Config register
	read_register(P0_Config0, PAGE0, &reg);
	config->config0 = reg.Bit32;
	read_register(P0_Config1, PAGE0, &reg);
	config->config1 = reg.Bit32;
	read_register(P16_Config2, PAGE16, &reg);
	config->config2 = reg.Bit32;

	// Pulse register
	read_register(P0_PulseWidth, PAGE0, &reg);
	config->P_width = reg.Bit32;
	read_register(P18_PulseRate, PAGE18, &reg);
	config->P_rate = reg.Bit32;
	read_register(P0_PulseCtrl, PAGE0, &reg);
	config->P_control = reg.Bit32;
}

void CIRRUS_DO_Configuration(CIRRUS_DO_Struct DO_struct)
{
	uint8_t lsb_1, lsb_2, msb, hsb;
	Bit_List msg;

	lsb_1 = lsb_2 = msb = hsb = 0;
	switch (DO_struct.DO1)
	{
		case CIRRUS_DO_Energy:
			lsb_1 = DO_EPG1;
			hsb = 0b00000001;
			break;
		case CIRRUS_DO_P1Sign:
			lsb_1 = DO_P1Sign;
			break;
		case CIRRUS_DO_P2Sign:
			lsb_1 = DO_P2Sign;
			break;
		case CIRRUS_DO_VZero:
			lsb_1 = DO_VZero;
			break;
		case CIRRUS_DO_IZero:
			lsb_1 = DO_IZero;
			break;
		case CIRRUS_DO_Interrupt:
			lsb_1 = DO_Interrupt;
			break;
		case CIRRUS_DO_Nothing:
			lsb_1 = DO_Nothing;
			break;
		default:
			lsb_1 = DO_Nothing;
			break;
	}

	switch (DO_struct.DO2)
	{
		case CIRRUS_DO_Energy:
			lsb_2 = DO_EPG2;
			hsb = hsb | 0b00000010;
			break;
		case CIRRUS_DO_P1Sign:
			lsb_2 = DO_P1Sign;
			break;
		case CIRRUS_DO_P2Sign:
			lsb_2 = DO_P2Sign;
			break;
		case CIRRUS_DO_VZero:
			lsb_2 = DO_VZero;
			break;
		case CIRRUS_DO_IZero:
			lsb_2 = DO_IZero;
			break;
		case CIRRUS_DO_Interrupt:
			lsb_2 = DO_Interrupt;
			break;
		case CIRRUS_DO_Nothing:
			lsb_2 = DO_Nothing;
			break;
		default:
			lsb_2 = DO_Nothing;
			break;
	}
	lsb_2 = (lsb_2 << 4) | lsb_1;

	switch (DO_struct.DO3)
	{
		case CIRRUS_DO_Energy:
			msb = DO_EPG3;
			hsb = hsb | 0b00000100;
			break;
		case CIRRUS_DO_P1Sign:
			msb = DO_P1Sign;
			break;
		case CIRRUS_DO_P2Sign:
			msb = DO_P2Sign;
			break;
		case CIRRUS_DO_VZero:
			msb = DO_VZero;
			break;
		case CIRRUS_DO_IZero:
			msb = DO_IZero;
			break;
		case CIRRUS_DO_Interrupt:
			msb = DO_Interrupt;
			break;
		case CIRRUS_DO_Nothing:
			msb = DO_Nothing;
			break;
		default:
			msb = DO_Nothing;
			break;
	}
	msb = (DO_Nothing << 4) | msb;

	hsb <<= 4;
	create_blist(lsb_2, msb, hsb, &msg);
	write_register(P0_Config1, PAGE0, &msg);
}

/**
 * Simple test the connexion
 * Verify that register Config0 in Page0 == 0xC02000 (default)
 */
bool CIRRUS_TryConnexion(void)
{
	Bit_List reg;
#ifdef DEBUG_CIRRUS
  CIRRUS_print_str("Test Page0 = 0xC02000");
#endif
	if (read_register(P0_Config0, PAGE0, &reg))
	{
		return (reg.Bit32 == 0xC02000);
	}
	return false;
}

// ********************************************************************************
// Utilitary functions
// ********************************************************************************

Bit_List* clear_blist(Bit_List *list)
{
	*list = Bit_List_Zero;
	return list;
}

Bit_List* create_blist(uint8_t LSB, uint8_t MSB, uint8_t HSB, Bit_List *list)
{
	list->LSB = LSB;
	list->MSB = MSB;
	list->HSB = HSB;
	list->CHECK = 0x00;
	return list;
}

Bit_List* copy_blist(Bit_List *src, Bit_List *dest, bool full)
{
	dest->Bit32 = src->Bit32;
	if (!full)
		dest->tab[3] = 0x00;
	return dest;
}

// ********************************************************************************
// Print functions
// ********************************************************************************

const bool print_hexa = true; // 0 pour décimal, 1 pour hexa

void CIRRUS_print_str(const char *str)
{
	PrintTerminal(str);
}

void CIRRUS_print_int(const char *str, uint32_t integer)
{
	sprintf(buffer, "%s : 0x%.6X = %d\r\n", str, (unsigned int)integer, (unsigned int)integer);
//  Print_Debug(buffer);
	PrintTerminal(buffer);
}

void CIRRUS_print_float(const char *str, float val)
{
	sprintf(buffer, "%s : %f\r\n", str, val);
//  Print_Debug(buffer);
	PrintTerminal(buffer);
}

void CIRRUS_print_blist(const char *str, Bit_List *list, uint8_t size)
{
	if (size == 1)
	{
		if (print_hexa)
			sprintf(buffer, "%s : (LSB: 0x%.2X)\r\n", str, list->LSB);
		else
			sprintf(buffer, "%s : (LSB: %.3d)\r\n", str, list->LSB);
	}
	else
	{
		if (print_hexa)
			sprintf(buffer, "%s : (LSB: 0x%.2X, MSB: 0x%.2X, HSB: 0x%.2X), Checksum: %d\r\n", str,
					list->LSB, list->MSB, list->HSB, list->CHECK);
		else
			sprintf(buffer, "%s : (LSB: %.3d, MSB: %.3d, HSB: %.3d), Checksum: %d\r\n", str, list->LSB,
					list->MSB, list->HSB, list->CHECK);
	}
//  Print_Debug(buffer);
	PrintTerminal(buffer);
}

void CIRRUS_print_bin(uint8_t val)
{
	uint8_t value = val;
//	uint8_t bin[8];
	uint8_t i;
	char str[] = "00000000";

	for (i = 0; i < 7; i++)
	{
//    if ((value & 0x80) == 0x80) bin[i] = 1; else bin[i] = 0;
//		bin[i] = ((value & 0x80) == 0x80);
//		value <<= 1;
//
//		if (bin[i] == 1)
//			str[i] = '1';
//		else
//			str[i] = '0';

		str[i] = '0' + ((value & 0x80) == 0x80);
		value <<= 1;
	}

	sprintf(buffer, "val = %.3d dec, %.2X hex, %s bin\r\n", val, val, str);
	PrintTerminal(buffer);
}

// ********************************************************************************
// Conversion functions
// ********************************************************************************

/*
 Transforms an integer to 3 bytes
 Negative numbers will correctly be represented in two's complement form
 @return list of 3 bytes (LSB,..,HSB)
 */
inline Bit_List* int_to_blist(uint32_t integer, Bit_List *list)
{
//  list->LSB = integer & 255;
//  list->MSB = (integer>>8) & 255;
//  list->HSB = (integer>>16) & 255;
//  list->CHECK = 0;
	list->Bit32 = integer;
	return list;
}

/*
 Transforms 3 bytes to an integer
 @bt list (or list) of 3 bytes (LSB,..,HSB)
 @return an unsigned integer
 */
inline uint32_t blist_to_int(Bit_List *bt)
{
	return bt->Bit32;
//  return (((uint32_t)(bt->tab[0])) + (((uint32_t)(bt->tab[1]))<<8) + (((uint32_t)(bt->tab[2]))<<16));
}

/*
 Transforms three bytes in 2 complement format to a number
 @bt list (or list) of three bytes (LSB,..,HSB)
 @return  a number >-1.0 and <1.0
 */
float twoscompl_to_real(Bit_List *twoscompl)
{
	float num;
	num = (float) blist_to_int(twoscompl);
	if ((twoscompl->tab[2] & 128) != 0)
		num -= pow2_24;
	return (float) (num * over_pow2_23);
}

/*
 Transforms a number to three bytes in 2 complement format
 @twocompl a number >-1.0 and <1.0
 @return list of three bytes (LSB,..,HSB) in two complements format
 */
Bit_List* real_to_twoscompl(float num, Bit_List *list)
{
	*list = Bit_List_Zero;
	if (fabs(num) >= 1.0)
		CIRRUS_print_str(CS_RES_TOO_BIG);
	int_to_blist((uint32_t) (num * pow2_23), list);
	return list;
}

// ********************************************************************************
// Checksum functions
// ********************************************************************************

/*
 resets bit RX_CSUM_OFF in SerialCtrl to start the checksum validation
 */
void set_checksum_validation(void)
{
	Bit_List reg = Bit_List_Zero;
	read_register(P0_SerialCtrl, PAGE0, &reg);
	reg.HSB &= 0b00000100;
	write_register(P0_SerialCtrl, PAGE0, &reg);
	comm_checksum = true;
}

/*
 sets bit RX_CSUM_OFF in SerialCtrl to stop the checksum validation
 */
void reset_checksum_validation(void)
{
	Bit_List reg = Bit_List_Zero;
	read_register(P0_SerialCtrl, PAGE0, &reg);
	reg.HSB = (reg.HSB | 0b00000010) & 0b00000110;
	write_register(P0_SerialCtrl, PAGE0, &reg);
	comm_checksum = false;
}

/*
 figures out the current mode the CIRRUS is operating in csum or not
 sets or resets the instance comm_checksum bit
 @return True if in csum communication mode, -1 if error detected
 */
bool detect_comm_csum_mode(bool *error)
{
	Bit_List reg = Bit_List_Zero;
	bool is_checksum_mode = false;
	*error = false;

	comm_checksum = false; // Try communication without checksum
	read_register(P0_SerialCtrl, PAGE0, &reg);

	if (read_data_nb < 3)
	{
		*error = true;
		CIRRUS_print_int(CS_RES_READ_ERROR, read_data_nb);
	}
	else
	{
		if (read_data_nb == 3)
		{
			if ((reg.tab[2] & 0b00000010) != 0)
			{
				// pas d'erreur, on fonctionne sans checksum
			}
			else
			{
				*error = true;
				CIRRUS_print_blist(CS_RES_CHECK_ERROR, &reg, 3);
			}
		}
		else
		{
			is_checksum_mode = true;
			comm_checksum = true; // Try communication with checksum
			CIRRUS_Soft_reset();  // CIRRUS sometimes 'hangs' when errorenous communications were received
			read_register(P0_SerialCtrl, PAGE0, &reg); // Check if it works now

			if (read_data_nb != 4)
			{
				*error = true;
				comm_checksum = false;
				is_checksum_mode = false;
				CIRRUS_print_blist(CS_RES_CHECK_STATUS, &reg, 3);
			}
			if ((reg.tab[2] & 0b00000010) != 0)
			{
				*error = true;
				comm_checksum = false;
				is_checksum_mode = false;
				CIRRUS_print_blist(CS_RES_CHECK_STATUS, &reg, 3);
			}
		}
	}
	return is_checksum_mode;
}

/*
 Adds a checksum byte if comm_checksum is set to True, otherwise leaves data as is
 @return list of 3 or 4 bytes (data + 1 checkum byte) depending on comm_checksum
 @data list of data bytes
 */
void add_comm_checksum(Bit_List *thedata)
{
	uint8_t csum = 0x00;
	uint8_t i;

	if (comm_checksum)
	{
		csum = 0xFF;
		// for byte in data:
		// work for 1 or 3 bytes since byte 2 and 3 are 0 in case we have 1 byte
		for (i = 0; i < 3; i++)
		{
			csum = (csum - thedata->tab[i]) & 0xFF;
		}
	}
	thedata->CHECK = csum;
}

/*
 Checks if checksum is correct if comm_checksum = True
 and removes the checksum byte if present
 @return list with 3 data bytes
 @data list of the data returned from the CIRRUS
 */
Bit_List* sub_comm_checksum(Bit_List *thedata)
{
	Bit_List csum;

	if (comm_checksum)
	{
		copy_blist(thedata, &csum, true);
		add_comm_checksum(&csum);
		if (csum.CHECK != thedata->CHECK)
		{
			CIRRUS_print_blist(CS_RES_CHECK_ERROR2, thedata, 3);
		}
	}
	thedata->CHECK = 0x00;

	return thedata;
}

// ********************************************************************************
// RX TX functions
// ********************************************************************************

bool CIRRUS_Is_Data_Ready(void)
{
	return csResponse;
}

// ********************************************************************************
// Basic functions
// ********************************************************************************

/**
 * Send data.
 * Si on a l'erreur "Transmission Error", c'est généralement à cause du TimeOut du Transmit qui est trop faible
 */
void CIRRUS_send(Bit_List *msg, uint8_t size)
{
#ifdef USE_CHECKSUN
  if (comm_checksum)
    size += 1;
#endif

  CIRRUS_Last_Error = Cirrus_Transmit((uint8_t*) msg, size);
	if (CIRRUS_Last_Error != CIRRUS_OK)
		CIRRUS_print_str(CS_RES_HAL_ERROR);

#ifdef DEBUG_CIRRUS
  CIRRUS_print_blist("Send list: ", msg, size);
#endif
}

void select_page(uint8_t page_no)
{
	Bit_List msg = Bit_List_Zero;

	// Si on change de page
	if (page_no != current_selected_page)
	{
		msg.LSB = PAGE_SELECT | page_no;
#ifdef USE_CHECKSUN
    add_comm_checksum(&msg);
#endif
		CIRRUS_send(&msg, 1);
		current_selected_page = page_no;
	}
}

void select_register(uint8_t register_no, CIRRUS_Reg_Operation operation)
{
	Bit_List msg = Bit_List_Zero;

	if (operation == FOR_READ)
	{
		msg.LSB = REGISTER_READ | register_no;
		csResponse = false;
	}
	else
		msg.LSB = REGISTER_WRITE | register_no;
#ifdef USE_CHECKSUN
  add_comm_checksum(&msg);
#endif
	CIRRUS_send(&msg, 1);
}

/*
 @return list with 3 bytes of the register
 */
bool read_register(uint8_t register_no, uint8_t page_no, Bit_List *result)
{
	if (IS_READ_REG)
		return false;
	IS_READ_REG = true;
	*result = Bit_List_Zero;
#ifdef USE_CHECKSUN
  if (comm_checksum)
    data_read_expected += 1;
#endif

#ifdef CIRRUS_USE_UART
	Cirrus_ClearBuffer();
#endif

	select_page(page_no);
	select_register(register_no, FOR_READ);

	CIRRUS_Last_Error = Cirrus_Receive(result);
	csResponse = (CIRRUS_Last_Error == CIRRUS_OK);

#ifdef USE_CHECKSUN
  sub_comm_checksum(result);
#endif

#ifdef DEBUG_CIRRUS
  CIRRUS_print_int("Result", result->Bit32);
#endif

	IS_READ_REG = false;
	return csResponse;
}

/*
 @data = list of 3 integers 0..255 (or 4 if checksum is True)
 */
void write_register(uint8_t register_no, uint8_t page_no, Bit_List *thedata)
{
	select_page(page_no);
	select_register(register_no, FOR_WRITE);
#ifdef USE_CHECKSUN
  add_comm_checksum(thedata);
#endif
	CIRRUS_send(thedata, 3);
	// Need time to process
	Cirrus_Delay(WRITE_REGISTER_DELAY_MS); // WRITE_REGISTER_DELAY_MS
}

/*
 @instruction 6 BIT CIRRUS instruction
 */
void send_instruction(uint8_t instruction)
{
	Bit_List msg = Bit_List_Zero;

	msg.LSB = INSTRUCTION | instruction;
#ifdef USE_CHECKSUN
  add_comm_checksum(&msg);
#endif
	CIRRUS_send(&msg, 1);
//  Cirrus_Delay(1);
}

void start_conversion(void)
{
	send_instruction(CONT_CONV);
}

void stop_conversion(void)
{
	send_instruction(HALT_CONV);
}

void single_conversion(void)
{
	send_instruction(SINGLE_CONV);
}

// ********************************************************************************
// Reset functions
// ********************************************************************************

void data_reset(void)
{
	comm_checksum = false;
	current_selected_page = 0xFF;  // pas de page sélectionnée
	csResponse = false;
}

void softsoft_reset(void)
{
	Bit_List msg = Bit_List_Zero;
	// bool old_checksum = comm_checksum;

	msg.LSB = 0xFF;  // This one works also when in CSUM mode, without adding the csum
//  comm_checksum = false; // no comm_checksum !
	CIRRUS_send(&msg, 1);
//  comm_checksum = old_checksum; // restore checksum
	Cirrus_Delay(10);
}

/**
 * Soft reset utilisant l'instruction reset
 * ATTENTION, en UART la vitesse est réinitialisée à 600 bauds
 */
void CIRRUS_Soft_reset(void)
{
	// The official, documented, reset
	send_instruction(SOFT_RESET);
	comm_checksum = false;  // Initial state, no checksum
	Cirrus_Delay(10);
}

/**
 * Hard reset utilisant le pin reset
 * Reset pin est mis à LOW pendant 20 ms
 * ATTENTION, en UART la vitesse est réinitialisée à 600 bauds
 */
void CIRRUS_Hard_Reset(void)
{
	Bit_List reg;

#ifdef DEBUG_CIRRUS
  CIRRUS_print_str("CIRRUS Hard reset\r\n");
#endif

	CIRRUS_Chip_Reset(GPIO_PIN_SET);
	Cirrus_Delay(2);
	CIRRUS_Chip_Reset(GPIO_PIN_RESET);
	Cirrus_Delay(200);    // Minimum 120 µs
	CIRRUS_Chip_Reset(GPIO_PIN_SET);
	Cirrus_Delay(2);
	data_reset();

	// Correction bug Erratum 2
	// Sélection du premier Cirrus, pas besoin du scale
	CIRRUS_Select(Cirrus_1, false);

	reg.Bit32 = 0x000016;
	write_register(28, PAGE0, &reg);
	reg.Bit32 = 0x0C0008;
	write_register(30, PAGE0, &reg);
	reg.Bit32 = 0x000000;
	write_register(28, PAGE0, &reg);

	// Sélection du second Cirrus si présent
	if (CIRRUS_Number == 2)
	{
		CIRRUS_Select(Cirrus_2, false);

		reg.Bit32 = 0x000016;
		write_register(28, PAGE0, &reg);
		reg.Bit32 = 0x0C0008;
		write_register(30, PAGE0, &reg);
		reg.Bit32 = 0x000000;
		write_register(28, PAGE0, &reg);
	}

	// Aucun Cirrus sélectionné
	CIRRUS_UnSelect();
#ifdef DEBUG_CIRRUS
  CIRRUS_print_str("CIRRUS end correction bug\r\n");
#endif
}

// ********************************************************************************
// Check bit registers  functions
// ********************************************************************************

/*
 @return True if TUP bit is set in status0
 */
CIRRUS_RegBit temp_updated(void)
{
	return get_bitmask(REG_MASK_TUP);
}

/*
 @return True if RX_TO bit is set in status0
 */
CIRRUS_RegBit rx_timeout()
{
	return get_bitmask(REG_MASK_RXTO);
}

/*
 @return True if RX_csum_err bit is set in status0
 */
CIRRUS_RegBit rx_checksum_err(void)
{
	return get_bitmask(REG_MASK_RX_CSUM_ERR);
}

/*
 @return True if IC bit is set in status0
 */
CIRRUS_RegBit invalid_cmnd()
{
	return get_bitmask(REG_MASK_IC);
}

/*
 @return True if DRDY bit is set in status0
 */
inline bool CIRRUS_data_ready(void)
{
	return (get_bitmask(REG_MASK_DRDY) == CIRRUS_RegBit_Set);
}

/**
 * Clear Data ready bit
 */
void CIRRUS_clear_data_ready(void)
{
	Bit_List reg;
	reg.Bit32 = 0x800000;
	write_register(0x17, PAGE0, &reg);
}

/**
 * Fonction d'attente pour avoir data ready
 * clear_ready : si True ré-initialise immédiatement le bit
 * Note sur les temps d'attente :
 * - entre le reset du bit DRDY et sa remise à 1, il s'écoule au moins Sample_Count_ms ms
 * soit le temps d'échantillonnage défini dans CIRRUS_Configuration
 * - l'interrogation du registre prend environ :
 * 	- 93 us à 512000 bauds
 * 	- entre 70000 et 90000 us à 600 bauds
 */
bool CIRRUS_wait_for_ready(bool clear_ready)
{
	Bit_List reg;
	uint32_t StartTime = millis(); //csReferenceTime
	bool IsNotTimeOut = true;

	CIRRUS_Last_Error = CIRRUS_OK;
	while ((!CIRRUS_data_ready()) && IsNotTimeOut)
	{
		IsNotTimeOut = ((millis() - StartTime) < Ready_TimeOut);  // csReferenceTime
		// Background Wifi process
		yield();
	}

	if ((!IsNotTimeOut) && (CIRRUS_Last_Error == CIRRUS_OK))
		CIRRUS_Last_Error = CIRRUS_READY_TIMEOUT;

	if ((clear_ready) && (CIRRUS_Last_Error == CIRRUS_OK))
	{
		// Clear flag data ready
		reg.Bit32 = 0x800000;
		write_register(0x17, PAGE0, &reg);
	}

	return (IsNotTimeOut && (CIRRUS_Last_Error == CIRRUS_OK));
}

/**
 * Return FALSE if P_AVG is negative on the channel
 * Bit 0 on Status2, Page0
 */
bool CIRRUS_Check_Positive_Power(CIRRUS_Channel channel)
{
	if (channel == Channel_1)
		return (bool) (get_bitmask(REG_MASK_P1SIGN) == CIRRUS_RegBit_UnSet);
	else
		return (bool) (get_bitmask(REG_MASK_P2SIGN) == CIRRUS_RegBit_UnSet);
}

/**
 * Lit un registre et renvoie l'état d'un bit
 * b_mask : PAGE, REGISTER, BIT (le bit à lire [0 .. 23]
 */
CIRRUS_RegBit get_bitmask(Reg_Mask b_mask)
{
	Bit_List reg = Bit_List_Zero;
	uint8_t mask = 0b00000001;
	mask <<= (b_mask.BIT % 8);
	read_register(b_mask.REGISTER, b_mask.PAGE, &reg);
	if (csResponse)
	{
		if ((reg.tab[(b_mask.BIT / 8)] & mask) != 0)
			return CIRRUS_RegBit_Set;
		else
			return CIRRUS_RegBit_UnSet;
	}
	return CIRRUS_RegBit_Error;
}

/**
 * Nettoie un bit en écrivant 1
 * b_mask : PAGE, REGISTER, BIT (le bit à lire [0 .. 23]
 */
void clear_bitmask(Reg_Mask b_mask)
{
	Bit_List reg = Bit_List_Zero;
	uint8_t mask = 0b00000001;
	mask <<= (b_mask.BIT % 8);
	reg.tab[(b_mask.BIT / 8)] = mask;
	write_register(b_mask.REGISTER, b_mask.PAGE, &reg);
}

/**
 * Special function : activate an interrupt (page0, address 3) for special event for STATUS0
 * bit : [0..23] voir page 47
 */
void interrupt_bitmask(uint8_t bit)
{
	Bit_List reg = Bit_List_Zero;
	uint8_t mask = 0b00000001;
	mask <<= (bit % 8);
	reg.tab[(bit / 8)] = mask;
	write_register(P0_Mask, PAGE0, &reg);
}

/*
 @return a 3 bit number composed POR, IOR, VOR bits in the status0 register
 any of these bits being one indicates out-of-range of the respective quantities
 */
uint8_t pivor(void)
{
	Bit_List status0 = Bit_List_Zero;
	read_register(P0_Status0, PAGE0, &status0);
	return (status0.tab[1] & 0b01010100);
}

/*
 @return True if I over current
 */
CIRRUS_RegBit ioc(void)
{
	return get_bitmask(REG_MASK_IOC);
}

/*
 @return True if modulation oscillation has been detected in the temperture ADC (TOD in status1)
 */
CIRRUS_RegBit tod(void)
{
	return get_bitmask(REG_MASK_TOD);
}

// ********************************************************************************
// Set bit registers functions
// ********************************************************************************

// Do not write "1" to unpublished bits or bits published as "0"
// Do not write "0" to bits published as "1"

/**
 set the IPGA bit for the correct current channel input gain
 using the settings from the hardware constants
 */
void set_ipga(CIRRUS_Current_Sensor sensor)
{
	Bit_List reg;
	float voltage = 0.0;
	read_register(P0_Config0, PAGE0, &reg);
	//IPGA = 00 --> 10x gain, max 250 mV_Peak
	//IPGA = 10 --> 50x gain, max 50 mV_Peak

	switch (sensor)
	{
		case CIRRUS_SHUNT:
			voltage = (I1_MAX_default * sqrt2 * SHUNT_R);
			break;
		case CIRRUS_CT:
			voltage = (I1_MAX_default * sqrt2 / TORE_RATIO);
			break;
		case CIRRUS_ROG:
			voltage = 0.0;  // ToDo Need to be corrected
	}

	if (voltage <= 50.0 / 1000.0)
		reg.tab[0] = (reg.tab[0] | 0b00100000) & 0b00110100;
	else
	{
		if (voltage <= 250.0 / 1000.0)
			reg.tab[0] = reg.tab[0] & 0b00000100;
		else
		{
			CIRRUS_print_blist(CS_RES_MAX_CURRENT, &reg, 3);
			reg.tab[0] = reg.tab[0] & 0b00000100;
		}
	}

	write_register(P0_Config0, PAGE0, &reg);
}

/**
 Set the high pass filter for voltage (vhpf) and current (ihpf) channels
 default : vhpf=False, ihpf=False
 */
void set_highpass_filter(bool vhpf, bool ihpf)
{
	if (vhpf)
	{
		if (ihpf)
			set_filter(CIRRUS_HP_Filter, CIRRUS_HP_Filter, CIRRUS_HP_Filter, CIRRUS_HP_Filter);
		else
			set_filter(CIRRUS_HP_Filter, CIRRUS_NO_Filter, CIRRUS_HP_Filter, CIRRUS_NO_Filter);
	}
	else
	{
		if (ihpf)
			set_filter(CIRRUS_NO_Filter, CIRRUS_HP_Filter, CIRRUS_NO_Filter, CIRRUS_HP_Filter);
		else
			set_filter(CIRRUS_NO_Filter, CIRRUS_NO_Filter, CIRRUS_NO_Filter, CIRRUS_NO_Filter);
	}
}

/**
 Set the APCM method : true Papparent = SQRT(Pavg² + Qavg²); false Papparent = Vrms x Irms
 default : apcm=False
 */
void set_apcm(bool apcm)
{
	Bit_List reg;
	read_register(P16_Config2, PAGE16, &reg);
	if (apcm)
		reg.tab[1] |= 0b01000000;
	else
		reg.tab[1] &= 0b00011111;
	write_register(P16_Config2, PAGE16, &reg);
}

/**
 Set the MCFG method : Meter Configuration bits are used to control how the meter interprets the
 current channels when calculating total power : independently or collectively
 00 : 1V, 1I + neutral mode; Psum = P1avg or P2avg idem Q et S (default)
 01 : 1V, 2I mode; Psum = (P1avg + P2avg)/2 idem Q et S
 10 et 11 : reserved
 default : mcfg=False
 */
void set_mcfg(bool mcfg)
{
	Bit_List reg = Bit_List_Zero;
	read_register(P16_Config2, PAGE16, &reg);
	if (mcfg)
		reg.HSB |= 0b00000010;
	else
		reg.HSB &= 0b11111001;
	write_register(P16_Config2, PAGE16, &reg);
}

/**
 * Set filter on I and V (page 40)
 * V1, V2 : CIRRUS_NO_Filter, CIRRUS_HP_Filter, CIRRUS_PM_Filter
 * I1, I2 : CIRRUS_NO_Filter, CIRRUS_HP_Filter, CIRRUS_PM_Filter, CIRRUS_ROG_Filter
 */
void set_filter(CIRRUS_Filter V1, CIRRUS_Filter I1, CIRRUS_Filter V2, CIRRUS_Filter I2)
{
	Bit_List reg;
	uint16_t filter;

	read_register(P16_Config2, PAGE16, &reg);
	filter = reg.LSB & 0b00000001; // Garde le premier bit IIR_OFF
	if (V1 != CIRRUS_ROG_Filter)
	{
		filter |= (V1 << 1);
	}
	filter |= (I1 << 3);

#ifdef CIRRUS_CS5480
  if (V2 != CIRRUS_ROG_Filter)
  {
    filter |= (V2 << 5);
  }
  filter |= ((uint16_t)I2 << 7);
  reg.MSB |= (uint8_t)(filter >> 8);
#else
	// For compiler
	(void) V2;
	(void) I2;
#endif
	reg.LSB = (uint8_t) filter;

	write_register(P16_Config2, PAGE16, &reg);
}

// ********************************************************************************
// Get data functions
// On est sensé vérifier data ready avant l'appel de la fonction
// ********************************************************************************

//Instantaneous quantities

/*
 @return the instantaneous voltage
 */
float get_instantaneous_voltage(void)
{
	Bit_List v;
	static float old_val = 0.0;

	if (read_register(P16_V1, PAGE16, &v))
		old_val = (twoscompl_to_real(&v) * (*V1_SCALE));
	return old_val;
}

/*
 @return the instantaneous current
 */
float get_instantaneous_current(void)
{
	Bit_List i;
	static float old_val = 0.0;

	if (read_register(P16_I1, PAGE16, &i))
		old_val = (twoscompl_to_real(&i) * (*I1_SCALE));
	return old_val;
}

/*
 @return the current power calculated from the voltage and current channels
 */
float get_instantaneous_power(void)
{
	Bit_List p;
	static float old_val = 0.0;

	if (read_register(P16_P1, PAGE16, &p))
		old_val = (twoscompl_to_real(&p) * (*P1_SCALE));
	return old_val;
}

/*
 @return the current quadrature power (Q)
 */
float get_instantaneous_quadrature_power(void)
{
	Bit_List q;
	static float old_val = 0.0;

	if (read_register(P16_Q1, PAGE16, &q))
		old_val = (twoscompl_to_real(&q) * (*P1_SCALE));
	return old_val;
}

//Average and RMS quantities

/*
 @return rms value of V calculated during each low-rate interval
 */
float get_rms_voltage(void)
{
	Bit_List reg;
	static float old_val = 0.0;

	if (read_register(P16_V1_RMS, PAGE16, &reg))
		old_val = (float) (blist_to_int(&reg) * over_pow2_24 * (*V1_SCALE));
	return old_val;
}

/*
 @return rms value of I calculated during each low-rate interval
 */
float get_rms_current(void)
{
	Bit_List reg;
	static float old_val = 0.0;

	if (read_register(P16_I1_RMS, PAGE16, &reg))
		old_val = (float) (blist_to_int(&reg) * over_pow2_24 * (*I1_SCALE));
	return old_val;
}

/*
 @return power averaged over each low-rate interval (samplecount samples)
 */
float get_average_power(void)
{
	Bit_List pavg;
	static float old_val = 0.0;

	if (read_register(P16_P1_AVG, PAGE16, &pavg))
		old_val = (twoscompl_to_real(&pavg) * (*P1_SCALE));
	return old_val;
}

/*
 @return reactive power (Q) averaged over each low-rate interval (samplecount samples)
 */
float get_average_reactive_power(void)
{
	Bit_List qavg;
	static float old_val = 0.0;

	if (read_register(P16_Q1_AVG, PAGE16, &qavg))
		old_val = (twoscompl_to_real(&qavg) * (*P1_SCALE));
	return old_val;
}

//Peak quantities

/*
 @return The peak voltage
 */
float get_peak_voltage(void)
{
	Bit_List v;
	static float old_val = 0.0;

	if (read_register(P0_V1_PEAK, PAGE0, &v))
		old_val = (twoscompl_to_real(&v) * (*V1_SCALE));
	return old_val;
}

/*
 @return The peak current
 */
float get_peak_current(void)
{
	Bit_List i;
	static float old_val = 0.0;

	if (read_register(P0_I1_PEAK, PAGE0, &i))
		old_val = (twoscompl_to_real(&i) * (*I1_SCALE));
	return old_val;
}

/*
 @return The apparent power (S)
 */
float get_apparent_power(void)
{
	Bit_List s;
	static float old_val = 0.0;

	if (read_register(P16_S1, PAGE16, &s))
		old_val = (fabs(twoscompl_to_real(&s)) * (*P1_SCALE));
	return old_val;
}

/*
 @return The power factor (ratio of P_avg and S) * sign(P_avg)
 */
float get_power_factor(void)
{
	Bit_List pf;
	static float old_val = 0.0;

	if (read_register(P16_PF1, PAGE16, &pf))
		old_val = (twoscompl_to_real(&pf));
	return old_val;
}

//Total sum quantities

/*
 @return Total active power, P_sum
 */
float get_sum_active_power(void)
{
	Bit_List p_sum;
	static float old_val = 0.0;

	if (read_register(P16_P_SUM, PAGE16, &p_sum))
		old_val = (twoscompl_to_real(&p_sum) * (*P1_SCALE));
	return old_val;
}

/*
 @return Total apparent power, S_sum
 */
float get_sum_apparent_power(void)
{
	Bit_List s_sum;
	static float old_val = 0.0;

	if (read_register(P16_S_SUM, PAGE16, &s_sum))
		old_val = (fabs(twoscompl_to_real(&s_sum)) * (*P1_SCALE));
	return old_val;
}

/*
 @return Total reactive power, Q_sum
 */
float get_sum_reactive_power(void)
{
	Bit_List q_sum;
	static float old_val = 0.0;

	if (read_register(P16_Q_SUM, PAGE16, &q_sum))
		old_val = (twoscompl_to_real(&q_sum) * (*P1_SCALE));
	return old_val;
}

/*
 @return uRMS and pRMS data for channel 1
 */
bool CIRRUS_get_rms_data(volatile float *uRMS, volatile float *pRMS)
{
	Bit_List reg;

	if (CIRRUS_wait_for_ready(true))
	{
		// U RMS
		if (read_register(P16_V1_RMS, PAGE16, &reg))
		{
			*uRMS = (float) (blist_to_int(&reg) * over_pow2_24 * (*V1_SCALE));

			// P RMS
			if (read_register(P16_P1_AVG, PAGE16, &reg))
			{
				*pRMS = (twoscompl_to_real(&reg) * (*P1_SCALE));
				return true;
			}
		}
	}
	return false;
}

/*
 @return all RMS data for channel 1
 */
bool CIRRUS_get_rms_data(float *uRMS, float *iRMS, float *pRMS, float *CosPhi)
{
	Bit_List reg;

	if (CIRRUS_wait_for_ready(true))
	{
		// U RMS
		if (read_register(P16_V1_RMS, PAGE16, &reg))
		{
			*uRMS = (float) (blist_to_int(&reg) * over_pow2_24 * (*V1_SCALE));

			// I RMS
			if (read_register(P16_I1_RMS, PAGE16, &reg))
			{
				*iRMS = (float) (blist_to_int(&reg) * over_pow2_24 * (*I1_SCALE));

				// P RMS
				if (read_register(P16_P1_AVG, PAGE16, &reg))
				{
					*pRMS = (twoscompl_to_real(&reg) * (*P1_SCALE));

					// CosPhi
					if (read_register(P16_PF1, PAGE16, &reg))
					{
						*CosPhi = (twoscompl_to_real(&reg));
						return true;
					}
				}
			}
		}
	}
	return false;
}

/**
 * Get the data _data of type CIRRUS_Data
 * The verification of data ready is NOT done, you must verify this point before
 */
float CIRRUS_get_data(CIRRUS_Data _data, volatile float *result)
{
	Bit_List reg = Bit_List_Zero;

	switch (_data)
	{
		case CIRRUS_Inst_Voltage1:
		{
			if (read_register(P16_V1, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*V1_SCALE));
			break;
		}
		case CIRRUS_Inst_Current1:
		{
			if (read_register(P16_I1, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*I1_SCALE));
			break;
		}
		case CIRRUS_Inst_Power1:
		{
			if (read_register(P16_P1, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P1_SCALE));
			break;
		}
		case CIRRUS_Inst_React_Power1:
		{
			if (read_register(P16_Q1, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P1_SCALE));
			break;
		}
		case CIRRUS_RMS_Voltage1:
		{
			if (read_register(P16_V1_RMS, PAGE16, &reg))
				*result = (float) (blist_to_int(&reg) * over_pow2_24 * (*V1_SCALE));
			break;
		}
		case CIRRUS_RMS_Current1:
		{
			if (read_register(P16_I1_RMS, PAGE16, &reg))
				*result = (float) (blist_to_int(&reg) * over_pow2_24 * (*I1_SCALE));
			break;
		}
		case CIRRUS_Peak_Voltage1:
		{
			if (read_register(P0_V1_PEAK, PAGE0, &reg))
				*result = (twoscompl_to_real(&reg) * (*V1_SCALE));
			break;
		}
		case CIRRUS_Peak_Current1:
		{
			if (read_register(P0_I1_PEAK, PAGE0, &reg))
				*result = (twoscompl_to_real(&reg) * (*I1_SCALE));
			break;
		}
		case CIRRUS_Active_Power1:
		{
			if (read_register(P16_P1_AVG, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P1_SCALE));
			break;
		}
		case CIRRUS_Reactive_Power1:
		{
			if (read_register(P16_Q1_AVG, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P1_SCALE));
			break;
		}
		case CIRRUS_Apparent_Power1:
		{
			if (read_register(P16_S1, PAGE16, &reg))
				*result = (fabs(twoscompl_to_real(&reg)) * (*P1_SCALE));
			break;
		}
		case CIRRUS_Power_Factor1:
		{
			if (read_register(P16_PF1, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg));
			break;
		}
		// Deuxième channel
	#ifdef CIRRUS_CS5480
		case CIRRUS_Inst_Voltage2:
		{
			if (read_register(P16_V2, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*V2_SCALE));
			break;
		}
		case CIRRUS_Inst_Current2:
		{
			if (read_register(P16_I2, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*I2_SCALE));
			break;
		}
		case CIRRUS_Inst_Power2:
		{
			if (read_register(P16_P2, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P2_SCALE));
			break;
		}
		case CIRRUS_Inst_React_Power2:
		{
			if (read_register(P16_Q2, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P2_SCALE));
			break;
		}
		case CIRRUS_RMS_Voltage2:
		{
			if (read_register(P16_V2_RMS, PAGE16, &reg))
				*result = (float) (blist_to_int(&reg) * over_pow2_24 * (*V2_SCALE));
			break;
		}
		case CIRRUS_RMS_Current2:
		{
			if (read_register(P16_I2_RMS, PAGE16, &reg))
				*result = (float) (blist_to_int(&reg) * over_pow2_24 * (*I2_SCALE));
			break;
		}
		case CIRRUS_Peak_Voltage2:
		{
			if (read_register(P0_V2_PEAK, PAGE0, &reg))
				*result = (twoscompl_to_real(&reg) * (*V2_SCALE));
			break;
		}
		case CIRRUS_Peak_Current2:
		{
			if (read_register(P0_I2_PEAK, PAGE0, &reg))
				*result = (twoscompl_to_real(&reg) * (*I2_SCALE));
			break;
		}
		case CIRRUS_Active_Power2:
		{
			if (read_register(P16_P2_AVG, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P2_SCALE));
			break;
		}
		case CIRRUS_Reactive_Power2:
		{
			if (read_register(P16_Q2_AVG, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P2_SCALE));
			break;
		}
		case CIRRUS_Apparent_Power2:
		{
			if (read_register(P16_S2, PAGE16, &reg))
				*result = (fabs(twoscompl_to_real(&reg)) * (*P2_SCALE));
			break;
		}
		case CIRRUS_Power_Factor2:
		{
			if (read_register(P16_PF2, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg));
			break;
		}
#endif
		case CIRRUS_Total_Active_Power:
		{
			if (read_register(P16_P_SUM, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P2_SCALE));
			break;
		}
		case CIRRUS_Total_Reactive_Power:
		{
			if (read_register(P16_Q_SUM, PAGE16, &reg))
				*result = (twoscompl_to_real(&reg) * (*P2_SCALE));
			break;
		}
		case CIRRUS_Total_Apparent_Power:
		{
			if (read_register(P16_S_SUM, PAGE16, &reg))
				*result = (fabs(twoscompl_to_real(&reg)) * (*P2_SCALE));
			break;
		}
		case CIRRUS_Frequency:
		{
			if (read_register(P16_Epsilon, PAGE16, &reg))
				*result = (fabs(twoscompl_to_real(&reg)) * 4000);
			break;
		}
	}
	return *result;
}

//Special quantities

/*
 Read onboard counter that is icreaed with 4.096 MHz
 */
bool CIRRUS_get_system_time(uint32_t *cstime)
{
	Bit_List t;

	if (read_register(P16_Time, PAGE16, &t))
	{
		*cstime = (blist_to_int(&t));
		return true;
	}
	return false;
}

/**
 * return the on-chip temperature
 * The temperature register updates every 2240 output word rate (OWR) samples.
 * The Status0 resister bit TUP indicates when T is updated.
 * DT : 335 us at 512 kbauds
 */
float CIRRUS_get_temperature(void)
{
	const float pow2_7 = 128.0;
	Bit_List t;
	Reg_Mask tup = REG_MASK_TUP;
	static float old_val = 0.0;

	if (get_bitmask(tup) == CIRRUS_RegBit_Set)
	{
		if (read_register(P16_T, PAGE16, &t))
			old_val = (twoscompl_to_real(&t) * pow2_7);
		clear_bitmask(tup);
	}
	return old_val;
}

float CIRRUS_get_frequency(void)
{
	Bit_List reg;
	Reg_Mask fup = REG_MASK_FUP;
	static float old_val = 0.0;

	if (get_bitmask(fup) == CIRRUS_RegBit_Set)
	{
		if (read_register(P16_Epsilon, PAGE16, &reg))
			old_val = (fabs(twoscompl_to_real(&reg)) * 4000);
		clear_bitmask(fup);
	}
	return old_val;
}

// ********************************************************************************
// Set settings functions
// ********************************************************************************

/*
 Set the number of Output Word Rate (OWR) (OWR is 4000Hz at 50Hz and 4096MHz)
 samples that will be used to allow filters to settle at the beginning of
 conversion and calibration commands
 Default: owr_samples=30
 */
void set_settle_time(uint32_t owr_samples)
{
	Bit_List samp;
	int_to_blist(owr_samples, &samp);
	write_register(P16_T_SETTLE, PAGE16, &samp);
}

/*
 Set the number of output word rate (OWR) samples to use in calculating low-rate results
 @N integer between 100 and pow(2,23-1)
 Default: N=100
 */
void set_sample_count(uint32_t N)
{
	const uint32_t pow2_22 = 4194304;
	Bit_List bn;

	if (N > pow2_22)
		N = pow2_22;
	if (N < 100)
		N = 100;
	int_to_blist(N, &bn);
	write_register(P16_SampleCount, PAGE16, &bn);
}

/*
 Set the SCALE_IGAIN register to scale
 @scale a number >=0 and <1
 */
void set_calibration_scale(float scale)
{
	Bit_List bscale;
	scale = fabs(scale);
	real_to_twoscompl(scale, &bscale);
	write_register(P18_Scale, PAGE18, &bscale);
}

// ********************************************************************************
// Calibration functions
// ********************************************************************************

/**
 Set the register V_GAIN and I_GAIN with the values from the class constants
 channel must be 1 or 2
 */
void set_gain_calibrations(Bit_List *v_gain, Bit_List *i_gain, uint8_t channel)
{
	if (channel == 1)
	{
		write_register(P16_V1_GAIN, PAGE16, v_gain);
		write_register(P16_I1_GAIN, PAGE16, i_gain);
	}
	else
		if (channel == 2)
		{
			write_register(P16_V2_GAIN, PAGE16, v_gain);
			write_register(P16_I2_GAIN, PAGE16, i_gain);
		}
}

/**
 Set the registers I_DC_OFFSET and V_DC_OFFSET to the values set in the class constants
 channel must be 1 or 2
 */
void set_dc_offset_calibrations(Bit_List *v_off, Bit_List *i_off, uint8_t channel)
{
	if (channel == 1)
	{
		write_register(P16_I1_DCOFF, PAGE16, i_off);
		write_register(P16_V1_DCOFF, PAGE16, v_off);
	}
	else
		if (channel == 2)
		{
			write_register(P16_I2_DCOFF, PAGE16, i_off);
			write_register(P16_V2_DCOFF, PAGE16, v_off);
		}
}

/**
 Set the register I_AC_OFFSET to the value set in the class constants
 channel must be 1 or 2
 */
void set_ac_offset_calibrations(Bit_List *i_acoff, uint8_t channel)
{
	if (channel == 1)
	{
		write_register(P16_I1_ACOFF, PAGE16, i_acoff);
	}
	else
		if (channel == 2)
		{
			write_register(P16_I2_ACOFF, PAGE16, i_acoff);
		}
}

/**
 Set the registers P_OFFSET and Q_OFFSET to the values set in the class constants
 channel must be 1 or 2
 */
void set_no_load_calibrations(Bit_List *p_off, Bit_List *q_off, uint8_t channel)
{
	if (channel == 1)
	{
		write_register(P16_P1_OFF, PAGE16, p_off);
		write_register(P16_Q1_OFF, PAGE16, q_off);
	}
	else
		if (channel == 2)
		{
			write_register(P16_P2_OFF, PAGE16, p_off);
			write_register(P16_Q2_OFF, PAGE16, q_off);
		}
}

/**
 * Perform DC offset calibration for current and voltage
 * The differential on VIN+- and IIn1+- (and IIn2+- for channel 2) should be 0 V
 * only_I : only current is performed
 * NOTE : HPF is disabled for the calibration
 * NOTE : if HPF is used, DC offset calibration is not necessary
 */
void do_dc_offset_calibration(bool only_I)
{
	Bit_List i_off, v_off;
	Bit_List temp0(0); // 0.0 value
	Bit_List temp1(0, 0, 64); // 1.0 value
//	Bit_List temp1, temp2;
	uint8_t piv;

	CIRRUS_print_str(CS_RES_NOLINE);
	stop_conversion();
	// set T_Settle to 2000ms and SampleCount to a large number
	set_settle_time(8000); // will add 2 sec to the time (2000 * 4)
	set_sample_count(16000);
	set_highpass_filter(false, false); // disable HPF

	// set I and V Gain to 1.0 and offset to 0.0
	set_gain_calibrations(&temp1, &temp1, 1);
#ifdef CIRRUS_CS5480
	set_gain_calibrations(&temp1, &temp1, 2);
#endif
	set_dc_offset_calibrations(&temp0, &temp0, 1);
#ifdef CIRRUS_CS5480
	set_dc_offset_calibrations(&temp0, &temp0, 2);
#endif

	// clear DRDY
	CIRRUS_clear_data_ready();

	// perform iv-dc calibration
	if (only_I)
	  send_instruction(CALIB_DCOFFS_I1);  // Channel 1 only
	else
		send_instruction(CALIB_DCOFFS_IV);

	Cirrus_Delay(4000);
	while (!(CIRRUS_data_ready()))
	{
		CIRRUS_print_str(CS_RES_CALIB);
		Cirrus_Delay(3000);
	}

	piv = pivor();
	if (piv != 0)
	{
		CIRRUS_print_int(CS_RES_PIVOR, piv);
		if (piv >= 4) return;
	}

	// get and store I & V offset registers
	// Premier channel
	read_register(P16_I1_DCOFF, PAGE16, &i_off);
	read_register(P16_V1_DCOFF, PAGE16, &v_off);

	// print result
	sprintf(buffer, "V1 DC OFFSET (V1_DCOFF): %.6f, (%d, %d, %d) = 0x%.6X\r\n", twoscompl_to_real(&v_off), v_off.LSB,
			v_off.MSB, v_off.HSB, (unsigned int)v_off.Bit32);
	CIRRUS_print_str(buffer);
	sprintf(buffer, "I1 DC OFFSET (I1_DCOFF): %.6f, (%d, %d, %d) = 0x%.6X\r\n", twoscompl_to_real(&i_off), i_off.LSB,
			i_off.MSB, i_off.HSB, (unsigned int)i_off.Bit32);
	CIRRUS_print_str(buffer);

	// Deuxième channel
#ifdef CIRRUS_CS5480
	read_register(P16_I2_DCOFF, PAGE16, &i_off);
	read_register(P16_V2_DCOFF, PAGE16, &v_off);

	// print result
	sprintf(buffer, "V2 DC OFFSET (V2_DCOFF): %.6f, (%d, %d, %d) = 0x%.6X\r\n", twoscompl_to_real(&v_off), v_off.LSB,
			v_off.MSB, v_off.HSB, (unsigned int)v_off.Bit32);
	CIRRUS_print_str(buffer);
	sprintf(buffer, "I2 DC OFFSET (I2_DCOFF): %.6f, (%d, %d, %d) = 0x%.6X\r\n", twoscompl_to_real(&i_off), i_off.LSB,
			i_off.MSB, i_off.HSB, (unsigned int)i_off.Bit32);
	CIRRUS_print_str(buffer);
#endif

	CIRRUS_print_str(CS_RES_REMEMBER);
}

/**
 * Perform AC offset calibration for current
 * The differential on IIn1+- (and IIn2+- for channel 2) should be 0 V
 */
void do_ac_offset_calibration(void)
{
	Bit_List i_acoff;
	Bit_List Zero(0);
	uint8_t piv;

	CIRRUS_print_str(CS_RES_NOLINE);
	stop_conversion();
	set_settle_time(8000); // will add 2 sec to the time (2000 * 4)
	set_sample_count(16000);
	set_highpass_filter(true, true); // So DC offset is not used

	// set the I AC offset channel to zero
	set_ac_offset_calibrations(&Zero, 1);

	// clear DRDY
	CIRRUS_clear_data_ready();

	// perform AC offset calibration
	// AC offset register will hold the square of the RMS offset
	send_instruction(CALIB_ACOFFS_I1);

	Cirrus_Delay(4000);
	while (!(CIRRUS_data_ready()))
	{
		CIRRUS_print_str(CS_RES_CALIB);
		Cirrus_Delay(3000);
	}

	piv = pivor();
	if (piv != 0)
	{
		CIRRUS_print_int(CS_RES_PIVOR, piv);
		if (piv >= 4) return;
	}

	// get and store I AC offset registers
	read_register(P16_I1_ACOFF, PAGE16, &i_acoff);

	// print result
	sprintf(buffer, "I1 AC OFFSET (I1_ACOFF): %.6f, (%d, %d, %d) = 0x%.6X\r\n", blist_to_int(&i_acoff) * over_pow2_24,
			i_acoff.LSB, i_acoff.MSB, i_acoff.HSB, (unsigned int)i_acoff.Bit32);
	CIRRUS_print_str(buffer);

	// Configuration deuxième channel
#ifdef CIRRUS_CS5480
	set_ac_offset_calibrations(&Zero, 2);

	// clear DRDY
	CIRRUS_clear_data_ready();

	// perform AC offset calibration
	// AC offset register will hold the square of the RMS offset
	send_instruction(CALIB_ACOFFS_I2);

	Cirrus_Delay(4000);
	while (!(CIRRUS_data_ready()))
	{
		CIRRUS_print_str(CS_RES_CALIB);
		Cirrus_Delay(3000);
	}

	piv = pivor();
	if (piv != 0)
	{
		CIRRUS_print_int(CS_RES_PIVOR, piv);
		if (piv >= 4) return;
	}

	// get and store I AC offset registers
	read_register(P16_I2_ACOFF, PAGE16, &i_acoff);

	// print result
	sprintf(buffer, "I2 AC OFFSET (I2_ACOFF): %.6f, (%d, %d, %d) = 0x%.6X\r\n", blist_to_int(&i_acoff) * over_pow2_24,
			i_acoff.LSB, i_acoff.MSB, i_acoff.HSB, (unsigned int)i_acoff.Bit32);
	CIRRUS_print_str(buffer);
#endif

	CIRRUS_print_str(CS_RES_REMEMBER);
}

/**
 * Perform gain calibration for current and voltage with pure resistive load
 * calib_vac : the voltage mesured
 * calib_r : the resistance of the charge
 */
void do_gain_calibration(float calib_vac, float calib_r)
{
	const float over_pow2_22 = 1.0 / (4194304.0 - 1.0);
	Bit_List i_gain, v_gain, i_acoff;
	Bit_List temp;
	Bit_List temp1(0, 0, 64);  // 1.0 value
	uint8_t piv;
	float init_scale_setting;
	float calib_iac = calib_vac / calib_r;
	float i_ac_offset;

	sprintf(buffer, CS_RES_LOAD, calib_vac, calib_iac, calib_r, calib_vac * calib_iac);
	CIRRUS_print_str(buffer);

	if ((I1_SCALE == NULL) || (*I1_SCALE == 0))
	{
		CIRRUS_print_str(CS_RES_NOCALIB);
		return;
	}

	stop_conversion();
	// set I and V Gain to 1.0
	// 1.0 = 0x400000 and 64 = 0x40.
	set_gain_calibrations(&temp1, &temp1, 1);
#ifdef CIRRUS_CS5480
	set_gain_calibrations(&temp1, &temp1, 2);
#endif
	// set T_Settle to 2000 ms and sample count to a big number
	set_settle_time(8000); // will add 2 sec to the time (2000 * 4)
	set_sample_count(16000);
	set_highpass_filter(true, true);

	// Set scale register for current (AN366REV2.pdf page 10)
  // init_scale_setting = Iref/Imax*0.6 = calib_iac/Imax*0.6
	// et Imax = I1_SCALE * 0.6
	init_scale_setting = 0.6 * calib_iac / (*I1_SCALE * 0.6);
	real_to_twoscompl(init_scale_setting, &temp);

	sprintf(buffer, "INIT_SCALE_SETTING:  %.4f = 0x%.6X \r\n", init_scale_setting, (unsigned int)temp.Bit32);
	CIRRUS_print_str(buffer);
	write_register(P18_Scale, PAGE18, &temp);

	// clear DRDY
	CIRRUS_clear_data_ready();

	// perform gain calibration
	send_instruction(CALIB_GAIN_IV);

	// wait till done, but poll quite often to get an idea of the process
	Cirrus_Delay(3000);
	while (!(CIRRUS_data_ready()))
	{
		CIRRUS_print_str(CS_RES_CALIB);
		Cirrus_Delay(3000);
	}

	//check IOR and VOR status bits
	piv = pivor();
	if (piv != 0)
	{
		CIRRUS_print_int(CS_RES_PIVOR, piv);
		if (piv >= 4) return;
	}

	// get and store I & V Gain registers
	// Premier channel
	read_register(P16_I1_GAIN, PAGE16, &i_gain);
	read_register(P16_V1_GAIN, PAGE16, &v_gain);

	// print result
	sprintf(buffer, "V1 GAIN: %.6f, (%d, %d, %d) = 0x%.6X\r\n", blist_to_int(&v_gain) * over_pow2_22,
			v_gain.LSB, v_gain.MSB, v_gain.HSB, (unsigned int)v_gain.Bit32);
	CIRRUS_print_str(buffer);
	sprintf(buffer, "I1 GAIN: %.6f, (%d, %d, %d) = 0x%.6X\r\n", blist_to_int(&i_gain) * over_pow2_22,
			i_gain.LSB, i_gain.MSB, i_gain.HSB, (unsigned int)i_gain.Bit32);
	CIRRUS_print_str(buffer);

	// Update the I AC offset
	// Get the old value ...
	read_register(P16_I1_ACOFF, PAGE16, &i_acoff);
	sprintf(buffer, "I1_ACOFF old: %.6f, (%d, %d, %d) = 0x%.6X\r\n", i_acoff.Bit32 * over_pow2_24,
			i_acoff.LSB, i_acoff.MSB, i_acoff.HSB, (unsigned int)i_acoff.Bit32);
	CIRRUS_print_str(buffer);

	i_ac_offset = i_acoff.Bit32;

	// then multiply by the I gain ...
	i_ac_offset *= (i_gain.Bit32 * over_pow2_22);

	// then update I AC offset
	i_acoff.Bit32 = (uint32_t) (i_ac_offset);
	write_register(P16_I1_ACOFF, PAGE16, &i_acoff);
	sprintf(buffer, "I1_ACOFF new: %.6f, (%d, %d, %d) = 0x%.6X\r\n", i_acoff.Bit32 * over_pow2_24,
			i_acoff.LSB, i_acoff.MSB, i_acoff.HSB, (unsigned int)i_acoff.Bit32);
	CIRRUS_print_str(buffer);

	// Deuxième channel
#ifdef CIRRUS_CS5480
	read_register(P16_I2_GAIN, PAGE16, &i_gain);
	read_register(P16_V2_GAIN, PAGE16, &v_gain);

	// print result
	sprintf(buffer, "V2 GAIN: %.6f, (%d, %d, %d) = 0x%.6X\r\n", blist_to_int(&v_gain) * over_pow2_22,
			v_gain.LSB, v_gain.MSB, v_gain.HSB, (unsigned int)v_gain.Bit32);
	CIRRUS_print_str(buffer);
	sprintf(buffer, "I2 GAIN: %.6f, (%d, %d, %d) = 0x%.6X\r\n", blist_to_int(&i_gain) * over_pow2_22,
			i_gain.LSB, i_gain.MSB, i_gain.HSB, (unsigned int)i_gain.Bit32);
	CIRRUS_print_str(buffer);
#endif

	CIRRUS_print_str(CS_RES_REMEMBER);
}

void set_phase_compensations(void)
{
	// set to zero compensation, all errors are accounted for in the gain calibration
//  pass
}

void do_noload_power_calibration()
{
	Bit_List temp, reg;

	CIRRUS_print_str(CS_RES_NOLOADPOWER);
	stop_conversion();
	write_register(P16_P1_OFF, PAGE16, &temp);
	write_register(P16_Q1_OFF, PAGE16, &temp);
	// Deuxième channel
#ifdef CIRRUS_CS5480
	write_register(P16_P2_OFF, PAGE16, &temp);
	write_register(P16_Q2_OFF, PAGE16, &temp);
#endif
	set_settle_time(8000);
	set_sample_count(16000);
	set_highpass_filter(true, true);
	CIRRUS_clear_data_ready();
	start_conversion();

	if (CIRRUS_wait_for_ready(true))
	{
		read_register(P16_P1_AVG, PAGE16, &reg);
		reg.HSB = reg.HSB | 0x80; // negate the value
		write_register(P16_P1_OFF, PAGE16, &reg);
		read_register(P16_Q1_AVG, PAGE16, &reg);
		reg.HSB = reg.HSB | 0x80; // negate the value
		write_register(P16_Q1_OFF, PAGE16, &reg);

#ifdef CIRRUS_CS5480
		read_register(P16_P2_AVG, PAGE16, &reg);
		reg.HSB = reg.HSB | 0x80; // negate the value
		write_register(P16_P2_OFF, PAGE16, &reg);
		read_register(P16_Q2_AVG, PAGE16, &reg);
		reg.HSB = reg.HSB | 0x80; // negate the value
		write_register(P16_Q2_OFF, PAGE16, &reg);
#endif

		CIRRUS_print_str("No load power calibration done.\r\n");
	}
	else
		CIRRUS_print_str("No load power calibration fail.\r\n");
}

void set_temp_calibrations(void)
{
	Bit_List temp1;
	write_register(P16_T_GAIN, PAGE16, create_blist(0x00, 0x00, 0x01, &temp1));
	write_register(P16_T_OFF, PAGE16, create_blist(0x00, 0x00, 0x00, &temp1));
}

//ToDo calibration registers checksum check

// ********************************************************************************
// Print utility functions
// ********************************************************************************

const char *CIRRUS_Last_Error_Mess[] = { "OK", "ERROR", "BUSY", "TIMEOUT", "READY TIMEOUT" };
const char* CIRRUS_Print_LastError()
{
	return CIRRUS_Last_Error_Mess[CIRRUS_Last_Error];
}

void CIRRUS_Print_FullData()
{
	char uart_Text[50] = { 0 };
	uint32_t cstime = 0;
	float data = 0;

	CIRRUS_print_str("***** Cirrus full data *****");

	CIRRUS_print_str("Wait for Data ready");
	while (!CIRRUS_data_ready())
		Cirrus_Delay(20);
	CIRRUS_print_str("Data is ready");
	CIRRUS_clear_data_ready();

	sprintf(uart_Text, "Inst_Voltage1 : %.2f", CIRRUS_get_data(CIRRUS_Inst_Voltage1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Inst_Current1 : %.2f", CIRRUS_get_data(CIRRUS_Inst_Current1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Inst_Power1 : %.2f", CIRRUS_get_data(CIRRUS_Inst_Power1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Inst_React_Power1 : %.2f", CIRRUS_get_data(CIRRUS_Inst_React_Power1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "RMS_Voltage1 : %.2f", CIRRUS_get_data(CIRRUS_RMS_Voltage1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "RMS_Current1 : %.2f", CIRRUS_get_data(CIRRUS_RMS_Current1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Peak_Voltage1 : %.2f", CIRRUS_get_data(CIRRUS_Peak_Voltage1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Peak_Current1 : %.2f", CIRRUS_get_data(CIRRUS_Peak_Current1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Active_Power1 : %.2f", CIRRUS_get_data(CIRRUS_Active_Power1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Reactive_Power1 : %.2f", CIRRUS_get_data(CIRRUS_Reactive_Power1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Apparent_Power1 : %.2f", CIRRUS_get_data(CIRRUS_Apparent_Power1, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Power_Factor1 : %.2f", CIRRUS_get_data(CIRRUS_Power_Factor1, &data));
	CIRRUS_print_str(uart_Text);
	// Channel 2
#ifdef CIRRUS_CS5480
  sprintf(uart_Text, "Inst_Voltage2 : %.2f", CIRRUS_get_data(CIRRUS_Inst_Voltage2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Inst_Current2 : %.2f", CIRRUS_get_data(CIRRUS_Inst_Current2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Inst_Power2 : %.2f", CIRRUS_get_data( CIRRUS_Inst_Power2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Inst_React_Power2 : %.2f", CIRRUS_get_data(CIRRUS_Inst_React_Power2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "RMS_Voltage2 : %.2f", CIRRUS_get_data(CIRRUS_RMS_Voltage2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "RMS_Current2 : %.2f", CIRRUS_get_data(CIRRUS_RMS_Current2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Peak_Voltage2 : %.2f", CIRRUS_get_data(CIRRUS_Peak_Voltage2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Peak_Current2 : %.2f", CIRRUS_get_data(CIRRUS_Peak_Current2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Active_Power2 : %.2f", CIRRUS_get_data(CIRRUS_Active_Power2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Reactive_Power2 : %.2f", CIRRUS_get_data(CIRRUS_Reactive_Power2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Apparent_Power2 : %.2f", CIRRUS_get_data(CIRRUS_Apparent_Power2, &data));
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text, "Power_Factor2 : %.2f", CIRRUS_get_data(CIRRUS_Power_Factor2, &data));
  CIRRUS_print_str(uart_Text);
#endif
	sprintf(uart_Text, "Total_Active_Power : %.2f",
			CIRRUS_get_data(CIRRUS_Total_Active_Power, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Total_Reactive_Power : %.2f",
			CIRRUS_get_data(CIRRUS_Total_Reactive_Power, &data));
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Total_Apparent_Power : %.2f",
			CIRRUS_get_data(CIRRUS_Total_Apparent_Power, &data));
	CIRRUS_print_str(uart_Text);

	CIRRUS_get_system_time(&cstime);
	sprintf(uart_Text, "time : %d", (unsigned int)cstime);

	sprintf(uart_Text, "Invalid command : %d", invalid_cmnd());
	CIRRUS_print_str(uart_Text);

	if (CIRRUS_Check_Positive_Power(Channel_1))
		CIRRUS_print_str("P1 positif");
	else
		CIRRUS_print_str("P1 négatif");
	sprintf(uart_Text, "Cirrus_Last_Error : %d", (int) CIRRUS_Last_Error);
	CIRRUS_print_str(uart_Text);

	CIRRUS_print_str("****************************\r\n");
}

void CIRRUS_Print_Calib(CIRRUS_Calib_typedef *calib)
{
	char uart_Text[100] = { 0 };

	CIRRUS_print_str("***** Cirrus Calibration data *****");

	sprintf(uart_Text, "V1_Calib : %.2f", calib->V1_Calib);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "I1_MAX : %.2f", calib->I1_MAX);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "V1GAIN : 0x%.6X", (unsigned int)calib->V1GAIN);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "I1GAIN : 0x%.6X", (unsigned int)calib->I1GAIN);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "I1ACOFF : 0x%.6X", (unsigned int)calib->I1ACOFF);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "P1OFF : 0x%.6X", (unsigned int)calib->P1OFF);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Q1OFF : 0x%.6X", (unsigned int)calib->Q1OFF);
	CIRRUS_print_str(uart_Text);

	sprintf(uart_Text, "CS_Calib1 = {%.2f, %.2f, 0x%.6X, 0x%.6X, 0x%.6X, 0x%.6X, 0x%.6X}",
			calib->V1_Calib, calib->I1_MAX, (unsigned int) calib->V1GAIN, (unsigned int) calib->I1GAIN,
			(unsigned int) calib->I1ACOFF, (unsigned int) calib->P1OFF, (unsigned int) calib->Q1OFF);
	CIRRUS_print_str(uart_Text);

	// Channel 2
#ifdef CIRRUS_CS5480
  sprintf(uart_Text,"V2_Calib : %.2f", calib->V2_Calib);
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text,"I2_MAX : %.2f", calib->I2_MAX);
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text,"V2GAIN : 0x%.6X", (unsigned int)calib->V2GAIN);
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text,"I2GAIN : 0x%.6X", (unsigned int)calib->I2GAIN);
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text,"I2ACOFF : 0x%.6X", (unsigned int)calib->I2ACOFF);
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text,"P2OFF : 0x%.6X", (unsigned int)calib->P2OFF);
  CIRRUS_print_str(uart_Text);
  sprintf(uart_Text,"Q2OFF : 0x%.6X", (unsigned int)calib->Q2OFF);
  CIRRUS_print_str(uart_Text);

	sprintf(uart_Text, "CS_Calib2 = {%.2f, %.2f, 0x%.6X, 0x%.6X, 0x%.6X, 0x%.6X, 0x%.6X}",
			calib->V2_Calib, calib->I2_MAX, (unsigned int) calib->V2GAIN, (unsigned int) calib->I2GAIN,
			(unsigned int) calib->I2ACOFF, (unsigned int) calib->P2OFF, (unsigned int) calib->Q2OFF);
	CIRRUS_print_str(uart_Text);
#endif

	CIRRUS_print_str("****************************\r\n");
}

void CIRRUS_Print_Config(CIRRUS_Config_typedef *config)
{
	char uart_Text[50] = { 0 };

	CIRRUS_print_str("***** Cirrus Configuration data *****");

	sprintf(uart_Text, "Config0 : 0x%.6X", (unsigned int)config->config0);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Config1 : 0x%.6X", (unsigned int)config->config1);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Config2 : 0x%.6X", (unsigned int)config->config2);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Pulse width : 0x%.6X", (unsigned int)config->P_width);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Pulse rate : 0x%.6X", (unsigned int)config->P_rate);
	CIRRUS_print_str(uart_Text);
	sprintf(uart_Text, "Pulse control : 0x%.6X", (unsigned int)config->P_control);
	CIRRUS_print_str(uart_Text);

	CIRRUS_print_str("****************************\r\n");
}

// ********************************************************************************
// End of file
// ********************************************************************************
