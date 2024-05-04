#include "TeleInfo.h"

//#define TI_DEBUG

#define TRUE 1
#define FALSE 0

#define TI_STX  	0x02 // Start Text
#define TI_ETX  	0x03 // End Text
#define TI_EOT  	0x04 // End Of Text
#define TI_LF   	0x0A // Line Feed
#define TI_CR   	0x0D // Carriage Return
#define TI_SP   	0x20 // Space
#define TI_6LB  	0x3F // 6 bits de poids faible
#define TI_7LB  	0x7F // 7 bits de poids faible

// Time counter : la base de temps. TI_tick défini l'intervalle de temps (en ms) entre deux prises de mesure
static uint32_t _timeCounter;
static uint32_t TI_tick;
static uint32_t conf_delay = 0; // le temps pour la configuration

#ifdef TI_DEBUG
void printc(char c);
void printval(uint32_t v);
#endif

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

/**
 * Constructeur
 * rxPin : pin TeleInfo
 * refresh_ms : le délai entre deux rafraichissements.
 * La réception d'une trame prend au moins 1 seconde, donc inutile de faire moins.
 * Par défaut, refresh_ms = 10000 ms
 * La vitesse baud est par défaut de 1200 (ancien compteur)
 * Pour ESP32, on utilise UART1 remapé sur GPIO 14 (RX) et 12 (TX non utilisé)
 */
TeleInfo::TeleInfo(uint8_t rxPin, uint32_t refresh_ms, uint32_t baud)
{
#ifdef ESP8266
	tiSerial = new SoftwareSerial(rxPin);
	tiSerial->begin(baud);
	//  tiSerial->begin(baud, SWSERIAL_8N1, rxPin, -1, false, FRAME_MAX_SIZE);
#elif ESP32
	tiSerial = new HardwareSerial(1);
	tiSerial->begin(baud, SERIAL_8N1, rxPin);
#endif
	TI_tick = refresh_ms;
	_timeCounter = millis();
	_tiRunning = true;
}

/**
 * Destructeur
 * Free SoftwareSerial
 */
TeleInfo::~TeleInfo()
{
	if (tiSerial != NULL)
	{
		delete tiSerial;
		tiSerial = NULL;
		_tiRunning = false;
	}
}

/**
 * Configure la trame pour accéder directement aux infos
 * timeout : temps d'attente maximum en ms
 * Note : l'acquisition d'une trame prend au moins 1 s, donc timeout >> 1000
 */
bool TeleInfo::Configure(uint32_t timeout)
{
	// On écoute le port
	Resume();

	uint32_t dt = millis();
	bool waiting = true;
	// Attend d'avoir une structure complète
#ifdef TI_DEBUG
	print_debug("*** Recherche d'une frame complète ***");
#endif
	while ((!_isAvailable) && waiting)
	{
		while (tiSerial->available())
		{
			try
			{
				uint8_t c = (uint8_t) tiSerial->read();
				DataReceived(c);
			} catch (...)
			{
				// On abandonne la lecture
				resetAll();
				print_debug("ERROR read on TeleInfo");
			}
			yield();
		}
		waiting = ((conf_delay = millis() - dt) < timeout);
	}

	// On a recu une trame dans les temps.
	// On construit la structure correspondant au compteur
	if (waiting)
	{
		build_Structure();
		use_structure = true;
	}

	// Si la configuration a échoué, on arrête d'écouter
	if (!waiting)
		Pause();

	return waiting;
}

const char* TeleInfo::getStringVal(const char *label)
{
	int i = 0;
	char *res = NULL;

	if (_isAvailable)
	{
		while ((i < _labelCount) && (strcmp(label, _label[i]) != 0))
		{
			i++;
		}
		if (i < _labelCount)
			res = _data[i];
	}
	return res;
}

int32_t TeleInfo::getLongVal(const char *label)
{
	const char *stringVal = getStringVal(label);
	long res = -1;

	if (stringVal != NULL)
	{
		res = strtol(stringVal, NULL, 10);
	}
	return res;
}

