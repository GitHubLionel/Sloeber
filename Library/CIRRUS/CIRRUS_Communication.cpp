#include "CIRRUS.h"
#include "CIRRUS_RES_EN.h"

// Calibration
#ifdef CIRRUS_CALIBRATION
#include "CIRRUS_Calibration.h"
extern CIRRUS_Calibration CS_Calibration;
bool Calibration = false;
#endif

// End Text string
#define LOG_ETX_STR  	"\03"


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

// Flash Data global instance
Cirrus_Flash Flash_data;

// ********************************************************************************
// Gestion de la sauvegarde des calibrations et configurations dans la FLASH
// ********************************************************************************

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

void CIRRUS_Communication::Register_To_FLASH(char id_cirrus)
{
	CIRRUS_Calib_typedef calib;
	CIRRUS_Config_typedef config;

	if (CurrentCirrus == NULL)
		return ;

	CurrentCirrus->Get_Parameters(&calib, &config);
	CIRRUS_Save_To_FLASH(id_cirrus, &calib, &config);
}
#endif  // Flash

#ifdef LOG_CIRRUS_CONNECT
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

// Un compteur des passages du zéro
//volatile uint32_t Zero_Cirrus = 0;
// Un top à chaque période (2 zéro)
//volatile uint8_t CIRRUS_Top_Period;

// Mode SPI
#ifndef CIRRUS_USE_UART
#ifdef ESP32
#define SPI_SCLK	GPIO_NUM_18
#define SPI_MISO	GPIO_NUM_19
#define SPI_MOSI	GPIO_NUM_23
#endif

#define DELAY_SPI	1	// Datasheet say 1 us between each send
#define SPI_BEGIN_TRANSACTION()	digitalWrite(Selected_Pin, GPIO_PIN_RESET); \
		delayMicroseconds(DELAY_SPI); \
		Cirrus_SPI->beginTransaction(spisettings)
#define SPI_END_TRANSACTION()	Cirrus_SPI->endTransaction(); \
		digitalWrite(Selected_Pin, GPIO_PIN_SET)
#endif

// ********************************************************************************
// Initialization CIRRUS
// ********************************************************************************
/**
 * La procédure d'initialisation doit suivre la procédure suivante :
 * - Déclaration de la communication CIRRUS_Communication
 * - Déclaration du/des Cirrus CIRRUS_CS5490, CIRRUS_CS548x
 * - Démarrage de la communication : CIRRUS_Communication.begin()
 * - Démarrage du/des Cirrus : CIRRUS_CS5490.begin()
 * - Si plusieurs Cirrus, les ajouter à la communication
 * - CIRRUS_CS5490.TryConnexion() : Si ok, on poursuit par :
 * - CIRRUS_CS5490.Calibration()
 * - CIRRUS_CS5490.Configuration()
 * Si plusieurs Cirrus, sélectionner le bon cirrus dans CIRRUS_Communication.
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

/**
 * Constructeur
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
	Cirrus_UART->begin(600);
	Cirrus_TimeOut = TIMEOUT600;
	CSDelay(100);  // For UART stabilize
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
	Cirrus_UART->updateBaudRate(baud);
	CSDelay(10);  // For UART stabilize

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
 * A n'utiliser que si on a plusieurs CS548x ou si on est en SPI.
 * Plusieurs cas possibles :
 * - Si on a qu'un seul CS548x, c'est nécessaire si on est en SPI. En UART, seulement si on contrôle
 * en software mais le plus simple est de mettre CS à LOW en hardware (dans ce cas mettre -1 pour CS1_Pin)
 * ou de ne pas appeler cette fonction.
 * - Si on a plusieurs CS548x, c'est nécessaire en SPI et UART.
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
 * Pour inactiver un Cirrus via son pin
 */
void CIRRUS_Communication::DisableCirrus(uint8_t select_Pin)
{
	if (select_Pin != 0)
	{
		pinMode(select_Pin, OUTPUT);
		digitalWrite(select_Pin, GPIO_PIN_SET);
	}
}

/**
 * Sélection d'un Cirrus (uniquement CS548x) :
 * Le CS utilisé est passé à LOW, l'autre à HIGH
 * En UART, on met le pin à LOW
 * En SPI, il est controlé au moment de la transaction
 */
