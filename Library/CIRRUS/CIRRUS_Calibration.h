/*
 * CIRRUS_Calibration.h
 */
#ifndef _CIRRUS_CALIBRATION_H_
#define _CIRRUS_CALIBRATION_H_

#include "CIRRUS.h"

class CIRRUS_Calibration
{
	public:
		CIRRUS_Calibration()
		{

		}
		CIRRUS_Calibration(CIRRUS_Base &cirrus)
		{
			Current_Cirrus = &cirrus;
		}
		void SetCirrus(CIRRUS_Base &cirrus)
		{
			Current_Cirrus = &cirrus;
		}
		void Complete(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R);
		void IACOffset(CIRRUS_Calib_typedef *Calib_base);
		void Gain(CIRRUS_Calib_typedef *Calib_base, float V_Ref, float R);
	protected:
		CIRRUS_Base *Current_Cirrus = NULL;
};

#endif /* _CIRRUS_CALIBRATION_H_ */
