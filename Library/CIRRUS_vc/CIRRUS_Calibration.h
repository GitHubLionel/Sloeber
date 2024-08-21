/*
 * CIRRUS_Calibration.h
 */

#ifndef _CIRRUS_CALIBRATION_H_
#define _CIRRUS_CALIBRATION_H_

#include "CIRRUS.h"

void Calibration_Complete(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R);
void Calibration_NoCharge(bool with_DC = true);
void Calibration_WithCharge(CIRRUS_Calib_typedef *Calib_base, float V1_Ref, float R);

#endif /* _CIRRUS_CALIBRATION_H_ */
