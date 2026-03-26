#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"

#define SYSCLK        16000000UL
#define CMD_BUF_SIZE  32

volatile char cmdBuffer[CMD_BUF_SIZE];
volatile uint32_t cmdIndex = 0;
volatile uint8_t cmdReady = 0;

volatile float frequencyHz = 1.0f;   /* initial frequency = 1.0 Hz */
volatile int dutyPercent = 50;       /* initial duty = 50% */
volatile uint8_t waveHigh = 0;       /* 0 = low phase, 1 = high phase */

void GPIO_PD0_Init(void);
void UART0_Init(void);
void UART5_Init(void);
void UART0_TxChar(char c);
void UART0_TxString(const char *s);
void SysTick_Init(void);
void SysTick_LoadNext(void);
void copyCommand(char *dst);
void applyCommand(const char *cmd);
void updateWave(float newFreq, int newDuty);
void printStatus(void);

int main(void)
{
    char localCmd[CMD_BUF_SIZE];

    SYSCTL->RCGCGPIO |= 0x08; /* enable clock to PORTD */
    while((SYSCTL->PRGPIO & 0x08) == 0) {} /* wait until PORTD is ready */

    /* configure PD0 to output the square wave */
    GPIOD->DIR |= 0x01; /* make PD0 output */
    GPIOD->DEN |= 0x01; /* make PD0 digital */
    GPIOD->AFSEL &= ~0x01; /* disable alternate function on PD0 */
    GPIOD->AMSEL &= ~0x01; /* disable analog function on PD0 */
    GPIOD->DATA &= ~0x01; /* start with PD0 low */

    /* UART0 initialization */
    SYSCTL->RCGCUART |= 0x01; /* provide clock to UART0 */
    SYSCTL->RCGCGPIO |= 0x01; /* enable clock to PORTA */
    while((SYSCTL->PRGPIO & 0x01) == 0) {} /* wait until PORTA is ready */

    UART0->CTL = 0; /* disable UART0 */
    UART0->IBRD = 104; /* 16MHz/(16*9600 baud rate) = 104.1666666666 */
    UART0->FBRD = 11; /* fraction part= 0.1666666*64+0.5 = 11.1666666 */
    UART0->CC = 0; /* use system clock */
    UART0->LCRH = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
    UART0->CTL = 0x301; /* enable UART0, TXE, RXE */

    /* UART0 TX0 and RX0 use PA1 and PA0. Set them up. */
    GPIOA->AFSEL |= 0x03; /* use PA0,PA1 alternate function */
    GPIOA->DEN   |= 0x03; /* make PA0 and PA1 as digital */
    GPIOA->AMSEL &= ~0x03; /* disable analog function on PA0 and PA1 */
    GPIOA->PCTL  &= ~0x000000FF;
    GPIOA->PCTL  |=  0x00000011; /* configure PA0 and PA1 for UART */

    /* UART5 initialization, enabling RX interrupt */
    SYSCTL->RCGCUART |= (1 << 5); /* provide clock to UART5 */
    SYSCTL->RCGCGPIO |= (1 << 4); /* enable clock to PORTE */
    while((SYSCTL->PRGPIO & (1 << 4)) == 0) {} /* wait until PORTE is ready */

    UART5->CTL = 0; /* disable UART5 */
    UART5->IBRD = 104; /* 16MHz/(16*9600 baud rate) = 104.1666666666 */
    UART5->FBRD = 11; /* fraction part= 0.1666666*64+0.5 = 11.1666666 */
    UART5->CC = 0; /* use system clock */
    UART5->LCRH = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
    UART5->ICR = 0x7FF; /* clear all UART5 interrupt flags */
    UART5->IM  |= 0x10; /* enable RX interrupt */
    UART5->CTL = 0x301; /* enable UART5, TXE, RXE */

    /* UART5 TX and RX use PE5 and PE4. Set them up. */
    GPIOE->AFSEL |= 0x30; /* use PE4,PE5 alternate function */
    GPIOE->DEN   |= 0x30; /* make PE4 and PE5 as digital */
    GPIOE->AMSEL &= ~0x30; /* disable analog function on PE4 and PE5 */
    GPIOE->PCTL  &= ~0x00FF0000;
    GPIOE->PCTL  |=  0x00110000; /* configure PE4 and PE5 for UART */

    /* enable UART5 interrupt in NVIC and set priority to 3 */
    NVIC->IP[61] = 3 << 5; /* set interrupt no 61 priority to 3 */
    NVIC->ISER[1] |= (1 << (61 - 32)); /* enable IRQ61 for UART5 */

    /* SysTick initialization */
    SysTick->CTRL = 0; /* disable SysTick during setup */
    SysTick_LoadNext();
    SysTick->VAL = 0; /* clear current SysTick value */
    SysTick->CTRL = 0x07; /* enable SysTick, interrupt, system clock */

    UART0_TxString("Q2 started\r\n");
    printStatus();

    __enable_irq(); /* global enable IRQs */

    while (1)
    {
        if (cmdReady)
        {
            copyCommand(localCmd);

            UART0_TxString("Received: ");
            UART0_TxString(localCmd);
            UART0_TxString("\r\n");

            applyCommand(localCmd);
        }
    }
}


