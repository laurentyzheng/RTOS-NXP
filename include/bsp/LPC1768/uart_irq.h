#ifndef UART_IRQ_H_
#define UART_IRQ_H_

#include <stdint.h>	
#include "uart_def.h"

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
 
extern uint8_t g_send_char;

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */


int uart_irq_init(int n_uart);		// initialize the n_uart to use interrupt

#endif // ! UART_IRQ_H_ 

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */