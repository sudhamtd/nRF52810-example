/** @file - referred github.com/NordicPlayground/nrf51-TIMER-examples/blob/master/timer_example_timer_mode/main.c
 * to ease up design process and to concentrate on core logic only
 * @brief Simple timer example project with interrupt. Timer is in timer mode.
 * 
 * In this example TIMER0 interrupt is enabled. The TIMER interrupt handler will toggle an LED whenever
 * the TIMER counter is equal to a value of compare register CC[0]. The timer is set to 32 bit. While
 * waiting for TIMER interrupt the CPU is put to sleep in the main loop with the __WFE() __SEV() __WFE()
 * command series.
 *
 * When the timer is in timer mode, it will periodically increment itself. The increment frequency is adjusted
 * by configuring the prescale value.
 */

#include <stdio.h>
#include <stdbool.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "boards.h"

#define GPIO_TOGGLE_PIN              (LED_0)
typedef void (*interrupt_handler)(void);

/**
 * start - configures corresponding settings in TIMER0. Timer is in timer mode.
 * Takes two arguments and returns zero parameters.
 *
 * period - argument representing number of micro-seconds per period.
 * handler - represents or points to a code (.text - rx) section.
 * Make a function call to 'handler' for every 'period' micro-seconds
 *
 */
void start(unsigned int period, interrupt_handler handler) {
	// Goal is to set timer 0 in timer mode & register interrupt related stuff

	// Some basic configuraions to avoid 'unpredictability'
	NRF_TIMER0->TASKS_STOP = 1;
	NRF_TIMER0->TASKS_CLEAR = 1;
	NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;

	// To ease up calculations, making f(TIMER) = 1MHz
	// so to maintain this tick, PRESCALER had to be = 4
	// Though PRESECALE's reset is 4, to avoid junk or previous writings it's re-written
	// Also f(TIMER) being 1MHz will save some power
	// as hardware will choose PCLK1M over PCLK16M
	NRF_TIMER0->PRESCALER = 4;

	// Timer will here run in 32 bit mode, to have a larger window for period
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit;

	// whenever the timer count matches capture-compare[0], EVENTS[0] is raised
	// as every tick is  1 micro-second long, period is copied into CC[0]
	// so that, after period micro-seconds a match occurs
	NRF_TIMER0->CC[0] = period;
	
	// Enable interrupt on CC[0] match & assign handler for IRQ handling
	NRF_TIMER0->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
	NVIC_EnableIRQ(handler);

	// Start timer
	NRF_TIMER0->TASKS_START = 1;
}

/**
 * stop - configures corresponding settings in TIMER0 such that,
 * it's timer timer is stopped and cleared
 *
 */
void stop(void) {
	// Goal is to clear timer configurations and interrupt registrations

	// basic clear stuff
	NRF_TIMER0->TASKS_STOP = 1;
	NRF_TIMER0->TASKS_CLEAR = 1;

	// interrupt de-registrations
	NRF_TIMER0->CC[0] = 0;
	NRF_TIMER0->INTENSET ~= (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
}

/** TIMTER0 peripheral interrupt handler. This interrupt handler is called whenever there it a TIMER0 interrupt
 */
void TIMER0_IRQHandler(void) {
	static unsigned int count;
	if ((NRF_TIMER0->EVENTS_COMPARE[0]) && (NRF_TIMER0->INTENSET & TIMER_INTENSET_COMPARE0_Msk)) {
		NRF_TIMER0->EVENTS_COMPARE[0] = 0;           //Clear compare register 0 event
		printf("%d\n", count++);
	}
}

int main(void) {
	nrf_gpio_cfg_output(GPIO_TOGGLE_PIN);		//Set LED pin as output	
	start(509, TIMER0_IRQHandler);				//Configure and start timer
	
	while(true) {
		// Enter System ON sleep mode
		__WFE();
		// Make sure any pending events are cleared
		__SEV();
		__WFE();
		
		// For more information on the WFE - SEV - WFE sequence, please refer to the following Devzone article:
		// https://devzone.nordicsemi.com/index.php/how-do-you-put-the-nrf51822-chip-to-sleep#reply-1589
	}
}
