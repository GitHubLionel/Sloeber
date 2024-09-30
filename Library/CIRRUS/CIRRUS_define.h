#pragma once

#include "Arduino.h"
#include <stdint.h>

// Top level commands:
#define REGISTER_READ   0b00000000    // 0x00
#define REGISTER_WRITE  0b01000000    // 0x40
#define PAGE_SELECT     0b10000000    // 0x80
#define INSTRUCTION     0b11000000    // 0xC0

// Controls Instructions
#define SOFT_RESET      0b000001 // Software Reset
#define STANDBY         0b000010 // Standby
#define WAKEUP          0b000011 // Wakeup
#define SINGLE_CONV     0b010100 // Single Conv.
#define CONT_CONV       0b010101 // Continuous Conv.
#define HALT_CONV       0b011000 // Halt Conv.

// Calibration Instructions
#define CALIB_DCOFFS_I1 0b100001 // DC Offset I1
#define CALIB_DCOFFS_V1 0b100010 // DC Offset V1
#define CALIB_DCOFFS_I2 0b100011 // DC Offset I2
#define CALIB_DCOFFS_V2 0b100100 // DC Offset V2
#define CALIB_DCOFFS_IV 0b100110 // DC Offset for all channel available

#define CALIB_ACOFFS_I1 0b110001 // AC Offset* I1 only current channel
#define CALIB_ACOFFS_I2 0b110011 // AC Offset* I2 only current channel
#define CALIB_ACOFFS_IALL	0b110110	// AC Offset* I1 and I2 only current channel

#define CALIB_GAIN_I1   0b111001 // Gain I1
#define CALIB_GAIN_V1 	0b111010 // Gain V1
#define CALIB_GAIN_I2 	0b111011 // Gain I2
#define CALIB_GAIN_V2 	0b111100 // Gain V2
#define CALIB_GAIN_IV 	0b111110 // Gain for all channel available

// Note: There is no instruction to calibrage P and Q offset since
// we just write Pavg and Qavg with no charge

#define CFG2VAL     0x10420A // Enable Highpass filters and APCM calculation
#define DATARDY     0x800000

// DO definition
#define DO_EPG1		0b00000000
#define DO_EPG2		0b00000001
#define DO_EPG3		0b00000010
#define DO_P1Sign	0b00000100
#define DO_P2Sign	0b00000101
#define DO_VZero	0b00001011
#define DO_IZero	0b00001100
#define DO_Interrupt	0b00001111
#define DO_Nothing	0b00001110  // Default


// DO NOT write a "1" to any unpublished register bit or to a bit published as "0"
// DO NOT write a "0" to any bit published as "1"
// DO NOT write to any unpublished register address
// (*) : Registers with checksum protection
// (**) : see datasheet
// Page 0 registers.
#define PAGE0           0x00
#define	P0_Config0	  	0x00	// Configuration 0 (*)				DSP: Y	HOST: Y	Default: 0x C0 2000
#define	P0_Config1	  	0x01	// Configuration 1 (*)				DSP: Y	HOST: Y	Default: 0x 00 EEEE
#define	P0_Mask		  		0x03	// Interrupt Mask (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P0_PC		  			0x05	// Phase Compensation Control (*)		DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P0_SerialCtrl	  0x07	// UART Control (*)				DSP: Y	HOST: Y	Default: 0x 02 004D
#define	P0_PulseWidth	  0x08	// Energy Pulse Width (*)			DSP: Y	HOST: Y	Default: 0x 00 0001
#define	P0_PulseCtrl	  0x09	// Energy Pulse Control (*)			DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P0_Status0	  	0x17	// Interrupt Status				DSP: N	HOST: N	Default: 0x 80 0000
#define	P0_Status1	  	0x18	// Chip Status 1				DSP: N	HOST: N	Default: 0x 80 1800
#define	P0_Status2	  	0x19	// Chip Status 2				DSP: N	HOST: N	Default: 0x 00 0000
#define	P0_RegLock	  	0x22	// Register Lock Control (*)			DSP: N	HOST: N	Default: 0x 00 0000
#define	P0_V1_PEAK	  	0x24	// V1 Peak Voltage				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P0_I1_PEAK	  	0x25	// I1 Peak Current				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P0_V2_PEAK	  	0x26	// V2 Peak Voltage				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P0_I2_PEAK	  	0x27	// I2 Peak Current				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P0_PSDC		  		0x30	// Phase Sequence Detection & Control		DSP: N	HOST: Y	Default: 0x 00 0000
#define	P0_ZX_NUM	  		0x37	// Num. Zero Crosses used for Line Freq.	DSP: Y	HOST: Y	Default: 0x 00 0064

