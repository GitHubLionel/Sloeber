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
		void Complete(CIRRUS_Calib_typedef *Calib_base, Cirrus_DoGain_typedef gain, float param1 = -1, float param2 = -1);
		void Gain(CIRRUS_Calib_typedef *Calib_base, Cirrus_DoGain_typedef gain, float param1 = -1, float param2 = -1);
		void IACOffset(CIRRUS_Calib_typedef *Calib_base);
		void PQOffset(CIRRUS_Calib_typedef *Calib_base);
	protected:
		CIRRUS_Base *Current_Cirrus = NULL;
};

#endif /* _CIRRUS_CALIBRATION_H_ */
