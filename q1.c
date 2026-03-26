#include <stdint.h>
#include "tm4c123gh6pm.h"
void UART0Tx(char const c);
void delayMs(int n);
int main(void)
{
SYSCTL->RCGCUART |= 1; /* provide clock to UART0 */
SYSCTL->RCGCGPIO |= 1; /* enable clock to PORTA */
/* UART0 initialization */
UART0->CTL = 0; /* disable UART0 */
UART0->IBRD = 104; /* 16MHz/(16*9600 baud rate) = 104.1666666666 */
UART0->FBRD = 11; /* fraction part = 0.1666666*64+0.5 = 11.1666666 */
UART0->CC = 0; /* use system clock */
UART0->LCRH = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
UART0->CTL = 0x301; /* enable UART0(bit0), TXE(bit8), RXE(bit9) */
/* UART0 TX0 and RX0 use PA1 and PA0. Set them up. */
GPIOA->DEN = 0x03; /* Make PA0 and PA1 as digital */
GPIOA->AFSEL = 0x03; /* Use PA0,PA1 alternate function */
GPIOA->PCTL = 0x11; /* configure PA0 and PA1 for UART */
//delayMs(25); /* wait for output line to stabilize */

/* UART1 TX0 and RX0 use PB1 and PB0. Set them up. */
GPIOB->DEN |= 0x03; /* Make PB0 and PB1 as digital */
GPIOB->AFSEL |= 0x03; /* Use PB0,PB1 alternate function */
GPIOB->PCTL &= ~0x000000FF;
GPIOB->PCTL |= 0x00000011; /* configure PB0 and PB1 for UART */

/* configure Port F pins 3,2,1 to control the LEDs */
GPIOF->DIR = 0x0E; /* configure Port F to control the LEDs */
GPIOF->DEN = 0x0E;
GPIOF->DATA = 0; /* turn LEDs off */

/* enable UART1 interrupt in NVIC and set priority to 3 */

// Since this is IP[6] so we need to enable the 6th bit in ISER
NVIC->IP[6] = 3 << 5; /* set interrupt no 6 priority to 3 */
NVIC->ISER[0] |= 0x00000040; /* enable IRQ6 for UART1 */

__enable_irq(); /* global enable IRQs */
	

while(1)
{
}
}
/* UART0 Transmit */
/* This function waits until the transmit buffer is available then writes */
/* the character in it. It does not wait for transmission to complete */
void UART0Tx(char const c)
{
	while((UART0->FR & 0x20)!= 0){} // Wait until Tx buffer is not full
	UART0->DR = c; // Write byte
}

void UART0_Handler(void)
{
volatile int readback;
char c;
	if (UART0->MIS & 0x0010) /* if a receive interrupt has occurred */
	{
		UART0Tx('Y');
		UART0Tx('e');
		UART0Tx('s');
		UART0Tx(' ');
		UART0->ICR = 0x0010; /* clear Rx interrupt flag */
		readback = UART0->ICR; /* a read to force clearing of interrupt flag */
	} 
}
