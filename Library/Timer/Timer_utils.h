/**
 * TimerInterrupt class
 * For ESP8266 boards
 * Inspired of ESP8266TimerInterrupt.h written by Khoi Hoang
 */

#pragma once

#include <Arduino.h>

#define MAX_ESP8266_NUM_TIMERS      1
#define MAX_ESP8266_COUNT           (8388607UL)

typedef void (*timer_interrupt_cb)();

#define TIM_DIV1_CLOCK		(80000000UL)	// 80000000 / 1   = 80.0  MHz (80 ticks/us - 104857.588 us max)
#define TIM_DIV16_CLOCK		(5000000UL)	  // 80000000 / 16  = 5.0   MHz (5 ticks/us - 1677721.4 us max)
#define TIM_DIV256_CLOCK	(312500UL)	  // 80000000 / 256 = 312.5 KHz (1 tick = 3.2us - 26843542.4 us max)

#if (defined(USING_TIM_DIV1) && USING_TIM_DIV1)
  #warning Using TIM_DIV1_CLOCK for shortest and most accurate timer
  #define TIM_CLOCK_FREQ        TIM_DIV1_CLOCK
  #define TIM_DIV               TIM_DIV1
#elif (defined(USING_TIM_DIV16) && USING_TIM_DIV16)
  #warning Using TIM_DIV16_CLOCK for medium time and medium accurate timer
  #define TIM_CLOCK_FREQ        TIM_DIV16_CLOCK
  #define TIM_DIV               TIM_DIV16
#elif (defined(USING_TIM_DIV256) && USING_TIM_DIV256)
  #warning Using TIM_DIV256_CLOCK for longest timer but least accurate
  #define TIM_CLOCK_FREQ        TIM_DIV256_CLOCK
  #define TIM_DIV               TIM_DIV256  
#else
#warning Default to using TIM_DIV256_CLOCK for longest timer but least accurate
#define TIM_CLOCK_FREQ        TIM_DIV256_CLOCK
#define TIM_DIV               TIM_DIV256
#endif  

#define MIN_FREQ	((uint32_t) (TIM_CLOCK_FREQ / MAX_ESP8266_COUNT))

/**
 * Class Timer using timer1 with interrupt
 * Call startTimer() to run the timer
 * ALWAYS stop the timer before any change !
 * Set interval in us and set frequency in hertz
 */
class TimerInterrupt
{
	private:
		timer_interrupt_cb _callback;		// pointer to the callback function
		uint32_t _timerCount;      			// count to activate timer
		uint8_t _loop;            			// timer loop

	public:

		TimerInterrupt()
		{
			if (this->NbTimer == 1)
				throw("Only one timer");

			_timerCount = 0;
			_callback = NULL;
			_loop = TIM_LOOP;
			this->NbTimer++;
		}

		~TimerInterrupt()
		{
			timer1_disable();
			timer1_detachInterrupt();
		}

		uint32_t IRAM_ATTR getFrequency(void)
		{
			if (_timerCount != 0)
			  return (uint32_t) (TIM_CLOCK_FREQ / _timerCount);
			else return 0;
		}

		// frequency (in hertz) NULL not tested !
		bool IRAM_ATTR setFrequency(const uint32_t frequency_Hz)
		{
			// ESP8266 only has one usable timer1, max count is only 8,388,607. So to get longer time, we use max available 256 divider
			// Will use later if very low frequency is needed.

#if (defined(USING_TIM_DIV1) && USING_TIM_DIV1)
			if (frequency_Hz < MIN_FREQ)
			{
				return false;
			}
#endif

			_timerCount = (uint32_t) (TIM_CLOCK_FREQ / frequency_Hz);

			if (_timerCount > MAX_ESP8266_COUNT)
			{
				_timerCount = MAX_ESP8266_COUNT;
				return false;
			}

			return true;
		}

		// frequency (in hertz) NULL not tested !
		bool IRAM_ATTR setFrequency(const uint32_t frequency_Hz, const timer_interrupt_cb &callback,
				bool loop = true)
		{
			if (setFrequency(frequency_Hz))
			{
				_callback = callback;
				_loop = (uint8_t) loop;
				timer1_disable();
				timer1_attachInterrupt(callback);
				return true;
			}
			else
				return false;
		}

		// interval (in microseconds) NULL not tested !
		bool IRAM_ATTR setInterval(const uint32_t interval_us)
		{
#if (defined(USING_TIM_DIV256) && USING_TIM_DIV256)
			return setFrequency(1000000UL / interval_us);
#else
			_timerCount = (uint32_t) ((TIM_CLOCK_FREQ / 1000000UL) * interval_us);

			if (_timerCount > MAX_ESP8266_COUNT)
			{
				_timerCount = MAX_ESP8266_COUNT;
				return false;
			}

			return true;
#endif
		}

		// interval (in microseconds) NULL not tested !
		bool IRAM_ATTR setInterval(const uint32_t interval_us, const timer_interrupt_cb &callback,
				bool loop = true)
		{
			return setFrequency(1000000UL / interval_us, callback, loop);
		}

		void IRAM_ATTR setCallback(const timer_interrupt_cb &callback)
		{
			_callback = callback;
			timer1_disable();
			timer1_attachInterrupt(callback);
		}

		// set timer loop
		void IRAM_ATTR setTimerLoop(bool loop)
		{
			_loop = (uint8_t) loop;
		}

		// start the timer
		void IRAM_ATTR startTimer(void)
		{
			timer1_write(_timerCount);
			timer1_enable(TIM_DIV, TIM_EDGE, _loop);
		}

		// stop the timer. Just stop clock source, clear the count
		void IRAM_ATTR stopTimer(void)
		{
			timer1_disable();
		}

		void IRAM_ATTR detachInterrupt()
		{
			timer1_detachInterrupt();
		}

		void IRAM_ATTR reattachInterrupt()
		{
			if (_callback != NULL)
				timer1_attachInterrupt(_callback);
		}

		static uint8_t NbTimer;
};

uint8_t TimerInterrupt::NbTimer = 0;
// class TimerInterrupt