uint32_t TeleInfo::getPowerVA()
{
	return strtol(TI_PowerVA_str, NULL, 10);
}

uint32_t TeleInfo::getIndexWh()
{
	return strtol(TI_IndexWh_str, NULL, 10);
}

void TeleInfo::PrintAllToSerial()
{
	int i;
	uint8_t buffer[50] = {0};

	if (_isAvailable)
	{
		for (i = 0; i < _labelCount; i++)
		{
			sprintf((char*) buffer, "%s => %s", _label[i], _data[i]);
			print_debug((char*) buffer);
		}
		sprintf((char*) buffer, "Label numbers => %d", _labelCount);
		print_debug((char*) buffer);
		sprintf((char*) buffer, "Trame length => %d", strlen(_frame));
		print_debug((char*) buffer);
		sprintf((char*) buffer, "Temps de configuration : %d ms", conf_delay);
		print_debug((char*) buffer);
	}
}

void TeleInfo::Test()
{
	const char *periodTarif;
	const char *opTarif;
	long power;
	char buf_power[50] = {0};

	if (Available())
	{
		PrintAllToSerial();

		print_debug("------ \r\n");

		periodTarif = getStringVal("PTEC");
		print_debug("Period Tarifaire = ");
		periodTarif == NULL ? print_debug("unknown") : print_debug(periodTarif);
		print_debug("\r\n");

		opTarif = getStringVal("OPTARIF");
		print_debug("Option Tarifaire = ");
		opTarif == NULL ? print_debug("unknown") : print_debug(opTarif);
		print_debug("\r\n");

		power = getLongVal("PAPP");
		if (power < 0)
			print_debug("Power = unknown\r\n");
		else
		{
			sprintf(buf_power, "Power = %ld VA\r\n", power);
			print_debug(buf_power);
		}

		ResetAvailable();
	}
}

bool TeleInfo::Available()
{
	return _isAvailable;
}

void TeleInfo::ResetAvailable()
{
	if (_isAvailable)
		resetAll();
}

/**
 * Fonction à mettre dans la loop principale.
 * Vérifie et analyse les données reçues.
 * Rafraichi les données si nécessaire en fonction du délai imposé.
 */
void TeleInfo::Process()
{
	if (!_tiRunning)
		return;

	if (_isAvailable && ((millis() - _timeCounter) > TI_tick))
	{
		resetAll();
	}

	while (tiSerial->available())
	{
		try
		{
			uint8_t c = (uint8_t) tiSerial->read();
			DataReceived(c);
		} catch (...)
		{
			// On abandonne la lecture
			resetAll();
			print_debug("ERROR read on TeleInfo");
		}
		yield();
	}
}

void TeleInfo::Pause()
{
	if (tiSerial != NULL)
	{
#ifdef ESP8266
	  tiSerial->stopListening();
#endif
		_tiRunning = false;
	}
}

void TeleInfo::Resume()
{
	if (tiSerial != NULL)
	{
#ifdef ESP8266
	  tiSerial->listen();
#endif
		delay(100);
		_tiRunning = true;
	}
}

// *************** Private part

