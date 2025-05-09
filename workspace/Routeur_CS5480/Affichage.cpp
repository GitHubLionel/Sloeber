/* Includes ------------------------------------------------------------------*/
#include "Affichage.h"
#include "Fast_Printf.h"

#include "RTCLocal.h"
#include "Server_utils.h"
#include "Debug_utils.h"
#include "display.h"
#include "SSR.h"
#include "Get_Data.h"
#ifdef USE_KEYBOARD
#include "Keyboard.h"
#endif
#include "Tasks_utils.h"
#include "Emul_PV.h"
#ifdef USE_RELAY
#include "Relay.h"
#endif
#ifdef USE_ADC
#include "ADC_Utils.h"
#endif

// Pre ++ operator for any enum type but is not circular
//https://stackoverflow.com/questions/36793526/how-to-create-a-template-operator-for-enums
//template <class E, class = std::enable_if_t<std::is_enum<E>{}>>
//E &operator ++ (E &e) {
//    return e = static_cast<E>(
//        static_cast<std::underlying_type_t<E>>(e) + 1
//    );
//}

typedef enum
{
	menuData,
#ifdef USE_RELAY
	menuRelais,
#endif
	menuSSR,
	menuWifi
} Menu_enum;

Menu_enum operator ++(Menu_enum &id, int)
{
	Menu_enum currentID = id;
	id = static_cast<Menu_enum>((static_cast<int>(id) + 1) % (menuWifi + 1));
	return (currentID);
}

typedef enum
{
	Page1,
	Page2,
	Page3,
	Page4,
	Page5
} page_typedef;

page_typedef operator ++(page_typedef &id, int)
{
	page_typedef currentID = id;
	id = static_cast<page_typedef>((static_cast<int>(id) + 1) % (Page5 + 1));
	return (currentID);
}

// Wifi
extern ServerConnexion myServer;

// Data
extern Data_Struct Current_Data;

// PV théorique
extern EmulPV_Class emul_PV;

// Relais
#ifdef USE_RELAY
extern Relay_Class Relay;
#endif

// Use ADC
#ifdef USE_ADC
extern bool ADC_OK;
extern ADC_Action_Enum ADC_Action; // Show PageTest
#endif

// Affichage
#define COLUMN	5
static char buffer[50] = {0};
uint16_t len = 0;

page_typedef current_page = Page1;
Menu_enum Menu = menuData;

uint8_t Cursor_Pos = 0;
bool Action_Needed = false;
int Action_Wifi = 0;

#define DELAY_LCD_ON   600  // 10 minutes
#define DELAY_PAGE     120  // 2 minutes
#define DELAY_ACTION     5  // 5 secondes pour valider une action

int Count_Action_Needed = 0;

/* Private function prototypes -----------------------------------------------*/
void Show_Page_Test(void);
void Show_Page1(void);
void Show_Page2(void);
void Show_Page3(void);
void Show_Page4(void);
void Show_Page5(void);
#ifdef USE_RELAY
void Show_Page_Relay(uint8_t cursor);
void Show_Page_Relay_Action(uint8_t cursor);
#endif
void Show_Page_SSR(uint8_t cursor);
void Show_Page_SSR_Action(uint8_t cursor);
void Show_Page_Wifi(void);
void Show_Page_Wifi_Action(void);

// *****************************************************************************
// Gestion IHM
// *****************************************************************************

#ifdef USE_KEYBOARD
/**
 * Gestion des actions des boutons
 * - Bouton du haut : Action type Menu, sélection d'un menu avec ses pages
 * - Bouton du milieu : Action type OK, validation choix
 * - Bouton du bas : Action type flèche, écran suivant ou sélection suivante
 */
