/**
  ******************************************************************************
  * @file    Font12.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    18-February-2014
  * @brief   This file provides text Font12 for STM32xx-EVAL's LCD driver. 
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "fonts_8_24.h"

// 
//  Font data for Courier New 12pt
// 

const uint8_t Font12_Table[] = 
{
	// @0 ' ' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        

	// @12 '!' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x00, //        
	0x00, //        
	0x10, //    #   
	0x00, //        
	0x00, //        
	0x00, //        

	// @24 '"' (7 pixels wide)
	0x00, //        
	0x6C, //  ## ## 
	0x48, //  #  #  
	0x48, //  #  #  
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        

	// @36 '#' (7 pixels wide)
	0x00, //        
	0x14, //    # # 
	0x14, //    # # 
	0x28, //   # #  
	0x7C, //  ##### 
	0x28, //   # #  
	0x7C, //  ##### 
	0x28, //   # #  
	0x50, //  # #   
	0x50, //  # #   
	0x00, //        
	0x00, //        

	// @48 '$' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x38, //   ###  
	0x40, //  #     
	0x40, //  #     
	0x38, //   ###  
	0x48, //  #  #  
	0x70, //  ###   
	0x10, //    #   
	0x10, //    #   
	0x00, //        
	0x00, //        

	// @60 '%' (7 pixels wide)
	0x00, //        
	0x20, //   #    
	0x50, //  # #   
	0x20, //   #    
	0x0C, //     ## 
	0x70, //  ###   
	0x08, //     #  
	0x14, //    # # 
	0x08, //     #  
	0x00, //        
	0x00, //        
	0x00, //        

	// @72 '&' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x18, //    ##  
	0x20, //   #    
	0x20, //   #    
	0x54, //  # # # 
	0x48, //  #  #  
	0x34, //   ## # 
	0x00, //        
	0x00, //        
	0x00, //        

	// @84 ''' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        

	// @96 '(' (7 pixels wide)
	0x00, //        
	0x08, //     #  
	0x08, //     #  
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x08, //     #  
	0x08, //     #  
	0x00, //        

	// @108 ')' (7 pixels wide)
	0x00, //        
	0x20, //   #    
	0x20, //   #    
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x20, //   #    
	0x20, //   #    
	0x00, //        

	// @120 '*' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x7C, //  ##### 
	0x10, //    #   
	0x28, //   # #  
	0x28, //   # #  
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        

	// @132 '+' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0xFE, // #######
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x00, //        
	0x00, //        
	0x00, //        

	// @144 ',' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x18, //    ##  
	0x10, //    #   
	0x30, //   ##   
	0x20, //   #    
	0x00, //        

	// @156 '-' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        

	// @168 '.' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x30, //   ##   
	0x30, //   ##   
	0x00, //        
	0x00, //        
	0x00, //        

	// @180 '/' (7 pixels wide)
	0x00, //        
	0x04, //      # 
	0x04, //      # 
	0x08, //     #  
	0x08, //     #  
	0x10, //    #   
	0x10, //    #   
	0x20, //   #    
	0x20, //   #    
	0x40, //  #     
	0x00, //        
	0x00, //        

	// @192 '0' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @204 '1' (7 pixels wide)
	0x00, //        
	0x30, //   ##   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @216 '2' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x04, //      # 
	0x08, //     #  
	0x10, //    #   
	0x20, //   #    
	0x44, //  #   # 
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @228 '3' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x04, //      # 
	0x18, //    ##  
	0x04, //      # 
	0x04, //      # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @240 '4' (7 pixels wide)
	0x00, //        
	0x0C, //     ## 
	0x14, //    # # 
	0x14, //    # # 
	0x24, //   #  # 
	0x44, //  #   # 
	0x7E, //  ######
	0x04, //      # 
	0x0E, //     ###
	0x00, //        
	0x00, //        
	0x00, //        

	// @252 '5' (7 pixels wide)
	0x00, //        
	0x3C, //   #### 
	0x20, //   #    
	0x20, //   #    
	0x38, //   ###  
	0x04, //      # 
	0x04, //      # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @264 '6' (7 pixels wide)
	0x00, //        
	0x1C, //    ### 
	0x20, //   #    
	0x40, //  #     
	0x78, //  ####  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @276 '7' (7 pixels wide)
	0x00, //        
	0x7C, //  ##### 
	0x44, //  #   # 
	0x04, //      # 
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x10, //    #   
	0x10, //    #   
	0x00, //        
	0x00, //        
	0x00, //        

	// @288 '8' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @300 '9' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x3C, //   #### 
	0x04, //      # 
	0x08, //     #  
	0x70, //  ###   
	0x00, //        
	0x00, //        
	0x00, //        

	// @312 ':' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x30, //   ##   
	0x30, //   ##   
	0x00, //        
	0x00, //        
	0x30, //   ##   
	0x30, //   ##   
	0x00, //        
	0x00, //        
	0x00, //        

	// @324 ';' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x18, //    ##  
	0x18, //    ##  
	0x00, //        
	0x00, //        
	0x18, //    ##  
	0x30, //   ##   
	0x20, //   #    
	0x00, //        
	0x00, //        

	// @336 '<' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x0C, //     ## 
	0x10, //    #   
	0x60, //  ##    
	0x80, // #      
	0x60, //  ##    
	0x10, //    #   
	0x0C, //     ## 
	0x00, //        
	0x00, //        
	0x00, //        

	// @348 '=' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x7C, //  ##### 
	0x00, //        
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        

	// @360 '>' (7 pixels wide)
	0x00, //        
	0x00, //        
	0xC0, // ##     
	0x20, //   #    
	0x18, //    ##  
	0x04, //      # 
	0x18, //    ##  
	0x20, //   #    
	0xC0, // ##     
	0x00, //        
	0x00, //        
	0x00, //        

	// @372 '?' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x18, //    ##  
	0x24, //   #  # 
	0x04, //      # 
	0x08, //     #  
	0x10, //    #   
	0x00, //        
	0x30, //   ##   
	0x00, //        
	0x00, //        
	0x00, //        

	// @384 '@' (7 pixels wide)
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x4C, //  #  ## 
	0x54, //  # # # 
	0x54, //  # # # 
	0x4C, //  #  ## 
	0x40, //  #     
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        

	// @396 'A' (7 pixels wide)
	0x00, //        
	0x30, //   ##   
	0x10, //    #   
	0x28, //   # #  
	0x28, //   # #  
	0x28, //   # #  
	0x7C, //  ##### 
	0x44, //  #   # 
	0xEE, // ### ###
	0x00, //        
	0x00, //        
	0x00, //        

	// @408 'B' (7 pixels wide)
	0x00, //        
	0xF8, // #####  
	0x44, //  #   # 
	0x44, //  #   # 
	0x78, //  ####  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0xF8, // #####  
	0x00, //        
	0x00, //        
	0x00, //        

	// @420 'C' (7 pixels wide)
	0x00, //        
	0x3C, //   #### 
	0x44, //  #   # 
	0x40, //  #     
	0x40, //  #     
	0x40, //  #     
	0x40, //  #     
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @432 'D' (7 pixels wide)
	0x00, //        
	0xF0, // ####   
	0x48, //  #  #  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x48, //  #  #  
	0xF0, // ####   
	0x00, //        
	0x00, //        
	0x00, //        

	// @444 'E' (7 pixels wide)
	0x00, //        
	0xFC, // ###### 
	0x44, //  #   # 
	0x50, //  # #   
	0x70, //  ###   
	0x50, //  # #   
	0x40, //  #     
	0x44, //  #   # 
	0xFC, // ###### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @456 'F' (7 pixels wide)
	0x00, //        
	0x7E, //  ######
	0x22, //   #   #
	0x28, //   # #  
	0x38, //   ###  
	0x28, //   # #  
	0x20, //   #    
	0x20, //   #    
	0x70, //  ###   
	0x00, //        
	0x00, //        
	0x00, //        

	// @468 'G' (7 pixels wide)
	0x00, //        
	0x3C, //   #### 
	0x44, //  #   # 
	0x40, //  #     
	0x40, //  #     
	0x4E, //  #  ###
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @480 'H' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x44, //  #   # 
	0x7C, //  ##### 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0xEE, // ### ###
	0x00, //        
	0x00, //        
	0x00, //        

	// @492 'I' (7 pixels wide)
	0x00, //        
	0x7C, //  ##### 
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @504 'J' (7 pixels wide)
	0x00, //        
	0x3C, //   #### 
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x48, //  #  #  
	0x48, //  #  #  
	0x48, //  #  #  
	0x30, //   ##   
	0x00, //        
	0x00, //        
	0x00, //        

	// @516 'K' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x48, //  #  #  
	0x50, //  # #   
	0x70, //  ###   
	0x48, //  #  #  
	0x44, //  #   # 
	0xE6, // ###  ##
	0x00, //        
	0x00, //        
	0x00, //        

	// @528 'L' (7 pixels wide)
	0x00, //        
	0x70, //  ###   
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x24, //   #  # 
	0x24, //   #  # 
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @540 'M' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x6C, //  ## ## 
	0x6C, //  ## ## 
	0x54, //  # # # 
	0x54, //  # # # 
	0x44, //  #   # 
	0x44, //  #   # 
	0xEE, // ### ###
	0x00, //        
	0x00, //        
	0x00, //        

	// @552 'N' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x64, //  ##  # 
	0x64, //  ##  # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x4C, //  #  ## 
	0xEC, // ### ## 
	0x00, //        
	0x00, //        
	0x00, //        

	// @564 'O' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @576 'P' (7 pixels wide)
	0x00, //        
	0x78, //  ####  
	0x24, //   #  # 
	0x24, //   #  # 
	0x24, //   #  # 
	0x38, //   ###  
	0x20, //   #    
	0x20, //   #    
	0x70, //  ###   
	0x00, //        
	0x00, //        
	0x00, //        

	// @588 'Q' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x1C, //    ### 
	0x00, //        
	0x00, //        

	// @600 'R' (7 pixels wide)
	0x00, //        
	0xF8, // #####  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x78, //  ####  
	0x48, //  #  #  
	0x44, //  #   # 
	0xE2, // ###   #
	0x00, //        
	0x00, //        
	0x00, //        

	// @612 'S' (7 pixels wide)
	0x00, //        
	0x34, //   ## # 
	0x4C, //  #  ## 
	0x40, //  #     
	0x38, //   ###  
	0x04, //      # 
	0x04, //      # 
	0x64, //  ##  # 
	0x58, //  # ##  
	0x00, //        
	0x00, //        
	0x00, //        

	// @624 'T' (7 pixels wide)
	0x00, //        
	0xFE, // #######
	0x92, // #  #  #
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @636 'U' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @648 'V' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x44, //  #   # 
	0x28, //   # #  
	0x28, //   # #  
	0x28, //   # #  
	0x10, //    #   
	0x10, //    #   
	0x00, //        
	0x00, //        
	0x00, //        

	// @660 'W' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x44, //  #   # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x28, //   # #  
	0x00, //        
	0x00, //        
	0x00, //        

	// @672 'X' (7 pixels wide)
	0x00, //        
	0xC6, // ##   ##
	0x44, //  #   # 
	0x28, //   # #  
	0x10, //    #   
	0x10, //    #   
	0x28, //   # #  
	0x44, //  #   # 
	0xC6, // ##   ##
	0x00, //        
	0x00, //        
	0x00, //        

	// @684 'Y' (7 pixels wide)
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x28, //   # #  
	0x28, //   # #  
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @696 'Z' (7 pixels wide)
	0x00, //        
	0x7C, //  ##### 
	0x44, //  #   # 
	0x08, //     #  
	0x10, //    #   
	0x10, //    #   
	0x20, //   #    
	0x44, //  #   # 
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @708 '[' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x38, //   ###  
	0x00, //        

	// @720 '\' (7 pixels wide)
	0x00, //        
	0x40, //  #     
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x10, //    #   
	0x10, //    #   
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x00, //        
	0x00, //        

	// @732 ']' (7 pixels wide)
	0x00, //        
	0x38, //   ###  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x38, //   ###  
	0x00, //        

	// @744 '^' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x10, //    #   
	0x28, //   # #  
	0x44, //  #   # 
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        

	// @756 '_' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0xFE, // #######

//	// @768 '`' (7 pixels wide)
//	0x00, //
//	0x10, //    #
//	0x08, //     #
//	0x00, //
//	0x00, //
//	0x00, //
//	0x00, //
//	0x00, //
//	0x00, //
//	0x00, //
//	0x00, //
//	0x00, //

	// @768 '`' (7 pixels wide) remplacer par °
	0x00, //        
	0x10, //    ##
	0x28, //   #  #
	0x10, //    ##
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //

	// @780 'a' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x3C, //   #### 
	0x44, //  #   # 
	0x44, //  #   # 
	0x3E, //   #####
	0x00, //        
	0x00, //        
	0x00, //        

	// @792 'b' (7 pixels wide)
	0x00, //        
	0xC0, // ##     
	0x40, //  #     
	0x58, //  # ##  
	0x64, //  ##  # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0xF8, // #####  
	0x00, //        
	0x00, //        
	0x00, //        

	// @804 'c' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x3C, //   #### 
	0x44, //  #   # 
	0x40, //  #     
	0x40, //  #     
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @816 'd' (7 pixels wide)
	0x00, //        
	0x0C, //     ## 
	0x04, //      # 
	0x34, //   ## # 
	0x4C, //  #  ## 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x3E, //   #####
	0x00, //        
	0x00, //        
	0x00, //        

	// @828 'e' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x7C, //  ##### 
	0x40, //  #     
	0x40, //  #     
	0x3C, //   #### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @840 'f' (7 pixels wide)
	0x00, //        
	0x1C, //    ### 
	0x20, //   #    
	0x7C, //  ##### 
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @852 'g' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x36, //   ## ##
	0x4C, //  #  ## 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x3C, //   #### 
	0x04, //      # 
	0x38, //   ###  
	0x00, //        

	// @864 'h' (7 pixels wide)
	0x00, //        
	0xC0, // ##     
	0x40, //  #     
	0x58, //  # ##  
	0x64, //  ##  # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0xEE, // ### ###
	0x00, //        
	0x00, //        
	0x00, //        

	// @876 'i' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x00, //        
	0x70, //  ###   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @888 'j' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x00, //        
	0x78, //  ####  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x08, //     #  
	0x70, //  ###   
	0x00, //        

	// @900 'k' (7 pixels wide)
	0x00, //        
	0xC0, // ##     
	0x40, //  #     
	0x5C, //  # ### 
	0x48, //  #  #  
	0x70, //  ###   
	0x50, //  # #   
	0x48, //  #  #  
	0xDC, // ## ### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @912 'l' (7 pixels wide)
	0x00, //        
	0x30, //   ##   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @924 'm' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xE8, // ### #  
	0x54, //  # # # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x54, //  # # # 
	0xFE, // #######
	0x00, //        
	0x00, //        
	0x00, //        

	// @936 'n' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xD8, // ## ##  
	0x64, //  ##  # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0xEE, // ### ###
	0x00, //        
	0x00, //        
	0x00, //        

	// @948 'o' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x38, //   ###  
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x38, //   ###  
	0x00, //        
	0x00, //        
	0x00, //        

	// @960 'p' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xD8, // ## ##  
	0x64, //  ##  # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x78, //  ####  
	0x40, //  #     
	0xE0, // ###    
	0x00, //        

	// @972 'q' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x36, //   ## ##
	0x4C, //  #  ## 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x3C, //   #### 
	0x04, //      # 
	0x0E, //     ###
	0x00, //        

	// @984 'r' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x6C, //  ## ## 
	0x30, //   ##   
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @996 's' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x3C, //   #### 
	0x44, //  #   # 
	0x38, //   ###  
	0x04, //      # 
	0x44, //  #   # 
	0x78, //  ####  
	0x00, //        
	0x00, //        
	0x00, //        

	// @1008 't' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x20, //   #    
	0x7C, //  ##### 
	0x20, //   #    
	0x20, //   #    
	0x20, //   #    
	0x22, //   #   #
	0x1C, //    ### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @1020 'u' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xCC, // ##  ## 
	0x44, //  #   # 
	0x44, //  #   # 
	0x44, //  #   # 
	0x4C, //  #  ## 
	0x36, //   ## ##
	0x00, //        
	0x00, //        
	0x00, //        

	// @1032 'v' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x44, //  #   # 
	0x28, //   # #  
	0x28, //   # #  
	0x10, //    #   
	0x00, //        
	0x00, //        
	0x00, //        

	// @1044 'w' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x54, //  # # # 
	0x28, //   # #  
	0x00, //        
	0x00, //        
	0x00, //        

	// @1056 'x' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xCC, // ##  ## 
	0x48, //  #  #  
	0x30, //   ##   
	0x30, //   ##   
	0x48, //  #  #  
	0xCC, // ##  ## 
	0x00, //        
	0x00, //        
	0x00, //        

	// @1068 'y' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0xEE, // ### ###
	0x44, //  #   # 
	0x24, //   #  # 
	0x28, //   # #  
	0x18, //    ##  
	0x10, //    #   
	0x10, //    #   
	0x78, //  ####  
	0x00, //        

	// @1080 'z' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x7C, //  ##### 
	0x48, //  #  #  
	0x10, //    #   
	0x20, //   #    
	0x44, //  #   # 
	0x7C, //  ##### 
	0x00, //        
	0x00, //        
	0x00, //        

	// @1092 '{' (7 pixels wide)
	0x00, //        
	0x08, //     #  
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x20, //   #    
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x08, //     #  
	0x00, //        

	// @1104 '|' (7 pixels wide)
	0x00, //        
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x00, //        
	0x00, //        

	// @1116 '}' (7 pixels wide)
	0x00, //        
	0x20, //   #    
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x08, //     #  
	0x10, //    #   
	0x10, //    #   
	0x10, //    #   
	0x20, //   #    
	0x00, //        

	// @1128 '~' (7 pixels wide)
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x24, //   #  # 
	0x58, //  # ##  
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
	0x00, //        
};

sFONT Font12 = {
  Font12_Table,
  7, /* Width */
  12, /* Height */
};

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
