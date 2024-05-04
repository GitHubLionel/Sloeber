
#include "Simple_Get_Data.h"
#ifdef USE_SSR
#include "SSR.h"
#endif
#include "display.h"

extern uint8_t CIRRUS_Number;

// Data 200 ms
volatile Simple_Data_Struct Simple_Current_Data; //{0,{0}};

// Tension et puissance du premier Cirrus, premier channel
volatile float Cirrus_voltage = 230.0;
volatile float Cirrus_power_signed = 0.0;

// Les données actualisées pour le SSR
#ifdef USE_SSR
extern Gestion_SSR_TypeDef Gestion_SSR_CallBack;
#endif

volatile bool Simple_Data_acquisition = false;

// UART message
#ifdef LOG_CIRRUS_CONNECT
extern volatile bool CIRRUS_Command;
extern volatile bool CIRRUS_Lock_IHM;
#endif

// ********************************************************************************
// Exemple d'acquisition de données
// ********************************************************************************

/**
 * Exemple de fonction d'acquisition des données du Cirrus
 * En pratique, il convient d'appeler régulièrement cette fonction dans un timer (200 ms ou 1 s)
 */
void Simple_Get_Data(void)
{
  volatile RMS_Data *pData;

#ifdef LOG_CIRRUS_CONNECT
  if (Simple_Data_acquisition || CIRRUS_Command || CIRRUS_Lock_IHM)
#else
  if (Simple_Data_acquisition)
#endif
  {
    return ; // On est déjà dans la procédure ou il y a un message pour le Cirrus
  }

  Simple_Data_acquisition = true;

#ifdef CIRRUS_CS5480
  // Si on a deux Cirrus CS5480, on se positionne sur le premier
  if (CIRRUS_Number == 2)
    CIRRUS_Select(Cirrus_1);
#endif

  // Cirrus 1 est ready, on lit les données
  // CIRRUS_wait_for_ready(true)
  if (CIRRUS_get_rms_data(&Cirrus_voltage, &Cirrus_power_signed))
  {
    // Tension et puissance du premier Cirrus, premier channel
//    CIRRUS_get_data(CIRRUS_RMS_Voltage1, &Cirrus_voltage);
//    CIRRUS_get_data(CIRRUS_Active_Power1, &Cirrus_power_signed);

    // Cirrus channel 1
    pData = &Simple_Current_Data.Cirrus1_ch1;
    pData->Voltage = Cirrus_voltage;
    CIRRUS_get_data(CIRRUS_RMS_Current1, &pData->Current);
    pData->Power = Cirrus_power_signed;

    // Cirrus channel 2
#ifdef CIRRUS_CS5480
    pData = &Simple_Current_Data.Cirrus1_ch2;
    CIRRUS_get_data(CIRRUS_RMS_Voltage2, &pData->Voltage);
    CIRRUS_get_data(CIRRUS_RMS_Current2, &pData->Current);
    CIRRUS_get_data(CIRRUS_Active_Power2, &pData->Power);
#endif

    // Extra
    CIRRUS_get_data(CIRRUS_Power_Factor1, &Simple_Current_Data.Cirrus1_PF); // PF sur channel 1
    CIRRUS_get_data(CIRRUS_Frequency, &Simple_Current_Data.Cirrus1_Freq);

    // Température Cirrus
    Simple_Current_Data.Cirrus1_Temp = CIRRUS_get_temperature();
  }
  else
    {
      PrintTerminal("simple Get Data Ready off\r\n");  // Pour debug
      PrintTerminal(CIRRUS_Print_LastError());
      Simple_Data_acquisition = false;
      return ;
    }

#ifdef CIRRUS_CS5480
  // Si on a deux Cirrus CS5480, on se positionne sur le deuxième
  if (CIRRUS_Number == 2)
  {
    CIRRUS_Select(Cirrus_2);

    // Cirrus 2 est ready, on lit les données
    if (CIRRUS_wait_for_ready(true))
    {
      // Cirrus channel 1
      pData = &Simple_Current_Data.Cirrus2_ch1;
      CIRRUS_get_data(CIRRUS_RMS_Voltage1, &pData->Voltage);
      CIRRUS_get_data(CIRRUS_RMS_Current1, &pData->Current);
      CIRRUS_get_data(CIRRUS_Active_Power1, &pData->Power);

      // Cirrus channel 2
      pData = &Simple_Current_Data.Cirrus2_ch2;
      CIRRUS_get_data(CIRRUS_RMS_Voltage2, &pData->Voltage);
      CIRRUS_get_data(CIRRUS_RMS_Current2, &pData->Current);
      CIRRUS_get_data(CIRRUS_Active_Power2, &pData->Power);

      // Extra
      CIRRUS_get_data(CIRRUS_Power_Factor1, &Simple_Current_Data.Cirrus2_PF); // PF sur channel 1
      CIRRUS_get_data(CIRRUS_Frequency, &Simple_Current_Data.Cirrus2_Freq);

      // Température Cirrus
      Simple_Current_Data.Cirrus2_Temp = CIRRUS_get_temperature();
    }
  }
#endif

#ifdef USE_SSR
  if (Gestion_SSR_CallBack != NULL)
    Gestion_SSR_CallBack();
#endif

  Simple_Data_acquisition = false;
}