void UserKeyboardAction(Btn_Action Btn_Clicked, uint32_t count)
{
//	if (Btn_Clicked != Btn_NOP)
//		Serial.println(Btn_Texte[Btn_Clicked]);

// On réveille l'écran et on se positionne sur la première page de data
	if (IHM_IsDisplayOff())
	{
		Menu = menuData;
		current_page = Page1;
		Action_Needed = false;
		IHM_DisplayOn();
		TaskList.ResumeTask("DISPLAY_Task");
		return;
	}

	switch (Btn_Clicked)
	{
		case Btn_K1: // Bouton du bas : Action type flèche, écran suivant ou sélection suivante
		{
			if (count == 1)
			{
				if (Action_Needed)
				{
					Cursor_Pos = 0;
					Action_Needed = false;
				}
				else
				{
					switch (Menu)
					{
						case menuData:
							current_page++;
							break;
#ifdef USE_RELAY
						case menuRelais:
							Cursor_Pos = (Cursor_Pos + 1) % Relay.size();
							break;
#endif
						case menuSSR:
							Cursor_Pos = (Cursor_Pos + 1) % 2;
						default:
							;
					}
				}
			}

			break;
		}

		case Btn_K2: // Bouton du milieu : Action type OK, validation choix
		{
			if (count == 1)
			{
				switch (Menu)
				{
					case menuData:
						Action_Needed = false;
						Action_Wifi = 0;
						break;
					case menuSSR:
						if (Action_Needed)
						{
							if (SSR_Get_Action() == SSR_Action_Surplus)
							{
								if (Cursor_Pos == 0)
								{
									SSR_Set_Action(SSR_Action_FULL, true);
									TaskList.ResumeTask("SSR_BOOST_Task");
								}
								else
									(SSR_Get_State() == SSR_OFF) ? SSR_Enable() : SSR_Disable();
							}
							else
							{
								TaskList.SuspendTask("SSR_BOOST_Task");
								SSR_Set_Action(SSR_Action_Surplus, true);
							}
							Action_Needed = false;
							Cursor_Pos = 0;
						}
						else
						{
							Action_Needed = true;
							Count_Action_Needed = DELAY_ACTION;
						}
						Action_Wifi = 0;
						break;

#ifdef USE_RELAY
					case menuRelais:
						if (Action_Needed)
						{
							Relay.toggleState(Cursor_Pos);
							Action_Needed = false;
							Cursor_Pos = 0;
						}
						else
						{
							Action_Needed = true;
							Count_Action_Needed = DELAY_ACTION;
						}
						Action_Wifi = 0;
						break;
#endif

					case menuWifi:
						if (Action_Wifi == 1)
						{
							myServer.KeepAlive();
						}
						else
						{
							Action_Wifi++;
							Count_Action_Needed = DELAY_ACTION;
						}
						Action_Needed = false;
						break;
				}
			}

			// On a appuyé plus de 5 secondes sur le bouton
			if ((Menu == menuWifi) && (count == SECOND_TO_DEBOUNCING(5)))
			{
				// Delete SSID file
				DeleteSSID();
				Action_Wifi = 0;
			}
			break;
		}

		case Btn_K3: // Bouton du haut : Action type Menu, sélection d'un menu avec ses pages
		{
			if (count == 1)
			{
				Action_Wifi = 0;
				Cursor_Pos = 0;
				if (!Action_Needed)
				{
					switch (Menu)
					{
						case menuData:
							break;
#ifdef USE_RELAY
						case menuRelais:
							break;
#endif
						case menuSSR:
							break;
						case menuWifi:
							current_page = Page1;
							break;
					}
					Menu++;
				}
				Action_Needed = false;
			}
			break;
		}

		default: // Btn_NOP ne devrait jamais arrivé
			;
	}
}
#endif

/**
 * Gestion de l'affichage des pages oled en fonction des boutons pressés
 */
void Display_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("DISPLAY_Task");

	for (EVER)
	{
#ifdef CIRRUS_CALIBRATION
		if (Calibration)
		{
			END_TASK_CODE(false);
			continue;
		}
#endif

		IHM_Clear();

		switch (Menu)
		{
			case menuData:
			{
				Fast_Set_Decimal_Separator(',');
#ifdef USE_ADC
				if ((ADC_Action == adc_Raw) || (ADC_Action == adc_Zero))
					Show_Page_Test();
				else
#endif
					switch (current_page)
					{
						case Page1:
							Show_Page1();
							break;
						case Page2:
							Show_Page2();
							break;
						case Page3:
							Show_Page3();
							break;
						case Page4:
							Show_Page4();
							break;
						case Page5:
							Show_Page5();
							break;

						default:
							;
					}
				Fast_Set_Decimal_Separator('.');
				break;
			}
#ifdef USE_RELAY
			case menuRelais:
			{
				if (Action_Needed)
				{
					Show_Page_Relay_Action(Cursor_Pos);
					if (--Count_Action_Needed == 0)
						Action_Needed = false;
				}
				else
					Show_Page_Relay(Cursor_Pos);
				break;
			}
#endif
			case menuSSR:
			{
				if (Action_Needed)
				{
					Show_Page_SSR_Action(Cursor_Pos);
					if (--Count_Action_Needed == 0)
						Action_Needed = false;
				}
				else
					Show_Page_SSR(Cursor_Pos);
				break;
			}
			case menuWifi:
			{
				if (Action_Wifi > 0)
				{
					Show_Page_Wifi_Action();
					if (--Count_Action_Needed == 0)
						Action_Wifi = 0;
				}
				else
					Show_Page_Wifi();
				break;
			}
			default:
				;
		}

		// Refresh display
		IHM_Display();

		// Test extinction de l'écran
		IHM_CheckTurnOff();

		// End task
		END_TASK_CODE(IHM_IsDisplayOff());
	}
}

