#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2024-08-25 11:51:22

#include "Arduino.h"
#include "Arduino.h"
#include "Server_utils.h"
#include "RTCLocal.h"
#include "Partition_utils.h"
#include "display.h"
#include "CIRRUS.h"
#include "Get_Data.h"
#include "DS18B20.h"
#include "SSR.h"
#include "Keyboard.h"
#include "Relay.h"
#include "Debug_utils.h"
#define USE_DS
#define USE_ZC_SSR
#define USE_RELAY
#define USE_KEYBOARD

void setup() ;
void loop() ;
void OnAfterConnexion(void) ;
void PrintTerminal(const char *text) ;
bool UserAnalyseMessage(void) ;
void handleInitialization(void) ;
void handleLastData(void) ;
void handleOperation(void) ;
void handleCirrus(void) ;

#include "Routeur_CS5490.ino"


#endif