// Page 16 registers.
#define PAGE16          0x10
#define	P16_Config2	 		0x00	// Configuration 2 (*)				DSP: Y	HOST: Y	Default: 0x 00 0200
#define	P16_RegChk	  	0x01	// Register Checksum				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_I1		  		0x02	// I1 Instantaneous Current			DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_V1		  		0x03	// V1 Instantaneous Voltage			DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_P1		  		0x04	// Instantaneous Power 1			DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_P1_AVG	  	0x05	// Active Power 1				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_I1_RMS	  	0x06	// I1 RMS Current				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_V1_RMS	  	0x07	// V1 RMS Voltage				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_I2		  		0x08	// I2 Instantaneous Current			DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_V2		  		0x09	// V2 Instantaneous Voltage			DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_P2		  		0x0A	// Instantaneous Power 2			DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_P2_AVG	  	0x0B	// Active Power 2				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_I2_RMS	  	0x0C	// I2 RMS Current				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_V2_RMS	  	0x0D	// V2 RMS Voltage				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_Q1_AVG	  	0x0E	// Reactive Power 1				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_Q1		  		0x0F	// Instantaneous Reactive Power 1		DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_Q2_AVG	  	0x10	// Reactive Power 2				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_Q2		  		0x11	// Instantaneous Reactive Power 2		DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_S1		  		0x14	// Apparent Power 1				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_PF1		  		0x15	// Power Factor 1				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_S2		  		0x18	// Apparent Power 2				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_PF2		  		0x19	// Power Factor 2				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_T		  			0x1B	// Temperature					DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_P_SUM	  		0x1D	// Total Active Power				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_S_SUM	  		0x1E	// Total Apparent Power				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_Q_SUM	  		0x1F	// Total Reactive Power				DSP: N	HOST: Y	Default: 0x 00 0000
#define	P16_I1_DCOFF	  0x20	// I1 DC Offset (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_I1_GAIN	  	0x21	// I1 Gain (*)					DSP: Y	HOST: Y	Default: 0x 40 0000
#define	P16_V1_DCOFF	  0x22	// V1 DC Offset (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_V1_GAIN	  	0x23	// V1 Gain (*)					DSP: Y	HOST: Y	Default: 0x 40 0000
#define	P16_P1_OFF	  	0x24	// Average Active Power 1 Offset (*)		DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_I1_ACOFF	  0x25	// I1 AC Offset (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_Q1_OFF	  	0x26	// Average Reactive Power 1 Offset (*)		DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_I2_DCOFF	  0x27	// I2 DC Offset (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_I2_GAIN	  	0x28	// I2 Gain (*)					DSP: Y	HOST: Y	Default: 0x 40 0000
#define	P16_V2_DCOFF	  0x29	// V2 DC Offset (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_V2_GAIN	  	0x2A	// V2 Gain (*)					DSP: Y	HOST: Y	Default: 0x 40 0000
#define	P16_P2_OFF	  	0x2B	// Average Active Power 2 Offset (*)		DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_I2_ACOFF	  0x2C	// I2 AC Offset (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_Q2_OFF	  	0x2D	// Average Reactive Power 2 Offset (*)		DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_Epsilon	  	0x31	// Ratio of Line to Sample Frequency		DSP: N	HOST: Y	Default: 0x 01 999A
#define	P16_Ichan_LEVEL	0x32	// Automatic Channel Select Level (*)		DSP: Y	HOST: Y	Default: 0x 82 8F5C
#define	P16_SampleCount	0x33	// Sample Count (**)				DSP: N	HOST: Y	Default: 0x 00 0FA0
#define	P16_T_GAIN	  	0x36	// Temperature Gain (*)				DSP: Y	HOST: Y	Default: 0x 06 B716
#define	P16_T_OFF	  		0x37	// Temperature Offset (*)			DSP: Y	HOST: Y	Default: 0x D5 3998
#define	P16_P_MIN_IRMS	0x38	// Channel Select Minimum Amplitude (*)		DSP: Y	HOST: Y	Default: 0x 00 624D
#define	P16_T_SETTLE	  0x39	// Filter Settling Time to Conv. Startup	DSP: Y	HOST: Y	Default: 0x 00 001E
#define	P16_Load_MIN	  0x3A	// No Load Threshold (*)			DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P16_VF_RMS	  	0x3B	// Voltage Fixed RMS Reference (*)		DSP: Y	HOST: Y	Default: 0x 5A 8279
#define	P16_SYS_GAIN	  0x3C	// System Gain (*)				DSP: N	HOST: Y	Default: 0x 50 0000
#define	P16_Time	  		0x3D	// System Time (in samples)			DSP: N	HOST: Y	Default: 0x 00 0000