void UART0_TxChar(char c)
{
    while((UART0->FR & 0x20) != 0) {}
    UART0->DR = c;
}

void UART0_TxString(const char *s)
{
    while(*s)
    {
        UART0_TxChar(*s++);
    }
}


void SysTick_LoadNext(void)
{
    float periodSec, onSec, offSec, phaseSec;
    uint32_t ticks;

    periodSec = 1.0f / frequencyHz;
    onSec  = periodSec * ((float)dutyPercent / 100.0f);
    offSec = periodSec - onSec;

    if (waveHigh)
        phaseSec = onSec;
    else
        phaseSec = offSec;

    ticks = (uint32_t)(phaseSec * (float)SYSCLK);

    if (ticks < 1)
        ticks = 1;

    if (ticks > 16777215)
        ticks = 16777215;                 

    SysTick->LOAD = ticks - 1;
    SysTick->VAL = 0;
}

void copyCommand(char *dst)
{
    uint32_t i;

    __disable_irq();
    for (i = 0; i < CMD_BUF_SIZE; i++)
    {
        dst[i] = cmdBuffer[i];
        if (cmdBuffer[i] == '\0')
            break;
    }
    cmdReady = 0;
    cmdIndex = 0;
    __enable_irq();
}

void applyCommand(const char *cmd)
{
    float newFreq = frequencyHz;
    int newDuty = dutyPercent;

    if      (strcmp(cmd, "freq+1") == 0)  newFreq += 0.1f;
    else if (strcmp(cmd, "freq-1") == 0)  newFreq -= 0.1f;
    else if (strcmp(cmd, "freq+2") == 0)  newFreq += 0.2f;
    else if (strcmp(cmd, "freq-2") == 0)  newFreq -= 0.2f;
    else if (strcmp(cmd, "duty+5") == 0)  newDuty += 5;
    else if (strcmp(cmd, "duty-5") == 0)  newDuty -= 5;
    else if (strcmp(cmd, "duty+10") == 0) newDuty += 10;
    else if (strcmp(cmd, "duty-10") == 0) newDuty -= 10;
    else
    {
        UART0_TxString("Invalid command\r\n");
        return;
    }

    if (newFreq < 0.1f) newFreq = 0.1f;
    if (newDuty < 5)    newDuty = 5;
    if (newDuty > 95)   newDuty = 95;

    updateWave(newFreq, newDuty);
    printStatus();
}

void updateWave(float newFreq, int newDuty)
{
    __disable_irq();
    frequencyHz = newFreq;
    dutyPercent = newDuty;
    SysTick_LoadNext();
    __enable_irq();
}

void printStatus(void)
{
    char msg[64];
    sprintf(msg, "Updated: freq = %.1f Hz, duty = %d%%\r\n", frequencyHz, dutyPercent);
    UART0_TxString(msg);
}

void UART5_Handler(void)
{
    char c;

    if (UART5->MIS & 0x10)
    {
        c = (char)(UART5->DR & 0xFF);

        // Remove ? chars
        if (c >= 32 && c <= 126)
        {
            UART0_TxChar(c);
        }
        else
        {
            UART0_TxString("\r\n");
        }

        if (!cmdReady)
        {
            
            if (c >= 32 && c <= 126)
            {
                if (cmdIndex < CMD_BUF_SIZE - 1)
                {
                    cmdBuffer[cmdIndex++] = c;
                }
            }
            
            else if (cmdIndex > 0)
            {
                cmdBuffer[cmdIndex] = '\0';
                cmdReady = 1;
            }
        }

        UART5->ICR = 0x10;   // clear RX interrupt 
    }
}

void SysTick_Handler(void)
{
    if (waveHigh)
    {
        GPIOD->DATA &= ~0x01;              
        waveHigh = 0;
    }
    else
    {
        GPIOD->DATA |= 0x01;             
        waveHigh = 1;
    }

    SysTick_LoadNext();
}
