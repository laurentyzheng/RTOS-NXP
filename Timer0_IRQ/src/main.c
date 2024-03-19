#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "timer.h"
#include "uart_polling.h" 
#include "printf.h"

extern volatile uint32_t g_timer_count;

int main () {

    volatile uint8_t sec = 0;
    TM_TICK tk ={ 0, 0};

    SystemInit();
    __disable_irq();
    timer_irq_init(TIMER0);     /* initialize timer0, fires interrupts per 500 usec */
    timer_freerun_init(TIMER1); /* initialize timer1, free running timer            */
    uart1_init();   
    init_printf(NULL, putc);
    __enable_irq();
    uart1_put_string("\r\nTimer0 IRQ fires every 500 microseconds.\r\n");
    uart1_put_string("Timer1 is a free running timer. PC increments very 10 nsec and TC increments every 1 sec.\r\n");
    while (1) {
        /* g_timer_count gets updated every 500 us */
        if (g_timer_count % 2000 == 0) { 
            int retval = get_tick(&tk, TIMER1);
            if ( retval != 0 ) {
                printf("ERR: reading timer1 register failed\r\n");
            }
            printf("%u: TC = %u, PC = %u\r\n", sec++, tk.tc, tk.pc);
        }     
    }

}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