// Page 17 registers.
#define PAGE17            0x11
#define	P17_V1Sag_DUR	  	0x00	// V1 Sag Duration (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P17_V1Sag_LEVEL	  0x01	// V1 Sag Level (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P17_I1Over_DUR	  0x04	// I1 Overcurrent Duration (*)			DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P17_I1Over_LEVEL  0x05	// I1 Overcurrent Level (*)			DSP: Y	HOST: Y	Default: 0x 7F FFFF
#define	P17_V2Sag_DUR	 		0x08	// V2 Sag Duration (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P17_V2Sag_LEVEL	  0x09	// V2 Sag Level (*)				DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P17_I2Over_DUR	  0x0C	// I2 Overcurrent Duration (*)			DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P17_I2Over_LEVEL  0x0D	// I2 Overcurrent Level (*)			DSP: Y	HOST: Y	Default: 0x 7F FFFF

// Page 18 registers.
#define PAGE18            0x12
#define	P18_IZX_LEVEL	  	0x18	// Zero-Cross Threshold for I-Channel (*)	DSP: Y	HOST: Y	Default: 0x 10 0000
#define	P18_PulseRate	  	0x1C	// Energy Pulse Rate (*)			DSP: Y	HOST: Y	Default: 0x 80 0000
#define	P18_INT_GAIN	  	0x2B	// Rogowski Coil lntegrator Gain (*)		DSP: Y	HOST: Y	Default: 0x 14 3958
#define	P18_V1Sweil_DUR	  0x2E	// V1 Swell Duration (*)			DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P18_V1Sweil_LEVEL 0x2F	// V1 Swell Level (*)				DSP: Y	HOST: Y	Default: 0x 7F FFFF
#define	P18_V2Sweil_DUR	  0x32	// V2 Swell Duration (*)			DSP: Y	HOST: Y	Default: 0x 00 0000
#define	P18_V2Sweil_LEVEL 0x33	// V2 Swell Level (*)				DSP: Y	HOST: Y	Default: 0x 7F FFFF
#define	P18_VZX_LEVEL	  	0x3A	// Zero-Cross Threshold for V-Channel (*)	DSP: Y	HOST: Y	Default: 0x 10 0000
#define	P18_CycleCount	  0x3E	// Line Cycle Count (**)			DSP: N	HOST: Y	Default: 0x 00 0064
#define	P18_Scale	  			0x3F	// I-Channel Gain Calibration Scale Value (*)	DSP: Y	HOST: Y	Default: 0x 4C CCCC

// Bit mask definition
// Syntax : (Page, Register, Bit)
// Permet d'accéder directement a un bit dans un registre d'une page donnée
// Bit entre 0 et 23 : LSB [0..7], MSB [8..15], HSB [16..23]

// PAGE0, Config0 (Configuration 0) pdf page 37
#define REG_MASK_INT_POL       ((Reg_Mask){PAGE0, P0_Config0, 8})  // Interrupt Polarity
#define REG_MASK_I2GPA1        ((Reg_Mask){PAGE0, P0_Config0, 7})  // Select PGA gain for I2 channel (1 = 50x gain)
#define REG_MASK_I2GPA0        ((Reg_Mask){PAGE0, P0_Config0, 6})  //
#define REG_MASK_I1GPA1        ((Reg_Mask){PAGE0, P0_Config0, 5})  // Select PGA gain for I1 channel (1 = 50x gain)
#define REG_MASK_I1GPA0        ((Reg_Mask){PAGE0, P0_Config0, 4})  //
#define REG_MASK_NO_OSC        ((Reg_Mask){PAGE0, P0_Config0, 2})  // Disable crystal oascillator (0 enabled, 1 disabled)
#define REG_MASK_IZX_CH        ((Reg_Mask){PAGE0, P0_Config0, 1})  // Select current channel for zero-cross detect (0 channel 1, 1 channel 2)

