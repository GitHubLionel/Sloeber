/*
 * CIRRUS_Calibration.cpp
 */
#include "CIRRUS_Calibration.h"
#include "display.h"

/**
 * Enchaine la calibration sans charge puis la calibration avec charge
 */
void CIRRUS_Calibration::Complete(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R)
{
	if (Current_Cirrus == NULL)
	{
		IHM_Clear(true);
		IHM_Print(1, "No Cirrus !", true);
		return;
	}

	// Gain calibration
	Gain(Calib_base, V1_Ref, R);

	// IAC offset calibration
	IACOffset(Calib_base);
}

/**
 * Calibration n°2 : IAC offset
 * Line voltage is fullscale and no current are applied to the CIRRUS
 */
void CIRRUS_Calibration::IACOffset(CIRRUS_Calib_typedef *Calib_base)
{
	if (Current_Cirrus == NULL)
	{
		IHM_Clear(true);
		IHM_Print(1, "No Cirrus !", true);
		return;
	}

	IHM_Clear(true);
	IHM_Print(1, "IAC off cal ...", true);

	// Delais pour déconnecter
	delay(5000);
//	if (with_DC)
//		Current_Cirrus->do_dc_offset_calibration(true);
	Current_Cirrus->do_Iac_offset_calibration();

	// Update Calib_base
	Bit_List i_acoff;
	// get I AC offset registers
	Current_Cirrus->read_register(P16_I1_ACOFF, PAGE16, &i_acoff);
	Calib_base->I1ACOFF = i_acoff.Bit32;

	IHM_Print(2, "Ended.    ");
	IHM_Print(3, "Reconnect ...", true);
	// Delais pour reconnecter
	delay(5000);
	IHM_Clear(true);
}

/**
 * Séquence de calibration n°2 :
 * Charge connue : tension, résistance R=U²/P
 * Test radiateur 1000 W sous 220 V ==> R = 48.4 et tension mesurée par Saia, ...
 * R = 48.4 théorique, 49.1 mesuré à l'ohmmètre
 * Calib_base : pour les coefficients V et I hardware et I1AC offset
 * V1_Ref : tension mesurée lors du test
 * R : résistance de la charge
 */
void CIRRUS_Calibration::Gain(CIRRUS_Calib_typedef *Calib_base, float V_Ref, float R)
{
	if (Current_Cirrus == NULL)
	{
		IHM_Clear(true);
		IHM_Print(1, "No Cirrus !", true);
		return;
	}

//	CIRRUS_Calib_typedef Calib = CS_CALIB0;
//	CIRRUS_Config_typedef Config = CS_CONFIG0;

	IHM_Clear(true);
	IHM_Print(1, "Gain ...", true);
//	Current_Cirrus->SetScale(scale);
//
//	// First channel
//	Calib.V1_Calib = Calib_base->V1_Calib;
//	Calib.I1_MAX = Calib_base->I1_MAX;
//	Calib.V1GAIN = 0x400000;  // Default value
//	Calib.I1GAIN = 0x400000;  // Default value
//	Calib.I1ACOFF = Calib_base->I1ACOFF;
//	// Second channel
//	Calib.V2_Calib = Calib_base->V2_Calib;
//	Calib.I2_MAX = Calib_base->I2_MAX;
//	Calib.V2GAIN = 0x400000;  // Default value
//	Calib.I2GAIN = 0x400000;  // Default value
//	Calib.I2ACOFF = Calib_base->I2ACOFF;

	// Set base calibration (soft reset)
//	Current_Cirrus->Calibration(&Calib);
	Current_Cirrus->do_gain_calibration(V_Ref, R);

//	Current_Cirrus->Get_Parameters(&Calib, &Config);
//	Current_Cirrus->Print_Calib(&Calib);

	Bit_List reg;
	// get gain offset registers and update base calibration
	// First channel
	Current_Cirrus->read_register(P16_V1_GAIN, PAGE16, &reg);
	Calib_base->V1GAIN = reg.Bit32;
	Current_Cirrus->read_register(P16_I1_GAIN, PAGE16, &reg);
	Calib_base->I1GAIN = reg.Bit32;
//	Current_Cirrus->read_register(P16_I1_ACOFF, PAGE16, &reg);
//	Calib_base->I1ACOFF = reg.Bit32;
	// Second channel
	Current_Cirrus->read_register(P16_V2_GAIN, PAGE16, &reg);
	Calib_base->V2GAIN = reg.Bit32;
	Current_Cirrus->read_register(P16_I2_GAIN, PAGE16, &reg);
	Calib_base->I2GAIN = reg.Bit32;
//	Current_Cirrus->read_register(P16_I2_ACOFF, PAGE16, &reg);
//	Calib_base->I2ACOFF = reg.Bit32;

	IHM_Print(2, "Ended.    ", true);
}

// ********************************************************************************
// End of file
// ********************************************************************************
