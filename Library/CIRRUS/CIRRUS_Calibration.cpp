/*
 * CIRRUS_Calibration.cpp
 */

#include "CIRRUS_Calibration.h"
#include "display.h"

/**
 * Enchaine la calibration sans charge puis la calibration avec charge
 */
void CIRRUS_Calibration::Calibration_Complete(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R)
{
	Calibration_NoCharge();

	// Update Calib_base
	Bit_List i_acoff;
	// get I AC offset registers
	read_register(P16_I1_ACOFF, PAGE16, &i_acoff);
	Calib_base->I1ACOFF = i_acoff.Bit32;

	Calibration_WithCharge(Calib_base, V1_Ref, R);
}

/**
 * Séquence de calibration n°1 :
 * Aucune charge doit être présente : I et V Cirrus déconnectés
 */
void CIRRUS_Calibration::Calibration_NoCharge(bool with_DC)
{
	IHM_Clear();
	IHM_Print(1, "Calibration ...", true);

	// Delais pour déconnecter
	delay(5000);
	if (with_DC)
		do_dc_offset_calibration(true);
	do_ac_offset_calibration();

	IHM_Print(2, "Ended.    ");
	IHM_Print(3, "Reconnect ...", true);
	// Delais pour reconnecter
	delay(5000);
	IHM_Clear();
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
void CIRRUS_Calibration::Calibration_WithCharge(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R)
{
	CIRRUS_Calib_typedef Calib = CS_CALIB0;
	CIRRUS_Config_typedef Config = CS_CONFIG0;

	IHM_Print(1, "Gain ...", true);
	Calib.V1_Calib = Calib_base->V1_Calib;
	Calib.I1_MAX = Calib_base->I1_MAX;
	Calib.V1GAIN = 0x400000;  // Default value
	Calib.I1GAIN = 0x400000;  // Default value
	Calib.I1ACOFF = Calib_base->I1ACOFF;

	Calibration(&Calib);
	do_gain_calibration(V1_Ref, R);

	Get_Parameters(&Calib, &Config);
	Print_Calib(&Calib);

	Bit_List reg;
	// get gain and I AC offset registers
	read_register(P16_V1_GAIN, PAGE16, &reg);
	Calib_base->V1GAIN = reg.Bit32;
	read_register(P16_I1_GAIN, PAGE16, &reg);
	Calib_base->I1GAIN = reg.Bit32;
	read_register(P16_I1_ACOFF, PAGE16, &reg);
	Calib_base->I1ACOFF = reg.Bit32;

	IHM_Print(2, "Ended.    ", true);
}

// ********************************************************************************
// End of file
// ********************************************************************************