uint16_t TeleInfo::DataReceived(uint8_t ch)
{
	char caractereRecu = '\0';
#ifdef TI_DEBUG
	static uint32_t _timeElapsed;
#endif

	if (!_isAvailable)
	{
		caractereRecu = ch & TI_7LB;

#ifdef TI_DEBUG
		if (caractereRecu == TI_SP || caractereRecu == TI_LF || caractereRecu == TI_CR
				|| caractereRecu == TI_ETX)
			print_debug("");
		else
		{
			printc(caractereRecu);
			print_debug(" ", false);
		}
#endif
		//"Start Text 0x02" - frame start
		if (caractereRecu == TI_STX)
		{
			_frameIndex = 0;
			_frameBegin = true;
#ifdef TI_DEBUG
			print_debug("*** START frame ***");
			_timeElapsed = _timeCounter;
#endif
		}
		if (_frameBegin)
		{
			// La frame a été interrompue
			if (caractereRecu == TI_EOT)
				resetAll();

			// TODO check _ frame overflow !!!
			if (_frameIndex >= FRAME_MAX_SIZE)
				_frameIndex = 0;

			// Build the frame char by char
			_frame[_frameIndex++] = caractereRecu;

			//  "EndText 0x03" - frame end
			if (caractereRecu == TI_ETX)
			{
				// Termine la string
				_frame[_frameIndex++] = '\0';

#ifdef TI_DEBUG
				print_debug("*** END frame ***");
				printval(_timeCounter - _timeElapsed);
				print_debug("*** will try to decode a new frame");
#endif

				// Analyse de la frame et extraction des données pertinentes
				if (use_structure)
				{
					get_Structure_Data(ID_IndexWh, TI_IndexWh_str);
					_isAvailable = get_Structure_Data(ID_PowerVA, TI_PowerVA_str);
				}
				else
					_isAvailable = decodeFrame();
#ifdef TI_DEBUG
				if (_isAvailable)
					print_debug("*** Decode frame OK ***");
				else
					print_debug("*** Decode frame not OK, we restart ***");
#endif
				// La frame est incorrecte, on réinitialise
				if (!_isAvailable)
					resetAll();
			}
		}
	}
	return _frameIndex;
}

#ifdef TI_DEBUG
void printc(char c)
{
	char buf_char[2];
	sprintf(buf_char, "%c", c);
	print_debug(buf_char, false);
}

void printval(uint32_t v)
{
	char buf_char[20];
	sprintf(buf_char, "Elapsed time %ld", v);
	print_debug(buf_char);
}
#endif

void TeleInfo::resetAll()
{
	_frame[0] = '\0';
	_frameIndex = 0;
	_frameBegin = false;
	_labelCount = 0;
	_isAvailable = false;
	_timeCounter = millis();
}

int TeleInfo::decode(int beginIndex, char *field, unsigned char *sum, uint8_t max_size)
{
	int i = beginIndex;
	int j = 0;

	while ((_frame[i] != TI_SP) && (j < max_size))
	{
		*sum += (field[j++] = _frame[i++]);
		// Code équivalent pour y voir clair !
		//    field[j] = _frame[i];
		//    *sum += field[j];
		//    i++;
		//    j++;
	}
	if (j == max_size + 1)
	{
		return -1;
	}
	else
	{
		field[j] = '\0'; // end string
		return (++i);
	}
}

/**
 * Décodage d'une frame. La structure est la suivante :
 * 0x02 (start byte)
 * 0x0A label 0x20 value 0x20 checksum 0x0D
 * séquence précédente répétée autant de fois qu'il y a de label
 * 0x03 (end byte)
 */
