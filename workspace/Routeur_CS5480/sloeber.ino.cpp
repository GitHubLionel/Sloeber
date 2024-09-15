#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2024-09-15 11:55:44

#include "Arduino.h"
#include "Arduino.h"
#include "Server_utils.h"
#include "RTCLocal.h"
#include "Partition_utils.h"
#include "display.h"
#include "CIRRUS.h"
#include "Get_Data.h"
#include "DS18B20.h"
#include "TeleInfo.h"
#include "SSR.h"
#include "Keyboard.h"
#include "Relay.h"
#include "Debug_utils.h"
#define USE_DS
#define USE_ZC_SSR
#define USE_RELAY
#define USE_KEYBOARD
#include "Tasks_utils.h"

void Display_Task_code(void *parameter) ;
void UserKeyboardAction(Btn_Action Btn_Clicked) ;
void setup() ;
void loop() ;
void OnAfterConnexion(void) ;
void PrintTerminal(const char *text) ;
bool UserAnalyseMessage(void) ;
void handleInitialization(CB_SERVER_PARAM) ;
void handleLastData(CB_SERVER_PARAM) ;
void handleOperation(CB_SERVER_PARAM) ;
String Handle_Wifi_Request(CS_Common_Request Wifi_Request, char *Request) ;

#include "Routeur_CS5480.ino"


#endif