// PAGE0, Config1 (Configuration 1) pdf page 38
#define REG_MASK_EPG3_ON       ((Reg_Mask){PAGE0, P0_Config1, 22})  // Enabled EPG3 block
#define REG_MASK_EPG2_ON       ((Reg_Mask){PAGE0, P0_Config1, 21})  // Enabled EPG2 block
#define REG_MASK_EPG1_ON       ((Reg_Mask){PAGE0, P0_Config1, 20})  // Enabled EPG1 block
#define REG_MASK_DO3_OD        ((Reg_Mask){PAGE0, P0_Config1, 18})  // Allow the DO3 pin to be an open-drain output
#define REG_MASK_DO2_OD        ((Reg_Mask){PAGE0, P0_Config1, 17})  // Allow the DO2 pin to be an open-drain output
#define REG_MASK_DO1_OD        ((Reg_Mask){PAGE0, P0_Config1, 16})  // Allow the DO1 pin to be an open-drain output
#define REG_MASK_DOMODE3       ((Reg_Mask){PAGE0, P0_Config1, 3})  // Output control for DO pin
#define REG_MASK_DOMODE2       ((Reg_Mask){PAGE0, P0_Config1, 2})  //
#define REG_MASK_DOMODE1       ((Reg_Mask){PAGE0, P0_Config1, 1})  //
#define REG_MASK_DOMODE0       ((Reg_Mask){PAGE0, P0_Config1, 0})  //

// PAGE0, Status0 (Interrupt Status) pdf page 46
#define REG_MASK_DRDY          ((Reg_Mask){PAGE0, P0_Status0, 23})  // Data Ready
#define REG_MASK_CRDY          ((Reg_Mask){PAGE0, P0_Status0, 22})  // Conversion Ready
#define REG_MASK_WOF           ((Reg_Mask){PAGE0, P0_Status0, 21})  // Watchdog timeout overflow
#define REG_MASK_MIPS          ((Reg_Mask){PAGE0, P0_Status0, 18})  // MIPS overflow
#define REG_MASK_VSWELL        ((Reg_Mask){PAGE0, P0_Status0, 16})  // Voltage channel swell event detected.
#define REG_MASK_POR           ((Reg_Mask){PAGE0, P0_Status0, 14})  // Power out of range.
#define REG_MASK_IOR           ((Reg_Mask){PAGE0, P0_Status0, 12})  // Current out of range.
#define REG_MASK_VOR           ((Reg_Mask){PAGE0, P0_Status0, 10})  // Voltage out of range.
#define REG_MASK_IOC           ((Reg_Mask){PAGE0, P0_Status0, 8})  // I Overcurrent.
#define REG_MASK_VSAG          ((Reg_Mask){PAGE0, P0_Status0, 6})  // Voltage channel sag event detected.
#define REG_MASK_TUP           ((Reg_Mask){PAGE0, P0_Status0, 5})  // Temperature updated. Indicates when the Temperature register (7) has been updated.
#define REG_MASK_FUP           ((Reg_Mask){PAGE0, P0_Status0, 4})  // Frequency updated. Indicates the Epsilon register has been updated.
#define REG_MASK_IC            ((Reg_Mask){PAGE0, P0_Status0, 3})  // Invalid command has been received.
#define REG_MASK_RX_CSUM_ERR   ((Reg_Mask){PAGE0, P0_Status0, 2})  // Received data checksum error.
#define REG_MASK_RXTO          ((Reg_Mask){PAGE0, P0_Status0, 0})  // SDI/RX time out. Sets to one automatically when SDI/RX time out occurs.

// PAGE0, Status1
#define REG_MASK_TOD           ((Reg_Mask){PAGE0, P0_Status1, 3})  // Modulator oscillation has been detected in the temperature ADC.
#define REG_MASK_VOD	       	 ((Reg_Mask){PAGE0, P0_Status1, 2})  // Modulator oscillation has been detected in the voltage ADC.
#define REG_MASK_IOD           ((Reg_Mask){PAGE0, P0_Status1, 0})  // Modulator oscillation has been detected in the current ADC.

// PAGE0, Status2 pdf page 48
#define REG_MASK_QSUM_SIGN     ((Reg_Mask){PAGE0, P0_Status2, 5})  // Indicates the sign of the value contained in Q_SUM
#define REG_MASK_Q2SIGN        ((Reg_Mask){PAGE0, P0_Status2, 4})  // Indicates the sign of the value contained in Q2_AVG
#define REG_MASK_Q1SIGN        ((Reg_Mask){PAGE0, P0_Status2, 3})  // Indicates the sign of the value contained in Q1_AVG
#define REG_MASK_PSUM_SIGN     ((Reg_Mask){PAGE0, P0_Status2, 2})  // Indicates the sign of the value contained in P_SUM
#define REG_MASK_P2SIGN        ((Reg_Mask){PAGE0, P0_Status2, 1})  // Indicates the sign of the value contained in P2_AVG
#define REG_MASK_P1SIGN        ((Reg_Mask){PAGE0, P0_Status2, 0})  // Indicates the sign of the value contained in P1_AVG
