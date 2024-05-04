#include "JSN-SR04T.h"

// ********************************************************************************
// Définition JSN
// ********************************************************************************
// JSN Trig (RX) RX ===> ESP01 TX (GPIO 1)
// JSN Echo (output) TX ===> ESP01 RX (GPIO 3)

// Utilisation de JSN en mode UART
// On peut tester en envoyant les 3 octets suivants : 0xFF, 0x07, 0xA1 (+ checksum)
// Le résultat est 1953 mm

// UART Mode
#define UART_BAUD	9600	// Vitesse pour JSN
#define JSN_STX  	0xFF	// Start of frame
#define JSN_QUERY  	0x55	// Request a measure (mode 3)

// Trigger Echo Mode
#define PULSE	10 // The pulse duration in us (normally 10 us)

// ********************************************************************************
// Functions prototype
// ********************************************************************************
#ifndef TX
#define TX	1
#endif
#ifndef RX
#define RX	3
#endif

/**
 * Constructor JSN
 * Set use_uart to true when you are in UART mode else mode is Trigger/Echo
 * If use UART, be sure that you don't use Serial in other place.
 */
JSN_SR04T::JSN_SR04T(bool use_uart)
{
	JSN_USE_UART = use_uart;
	trigPin = TX;
	echoPin = RX;
}

/**
 * Change the method. Must be call before Initialize.
 */
void JSN_SR04T::Mode(bool use_uart)
{
	JSN_USE_UART = use_uart;
}

/**
 * Initialize the JSN
 * in Trigger/Echo mode, set JSN trigger Pin RX (default 1 = TX) and echo pin TX (default 3 = RX)
 * or leave empty if use default.
 * in UART mode, leave empty
 */
void JSN_SR04T::Initialize(uint8_t trig_Pin, uint8_t echo_Pin)
{
	if (JSN_USE_UART)
	{
		// Start uart for JSN
		Serial.begin(UART_BAUD);
		delay(100);  // Pour stabiliser UART
	}
	else
	{
		trigPin = trig_Pin;
		echoPin = echo_Pin;
		//GPIO 1 (TX) swap the pin to a GPIO.
		pinMode(trigPin, FUNCTION_3);
		//GPIO 3 (RX) swap the pin to a GPIO.
		pinMode(echoPin, FUNCTION_3);
		pinMode(trigPin, OUTPUT);
		pinMode(echoPin, INPUT_PULLUP);

		digitalWrite(trigPin, LOW);
	}
}

// ********************************************************************************
// JSN treatement
// ********************************************************************************

/**
 * Listen serial port to get distance
 */
void JSN_SR04T::CheckJSNMessage(void)
{
	static bool frameStart = false;
	uint8_t data;

	while (Serial.available())
	{
		data = Serial.read();

		if (data == JSN_STX)
		{
			frameStart = true;
			JSN_Count = 0;
			JSN_Data[JSN_Count] = data;
		}
		else
		{
			if (frameStart)
			{
				JSN_Count++;
				JSN_Data[JSN_Count] = data;
				if (JSN_Count == 3)
				{
					computeDist();
					frameStart = false;
				}
			}
		}
	}
}

/**
 * Compute the distance : UART method
 * The structure of the frame is :
 * Data[0] 0xFF: for a frame to start the data, used to judge
 * Data[1] H_DATA: the upper 8 bits of the distance data
 * Data[2] L_DATA: the lower 8 bits of the distance data
 * Data[3] SUM: data and, for the effect of its 0xFF + H_DATA + L_DATA = SUM (only low 8)
 * Exemple : 0xFF 0x07 0xA1 0xA7 ==> 1953 mm
 */
void JSN_SR04T::computeDist(void)
{
	uint8_t csum = 0;
	uint16_t distance = 0;
	float dist = 0.0;

	csum = JSN_Data[0] + JSN_Data[1] + JSN_Data[2];
	if ((csum & JSN_STX) == JSN_Data[3])
	{
		distance = (JSN_Data[1] << 8) + JSN_Data[2];
		dist = (distance / 10.0);
		JSN_Cumul += dist;
		JSN_Cumul_Count++;
		JSN_Distance_cm = dist;
	}
}

/**
 * Compute the distance : Trigger method
 */
void JSN_SR04T::triggerJSN(void)
{
	float dist = 0.0;
	uint32_t duration = 0;

	// Trigger the sensor by setting the trigPin high for 10 microseconds:
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(PULSE); // normalement 10 us
	digitalWrite(trigPin, LOW);

	// Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
	// Echo is in 150us - 25 ms and 38 ms if no obstacle
	duration = pulseIn(echoPin, HIGH, 38000);

	if (duration > 26000)	// La mesure a échoué
		duration = 0;

	// Calculate the distance in cm :
	// D = duration * 1e-6 * 343/2 m. The speed of sound : 343 m/s at 20°C
	// In cm : D = duration * 1e-6 * 343/2 * 1e+2 = duration * 0.01715 ~ duration / 58
	// Variation de la vitesse en fonction de la température (en °C) : v = 331.5 + 0.607 * T
	if (duration != 0)
	{
		dist = duration * 0.01724138;
		JSN_Cumul += dist;
		JSN_Cumul_Count++;
		JSN_Distance_cm = dist;
	}

	// Set all pin to low
	digitalWrite(trigPin, LOW);
	digitalWrite(echoPin, LOW);
}

/**
 * Interrogation de JSN controler par un timer ou dans la loop principale
 * UART method : send JSN_QUERY then listen serial port with CheckJSNMessage() function
 * Trigger method : send pulse and listen echo (max delay to perform operation : 38 ms)
 */
void JSN_SR04T::askJSN(void)
{
	if (JSN_USE_UART)
	{
		Serial.printf("%c", JSN_QUERY);
		delay(1);
		CheckJSNMessage();
	}
	else
	{
		yield();  // Background operation before
		triggerJSN();
		yield();  // Background operation after
	}
}

float JSN_SR04T::get_DistanceMean(void)
{
	JSN_Cumul_Error = (JSN_Cumul_Count == 0);
	if (!JSN_Cumul_Error)
	{
		JSN_Distance_mean = JSN_Cumul / JSN_Cumul_Count;
		JSN_Cumul = 0.0;
		JSN_Cumul_Count = 0;
	}
	return JSN_Distance_mean;
}

// ********************************************************************************
// End of file
// ********************************************************************************
