#include "TM4C123GH6PM.h"

#define RED 0x02
#define BLUE 0x04
#define GREEN 0x08
#define IS_FLASHING 0x10
#define NOT_FLASHING 0x0
#define SW1 0x10 // PF4
#define SW2 0x01 // PF0

uint32_t state = RED;
uint32_t flashing_state = NOT_FLASHING;
uint32_t flag = 0;


unsigned int col = 0;
unsigned int colors[] = {0x08, 0x06};
int main(void)
{
	 SYSCTL->RCGCGPIO |= 0x20; /* enable clock to PORTF */
	 /* PORTF0 has special function, need to unlock to modify */
	 GPIOF->LOCK = 0x4C4F434B; /* unlock commit register */
	 GPIOF->CR |= 0x01; /* make PORTF0 configurable */
	 GPIOF->LOCK = 0; /* lock commit register */
	 /* configure PORTF for switch input and LED output */
	 GPIOF->DIR &= ~0x11; /* make PORTF4,0 input for switch */
	 GPIOF->DIR |= 0x0E; /* make PORTF3, 2, 1 output for LEDs */
	 GPIOF->DEN |= 0x1F; /* make PORTF4-0 digital pins */
	 GPIOF->PUR |= 0x11; /* enable pull up for PORTF4,0 */
		/* configure PORTF4, 0 for falling edge trigger interrupt */
	 GPIOF->IS &= ~0x11; /* make bit 4, 0 edge sensitive */
	 GPIOF->IBE &= ~0x11; /* trigger is controlled by IEV */
	 GPIOF->IEV &= ~0x11; /* falling edge trigger */
	 GPIOF->ICR |= 0x11; /* clear any prior interrupt */
	 GPIOF->IM |= 0x11; /* unmask interrupt for PF4,PF0 */
	 /* enable interrupt in NVIC and set priority to 3 */
	 NVIC->IP[30] = 0 << 5; /* set interrupt priority to 3 */ //set to 0 // we3 shift 5 since we want to check the last 3 bits
	 NVIC->ISER[0] |= 0x40000000; /* enable IRQ30 (D30 of ISER[0]) */
	
	
	
	/* Configure SysTick */
	 SysTick->LOAD = 16000000-1; /* reload with number of clocks per second */
	 SysTick->CTRL = 7; /* enable SysTick interrupt, use system clock */
	 __enable_irq(); /* global enable interru*/

 /* toggle the green/violet LED colors continuously */
 while(1)
 {

 }
}

void SysTick_Handler(void)
{
	
	if (flashing_state == NOT_FLASHING) {
			GPIOF->DATA = colors[col];  // solid
	}
	else if(flashing_state == IS_FLASHING){
		GPIOF->DATA = (flag)?colors[col] :0;
		flag = !flag;
		
	}

}

/* SW1 is connected to PF4 pin, SW2 is connected to PF0. */
/* Both of them trigger PORTF interrupt */
void GPIOF_Handler(void)
{
	
	volatile int readback;
	//SW1
	if (GPIOF->MIS & SW1) {  //GPIOF->RIS is active high

			 col = ! col;
			 GPIOF->ICR |= 0x11; /* clear the interrupt flag before return */
			 readback = GPIOF->ICR; /* a read to force clearing of interrupt flag */

    }

	 
	
	//SW2
	if (GPIOF->MIS & SW2) {
        flashing_state = (flashing_state == IS_FLASHING) ?
                          NOT_FLASHING : IS_FLASHING;

				
				GPIOF->ICR |= 0x11; /* clear the interrupt flag before return */
				readback = GPIOF->ICR; /* a read to force clearing of interrupt flag */
		
    }
	
	
}