// *****************************************************************
// Affichage des pages
// *****************************************************************

// Une page pour faire des tests
void Show_Page_Test(void)
{
#ifdef USE_ADC
	uint8_t line = 0;
	String Temp_str = "";
	if (ADC_OK)
	{
		if (ADC_Action == adc_Raw)
		{
			int raw = ADC_Read0();
			Temp_str = "Raw0: " + String(raw) + "   ";
			IHM_Print(line++, (const char*) Temp_str.c_str(), false);
			raw = ADC_Read1();
			Temp_str = "Raw1: " + String(raw) + "   ";
			IHM_Print(line++, (const char*) Temp_str.c_str(), false);
		}
		else
			if (ADC_Action == adc_Zero)
			{
				uint32_t count;
				float zero = ADC_GetZero(&count);
				Temp_str = "Zero: " + String(zero);
				IHM_Print(line++, (const char*) Temp_str.c_str(), false);
				Temp_str = "Count: " + String(count);
				IHM_Print(line++, (const char*) Temp_str.c_str(), false);
				print_debug(zero);
			}
	}
#endif
}

void Show_Page1(void)
{
	uint8_t line = 0;

	// date
	IHM_Print(line++, 6, (char*) RTC_Local.the_time(), false);
	line++;

	// Puissance conso
	IHM_Print(line++, 1, "P conso : ", false);
	Fast_Printf(&buffer[0], Current_Data.Cirrus_ch1.ActivePower, 2, "", " W", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Puissance conso
	IHM_Print(line++, 1, "P prod : ", false);
	Fast_Printf(&buffer[0], Current_Data.Cirrus_ch2.ActivePower, 2, "", " W", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Puissance Talema
	IHM_Print(line++, 1, "P Talema : ", false);
	Fast_Printf(&buffer[0], Current_Data.Talema_Power, 2, "", " W", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Afficher Idle
#if USE_IDLE_TASK == true
	IHM_Print(line++, TaskList.GetIdleStr().c_str(), false);
#endif
}

void Show_Page2(void)
{
	uint8_t line = 0;

	IHM_Print(line++, 5, "Energie", false);
	line++;

	// Energie conso
	IHM_Print(line++, 1, "E conso : ", false);
	Fast_Printf(&buffer[0], Current_Data.energy_day_conso, 2, "", " Wh", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Energie surplus
	IHM_Print(line++, 1, "E surplus : ", false);
	Fast_Printf(&buffer[0], Current_Data.energy_day_surplus, 2, "", " Wh", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Energie prod
	IHM_Print(line++, 1, "E prod : ", false);
	Fast_Printf(&buffer[0], Current_Data.energy_day_prod, 2, "", " Wh", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);
}

void Show_Page3(void)
{
	uint8_t line = 0;

	// Température
	IHM_Print(line++, 5, "Temperature", false);
	line++;

	// Cirrus
	IHM_Print(line++, 1, "Cirrus : ", false);
	Fast_Printf(&buffer[0], Current_Data.Cirrus_ch1.Temperature, 2, "", " `C", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// DS18B20
	IHM_Print(line++, 1, "DS18B20 interne : ", false);
	Fast_Printf(&buffer[0], Current_Data.DS18B20_Int, 2, "", " `C", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// DS18B20
	IHM_Print(line++, 1, "DS18B20 externe : ", false);
	Fast_Printf(&buffer[0], Current_Data.DS18B20_Ext, 2, "", " `C", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);
}

void Show_Page4(void)
{
	uint8_t line = 0;

	// Tension
	IHM_Print(line++, 1, "Tension : ", false);
	Fast_Printf(&buffer[0], Current_Data.Cirrus_ch1.Voltage, 2, "", " V", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Cosphi
	IHM_Print(line++, 1, "Cosphi : ", false);
	Fast_Printf(&buffer[0], Current_Data.Cirrus_ch1.PowerFactor, 2, "", "", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Puissance apparente
	IHM_Print(line++, 1, "P apparente : ", false);
	Fast_Printf(&buffer[0], Current_Data.Cirrus_ch1.ApparentPower, 2, "", " VA", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Puissance TI
	IHM_Print(line++, 1, "P compteur TI : ", false);
	Fast_Printf(&buffer[0], Current_Data.TI_Power, 0, "", " VA", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);
}

void Show_Page5(void)
{
	uint8_t line = 0;

	// Température
	IHM_Print(line++, 5, "PV info", false);
	line++;

	// Puissance prod
	IHM_Print(line++, 1, "P prod : ", false);
	Fast_Printf(&buffer[0], Current_Data.Cirrus_ch2.ActivePower, 2, "", " W", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Puissance théorique
	IHM_Print(line++, 1, "P theorique : ", false);
	Fast_Printf(&buffer[0], Current_Data.Prod_Th, 2, "", " W", Buffer_End, &len);
	IHM_Print(line++, COLUMN, buffer, false);

	// Heure lever, coucher Soleil
	char rise[9], set[9];

	DateTimeToTimeStr(emul_PV.getSunRise(true), rise);
	DateTimeToTimeStr(emul_PV.getSunSet(true), set);
	line++;
	IHM_Print(line++, 3, "** Soleil **", false);
	IHM_Print(line, 0, "Lever :", false);
	IHM_Print(line++, 10, rise, false);
	IHM_Print(line, 0, "Coucher :", false);
	IHM_Print(line++, 10, set, false);
}

#ifdef USE_RELAY
void Show_Page_Relay(uint8_t cursor)
{
	char R_ON[] = "Rx ON";
	char R_OFF[] = "Rx OFF";

	IHM_Print(1, 4, "Etat relais", false);

	for (int i = 0; i < Relay.size(); i++)
	{
		if (Relay.getState(i))
		{
			R_ON[1] = (char) (48 + i);
			IHM_Print(3 + i, 1, R_ON, false);
		}
		else
		{
			R_OFF[1] = (char) (48 + i);
			IHM_Print(3 + i, 1, R_OFF, false);
		}
	}

	// Affichage du cursor
	IHM_Print(3 + cursor, 10, "#", false);
}

void Show_Page_Relay_Action(uint8_t cursor)
{
	IHM_Print(1, 1, "Confirmation", false);

	if (Relay.getState(cursor))
		IHM_Print(3, 3, "Stop Relais ?", false);
	else
		IHM_Print(3, 3, "Run Relais ?", false);

	IHM_Print(5, 1, "OK pour confirmer", false);
}
#endif

void Show_Page_SSR(uint8_t cursor)
{
	IHM_Print(1, 3, "Regulation", false);

	switch (SSR_Get_Action())
	{
		case SSR_Action_Surplus:
			if (SSR_Get_State() != SSR_OFF)
				IHM_Print(3, 1, "SSR ON", false);
			else
				IHM_Print(3, 1, "SSR OFF", false);
			IHM_Print(5, 1, "Boost CE (1 h)", false);
			IHM_Print(6, 1, "Toggle SSR", false);
			// Affichage du cursor
			IHM_Print(5 + cursor, 16, "#", false);
			break;
		case SSR_Action_FULL:
			IHM_Print(3, 1, "SSR FULL 1h", false);
			IHM_Print(5, 1, "Stop Boost CE", false);
			// Affichage du cursor
			IHM_Print(5, 16, "#", false);
			break;
		default:
			IHM_Print(3, 1, "SSR OFF", false);
	}
}

void Show_Page_SSR_Action(uint8_t cursor)
{
	IHM_Print(1, 1, "Confirmation", false);

	switch (SSR_Get_Action())
	{
		case SSR_Action_Surplus:
			if (cursor == 0)
				IHM_Print(3, 1, "Boost CE (1 h)", false);
			else
				IHM_Print(3, 1, "Toggle SSR", false);
			break;
		case SSR_Action_FULL:
			IHM_Print(3, 1, "Stop Boost CE", false);
			break;
		default:
			;
	}

	IHM_Print(5, 1, "OK pour confirmer", false);
}

void Show_Page_Wifi(void)
{
	String mac;
	IHM_Print(1, 7, "Wifi", false);

	IHM_Print(3, 1, myServer.IPaddress().c_str(), false);
	IHM_Print(4, 1, myServer.getCurrentRSSI().c_str(), false);
	if (getESPMacAddress(mac))
		IHM_Print(5, 1, mac.c_str(), false);

	IHM_Print(7, 1, "Reset Wifi ?", false);
}

void Show_Page_Wifi_Action(void)
{
	IHM_Print(1, 1, "Confirmation", false);

	IHM_Print(3, 3, "Reset Wifi ?", false);
	IHM_Print(4, 1, "Un appui de 5 s", false);
	IHM_Print(5, 1, "reset le SSID", false);

	IHM_Print(7, 1, "OK pour confirmer", false);
}

// *****************************************************************
// ***** Fin
// *****************************************************************
