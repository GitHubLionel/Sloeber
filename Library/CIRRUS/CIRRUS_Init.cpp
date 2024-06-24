#include "CIRRUS.h"
#include "CIRRUS_RES_EN.h"

#ifdef CIRRUS_FLASH
#include <Preferences.h>

typedef struct
{
		uint8_t signature[2];
		CIRRUS_Calib_typedef Calib;
		CIRRUS_Config_typedef Config;
} Cirrus_Flash_Struct;

class Cirrus_Flash
{
	private:
		const uint8_t EEPROM_SIG[2] = {0xee, 0x11};
		Cirrus_Flash_Struct _data;
		Preferences *cirrus_conf = NULL;

	public:
		Cirrus_Flash()
		{
			cirrus_conf = new Preferences();
		}
		~Cirrus_Flash()
		{
			delete cirrus_conf;
		}

		bool Load(CIRRUS_Cirrus cirrus)
		{
			cirrus_conf->begin("Cirrus");
			if (cirrus == Cirrus_1)
				cirrus_conf->getBytes("data_cs1", (void*) &_data, sizeof(Cirrus_Flash_Struct));
			else
				cirrus_conf->getBytes("data_cs2", (void*) &_data, sizeof(Cirrus_Flash_Struct));
			cirrus_conf->end();

			// Vérifie la signature
			if (_data.signature[0] != EEPROM_SIG[0] && _data.signature[1] != EEPROM_SIG[1])
				return false;

			return true;
		}

		void Save(CIRRUS_Cirrus cirrus, CIRRUS_Calib_typedef *Calib, CIRRUS_Config_typedef *Config)
		{
			_data.signature[0] = EEPROM_SIG[0];
			_data.signature[1] = EEPROM_SIG[1];
			_data.Calib = *Calib;
			_data.Config = *Config;

			cirrus_conf->begin("Cirrus");
			if (cirrus == Cirrus_1)
				cirrus_conf->putBytes("data_cs1", (void*) &_data, sizeof(Cirrus_Flash_Struct));
			else
				cirrus_conf->putBytes("data_cs2", (void*) &_data, sizeof(Cirrus_Flash_Struct));
			cirrus_conf->end();
		}

		void Get_Calib(CIRRUS_Calib_typedef *Calib)
		{
			*Calib = _data.Calib;
		}

		void Get_Config(CIRRUS_Config_typedef *Config)
		{
			*Config = _data.Config;
		}
};

Cirrus_Flash Flash_data;

#endif

#ifdef LOG_CIRRUS_CONNECT
#include "Server_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#ifdef CIRRUS_USE_UART
// UART handle initialization
CIRRUS_SERIAL_MODE *Cirrus_UART = NULL;
#else
// SPI handle initialization
SPIClass *Cirrus_SPI = NULL;
// 2 MHz, MSB first, clock polarity high, clock phase 2 edge
SPISettings spisettings = SPISettings(2000000, MSBFIRST, SPI_MODE3);  // A priori mode 1 ou 3 SPI_MODE3

#ifdef ESP32
#define SPI_SCLK	GPIO_NUM_18
#define SPI_MISO	GPIO_NUM_19
#define SPI_MOSI	GPIO_NUM_23
#endif

#endif

// Le timeout pour les opérations uart et spi
uint32_t Cirrus_TimeOut = TIMEOUT600;

// Cirrus Reset pin
uint8_t Cirrus_RESET_Pin;

// Cirrus Select pin
uint8_t Cirrus1_Pin = 0, Cirrus2_Pin = 0;

// Le nombre de Cirrus présent. Au moins 1 !
volatile uint8_t CIRRUS_Number = 1;

// Le Cirrus en cours
volatile CIRRUS_Cirrus CIRRUS_Selected = Cirrus_None;
volatile uint8_t CirrusSelected_Pin = 0; // Initialisé à Cirrus1_Pin ou Cirrus2_Pin

// Un compteur des passages du zéro
//volatile uint32_t Zero_Cirrus = 0;
// Un top à chaque période (2 zéro)
//volatile uint8_t CIRRUS_Top_Period;

