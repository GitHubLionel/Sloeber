#include "CIRRUS.h"
#include "display.h"
#include "Debug_utils.h"
#include "Server_utils.h"

/**
 * Generic initialization.
 * After CIRRUS_Communication.begin() and CIRRUS_Base.begin(), we need to validate the connexion
 * and set the calibration and configuration.
 * If use Flash, set Flash_op_load to true to load the calibration and configuration, false to save.
 * Flash_id is the char number of the Cirrus.
 */
bool CIRRUS_Generic_Initialization(CIRRUS_Base &Cirrus, CIRRUS_Calib_typedef *CS_Calib,
		CIRRUS_Config_typedef *CS_Config, bool print_data, bool Flash_op_load, char Flash_id)
{
	if (Cirrus.TryConnexion())
	{
		IHM_Print0((Cirrus.GetName() + " OK").c_str());
		print_debug((Cirrus.GetName() + " OK").c_str());

#ifdef CIRRUS_CALIBRATION
//	CS_Calibration.Complete(&CS_CalibHandle, 227.0, 49.1);
#endif

#ifdef CIRRUS_FLASH
		if (Flash_op_load)
			CIRRUS_Load_From_FLASH(Flash_id, CS_Calib, CS_Config);
		else
			CIRRUS_Save_To_FLASH(Flash_id, CS_Calib, CS_Config);
		Cirrus.Print_Calib(CS_Calib);
		Cirrus.Print_Config(CS_Config);
#else
		(void) Flash_op_load;
		(void) Flash_id;
#endif
		Cirrus.Calibration(CS_Calib);
		Cirrus.Configuration(100, CS_Config, true);
		if (print_data)
			Cirrus.Print_FullData();

		// Check puissance positive
		if (!Cirrus.Check_Positive_Power(Channel_1))
		{
			IHM_Print(1, "P negatif");
			IHM_Print(2, "Retournez tore", true);
			print_debug(F("P est negatif"));
			delay(5000);
		}
		if (Cirrus.IsTwoChannel())
		{
			if (!Cirrus.Check_Positive_Power(Channel_2))
			{
				IHM_Print(3, "P ch2 negatif");
				IHM_Print(4, "Retournez tore", true);
				print_debug(F("P ch2 est negatif"));
				delay(5000);
			}
		}
		return true;
	}
	else
	{
		IHM_Print0((Cirrus.GetName() + " fail").c_str());
		print_debug((Cirrus.GetName() + " connexion fail").c_str());
		delay(2000);
		return false;
	}
	IHM_Clear(true);
}

/**
 * Restart the Cirrus by reloading calibration and configuration
 */
void CIRRUS_Restart(CIRRUS_Base &Cirrus, CIRRUS_Calib_typedef *CS_Calib,
		CIRRUS_Config_typedef *CS_Config)
{
	delay(2000);
	IHM_Clear(true);
// Then restart Cirrus
	Cirrus.Calibration(CS_Calib);
	Cirrus.Configuration(100, CS_Config, true);
	delay(100); // delay to get first data
}

/**
 * A general response for a Wifi "/getCirrus" request
 * Use Handle_Wifi_Request() that must be defined in the main file.
 * For example :
 String Handle_Wifi_Request(CS_Common_Request Wifi_Request, char *Request)
{
	return CS_Com.Handle_Common_Request(Wifi_Request, Request, &CS_Calib, &CS_Config);
}
 * Work only with the LOG_CIRRUS_CONNECT directive
 */
void handleCirrus(CB_SERVER_PARAM)
{
	// Default
	RETURN_BAD_ARGUMENT();

//	print_debug("Cirrus Operation: " + pserver->argName(0) + " = " + pserver->arg((int) 0));

#ifdef LOG_CIRRUS_CONNECT
	CS_Common_Request Wifi_Request = csw_NONE;
	char Request[255] = {0};

	// Demande d'un registre
	if (pserver->hasArg("REG"))
	{
		Wifi_Request = csw_REG;
		strcpy(Request, (const char*) pserver->arg("REG").c_str());
	}
	else
		// Demande des scales (U_Calib, I_Max)
		if (pserver->hasArg("SCALE"))
		{
			Wifi_Request = csw_SCALE;
			strcpy(Request, (const char*) pserver->arg("SCALE").c_str());
		}
		else
			// Demande de plusieurs registres (graphe, dump)
			if (pserver->hasArg("REG_MULTI"))
			{
				Wifi_Request = csw_REG_MULTI;
				strcpy(Request, (const char*) pserver->arg("REG_MULTI").c_str());
			}
			else
				// Changement de la vitesse
				if (pserver->hasArg("BAUD"))
				{
					Wifi_Request = csw_BAUD;
					strcpy(Request, (const char*) pserver->arg("BAUD").c_str());
				}
#ifdef CIRRUS_CALIBRATION
				else
					// Demande calibration sans charge
					if (pserver->hasArg("NOLOAD"))
					{
						Wifi_Request = csw_NOLOAD;
						strcpy(Request, "NOLOAD");
					}
					else
						// Demande calibration gain (avec charge)
						if (pserver->hasArg("GAIN"))
						{
							Wifi_Request = csw_GAIN;
							strcpy(Request, (const char*) pserver->arg("GAIN").c_str());
						}
#endif // CALIBRATION

	String result = "";
	if (Wifi_Request != csw_NONE)
	{
		result = Handle_Wifi_Request(Wifi_Request, Request);
	}

	if (result.isEmpty())
		pserver->send(204, "text/plain", "");
	else
		pserver->send(200, "text/plain", result);
#else
	pserver->send(204, "text/plain", "No CIRRUS_CONNECT");
#endif
}

// ********************************************************************************
// End of file
// ********************************************************************************
