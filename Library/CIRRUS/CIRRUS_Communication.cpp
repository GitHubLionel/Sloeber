#include "CIRRUS.h"
#include "CIRRUS_RES_EN.h"
#include "Debug_utils.h"

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

		bool Load(char id)
		{
			const char *key = "data_cs" + id;

			cirrus_conf->begin("Cirrus");
			cirrus_conf->getBytes(key, (void*) &_data, sizeof(Cirrus_Flash_Struct));
			cirrus_conf->end();

			// Vérifie la signature
			if (_data.signature[0] != EEPROM_SIG[0] && _data.signature[1] != EEPROM_SIG[1])
				return false;

			return true;
		}

		void Save(char id, CIRRUS_Calib_typedef *Calib, CIRRUS_Config_typedef *Config)
		{
			_data.signature[0] = EEPROM_SIG[0];
			_data.signature[1] = EEPROM_SIG[1];
			_data.Calib = *Calib;
			_data.Config = *Config;
			const char *key = "data_cs" + id;

			cirrus_conf->begin("Cirrus");
			cirrus_conf->putBytes(key, (void*) &_data, sizeof(Cirrus_Flash_Struct));
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#ifndef CIRRUS_USE_UART
#ifdef ESP32
#define SPI_SCLK	GPIO_NUM_18
#define SPI_MISO	GPIO_NUM_19
#define SPI_MOSI	GPIO_NUM_23
#endif
#endif

// Todo à corriger
CIRRUS_Base *Current_Cirrus;

// Un compteur des passages du zéro
//volatile uint32_t Zero_Cirrus = 0;
// Un top à chaque période (2 zéro)
//volatile uint8_t CIRRUS_Top_Period;

#ifndef CIRRUS_USE_UART
#define DELAY_SPI	1	// Datasheet say 1 us between each send
#define SPI_BEGIN_TRANSACTION()	digitalWrite(CirrusSelected_Pin, GPIO_PIN_RESET); \
		delayMicroseconds(DELAY_SPI); \
		Cirrus_SPI->beginTransaction(spisettings)
#define SPI_END_TRANSACTION()	Cirrus_SPI->endTransaction(); \
		digitalWrite(CirrusSelected_Pin, GPIO_PIN_SET)
#endif

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

CIRRUS_Communication::CIRRUS_Communication(void *com, uint8_t RESET_Pin)
{
	Initialize(com, RESET_Pin);
}

/**
 * Initialise la communication
 */
void CIRRUS_Communication::Initialize(void *com, uint8_t RESET_Pin)
{
#ifdef CIRRUS_USE_UART
	Cirrus_UART = (CIRRUS_SERIAL_MODE*) com;
	Cirrus_TimeOut = TIMEOUT600;
#else
	Cirrus_SPI = (SPIClass*) com;
#endif
	Cirrus_RESET_Pin = RESET_Pin;
	pinMode(Cirrus_RESET_Pin, OUTPUT);
	Chip_Reset(GPIO_PIN_RESET);
}

/**
 * Start the communication
 */
void CIRRUS_Communication::begin(void)
{
#ifdef CIRRUS_USE_UART
#ifdef ESP32
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)) // ESP32 2.0.x
	// Pour ESP32 3.x (IDF 5.x), ils ont changé les numéros des GPIO !!!!!!
	// On doit donc ré-affecter les bons GPIO
	// https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html
	Cirrus_UART->setPins(CIRRUS_RX_GPIO, CIRRUS_TX_GPIO);
#endif
#endif

	// Première initialisation à 600 bauds
	UART_Change_Baud(600);
#else
	Cirrus_SPI->begin(SPI_SCLK, SPI_MISO, SPI_MOSI, -1);
#endif

	// Hard reset the cirrus (Cirrus baudrate is 600)
	Hard_Reset();

	Comm_started = true;
}

#ifdef CIRRUS_USE_UART
void CIRRUS_Communication::UART_Change_Baud(uint32_t baud)
{
	// Pas besoin de faire Cirrus_UART->end() car fait dans le begin()
	Cirrus_UART->begin(baud);
	CSDelay(100);  // For UART stabilize

	if (baud >= 128000)
		Cirrus_TimeOut = TIMEOUT512K;
	else
		Cirrus_TimeOut = TIMEOUT600;
}
#endif

void CIRRUS_Communication::Chip_Reset(GPIO_PinState PinState)
{
	digitalWrite(Cirrus_RESET_Pin, PinState);
}

/**
 * Hard reset utilisant le pin reset
 * Reset pin est mis à LOW pendant 2 ms (minimum 120 us)
 * ATTENTION, en UART la vitesse est réinitialisée à 600 bauds
 */