#ifdef CIRRUS_USE_UART
// UART initialisation
extern bool CIRRUS_Start(uint32_t baud);
#else
#define DELAY_SPI	1	// Datasheet say 1 us between each send
#define SPI_BEGIN_TRANSACTION()	digitalWrite(CirrusSelected_Pin, GPIO_PIN_RESET); \
		delayMicroseconds(DELAY_SPI); \
		Cirrus_SPI->beginTransaction(spisettings)
#define SPI_END_TRANSACTION()	Cirrus_SPI->endTransaction(); \
		digitalWrite(CirrusSelected_Pin, GPIO_PIN_SET)
#endif

#ifdef CIRRUS_CS5480
extern void CIRRUS_Select_Scale(CIRRUS_Cirrus cirrus);
#endif

/**
 * With UART, DATA is transmitted and received LSB first
 * With SPI, DATA is transmitted and received HSB first
 */
CIRRUS_State_typedef Cirrus_Transmit(uint8_t *pData, uint16_t Size)
{
	CIRRUS_State_typedef result = CIRRUS_OK;
#ifdef CIRRUS_USE_UART
	if (Size == 1)
		Cirrus_UART->write(*pData);
	else
	{
		for (int i = 0; i < Size; i++)
		{
			Cirrus_UART->write(*pData++);
		}
	}
#else
	SPI_BEGIN_TRANSACTION();
	if (Size == 1)
	{
		Cirrus_SPI->transfer(*pData);
    delayMicroseconds(DELAY_SPI);
	}
	else
	{
		pData += (Size - 1);
		do
		{
			Cirrus_SPI->transfer(*pData--);
			delayMicroseconds(DELAY_SPI);

		} while (Size-- > 1);
	}
	SPI_END_TRANSACTION();
#endif
	return result;
}

#ifdef CIRRUS_USE_UART
void Cirrus_ClearBuffer(void)
{
	while (Cirrus_UART->available())
		Cirrus_UART->read();
}
#endif

CIRRUS_State_typedef Cirrus_Receive(Bit_List *pResult)
{
#ifdef CIRRUS_USE_UART
	uint32_t timeout = millis();

	while ((Cirrus_UART->available() < 3) && ((millis() - timeout) < Cirrus_TimeOut))
		yield();
	if (Cirrus_UART->available() < 3)
	{
		CIRRUS_print_str("TIMEOUT");
		return CIRRUS_TIMEOUT;
	}
	else
	{
		pResult->LSB = (uint8_t) Cirrus_UART->read();
		pResult->MSB = (uint8_t) Cirrus_UART->read();
		pResult->HSB = (uint8_t) Cirrus_UART->read();
		return CIRRUS_OK;
	}
#else
	SPI_BEGIN_TRANSACTION();
	pResult->HSB = Cirrus_SPI->transfer(0xFF);
	delayMicroseconds(DELAY_SPI);
	pResult->MSB = Cirrus_SPI->transfer(0xFF);
	delayMicroseconds(DELAY_SPI);
	pResult->LSB = Cirrus_SPI->transfer(0xFF);
	SPI_END_TRANSACTION();
	delayMicroseconds(DELAY_SPI);
	return CIRRUS_OK;
#endif
}

// ********************************************************************************
// Initialization CIRRUS
// ********************************************************************************
/**
 * La procédure d'initialisation doit suivre la procédure suivante :
 * - CIRRUS_Select_Init() pour CS5480
 * - CIRRUS_UART_Init() ou CIRRUS_SPI_Init() suivant UART ou SPI
 * - CIRRUS_Select() pour CS5480 : sélection du Cirrus pour la calibration et configuration
 * - CIRRUS_TryConnexion() : Si ok, on poursuit par :
 * - CIRRUS_Calibration()
 * - CIRRUS_Configuration()
 * Si plusieurs Cirrus, on répète les 4 dernières opérations.
 */

