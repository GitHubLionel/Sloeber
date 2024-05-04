#include "Clavier.h"

const char *Btn_Texte[BTN_MAX] = {"NO btn pressed", "K1 pressed", "K2 pressed", "K3 pressed",
		"K4 pressed"};

// Variables clavier
static uint16_t Low_sampling[BTN_MAX];  // tableau de stockage des valeurs basses

static uint8_t Clavier_Channel;
static uint8_t Btn_Count = 0;
static bool Clavier_Initialized = false;
volatile uint32_t Start_click = 0;
volatile Btn_Action Btn_Clicked = Btn_NOP;
volatile uint16_t last_ADC = 0;

static uint32_t ADC_res = 1023;  // Echantillonnage 10 bits
#define DEBOUNCING      300  // Temps en ms d'appuie sur le bouton

void Btn_Definition_1B();
void Btn_Definition_2B();
void Btn_Definition_3B();
void Btn_Definition_4B();

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

// ********************************************************************************
// Local initialization functions
// ********************************************************************************

/**
 * Initialisation du clavier.
 * channel : ADC pin
 * nbButton : number of button, between 1 to 4
 * sampling : 10, 12 or 16 bits
 * Ne pas oublier de mettre la fonction Clavier_UpdateTime() dans le loop principal
 */
void Clavier_Init(uint8_t channel, uint8_t nbButton, ADC_Sampling sampling)
{
	Clavier_Channel = channel;
	Clavier_Initialized = false;
	Btn_Count = nbButton;

	switch (sampling)
	{
		case ADC_10bits:
			ADC_res = 1023;
			break;
		case ADC_12bits:
			ADC_res = 4095;
			break;
		case ADC_16bits:
			ADC_res = 65535;
			break;
	}

	// Definition clavier
	switch (Btn_Count)
	{
		case 1:
			Btn_Definition_1B();
			break;
		case 2:
			Btn_Definition_2B();
			break;
		case 3:
			Btn_Definition_3B();
			break;
		case 4:
			Btn_Definition_4B();
			break;
		default:
			print_debug("Erreur définition clavier.");
	}
}

/**
 * Initialisation du clavier.
 * channel : ADC pin
 * nbButton : number of button, max 4
 * interval : intervals to be considered, max to min : nbButton + 1 values
 * Ne pas oublier de mettre la fonction Clavier_UpdateTime() dans le loop principal
 */
void Clavier_Init(uint8_t channel, uint8_t nbButton, uint16_t interval[])
{
	Clavier_Channel = channel;
	Clavier_Initialized = false;
	Btn_Count = nbButton;

	ADC_res = interval[0];
	for (uint8_t i = 0; i < Btn_Count; i++)
	{
		Low_sampling[i] = interval[i + 1];
	}
	Clavier_Initialized = true;
}

/**
 * Fonction d'actualisation de l'état du clavier
 * Doit être mise dans la boucle principale
 */
void Clavier_UpdateTime()
{
	static uint32_t lastTimeRead = 0;
	uint32_t deltaT;
	Btn_Action Btn_test;

	if (Clavier_Initialized)
	{
		deltaT = millis() - lastTimeRead;
		if (deltaT > 0)
		{
			lastTimeRead = millis();
			if (Start_click == 0)
			{
				Btn_test = Btn_Click();
				if (Btn_test != Btn_NOP)
				{
					Btn_Clicked = Btn_test;
					Start_click = DEBOUNCING / deltaT;
				}
			}
			else
				Start_click--;
		}
	}
}

/**
 * Check le clavier
 * Renvoie true si un bouton a été appuyé
 * Btn : le numéro du bouton
 * Fonction à tester régulièrement dans la boucle principale
 */
bool Check_Clavier(Btn_Action *Btn)
{
	*Btn = Btn_Clicked;
	Btn_Clicked = Btn_NOP;  // On "mange" la valeur
	Start_click = 0;
	return (bool) (*Btn != Btn_NOP);
}

// ********************************************************************************
// Gestion clavier
// ********************************************************************************

void Btn_Definition_1B()
{
	// raw adc value :
	// K1 pressed ==> val = 1024

	Low_sampling[0] = 90 * (ADC_res / 100);  //1000;

	Clavier_Initialized = true;
}

void Btn_Definition_2B()
{
	// raw adc value :
	// K1 pressed ==> val = 534
	// K2 pressed ==> val = 900

	Low_sampling[0] = 90 * (ADC_res / 100); // 920;
	Low_sampling[1] = 60 * (ADC_res / 100); // 600;

	Clavier_Initialized = true;
}

void Btn_Definition_3B()
{
	// raw adc value :
	// K1 pressed ==> val = 534
	// K2 pressed ==> val = 794
	// K3 pressed ==> val = 1024

	Low_sampling[0] = 90 * (ADC_res / 100); // 920;
	Low_sampling[1] = 60 * (ADC_res / 100); // 650;
	Low_sampling[2] = 40 * (ADC_res / 100); // 450;

	Clavier_Initialized = true;
}

void Btn_Definition_4B()
{
	// raw adc value :
	// K1 pressed ==> val = 447
	// K2 pressed ==> val = 663
	// K3 pressed ==> val = 888
	// K4 pressed ==> val = 1024

	Low_sampling[0] = 80 * (ADC_res / 100); // 920;
	Low_sampling[1] = 60 * (ADC_res / 100); // 650;
	Low_sampling[2] = 40 * (ADC_res / 100); // 450;
	Low_sampling[3] = 30 * (ADC_res / 100); // 300;

	Clavier_Initialized = true;
}

Btn_Action Btn_Click()
{
	uint8_t i;
	Btn_Action btn = Btn_NOP;
	uint16_t val = analogRead(Clavier_Channel);
	yield();

	// On a appuyé sur un bouton
	if (val > Low_sampling[Btn_Count - 1])
	{ // si différent de 0
		for (i = 0; i < Btn_Count; i++)
		{ // parcours des valeurs
			if (val >= Low_sampling[i])
			{ // test si supérieur à valeur concernée
				btn = (Btn_Action) (Btn_Count - i);
				break; //sortie de boucle
			}
		}
	}
	last_ADC = val;
	return btn;
}

const char* Btn_Click_Name()
{
	return Btn_Texte[Btn_Click()];
}

uint16_t Btn_Click_Val()
{
	return analogRead(Clavier_Channel);
}

/**
 * Test de l'intervalle de détection des boutons
 * Relevé de mesures pendant 10 secondes (défaut) ou boucle infinie (infinite = true)
 */
void Btn_Check_Config(bool infinite)
{
	char buffer_deb[20] = {0};
	char buffer[50] = {0};
	uint32_t Check_count = 0;

	print_debug("\r\n ** Info Clavier **");
	sprintf(buffer_deb, "[%d - ", ADC_res);
	for (uint8_t i = 0; i < Btn_Count; i++)
	{
		sprintf(buffer, "%s%d] ==> %s", buffer_deb, Low_sampling[i], Btn_Texte[Btn_Count - i]);
		print_debug(buffer);
		sprintf(buffer_deb, "[%d - ", Low_sampling[i]);
	}
	sprintf(buffer, "%s0] ==> %s\r\n", buffer_deb, Btn_Texte[0]);
	print_debug(buffer);

	print_debug("Check clavier :");
	while (1)
	{
		sprintf(buffer, "%s ==> val = %d", Btn_Click_Name(), last_ADC);
		print_debug(buffer);

		delay(500);
		Check_count++;
		if (!infinite)
		{
			if (Check_count > 20)
				break;
		}
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
