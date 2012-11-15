/*
 * main.c
 *
 *  Created on: 14/12/2011
 *      Author: Jorge Querol
 */

#include "../LPC17xx.h"
#include "../adc/adc.h"
#include "../bang/bang.h"
#include "../control/control.h"
//#include "../easyweb.h"

extern void SystemInit(void);

extern void comm_init(void);
extern void handle_command();

extern void welcomeMsg(void);
extern void set_echo(void);
extern uint8_t printchar(const char *charBuf);
extern int easyWEB_init(void);
extern void easyWEB_s_loop(void);

extern int UpdateChannel;
extern int Vout, Vin, Il,mode;
int Iref;

int main(void)
{
	/* Start led connected to P1.29 and P1.18 */
	LPC_GPIO1->FIODIR |= 0x20040000;
	LPC_GPIO1->FIOSET |= (1 << 29);
	LPC_GPIO1->FIOCLR |= (1 << 18);

	SystemInit(); // lpc1768_startup.c
	SystemCoreClockUpdate();
	GPIOInit();
	TimerInit();
	ValueInit();
	ADCInit();

	comm_init();
//	welcomeMsg();
	set_echo();
	easyWEB_init();
	printchar("at+ndhcp=1\r");
	printchar("at+wa=greenet\r");
	printchar("at+nstcp=20\r");
	printchar("                              \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

	while(1)
	{
		easyWEB_s_loop();
		//handle_command();	//UART command
		if(UpdateChannel >= 3)
		{
			UpdateChannel = -1;
			ADCRead(0);
			if (mode ==1)  //decide which ADC channel to read when the converter is working in different direction
			{
				Vout = ADCValues(0);
				Vin = ADCValues(1);
			}
			else
			{
				Vout = ADCValues(1);
				Vin = ADCValues(0);
			}
			Iref = ADCValues(2);
			Il = abs (ADCValues(3) - Iref); //get the correct inductor current
			MeanValues();
			BangBang();
			LPC_GPIO1->FIOPIN ^= (1 << 29);
			LPC_GPIO1->FIOPIN ^= (1 << 18);
		}

	}

	return 0;
}