#ifdef CIRRUS_USE_UART
/**
 * Initialisation des Cirrus en mode UART
 * - RESET_Pin : pin RESET
 * - baud : la vitesse de transmission souhaitée (note : après un reset, la vitesse est de 600 bauds)
 * UART_NB_BIT_8, UART_PARITY_NONE, UART_NB_STOP_BIT_1
 * Le pin RESET est en Push pull, no pull-up no pull-down, vitesse Low.
 * RESET est High en fonctionnement
 * Si on utilise le zero cross pour une autre fonction, la fonction CIRRUS_DO_Pin_Callback doit être
 * appelée dans HAL_GPIO_EXTI_Callback (ne pas oublier d'activer l'interruption).
 *
 * IMPORTANT pour le CS5480 :
 *    - on doit initialiser AVANT les pins de sélection avec CIRRUS_Select_Init
 *    - si on n'a qu'un seul CS5480, le pin doit rester LOW (en mode hardware)
 * NOTE: On n'utilise pas d'interruption
 * Aucun Cirrus de sélectionner à la fin
 */
void CIRRUS_UART_Init(void *puart, uint8_t RESET_Pin, uint32_t baud)
{
	Cirrus_UART = (CIRRUS_SERIAL_MODE*) puart;
	Cirrus_RESET_Pin = RESET_Pin;
	pinMode(RESET_Pin, OUTPUT);
	CIRRUS_Chip_Reset(GPIO_PIN_RESET);

	Cirrus_TimeOut = TIMEOUT600;

#ifdef DEBUG_CIRRUS
#ifdef CIRRUS_CS5480
  CIRRUS_print_str("*** CIRRUS CS5480 ***\r\n");
#else
  CIRRUS_print_str("*** CIRRUS CS5490 ***\r\n");
#endif
#endif

	// Première initialisation à 600 bauds
	CIRRUS_UART_Change_Baud(600);
	CIRRUS_print_str("CIRRUS UART Started\r\n");

	// Definition of UART and new baud rate
	if (CIRRUS_Start(baud))
	{
		// Second initialization at new baud rate
		if (baud != 600)
		{
			CIRRUS_UART_Change_Baud(baud);

#ifdef DEBUG_CIRRUS_BAUD
		Bit_List reg;
    // Verification
    if (read_register(P0_SerialCtrl, PAGE0, &reg))
    {
    	CIRRUS_print_blist(CS_RES_BAUD_NEW, &reg, 3);
    	CIRRUS_print_int("baud = ",baud);
    }
    else
    	CIRRUS_print_str(CS_RES_BAUD_VERIF_FAIL);
#endif
		}
	}
	else
		CIRRUS_print_str("CIRRUS Start Error\r\n");
}
#else
/**
 * Initialisation des Cirrus en mode SPI
 * Idem commentaire UART
 * Configuration spéciale de SPI :
 * - Baud rate <= 2 Mbits/s
 * - MSB First
 * - Clock Polarity : High
 * - Clock Phase : 2 Edge
 * Le CS n'est pas donné car utilise CirrusSelected_Pin
 * Aucun Cirrus de sélectionner à la fin
 */
void CIRRUS_SPI_Init(void *pspi, uint8_t RESET_Pin)
{
	Cirrus_SPI = (SPIClass*) pspi;
	Cirrus_RESET_Pin = RESET_Pin;
	pinMode(RESET_Pin, OUTPUT);

	CIRRUS_Chip_Reset(GPIO_PIN_RESET);

	Cirrus_SPI->begin(SPI_SCLK, SPI_MISO, SPI_MOSI, -1);

#ifdef DEBUG_CIRRUS
#ifdef CIRRUS_CS5480
  CIRRUS_print_str("*** CIRRUS CS5480 ***\r\n");
#else
  CIRRUS_print_str("*** CIRRUS CS5490 ***\r\n");
#endif
#endif

	CIRRUS_print_str("CIRRUS SPI Started\r\n");

	// Hard reset
	CIRRUS_Hard_Reset();
}
#endif

/**
 * Initialisation des pins de sélection des Cirrus
 * A n'utiliser que si on a plusieurs CS5480.
 * Plusieurs cas possibles :
 * - Si on a qu'un seul CS5480, c'est nécessaire si on est en SPI. En UART, seulement si on contrôle
 * en software mais le plus simple est de mettre CS à LOW en hardware (dans ce cas mettre -1 pour CS1_Pin)
 * ou de ne pas appeler cette fonction.
 * - Si on a plusieurs CS5480, c'est nécessaire en SPI et UART.
 * Au début, les CS sont HIGH (Cirrus non sélectionné)
 */
