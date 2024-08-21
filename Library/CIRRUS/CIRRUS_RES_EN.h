#pragma once

#define  CS_RES_HAL_ERROR        "Transmission error\r\n"
#define  CS_RES_BAUD_INIT        "Initial baud rate :"
#define  CS_RES_BAUD_NEW         "New baud rate :"
#define  CS_RES_BAUD_NEW2        "New baud rate is written\r\n"
#define  CS_RES_BAUD_NO          "Can not read initial baud rate\r\n"
#define  CS_RES_BAUD_VERIF       "Baud rate verification\r\n"
#define  CS_RES_BAUD_VERIF_FAIL  "Baud verification failed\r\n"
#define  CS_RES_CHECK_TEST       "Checksum test error\r\n"
#define  CS_RES_TOO_BIG          "Too big number to convert in 2-complement format\r\n"
#define  CS_RES_READ_ERROR       "Read byte error: number of byte read: "
#define  CS_RES_CHECK_ERROR      "Checksum error, register: "
#define  CS_RES_CHECK_STATUS     "Cannot figure the checksum status of the CIRRUS, value of the register: "
#define  CS_RES_CHECK_ERROR2     "CIRRUS read data checksum error, data: "
#define  CS_RES_MAX_CURRENT      "The max current is more than the current shunt resistor can handle!"
#define  CS_RES_NOCALIB          "I1_SCALE not defined! Please call CIRRUS_Calibration before.\r\n"
#define  CS_RES_CALIB            "Calibration in progress...\r\n"
#define  CS_RES_PIVOR            "PIVOR! P,I or V out of range: "
#define  CS_RES_REMEMBER         "Remember to store these values\r\n" 
#define  CS_RES_NOLINE           "It's assumed no line and no voltage/current are applied to the CIRRUS\r\n"
#define  CS_RES_LOAD             "A load is assumed to be present: V_RMS = %.2f V, I_RMS = %.2f A, R = %.2f Ohm, P_RMS = %.2f W\r\n"
#define  CS_RES_NOLOADPOWER      "It's assumed full scale voltage and no current (no load) are applied to the CIRRUS\r\n"