// ********************************************************************************
// Affichage des données
// ********************************************************************************

/**
 * Fonction d'affichage simple des données de Simple_Get_Data
 */
void Simple_Update_IHM(const char *first_text, const char *last_text)
{
  char buffer[30];  // Normalement 20 caractères + eol mais on sait jamais
  uint8_t line = 0;

  // Efface la mémoire de l'écran si nécessaire
  IHM_Clear();

#ifdef LOG_CIRRUS_CONNECT
  if (CIRRUS_Lock_IHM)
  {
    if (CIRRUS_Selected == Cirrus_1)
    	IHM_Print(0, "Cirrus 1");
    else
      if (CIRRUS_Selected == Cirrus_2)
      	IHM_Print(0, "Cirrus 2");
      else IHM_Print(0, "No Cirrus");

    // Actualise l'écran si nécessaire
    IHM_Display();

    return;
  }
#endif

  if (strlen(first_text) > 0)
  {
  	IHM_Print(line++, first_text);
  }

  sprintf(buffer,"U1_1: %.2f", Simple_Current_Data.Cirrus1_ch1.Voltage);
  IHM_Print(line++, (char*)buffer);

  if (CIRRUS_Number == 2)
    sprintf(buffer,"I1:%.2f, I2:%.2f  ", Simple_Current_Data.Cirrus1_ch1.Current, Simple_Current_Data.Cirrus2_ch1.Current);
  else
    sprintf(buffer,"I1:%.2f  ", Simple_Current_Data.Cirrus1_ch1.Current);
  IHM_Print(line++, (char*)buffer);

  sprintf(buffer,"P1_1:%.2f   ", Simple_Current_Data.Cirrus1_ch1.Power);
  IHM_Print(line++, (char*)buffer);

#ifdef CIRRUS_CS5480
  sprintf(buffer,"P1_2:%.2f   ", Simple_Current_Data.Cirrus1_ch2.Power);
  IHM_Print(line++, (char*)buffer);

  if (CIRRUS_Number == 2)
  {
    sprintf(buffer,"P2_1:%.2f   ", Simple_Current_Data.Cirrus2_ch1.Power);
    IHM_Print(line++, (char*)buffer);

    sprintf(buffer,"P2_2:%.2f   ", Simple_Current_Data.Cirrus2_ch2.Power);
    IHM_Print(line++, (char*)buffer);
  }
#endif

  sprintf(buffer,"T1:%.2f", Simple_Current_Data.Cirrus1_Temp);
  IHM_Print(line++, (char*)buffer);

  if (CIRRUS_Number == 2)
  {
    sprintf(buffer,"T2:%.2f", Simple_Current_Data.Cirrus2_Temp);
    IHM_Print(line++, (char*)buffer);
  }

  if (strlen(last_text) > 0)
  {
  	IHM_Print(line++, last_text);
  }

  // Actualise l'écran si nécessaire
  IHM_Display();
}

// ********************************************************************************
// End of file
// ********************************************************************************