void CIRRUS_Select_Init(uint8_t CS1_Pin, uint8_t CS2_Pin)
{
	CIRRUS_Number = 1;
#ifdef CIRRUS_CS5480
	if (CS1_Pin > 0) // pas en UART Hardware
	{
		Cirrus1_Pin = CS1_Pin;
		pinMode(Cirrus1_Pin, OUTPUT);
		digitalWrite(Cirrus1_Pin, GPIO_PIN_SET);
		if (CS2_Pin > 0)
		{
			CIRRUS_Number = 2;
			Cirrus2_Pin = CS2_Pin;
			pinMode(Cirrus2_Pin, OUTPUT);
			digitalWrite(Cirrus2_Pin, GPIO_PIN_SET);
		}
	}
#else
	// Cas du CS5490, cette procédure est inutile
	(void) CS1_Pin;
	(void) CS2_Pin;
	return;
#endif
}

/**
 * Sélection d'un Cirrus (uniquement CS5480) :
 * Le CS utilisé est passé à LOW, l'autre à HIGH
 * En UART, on met le pin à LOW
 * En SPI, il est controlé au moment de la transaction
 */
void CIRRUS_Select(CIRRUS_Cirrus cirrus, bool with_scale)
{
#ifdef DEBUG_CIRRUS
  CIRRUS_print_str("Enter CIRRUS Select\r\n");
  CIRRUS_print_int("Number of Cirrus : ", CIRRUS_Number);
  CIRRUS_print_int("Selected : ", CIRRUS_Selected);
  CIRRUS_print_int("New Selected : ", cirrus);
#endif

#ifdef CIRRUS_CS5480
	if (CIRRUS_Selected == cirrus)
		return;

	// On déselect les cirrus
	if (cirrus == Cirrus_None)
	{
		CIRRUS_UnSelect();
		return;
	}

	// Un seul Cirrus
	if (CIRRUS_Number == 1)
	{
		CIRRUS_Selected = Cirrus_1;
		if (with_scale)
			CIRRUS_Select_Scale(cirrus);
		// Le pin select n'est pas hardware
		if (Cirrus1_Pin > 0)
		{
			CirrusSelected_Pin = Cirrus1_Pin;
#ifdef CIRRUS_USE_UART
			digitalWrite(Cirrus1_Pin, GPIO_PIN_RESET);
#endif
		}
		return;
	}

	// Deux Cirrus
	if (cirrus == Cirrus_1)
	{
		CirrusSelected_Pin = Cirrus1_Pin;
		digitalWrite(Cirrus2_Pin, GPIO_PIN_SET);
#ifdef CIRRUS_USE_UART
		digitalWrite(Cirrus1_Pin, GPIO_PIN_RESET);
#endif
//			digitalWrite(Cirrus2_Pin, GPIO_PIN_SET);
//			digitalWrite(Cirrus1_Pin, GPIO_PIN_RESET);
	}
	else
		if (cirrus == Cirrus_2)
		{
			CirrusSelected_Pin = Cirrus2_Pin;
			digitalWrite(Cirrus1_Pin, GPIO_PIN_SET);
#ifdef CIRRUS_USE_UART
			digitalWrite(Cirrus2_Pin, GPIO_PIN_RESET);
#endif
//				digitalWrite(Cirrus1_Pin, GPIO_PIN_SET);
//				digitalWrite(Cirrus2_Pin, GPIO_PIN_RESET);
		}

	CIRRUS_Selected = cirrus;
	if (with_scale)
		CIRRUS_Select_Scale(cirrus);
#else
	// Cas du CS5490 où on ne défini pas de CS
	CIRRUS_Selected = cirrus;
	(void) with_scale;
	return;
#endif
	// Mettre peut-être une tempo ici
}

void CIRRUS_UnSelect()
{
	CIRRUS_Selected = Cirrus_None;
	CirrusSelected_Pin = 0;
#ifdef CIRRUS_CS5480
	if (Cirrus1_Pin > 0)
		digitalWrite(Cirrus1_Pin, GPIO_PIN_SET);
	if (Cirrus2_Pin > 0)
		digitalWrite(Cirrus2_Pin, GPIO_PIN_SET);
#endif
}