bool TeleInfo::decodeFrame()
{
	int i = 1, lineIndex = 0;
	bool frameOk = true;
	char checksum;
	unsigned char sum = TI_SP;
	char *_power = NULL;
	char *_index = NULL;

	// start i at 1 to skip first char 0x02 (start byte)
	while (_frame[i] != TI_ETX)
	{
		// Test qu'on dépasse pas le nombre de lignes MAX
		if (lineIndex >= LINE_MAX_COUNT)
		{
#ifdef TI_DEBUG
			print_debug("frame KO LINE_MAX_COUNT");
#endif
			frameOk = false;
			break;
		}

		// Début d'une ligne d'info et avancement dans la frame
		if (_frame[i++] != TI_LF)
		{
#ifdef TI_DEBUG
			printc(_frame[i]);
			print_debug("frame KO 0A -- ");
#endif
			frameOk = false;
			break;
		}

		sum = TI_SP;
		// Décodage du label
		i = decode(i, _label[lineIndex], &sum, LABEL_MAX_SIZE);
		if (i < 0)
		{
			frameOk = false;
			break;
		}

		// Décodage de la donnée
		i = decode(i, _data[lineIndex], &sum, DATA_MAX_SIZE);
		if (i < 0)
		{
			frameOk = false;
			break;
		}

		// Valeur du checksum et avancement dans la frame
		checksum = _frame[i++];
#ifdef TI_DEBUG
		print_debug("checksum is ", false);
		printc(checksum);
		print_debug("");
#endif

		// Fin de la ligne d'info et avancement dans la frame
		if (_frame[i++] != TI_CR)
		{
			frameOk = false;
			break;
		}

		// Vérification du checkum
		// On conserve les 6 bits de poids faible + 0x20
		if (((sum & TI_6LB) + TI_SP) != checksum)
		{
			frameOk = false;
			break;
		}

		// Extraction de la puissance et de l'index
		if ((_power == NULL) && (strcmp("PAPP", _label[lineIndex]) == 0))
		{
			_power = _data[lineIndex];
		}

		if ((_index == NULL) && (strcmp("BASE", _label[lineIndex]) == 0))
		{
			_index = _data[lineIndex];
		}

		// Passage à la ligne d'info suivante
		lineIndex++;
	}

	if (frameOk && (lineIndex > 0) && (_power != NULL) && (_index != NULL))
	{
		_labelCount = lineIndex;
		strcpy(TI_PowerVA_str, _power);
		strcpy(TI_IndexWh_str, _index);
	}
	else
	{
		frameOk = false;
		_labelCount = 0;
	}

	return frameOk;
}

void TeleInfo::build_Structure()
{
	uint8_t i, j;
	unsigned char sum = TI_SP;
	char *pindex;

	if (_isAvailable)
	{
		pindex = &_frame[2]; // 0x02 0x0A
		for (i = 0; i < _labelCount; i++)
		{
			_structure[i].label = pindex;
			sum = TI_SP;
			for (j = 0; j < strlen(_label[i]); j++)
				sum += _label[i][j];
			_structure[i].checkLabel = sum;
			pindex += strlen(_label[i]);
			pindex++; // L'espace
			_structure[i].data = pindex;
			_structure[i].lenData = strlen(_data[i]);
			pindex += strlen(_data[i]);
			pindex++; // L'espace
			_structure[i].checksum = pindex;
			pindex += 3; // Checksum + 0x0D + 0x0A
		}

		// Recherche des ID power et index (HP dans le cas où on n'est pas en BASE
		for (i = 0; i < _labelCount; i++)
		{
			if ((strcmp(_label[i], "BASE") == 0) || (strcmp(_label[i], "HCHP") == 0)
					|| (strcmp(_label[i], "BBRHPJB") == 0))
			{
				ID_IndexWh = i;
				break;
			}
		}
		if (i == _labelCount)
			i = 0;
		for (; i < _labelCount; i++)
		{
			if (strcmp(_label[i], "PAPP") == 0)
			{
				ID_PowerVA = i;
				break;
			}
		}
	}
}

bool TeleInfo::get_Structure_Data(uint8_t id, char *result)
{
	// Energie index : 3  BASE
	// Power index : 7  PAPP
	TI_TypeDef *record;
	uint8_t i;
	unsigned char sum;
	char value[DATA_MAX_SIZE + 1];
#ifdef TI_DEBUG
	char tmp[10];
#endif

	record = &_structure[id];

#ifdef TI_DEBUG
	strncpy(tmp, record->label, 9);
	tmp[9] = 0;
	print_debug(tmp);

	strncpy(tmp, record->data, record->lenData);
	tmp[record->lenData] = 0;
	print_debug(tmp);

	strncpy(tmp, record->checksum, 1);
	tmp[1] = 0;
	print_debug(tmp);
#endif

	// Récupérer la data et vérifier le checksum
	sum = record->checkLabel;
	for (i = 0; i < record->lenData; i++)
	{
		sum += (value[i] = record->data[i]);
	}

	if (((sum & TI_6LB) + TI_SP) == record->checksum[0])
	{
		value[record->lenData] = 0;  // Finir la chaine !
		strcpy(result, value);
		return true;
	}
	return false;
}

// ********************************************************************************
// End of file
// ********************************************************************************
