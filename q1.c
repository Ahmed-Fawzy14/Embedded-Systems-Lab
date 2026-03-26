
#include "tm4c123gh6pm.h"
#include<stdio.h>
void delayMs(int n);

volatile char yes[] = "yes ";
volatile int i = 0;
int main(void)
{
SYSCTL->RCGCUART |= 1; /* provide clock to UART0 */
SYSCTL->RCGCGPIO |= 1; /* enable clock to PORTA */
SYSCTL->RCGCGPIO |= 0x20; /* enable clock to PORTF */
/* UART0 initialization */
UART0->CTL = 0; /* disable UART0 */
UART0->IBRD = 104; /* 16MHz/(16*9600 baud rate) = 104.1666666666 */
UART0->FBRD = 11; /* fraction part= 0.1666666*64+0.5 = 11.1666666 */
UART0->CC = 0; /* use system clock */
UART0->LCRH = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
UART0->IM |= 0x20; /* enable TX interrupt */
UART0->CTL = 0x301; /* enable UART0, TXE, RXE */
/* UART0 TX0 and RX0 use PA1 and PA0. Set them up. */
GPIOA->DEN = 0x03; /* Make PA0 and PA1 as digital */
GPIOA->AFSEL = 0x03; /* Use PA0,PA1 alternate function */
GPIOA->PCTL = 0x11; /* configure PA0 and PA1 for UART */
GPIOF->DIR = 0x0E; /* configure Port F to control the LEDs */
GPIOF->DEN = 0x0E;
GPIOF->DATA = 0; /* turn LEDs off */
/* enable interrupt in NVIC and set priority to 3 */
NVIC->IP[5] = 3 << 5; /* set interrupt no 5 priority to 3 */
NVIC->ISER[0] |= 0x00000020; /* enable IRQ5 for UART0 */
__enable_irq(); /* global enable IRQs */

delayMs(100); /* wait for output line to stabilize */


UART0->DR = yes[i++]; // Write first one

if(yes[i] == '\0')
	i = 0;
	


for(;;){}
}


void UART0_Handler(void)
{
if (UART0->MIS & 0x20) /* if a receive interrupt has occurred */
{

	UART0->DR = yes[i++]; // Write char
	
	if(yes[i] == '\0')
		i = 0;

	UART0->ICR = 0x20; /* clear Rx interrupt flag */
} 
}

void delayMs(int n)
{
int i, j;
for(i = 0 ; i < n; i++)
for(j = 0; j < 3180; j++)
{} // do nothing for 1 ms
}