CIRRUS_Base* CIRRUS_Communication::SelectCirrus(uint8_t position, CIRRUS_Channel channel)
{
	if (m_Cirrus.size() == 0)
		return NULL;

#ifdef DEBUG_CIRRUS
	CurrentCirrus->print_str("Enter CIRRUS Select\r\n");
	CurrentCirrus->print_int("Number of Cirrus : ", m_Cirrus.size());
	CurrentCirrus->print_int("Selected : ", Selected);
	CurrentCirrus->print_int("New Selected : ", position);
#endif

	// We already are in the selected Cirrus, just update pointer
	if (Selected == position)
	{
		CIRRUS_Base *cs = GetCirrus(position);
		if (channel != Channel_none)
			cs->SelectChannel(channel);
		CurrentCirrus = cs;
		Selected_Pin = cs->Cirrus_Pin;
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

	CurrentCirrus = cs;

	if ((CurrentCirrus != NULL) && (SelectChange_cb != NULL))
		SelectChange_cb(*CurrentCirrus);

	return cs;
}

CIRRUS_Base* CIRRUS_Communication::GetCirrus(int position)
{
	if ((position >= (int) m_Cirrus.size()) || position < 0)
		return NULL;
	return m_Cirrus[position];
}

uint8_t CIRRUS_Communication::GetNumberCirrus(void)
{
	if (m_Cirrus.size() > 0)
		return (uint8_t) m_Cirrus.size();
	else
	{
		if (CurrentCirrus != NULL)
			return 1;
		else
			return 0;
	}
}

/**
 * Use the current cirrus
 */
bool CIRRUS_Communication::Get_rms_data(float *uRMS, float *pRMS)
{
	if (CurrentCirrus != NULL)
	  return CurrentCirrus->get_rms_data(uRMS, pRMS);
	else
		return false;
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
//		for (int i = 0; i < Size; i++)
//			Cirrus_UART->write(*pData++);
		Cirrus_UART->write(pData, Size);
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
#ifdef ESP8266
		yield();
#else
		taskYIELD();
#endif
	if (Cirrus_UART->available() < Size)
	{
//		CurrentCirrus->print_str("TIMEOUT");
		return CIRRUS_TIMEOUT;
	}
	else
	{
//		pResult->LSB = (uint8_t) Cirrus_UART->read();
//		pResult->MSB = (uint8_t) Cirrus_UART->read();
//		pResult->HSB = (uint8_t) Cirrus_UART->read();
//		pResult->CHECK = (uint8_t) Cirrus_UART->read();
		Cirrus_UART->readBytes(pResult->tab, Size);
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
// Gestion de la communication avec Cirrus_Connect
// ********************************************************************************

/**
 * Change current Cirrus when we have several Cirrus
 * Request should be '1', or '2' ...
 */
void CIRRUS_Communication::COM_ChangeCirrus(uint8_t *Request)
{
	if (CurrentCirrus == NULL)
		return;

	// we must have more than one cirrus
	if (GetNumberCirrus() > 1)
	{
		// Request is 1 or 2
		int id = strtol((char*) Request, NULL, 10) - 1;
		if (GetSelectedID() != id)
		{
			SelectCirrus(id, Channel_1);
			char message[30];
			sprintf(message, "Cirrus %d selected\r\n", id + 1);
			CurrentCirrus->print_str(message);
		}
	}
	else
		CurrentCirrus->print_str("Only one Cirrus is present\r\n");
}

#ifdef LOG_CIRRUS_CONNECT
//#define DEBUG_CONNECT

void CIRRUS_Communication::Do_Lock_IHM(bool op)
{
	if (op)
	{
		CIRRUS_Lock_IHM++;
	}
	else
		CIRRUS_Lock_IHM--;
}

/**
 * Analyse du message reçu de la part de Cirrus_Connect par ordre d'importance.
 * Ne pas oublier de remplacer HAL_UART_IRQHandler par Log_UART_RX_Callback dans le fichier d'interruption
 * Pour envoyer un message DEBUG en réponse à un message REG, utilisez le format ":#le message\03"
 * La gestion principale se trouve dans l'unité Log_UART
 * Le caractère de fin \03 est écrit en dur
 */
extern char* Search_Balise(uint8_t *data, const char *B_Begin, const char *B_end, char *value,
		uint16_t *len);
uint8_t CIRRUS_Communication::UART_Message_Cirrus(uint8_t *RxBuffer)
{
	static uint8_t Cirrus_message[100];
	uint8_t message[100] = {0};
	uint16_t len = 0;

	if (CurrentCirrus == NULL)
		return 1;

//  ************** Lecture/écriture registre du Cirrus ************************
	if (Search_Balise(RxBuffer, "REG=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		char response[50] = {0};
		COM_Register(message, response);
		// Expédition du résultat au format ":résultat en hexa (sans 0x)\03"
		// : et \03 sont les caractères de début et de fin du message
		char *presponse = &response[0];
		if (response[0] != '#') // This is not an info message
			presponse += 2; // Skip 0x
		sprintf((char*) Cirrus_message, ":%s\03\r\n", presponse);
		CurrentCirrus->print_str((char*) Cirrus_message);
		return 1;
	}

//  ************** Message REPEAT pour le Cirrus ******************************
	if (strstr((char*) RxBuffer, "REPEAT=") != NULL)
	{
		// On renvoie le dernier message
		CurrentCirrus->print_str((char*) Cirrus_message);
		return 1;
	}

//  ************** Changement de vitesse du Cirrus (mode UART) ****************
	if (Search_Balise(RxBuffer, "BAUD=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		char response[50];
		COM_ChangeBaud(message, response);
		return 1;
	}

//  ************** Lecture des coefficient de SCALE du Cirrus *****************
	if (Search_Balise(RxBuffer, "SCALE=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		float scale[4] = {0};
		char response[50];
		COM_Scale(message, &scale[0], response);
		CurrentCirrus->print_str((char*) response);
		return 1;
	}

//  ************** Ecriture des registres du Cirrus dans la FLASH *************
#ifdef CIRRUS_FLASH
	if (strstr((char*) RxBuffer, "FLASH=") != NULL)
	{
		// Sauvegarde dans la FLASH
		char id = '1';
		if (Selected != -1) // In case we have several Cirrus
			id += Selected;
		Register_To_FLASH(id);
		CurrentCirrus->print_str("FLASH Data OK\r\n");
		return 1;
	}
#endif

//  ************** Changement de Cirrus ***************************************
	if (Search_Balise(RxBuffer, "CS=", LOG_ETX_STR, (char*) message, &len) != NULL)
	{
		COM_ChangeCirrus((uint8_t*) message);
		return 1;
	}

//  ************** Message lock/unlock IHM pour calibration du Cirrus ****************
	if (strstr((char*) RxBuffer, "LOCK=") != NULL)
	{
		// Toggle lock IHM
		if (Is_IHM_Locked())
			Do_Lock_IHM(false);
		else
			Do_Lock_IHM(true);
		if (Is_IHM_Locked())
			CurrentCirrus->print_str("IHM Locked\r\n");
		else
			CurrentCirrus->print_str("IHM unLocked\r\n");
		return 1;
	}

//  ************** Print Config1 register ****************
	if (strstr((char*) RxBuffer, "CONFIG1=") != NULL)
	{
		Bit_List reg;
		char mess[255] = {0};
		Config1_Register config1;
		config1.Print_Config1(mess);
		CurrentCirrus->print_str(mess);

		CurrentCirrus->read_register(P0_Config1, PAGE0, &reg);
		config1.SetConfig1(reg.Bit32);
		config1.Print_Config1(mess);
		CurrentCirrus->print_str(mess);
		return 1;
	}

	return 0;
} // end UART_Message_Cirrus()

// ********************************************************************************
// Fonction pour la communication Wifi
// ********************************************************************************

/**
 * Handle common requests to tue Cirrus
 */
String CIRRUS_Communication::Handle_Common_Request(CS_Common_Request Common_Request, char *Request,
		CIRRUS_Calib_typedef *CS_Calib, CIRRUS_Config_typedef *CS_Config)
{
	char response[255] = {0};

	if (CurrentCirrus == NULL)
		return "ERROR: CurrentCirrus is NULL in Handle_Common_Request\n";

	// Lock IHM
	Do_Lock_IHM(true);

	switch (Common_Request)
	{
		case csw_REG: // Demande d'un registre
		{
			COM_Register((uint8_t*) Request, response);
			break;
		}

		case csw_SCALE: // Demande des scales (U_Calib, I_Max)
		{
			float scale[4] = {0};
			COM_Scale((uint8_t*) Request, &scale[0], response);
			// Update local scale
			CS_Calib->V1_Calib = scale[0];
			CS_Calib->I1_MAX = scale[1];
			CS_Calib->V2_Calib = scale[2];
			CS_Calib->I2_MAX = scale[3];
			break;
		}

		case csw_REG_MULTI: // Demande de plusieurs registres (graphe, dump)
		{
			COM_Register_Multi((uint8_t*) Request, response);
			break;
		}

		case csw_BAUD: // Changement de la vitesse
		{
			COM_ChangeBaud((uint8_t*) Request, response);
			break;
		}

		case csw_CS: // Changement de Cirrus
		{
			COM_ChangeCirrus((uint8_t*) Request);
			break;
		}

		case csw_LOCK: // Lock de l'IHM
		{
			bool status = (*Request == '1');
			// Toggle lock IHM
			Do_Lock_IHM(status);
			if (Is_IHM_Locked())
				CurrentCirrus->print_str("IHM Locked\r\n");
			else
				CurrentCirrus->print_str("IHM unLocked OK\r\n");
			break;
		}

#ifdef CIRRUS_FLASH
		case csw_FLASH: // Sauvegarde dans la FLASH
		{
			char id = '1';
			if (Selected != -1) // In case we have several Cirrus
				id += Selected;
			Register_To_FLASH(id);
			CurrentCirrus->print_str("FLASH Data OK\r\n");
			break;
		}
#endif

#ifdef CIRRUS_CALIBRATION
			// Demande calibration gain (I AC et U AC Gain)
			// Request param is: empty or Iref#spire or Uref#Iref#spire
		case csw_GAIN:
		{
			float p1, p2, p3;
			uint8_t *Request2 = (uint8_t*) Request;
			char *pbuffer = strtok((char*) Request2, ";#");

			Calibration = true;
			if (pbuffer == NULL)
			{
				CS_Calibration.Gain(CS_Calib, gain_Full_IUScale);
			}
			else
			{
				p1 = strtof(pbuffer, NULL);
				pbuffer = strtok(NULL, ";#");
				p2 = strtof(pbuffer, NULL);
				pbuffer = strtok(NULL, ";#");
				if (pbuffer == NULL)
				{
					// Here p1 = Iref and p2 = nb spire
					CS_Calibration.Gain(CS_Calib, gain_Full_UScale, p1 * p2);
				}
				else
				{
					// Here p1 = Uref, p2 = Iref and p3 = nb spire
					p3 = strtof(pbuffer, NULL);
					CS_Calibration.Gain(CS_Calib, gain_IUScale, p1, p2 * p3);
				}
			}

			Calibration = false;
			strcpy(response, "GAIN_OK");

			// Redémarrage du Cirrus
			CIRRUS_Restart(*GetCurrentCirrus(), CS_Calib, CS_Config);
			break;
		}

		case csw_IACOFF: // Demande calibration I AC Offset
		{
			Calibration = true;
			CS_Calibration.IACOffset(CS_Calib);
			Calibration = false;
			strcpy(response, "IACOffset_OK");

			// Redémarrage du Cirrus
			CIRRUS_Restart(*GetCurrentCirrus(), CS_Calib, CS_Config);
			break;
		}

		case csw_PQOFF: // Demande calibration P et Q Offset
		{
			Calibration = true;
			CS_Calibration.PQOffset(CS_Calib);
			Calibration = false;
			strcpy(response, "PQOffset_OK");

			// Redémarrage du Cirrus
			CIRRUS_Restart(*GetCurrentCirrus(), CS_Calib, CS_Config);
			break;
		}
#endif // CALIBRATION
		default:
			;
	}
#ifndef CIRRUS_CALIBRATION
		(void) CS_Config; // For compiler
#endif

	// UnLock IHM
	Do_Lock_IHM(false);

	return String(response);
}

/**
 * Change Cirrus baud when use UART
 * Note: If we have several Cirrus, we must change the speed of all.
 */
bool CIRRUS_Communication::COM_ChangeBaud(uint8_t *Baud, char *response)
{
#ifdef CIRRUS_USE_UART

	uint8_t nb = GetNumberCirrus();
	if (nb == 0)
	{
		strcpy(response, "NO_CIRRUS");
		return false;
	}

	int CirrusNewBaud = strtol((char*) Baud, NULL, 10);

	if (Cirrus_UART->baudRate() == CirrusNewBaud)
	{
		strcpy(response, "BAUD_OK");
		return true;
	}

	if (CurrentCirrus == NULL)
		return "ERROR: CurrentCirrus is NULL in COM_ChangeBaud\n";

	bool result = true;
	Do_Lock_IHM(true);
	CSDelay(1);
	if (nb == 1)
	{
		result = CurrentCirrus->SetUARTBaud(CirrusNewBaud, true);
	}
	else
	{
		// The current Cirrus used
		uint8_t id = GetSelectedID();
		for (uint8_t i = 0; i < nb; i++)
		{
			SelectCirrus(i);
			CSDelay(1);
			result &= CurrentCirrus->SetUARTBaud(CirrusNewBaud, (i == nb - 1));
			CSDelay(1);
		}

		// Go back to the initial Cirrus selected
		SelectCirrus(id);
	}
	Do_Lock_IHM(false);

	if (!result)
	{
		strcpy(response, "BAUD_ERROR");
		return false;
	}

#ifdef DEBUG_CONNECT
	char message[100] = {0};
	sprintf((char*) message, ":#CirrusNewBaud=%d\03\r\n", CirrusNewBaud);
	CurrentCirrus->print_str((char*) message);
	delay(1);
#endif
	strcpy(response, "BAUD_OK");

#else
	// On est en SPI, donc on ignore cette commande
	strcpy(response, "BAUD_ERROR");
	CurrentCirrus->print_str("Baud ignore, use SPI.\r\n");
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
char* CIRRUS_Communication::COM_Scale(uint8_t *Request, float *scale, char *response)
{
	char *pbuffer;
	uint8_t len = strlen((const char*) Request);

	if (CurrentCirrus == NULL)
	{
		strcpy(response, "ERROR: CurrentCirrus is NULL in COM_Scale\n");
		return response;
	}

	// Récupération des SCALE
	if (len == 0)
	{
		CurrentCirrus->GetScale(scale);
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
				pbuffer = strtok(NULL, ";#");
				scale[len] = strtof(pbuffer, NULL);
			}
			CurrentCirrus->SetScale(scale);
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
char* CIRRUS_Communication::COM_Register(uint8_t *Request, char *response)
{
	uint64_t command;
	bool op; // READ = true
	uint8_t registre, page;
	Bit_List result;

	// Cirrus est occupé
	if (CIRRUS_Command)
	{
		strcpy(response, "#Cirrus busy");
		return response;
	}

	if (CurrentCirrus == NULL)
	{
		strcpy(response, "ERROR: CurrentCirrus is NULL in COM_Register\n");
		return response;
	}

	CIRRUS_Command = true;

	// Le format de la commande est hhmmllpprrcc\03
	// hhmmll : La valeur du registre à écrire
	// pp est la page, rr est le registre et cc la commande (1 = READ, 0 = WRITE)
	// Si commande = WRITE et Page = 1 on a une commande simple

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
		strcpy(response, "#Cirrus page error");
		return response;
	}

#ifdef DEBUG_CONNECT
	char message[100] = {0};
	sprintf((char*) message, "#Page=%d; Registre=%d", page, registre);
	CurrentCirrus->print_str((char*) message);
#endif
	Do_Lock_IHM(true);
	if (op) // READ
	{
		if (CurrentCirrus->read_register(registre, page, &result))
		{
			// Expédition du résultat en hexa au format : 0xXXXXXX
			sprintf(response, "0x%.6X", (unsigned int) result.Bit32);
		}
		else
			strcpy(response, "#REG_ERROR");
	}
	else // WRITE
	{
		// PAGE = 1 n'existe pas. Est utilisé pour envoyer une instruction.
		if (page == 1)
		{
			CurrentCirrus->send_instruction(registre);
		}
		else
		{
			result.Bit32 = (command >> 8);
#ifdef DEBUG_CONNECT
			sprintf((char*) message, "#Command : (LSB: 0x%.2X, MSB: 0x%.2X, HSB: 0x%.2X), Checksum: %d",
					result.LSB, result.MSB, result.HSB, result.CHECK);
			CurrentCirrus->print_str((char*) message);
#endif
			CurrentCirrus->write_register(registre, page, &result);
		}
		strcpy(response, "#REG_OK");
	}
	Do_Lock_IHM(false);
	CIRRUS_Command = false;
	return response;
}

/**
 * Get several registers
 * Request = Request1#Request2#Request3 ....
 * Return val1#val2#val3 .... en hexa au format : 0xXXXXXX
 */
char* CIRRUS_Communication::COM_Register_Multi(uint8_t *Request, char *response)
{
	char *message[255] = {0};
	char *pbuffer;

	strcpy(response, "");
	pbuffer = strtok((char*) Request, ";#");
	while (pbuffer != NULL)
	{
		strcat(response, COM_Register((uint8_t*) pbuffer, (char*) message));
		if ((pbuffer = strtok(NULL, ";#")) != NULL)
			strcat(response, "#");
	}
	return response;
}

#ifdef CIRRUS_FLASH
char* CIRRUS_Communication::COM_Flash(char id_cirrus, char *response)
{
	// Sauvegarde dans la FLASH
	Register_To_FLASH(id_cirrus);
//	CIRRUS_print_str("FLASH Data OK\r\n");
	strcpy(response, "FLASH_OK");
	return response;
}
#endif

#endif // LOG_CIRRUS_CONNECT

// ********************************************************************************
// End of file
// ********************************************************************************