#ifdef CIRRUS_USE_UART
void CIRRUS_UART_Change_Baud(uint32_t baud)
{
	// Pas besoin de faire Cirrus_UART->end() car fait dans le begin()
	Cirrus_UART->begin(baud);
	delay(100);  // For UART stabilize

	if (baud >= 128000)
		Cirrus_TimeOut = TIMEOUT512K;
	else
		Cirrus_TimeOut = TIMEOUT600;
}
#endif

void CIRRUS_Chip_Reset(GPIO_PinState PinState)
{
	digitalWrite(Cirrus_RESET_Pin, PinState);
}

void CIRRUS_DO_Pin_Callback()
{
//  Zero_Cirrus++;
//  if (Zero_Cirrus % 2 == 0)  CIRRUS_Top_Period = 1;

	// Le SSR en premier car on veut le maximum de précision
	CIRRUS_Interrupt_DO_Action_SSR();
	CIRRUS_Interrupt_DO_Action_Tore();
}

void CIRRUS_Interrupt_DO_Action_Tore()
{
}
void CIRRUS_Interrupt_DO_Action_SSR()
{
}

// ********************************************************************************
// Gestion de la sauvegarde des calibrations et configurations dans la FLASH
// ********************************************************************************
#ifdef CIRRUS_FLASH

/**
 * Lecture des données de calibration et de configuration dans la FLASH
 */
void CIRRUS_Load_From_FLASH(CIRRUS_Cirrus cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config)
{
	if (Flash_data.Load(cirrus))
	{
		Flash_data.Get_Calib(calib);
		Flash_data.Get_Config(config);
	}
}

/**
 * Sauvegarde des données de calibration et de configuration dans la FLASH
 */
void CIRRUS_Save_To_FLASH(CIRRUS_Cirrus cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config)
{
	Flash_data.Save(cirrus, calib, config);
}

void CIRRUS_Register_To_FLASH(void)
{
	CIRRUS_Calib_typedef calib;
	CIRRUS_Config_typedef config;

	CIRRUS_Get_Parameters(&calib, &config);
	CIRRUS_Save_To_FLASH(CIRRUS_Selected, &calib, &config);
}

#endif

// ********************************************************************************
// Gestion de la communication avec Cirrus_Connect
// ********************************************************************************

#ifdef CIRRUS_USE_UART
volatile uint32_t CirrusNewBaud = 0;
#ifdef CIRRUS_CS5480
CIRRUS_Cirrus LastCirrus;
#endif
#ifdef LOG_CIRRUS_CONNECT
extern bool CIRRUS_set_uart_baudrate(uint32_t baud);
#endif
#endif

#ifdef LOG_CIRRUS_CONNECT

// Commande Cirrus
// L'IHM est prioritaire sur les opérations de log
volatile bool CIRRUS_Lock_IHM = false;
// Une commande pour le Cirrus est en attente de traitement
volatile bool CIRRUS_Command = false;

/**
 * Analyse du message reçu de la part de Cirrus_Connect par ordre d'importance.
 * Ne pas oublier de remplacer HAL_UART_IRQHandler par Log_UART_RX_Callback dans le fichier d'interruption
 * Pour envoyer un message DEBUG en réponse à un message REG, utilisez le format ":#le message\03"
 * La gestion principale se trouve dans l'unité Log_UART
 * Le caractère de fin \03 est écrit en dur
 */