void CIRRUS_Communication::Hard_Reset(void)
{
	Chip_Reset(GPIO_PIN_SET);
	CSDelay(2);
	Chip_Reset(GPIO_PIN_RESET);
	CSDelay(2);    // Minimum 120 µs
	Chip_Reset(GPIO_PIN_SET);
	CSDelay(2);
}

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
/**
 * Ajout de Cirrus pour la communication.
 * Seulement quand on a plusieurs Cirrus
 */
void CIRRUS_Communication::AddCirrus(CIRRUS_Base *cirrus, uint8_t select_Pin)
{
	if (cirrus != NULL)
	{
		cirrus->SetCommunication(*this);
		cirrus->Cirrus_Pin = select_Pin;
		if (select_Pin != 0)
		{
			pinMode(select_Pin, OUTPUT);
			digitalWrite(select_Pin, GPIO_PIN_SET);
		}
		m_Cirrus.push_back(cirrus);
	}
}

/**
 * Sélection d'un Cirrus (uniquement CS5480) :
 * Le CS utilisé est passé à LOW, l'autre à HIGH
 * En UART, on met le pin à LOW
 * En SPI, il est controlé au moment de la transaction
 */
CIRRUS_Base* CIRRUS_Communication::SelectCirrus(uint8_t position, CIRRUS_Channel channel)
{
//#ifdef DEBUG_CIRRUS
//	  CIRRUS_print_str("Enter CIRRUS Select\r\n");
//	  CIRRUS_print_int("Number of Cirrus : ", CIRRUS_Number);
//	  CIRRUS_print_int("Selected : ", CIRRUS_Selected);
//	  CIRRUS_print_int("New Selected : ", cirrus);
//	#endif

	if (m_Cirrus.size() == 0)
		return NULL;

	if (Selected == position)
	{
		CIRRUS_Base *cs = GetCirrus(position);
		if (channel != Channel_none)
			cs->SelectChannel(channel);
		return cs;
	}

	if (m_Cirrus.size() > 1)
	{
		// We turn all select pin to high
		for (CirrusList::iterator it = m_Cirrus.begin(); it != m_Cirrus.end(); it++)
		{
			if ((*it)->Cirrus_Pin != 0)
			{
				digitalWrite((*it)->Cirrus_Pin, GPIO_PIN_SET);
			}
		}
	}
	Selected = position;
	CIRRUS_Base *cs = GetCirrus(position);
	if (cs != NULL)
	{
		Selected_Pin = cs->Cirrus_Pin;
#ifdef CIRRUS_USE_UART
		if (Selected_Pin != 0)
			digitalWrite(Selected_Pin, GPIO_PIN_RESET);
#endif
		if (channel != Channel_none)
			cs->SelectChannel(channel);
	}
	else
		Selected_Pin = 0;
	return cs;
}

CIRRUS_Base* CIRRUS_Communication::GetCirrus(int position)
{
	if ((position >= (int) m_Cirrus.size()) || position < 0)
		return NULL;
	return m_Cirrus[position];
}

// ********************************************************************************
// Communication CIRRUS
// ********************************************************************************

/**
 * With UART, DATA is transmitted and received LSB first
 * With SPI, DATA is transmitted and received HSB first
 */
CIRRUS_State_typedef CIRRUS_Communication::Transmit(uint8_t *pData, uint16_t Size)
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
void CIRRUS_Communication::ClearBuffer(void)
{
	while (Cirrus_UART->available())
		Cirrus_UART->read();
}
#endif

/**
 * Par défaut, Size = 3, pas de checksum
 */
