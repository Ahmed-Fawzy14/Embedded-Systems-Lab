/* Receive characters from phone using UART1 and send it to PC using UART0 */
#include "TM4C123GH6PM.h"
void UART0Tx(char c);

int main(void)
{
    SYSCTL->RCGCUART |= 0x03; /* provide clock to UART0 and UART1 */
    SYSCTL->RCGCGPIO |= 0x01; /* enable clock to PORTA */

    /* provide clock to UART1 */
    SYSCTL->RCGCGPIO |= 0x02; /* enable clock to PORTB */
    /* enable clock to PORTF */
    SYSCTL->RCGCGPIO |= 0x20;

    /* UART0 initialization */
    UART0->CTL = 0; /* disable UART0 */
    UART0->IBRD = 104; /* 16MHz/(16*9600 baud rate) = 104.1666666666 */
    UART0->FBRD = 11; /* fraction part= 0.1666666*64+0.5 = 11.1666666 */
    UART0->CC = 0; /* use system clock */
    UART0->LCRH = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
    UART0->CTL = 0x301; /* enable UART0, TXE, RXE */

    /* UART0 TX0 and RX0 use PA1 and PA0. Set them up. */
    GPIOA->DEN |= 0x03; /* Make PA0 and PA1 as digital */
    GPIOA->AFSEL |= 0x03; /* Use PA0,PA1 alternate function */
    GPIOA->PCTL &= ~0x000000FF;
    GPIOA->PCTL |= 0x00000011; /* configure PA0 and PA1 for UART */

    /* UART1 initialization, enabling RX interrupt */
    UART1->CTL = 0; /* disable UART1 */
    UART1->IBRD = 104; /* 16MHz/(16*9600 baud rate) = 104.1666666666 */
    UART1->FBRD = 11; /* fraction part= 0.1666666*64+0.5 = 11.1666666 */
    UART1->CC = 0; /* use system clock */
    UART1->LCRH = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
    UART1->IM |= 0x0010; /* enable RX interrupt */
    UART1->CTL = 0x301; /* enable UART1, TXE, RXE */

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

    while (1) {}
}

void UART1_Handler(void)
{
    volatile int readback;
    char c;

    if (UART1->MIS & 0x0010) /* if a receive interrupt has occurred */
    {
        c = UART1->DR; /* read the received data */

        switch(c) {
            case 'R':
            case 'r':
                GPIOF->DATA = 0x02;
                break;

            case 'G':
            case 'g':
                GPIOF->DATA = 0x08;
                break;

            case 'B':
            case 'b':
                GPIOF->DATA = 0x04;
                break;

            default:
                GPIOF->DATA = 0x0;
                break;
        }

        UART0Tx(c); /* send received character to UART0 */

        UART1->ICR = 0x0010; /* clear Rx interrupt flag */
        readback = UART1->ICR; /* a read to force clearing of interrupt flag */
    }
    else
    {
        /* should not get here. But if it does, */
        UART1->ICR = UART1->MIS; /* clear all interrupt flags */
        readback = UART1->ICR; /* a read to force clearing of interrupt flag */
    }
}

void UART0Tx(char c)
{
    /* send a character to UART0 */
    while((UART0->FR & 0x20) != 0) {} // Wait until Tx buffer is not full
    UART0->DR = c; // Write byte
}