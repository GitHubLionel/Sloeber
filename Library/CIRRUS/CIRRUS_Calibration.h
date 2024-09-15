/*
 * CIRRUS_Calibration.h
 */
#ifndef _CIRRUS_CALIBRATION_H_
#define _CIRRUS_CALIBRATION_H_

#include "CIRRUS.h"

class CIRRUS_Calibration
{
	public:
		CIRRUS_Calibration(CIRRUS_Base &cirrus)
		{
			Current_Cirrus = &cirrus;
		}
		void Complete(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R);
		void NoCharge(CIRRUS_Calib_typedef *Calib_base, bool with_DC = true);
		void WithCharge(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R);
	protected:
		CIRRUS_Base *Current_Cirrus = NULL;
};

#endif /* _CIRRUS_CALIBRATION_H_ */