CIRRUS_State_typedef CIRRUS_Communication::Receive(Bit_List *pResult, uint8_t Size)
{
#ifdef CIRRUS_USE_UART
	uint32_t timeout = millis();

	while ((Cirrus_UART->available() < Size) && ((millis() - timeout) < Cirrus_TimeOut))
		yield();
	if (Cirrus_UART->available() < Size)
	{
//		CIRRUS_print_str("TIMEOUT");
		return CIRRUS_TIMEOUT;
	}
	else
	{
		pResult->LSB = (uint8_t) Cirrus_UART->read();
		pResult->MSB = (uint8_t) Cirrus_UART->read();
		pResult->HSB = (uint8_t) Cirrus_UART->read();
//		pResult->CHECK = (uint8_t) Cirrus_UART->read();
		return CIRRUS_OK;
	}
#else
	SPI_BEGIN_TRANSACTION();
//	pResult->CHECK = Cirrus_SPI->transfer(0xFF);
//	delayMicroseconds(DELAY_SPI);
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
void CIRRUS_Load_From_FLASH(char id_cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config)
{
	if (Flash_data.Load(id_cirrus))
	{
		Flash_data.Get_Calib(calib);
		Flash_data.Get_Config(config);
	}
}

/**
 * Sauvegarde des données de calibration et de configuration dans la FLASH
 */
void CIRRUS_Save_To_FLASH(char id_cirrus, CIRRUS_Calib_typedef *calib,
		CIRRUS_Config_typedef *config)
{
	Flash_data.Save(id_cirrus, calib, config);
}

void CIRRUS_Register_To_FLASH(void)
{
	CIRRUS_Calib_typedef calib;
	CIRRUS_Config_typedef config;

	Current_Cirrus->Get_Parameters(&calib, &config);
	CIRRUS_Save_To_FLASH('1', &calib, &config);
}

#endif

// ********************************************************************************
// Gestion de la communication avec Cirrus_Connect
// ********************************************************************************

#ifdef CIRRUS_USE_UART
//volatile uint32_t CirrusNewBaud = 0;
//#ifdef CIRRUS_CS5480
//CIRRUS_Cirrus LastCirrus;
//#endif
#ifdef LOG_CIRRUS_CONNECT
//extern bool CIRRUS_set_uart_baudrate(uint32_t baud);
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
			Current_Cirrus->print_str(":#Cirrus busy\03\r\n");
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
    Current_Cirrus->print_str((char *)message);
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
			Current_Cirrus->print_str(":#Cirrus page error\03\r\n");
			CIRRUS_Command = false;
			return 1;
		}

#ifdef DEBUG_CONNECT
    sprintf((char *)message,":#Page=%d; Registre=%d \03\r\n", page, registre);
    Current_Cirrus->print_str((char *)message);
    HAL_Delay(1);
#endif
		if (op) // READ
		{
			if (Current_Cirrus->read_register(registre, page, &result))
			{
				// Expédition du résultat au format ":résultat en hexa\03"
				// : et \03 sont les caractères de début et de fin du message
				sprintf((char*) Cirrus_message, ":%X\03\r\n", (unsigned int) result.Bit32);
				Current_Cirrus->print_str((char*) Cirrus_message);
			}
			else
				Current_Cirrus->print_str(":#ERROR\03\r\n");
		}
		else // WRITE
		{
			// PAGE = 1 n'existe pas. Est utilisé pour envoyer une instruction.
			if (page == 1)
			{
				Current_Cirrus->send_instruction(registre);
			}
			else
			{
				result.Bit32 = (command >> 8);
#ifdef DEBUG_CONNECT
	sprintf((char *)message,":#Command : (LSB: 0x%.2X, MSB: 0x%.2X, HSB: 0x%.2X), Checksum: %d\03\r\n",
		result.LSB, result.MSB, result.HSB, result.CHECK);
	Current_Cirrus->print_str((char *)message);
	HAL_Delay(1);
#endif
				Current_Cirrus->write_register(registre, page, &result);
			}
			Current_Cirrus->print_str(":#REG_OK\03\r\n");
		}
		CIRRUS_Command = false;
		return 1;
	}

//  ************** Message REPEAT pour le Cirrus ******************************
	if (strstr((char*) RxBuffer, "REPEAT=") != NULL)
	{
		// On renvoie le dernier message
		Current_Cirrus->print_str((char*) Cirrus_message);
		return 1;
	}