uint8_t UART_Message_Cirrus(uint8_t *RxBuffer)
{
	static uint8_t Cirrus_message[12];
	uint8_t message[100] = {0};
	uint64_t command;
	uint16_t len = 0;
	bool op; // READ = true
	uint8_t registre, page;
	Bit_List result;
	float scale[4] = {0};
	char *pbuffer;

//  ************** Lecture/écriture registre du Cirrus ************************
	if (Search_Balise(RxBuffer, "REG=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		// Cirrus est occupé
		if (CIRRUS_Command)
		{
			CIRRUS_print_str(":#Cirrus busy\03\r\n");
			return 1;
		}

		CIRRUS_Command = true;
		// Le format de la commande est hhmmllpprrcc\03
		// hhmmll : La valeur du registre à écrire
		// pp est la page, rr est le registre et cc la commande (1 = READ, 0 = WRITE)
		// Si commande = WRITE et Page = 1 on a une commande simple
		command = strtoll((char*) message, NULL, 10);
#ifdef DEBUG_CONNECT
    sprintf((char *)message,":#%s\r\n", (char *)aRxBuffer); // Pas le \03
    CIRRUS_print_str((char *)message);
    HAL_Delay(1);
#endif
		op = ((command & 0x01) == 1);
		command >>= 8;
		registre = (uint8_t) (command & 0xFF);
		command >>= 8;
		page = (uint8_t) (command & 0xFF);

		// On vérifie au moins que la page est bonne
		if ((page != 0) && (page != 1) && (page != 16) && (page != 17) && (page != 18))
		{
			CIRRUS_print_str(":#Cirrus page error\03\r\n");
			CIRRUS_Command = false;
			return 1;
		}

#ifdef DEBUG_CONNECT
    sprintf((char *)message,":#Page=%d; Registre=%d \03\r\n", page, registre);
    CIRRUS_print_str((char *)message);
    HAL_Delay(1);
#endif
		if (op) // READ
		{
			if (read_register(registre, page, &result))
			{
				// Expédition du résultat au format ":résultat en hexa\03"
				// : et \03 sont les caractères de début et de fin du message
				sprintf((char*) Cirrus_message, ":%X\03\r\n", (unsigned int) result.Bit32);
				CIRRUS_print_str((char*) Cirrus_message);
			}
			else
				CIRRUS_print_str(":#ERROR\03\r\n");
		}
		else // WRITE
		{
			// PAGE = 1 n'existe pas. Est utilisé pour envoyer une instruction.
			if (page == 1)
			{
				send_instruction(registre);
			}
			else
			{
				result.Bit32 = (command >> 8);
#ifdef DEBUG_CONNECT
	sprintf((char *)message,":#Command : (LSB: 0x%.2X, MSB: 0x%.2X, HSB: 0x%.2X), Checksum: %d\03\r\n",
		result.LSB, result.MSB, result.HSB, result.CHECK);
	CIRRUS_print_str((char *)message);
	HAL_Delay(1);
#endif
				write_register(registre, page, &result);
			}
			CIRRUS_print_str(":#REG_OK\03\r\n");
		}
		CIRRUS_Command = false;
		return 1;
	}

//  ************** Message REPEAT pour le Cirrus ******************************
	if (strstr((char*) RxBuffer, "REPEAT=") != NULL)
	{
		// On renvoie le dernier message
		CIRRUS_print_str((char*) Cirrus_message);
		return 1;
	}

//  ************** Changement de vitesse du Cirrus (mode UART) ****************
	if (Search_Balise(RxBuffer, "BAUD=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
#ifdef CIRRUS_USE_UART
		CirrusNewBaud = strtol((char*) message, NULL, 10);
		Cirrus_TimeOut = TIMEOUT600;
		// First change Cirrus baud rate
		CIRRUS_set_uart_baudrate(CirrusNewBaud);

#ifdef CIRRUS_CS5480
		// Si on a 2 Cirrus
		if (CIRRUS_Number == 2)
		{
			LastCirrus = CIRRUS_Selected;
			// On sélectionne le deuxième Cirrus et on change sa vitesse
			if (CIRRUS_Selected == Cirrus_1)
				CIRRUS_Select(Cirrus_2);
			else
				CIRRUS_Select(Cirrus_1);
			CIRRUS_set_uart_baudrate(CirrusNewBaud);
			// On revient sur le Cirrus en cours
			CIRRUS_Select(LastCirrus);
		}
#endif

		// Second change UART communication baud rate
		CIRRUS_UART_Change_Baud(CirrusNewBaud);

#ifdef DEBUG_CONNECT
    sprintf((char *)message,":#CirrusNewBaud=%ld\03\r\n", CirrusNewBaud);
    CIRRUS_print_str((char *)message);
    delay(1);
#endif

#else
		// On est en SPI, donc on ignore cette commande
		CIRRUS_print_str("Baud ignore, use SPI.\r\n");
#endif
		return 1;
	}

//  ************** Lecture des coefficient de SCALE du Cirrus *****************
	if (Search_Balise(RxBuffer, "SCALE=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		// Récupération des SCALE
		if (len == 0)
		{
			CIRRUS_GetScale((float*) &scale);
			sprintf((char*) message, "%.2f;%.2f;%.2f;%.2f;\03\r\n", scale[0], scale[1], scale[2],
					scale[3]);
			CIRRUS_print_str((char*) message);
		}
		else
		{
			pbuffer = strtok((char*) message, ";");
			scale[0] = strtod(pbuffer, NULL);
			for (len = 1; len < 4; len++)
			{
				scale[len] = strtod(strtok(NULL, ";"), NULL);
			}
			CIRRUS_SetScale((float*) &scale);
		}
		return 1;
	}

//  ************** Ecriture des registres du Cirrus dans la FLASH *************
#ifdef CIRRUS_FLASH
	if (strstr((char*) RxBuffer, "FLASH=") != NULL)
	{
		// Sauvegarde dans la FLASH
		CIRRUS_Register_To_FLASH();
		CIRRUS_print_str("FLASH Data OK\r\n");
		return 1;
	}
#endif

//  ************** Changement de Cirrus ***************************************
	if (Search_Balise(RxBuffer, "CS=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		if (strcmp((char*) message, "0") == 0)
		{
			CIRRUS_Select(Cirrus_1);
			CIRRUS_print_str("Cirrus 1 selected\r\n");
		}
		else
		{
			CIRRUS_Select(Cirrus_2);
			CIRRUS_print_str("Cirrus 2 selected\r\n");
		}
		return 1;
	}

//  ************** Message lock IHM pour calibration du Cirrus ****************
	if (strstr((char*) RxBuffer, "LOCK=") != NULL)
	{
		// Toggle flag Calibration
		CIRRUS_Lock_IHM = !CIRRUS_Lock_IHM;
		if (CIRRUS_Lock_IHM)
			CIRRUS_print_str("IHM Locked\r\n");
		else
			CIRRUS_print_str("IHM unLocked\r\n");
		return 1;
	}

	return 0;
} // end UART_Message_Cirrus()

// ********************************************************************************
// Fonction pour la communication Wifi
// ********************************************************************************

/**
 * Change Cirrus baud when use UART
 */
bool CIRRUS_COM_ChangeBaud(uint8_t *Baud, char *response)
{
#ifdef CIRRUS_USE_UART
	CirrusNewBaud = strtol((char*) Baud, NULL, 10);
	Cirrus_TimeOut = TIMEOUT600;
	// First change Cirrus baud rate
	if (!CIRRUS_set_uart_baudrate(CirrusNewBaud))
	{
		strcpy(response, "BAUD_ERROR");
		return false;
	}

#ifdef CIRRUS_CS5480
	// Si on a 2 Cirrus
	if (CIRRUS_Number == 2)
	{
		LastCirrus = CIRRUS_Selected;
		// On sélectionne le deuxième Cirrus et on change sa vitesse
		if (CIRRUS_Selected == Cirrus_1)
			CIRRUS_Select(Cirrus_2);
		else
			CIRRUS_Select(Cirrus_1);
		CIRRUS_set_uart_baudrate(CirrusNewBaud);
		// On revient sur le Cirrus en cours
		CIRRUS_Select(LastCirrus);
	}
#endif

	// Second change UART communication baud rate
	CIRRUS_UART_Change_Baud(CirrusNewBaud);

#ifdef DEBUG_CONNECT
  sprintf((char *)message,":#CirrusNewBaud=%ld\03\r\n", CirrusNewBaud);
  CIRRUS_print_str((char *)message);
  delay(1);
#endif
	strcpy(response, "BAUD_OK");

#else
	// On est en SPI, donc on ignore cette commande
	CIRRUS_print_str("Baud ignore, use SPI.\r\n");
#endif
	return true;
}

/**
 * Get and Set Scale
 * Get scale if string Request is empty
 * Set scale if string Request is not empty
 * Scale format is V1_SCALE;I1_SCALE;V2_SCALE;I2_SCALE;LOG_ETX_STR
 * If channel 2 is not present, V2_SCALE and I2_SCALE = 0
 */
char* CIRRUS_COM_Scale(uint8_t *Request, float *scale, char *response)
{
	char *pbuffer;
	uint8_t len = strlen((const char*) Request);

	// Récupération des SCALE
	if (len == 0)
	{
		CIRRUS_GetScale(scale);
		sprintf((char*) response, "%.2f;%.2f;%.2f;%.2f;\03\r\n", scale[0], scale[1], scale[2],
				scale[3]);
	}
	else
	{
		pbuffer = strtok((char*) Request, ";#");
		if (pbuffer != NULL)
		{
			scale[0] = strtof(pbuffer, NULL);
			for (len = 1; len < 4; len++)
			{
				scale[len] = strtof(strtok(NULL, ";#"), NULL);
			}
			CIRRUS_SetScale(scale);
			strcpy(response, "SCALE_OK");
		}
		else
			strcpy(response, "SCALE_ERROR");
	}

	return response;
}

/**
 * Get/Set Cirrus register
 * Le format de la commande est hhmmllpprrcc (uint64_t string)
 * hhmmll : La valeur du registre à lire/écrire
 * pp est la page, rr est le registre et cc la commande (1 = READ, 0 = WRITE)
 * Si commande = WRITE et Page = 1 on a une instruction
 */
char* CIRRUS_COM_Register(uint8_t *Request, char *response)
{
	uint64_t command;
	bool op; // READ = true
	uint8_t registre, page;
	Bit_List result;

	// Cirrus est occupé
	if (CIRRUS_Command)
	{
		strcpy(response, "Cirrus busy");
		return response;
	}

	CIRRUS_Command = true;

	// command is a uint64_t
	command = strtoull((char*) Request, NULL, 10);
	op = ((command & 0x01) == 1);
	command >>= 8;
	registre = (uint8_t) (command & 0xFF);
	command >>= 8;
	page = (uint8_t) (command & 0xFF);

	// On vérifie au moins que la page est bonne
	if ((page != 0) && (page != 1) && (page != 16) && (page != 17) && (page != 18))
	{
		CIRRUS_Command = false;
		strcpy(response, "Cirrus page error");
		return response;
	}

	if (op) // READ
	{
		if (read_register(registre, page, &result))
		{
			// Expédition du résultat en hexa au format : 0xXXXXXX
			sprintf(response, "0x%.6X", (unsigned int)result.Bit32);
		}
		else
			strcpy(response, "REG_ERROR");
	}
	else // WRITE
	{
		// PAGE = 1 n'existe pas. Est utilisé pour envoyer une instruction.
		if (page == 1)
		{
			send_instruction(registre);
		}
		else
		{
			result.Bit32 = (command >> 8);
			write_register(registre, page, &result);
		}
		strcpy(response, "REG_OK");
	}
	CIRRUS_Command = false;
	return response;
}

/**
 * Get several registers
 * Request = Request1#Request2#Request3 ....
 * Return val1#val2#val3 .... en hexa au format : 0xXXXXXX
 */
char* CIRRUS_COM_Register_Multi(uint8_t *Request, char *response)
{
	char *message[255] = {0};
	char *pbuffer;

	strcpy(response, "");
	pbuffer = strtok((char*) Request, ";#");
	while (pbuffer != NULL)
	{
		strcat(response, CIRRUS_COM_Register((uint8_t*) pbuffer, (char*) message));
		if ((pbuffer = strtok(NULL, ";#")) != NULL)
			strcat(response, "#");
	}

	return response;
}

#ifdef CIRRUS_FLASH
char* CIRRUS_COM_Flash(char *response)
{
	// Sauvegarde dans la FLASH
	CIRRUS_Register_To_FLASH();
	CIRRUS_print_str("FLASH Data OK\r\n");
	strcpy(response, "FLASH_OK");
	return response;
}
#endif

#endif // LOG_CIRRUS_CONNECT

// ********************************************************************************
// End of file
// ********************************************************************************