//  ************** Changement de vitesse du Cirrus (mode UART) ****************
	if (Search_Balise(RxBuffer, "BAUD=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
#ifdef CIRRUS_USE_UART
		uint32_t CirrusNewBaud = strtol((char*) message, NULL, 10);
//		Cirrus_TimeOut = TIMEOUT600;
		// First change Cirrus baud rate
		Current_Cirrus->set_uart_baudrate(CirrusNewBaud);

//#ifdef CIRRUS_CS5480
//		// Si on a 2 Cirrus
//		if (CIRRUS_Number == 2)
//		{
//			LastCirrus = CIRRUS_Selected;
//			// On sélectionne le deuxième Cirrus et on change sa vitesse
//			if (CIRRUS_Selected == Cirrus_1)
//				CIRRUS_Select(Cirrus_2);
//			else
//				CIRRUS_Select(Cirrus_1);
//			Current_Cirrus->set_uart_baudrate(CirrusNewBaud);
//			// On revient sur le Cirrus en cours
//			CIRRUS_Select(LastCirrus);
//		}
//#endif

		// Second change UART communication baud rate
		Current_Cirrus->SetUARTBaud(CirrusNewBaud, true);

#ifdef DEBUG_CONNECT
    sprintf((char *)message,":#CirrusNewBaud=%ld\03\r\n", CirrusNewBaud);
    Current_Cirrus->print_str((char *)message);
    delay(1);
#endif

#else
		// On est en SPI, donc on ignore cette commande
		Current_Cirrus->print_str("Baud ignore, use SPI.\r\n");
#endif
		return 1;
	}

//  ************** Lecture des coefficient de SCALE du Cirrus *****************
	if (Search_Balise(RxBuffer, "SCALE=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		// Récupération des SCALE
		if (len == 0)
		{
			Current_Cirrus->GetScale((float*) &scale);
			sprintf((char*) message, "%.2f;%.2f;%.2f;%.2f;\03\r\n", scale[0], scale[1], scale[2],
					scale[3]);
			Current_Cirrus->print_str((char*) message);
		}
		else
		{
			pbuffer = strtok((char*) message, ";");
			scale[0] = strtod(pbuffer, NULL);
			for (len = 1; len < 4; len++)
			{
				scale[len] = strtod(strtok(NULL, ";"), NULL);
			}
			Current_Cirrus->SetScale((float*) &scale);
		}
		return 1;
	}

//  ************** Ecriture des registres du Cirrus dans la FLASH *************
#ifdef CIRRUS_FLASH
	if (strstr((char*) RxBuffer, "FLASH=") != NULL)
	{
		// Sauvegarde dans la FLASH
		CIRRUS_Register_To_FLASH();
		Current_Cirrus->print_str("FLASH Data OK\r\n");
		return 1;
	}
#endif

//  ************** Changement de Cirrus ***************************************
	if (Search_Balise(RxBuffer, "CS=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		if (strcmp((char*) message, "0") == 0)
		{
//			Current_Cirrus->Select(Cirrus_1);
			Current_Cirrus->print_str("Cirrus 1 selected\r\n");
		}
		else
		{
//			Current_Cirrus->Select(Cirrus_2);
			Current_Cirrus->print_str("Cirrus 2 selected\r\n");
		}
		return 1;
	}

//  ************** Message lock IHM pour calibration du Cirrus ****************
	if (strstr((char*) RxBuffer, "LOCK=") != NULL)
	{
		// Toggle flag Calibration
		CIRRUS_Lock_IHM = !CIRRUS_Lock_IHM;
		if (CIRRUS_Lock_IHM)
			Current_Cirrus->print_str("IHM Locked\r\n");
		else
			Current_Cirrus->print_str("IHM unLocked\r\n");
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
	uint32_t CirrusNewBaud = strtol((char*) Baud, NULL, 10);
//	Cirrus_TimeOut = TIMEOUT600;
	// First change Cirrus baud rate
	if (!Current_Cirrus->set_uart_baudrate(CirrusNewBaud))
	{
		strcpy(response, "BAUD_ERROR");
		return false;
	}

//#ifdef CIRRUS_CS5480
//	// Si on a 2 Cirrus
//	if (CIRRUS_Number == 2)
//	{
//		LastCirrus = CIRRUS_Selected;
//		// On sélectionne le deuxième Cirrus et on change sa vitesse
//		if (CIRRUS_Selected == Cirrus_1)
//			CIRRUS_Select(Cirrus_2);
//		else
//			CIRRUS_Select(Cirrus_1);
//		Current_Cirrus->set_uart_baudrate(CirrusNewBaud);
//		// On revient sur le Cirrus en cours
//		CIRRUS_Select(LastCirrus);
//	}
//#endif

	// Second change UART communication baud rate
	Current_Cirrus->SetUARTBaud(CirrusNewBaud, true);

#ifdef DEBUG_CONNECT
  sprintf((char *)message,":#CirrusNewBaud=%ld\03\r\n", CirrusNewBaud);
  Current_Cirrus->print_str((char *)message);
  delay(1);
#endif
	strcpy(response, "BAUD_OK");

#else
	// On est en SPI, donc on ignore cette commande
	Current_Cirrus->print_str("Baud ignore, use SPI.\r\n");
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
		Current_Cirrus->GetScale(scale);
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
			Current_Cirrus->SetScale(scale);
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
		if (Current_Cirrus->read_register(registre, page, &result))
		{
			// Expédition du résultat en hexa au format : 0xXXXXXX
			sprintf(response, "0x%.6X", (unsigned int) result.Bit32);
		}
		else
			strcpy(response, "REG_ERROR");
	}
	else // WRITE
	{
		// PAGE = 1 n'existe pas. Est utilisé pour envoyer une instruction.
		if (page == 1)
		{
			Current_Cirrus->send_instruction(registre);
		}
		else
		{
			result.Bit32 = (command >> 8);
			Current_Cirrus->write_register(registre, page, &result);
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
//	CIRRUS_print_str("FLASH Data OK\r\n");
	strcpy(response, "FLASH_OK");
	return response;
}
#endif

#endif // LOG_CIRRUS_CONNECT

// ********************************************************************************
// End of file
// ********************************************************************************
