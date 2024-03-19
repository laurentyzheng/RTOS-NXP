/**
 * @brief The Console Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include <LPC17xx.h>
#include "uart_irq.h"
#include "uart_polling.h"
#include "printf.h"

extern uint8_t g_tx_irq;    


void task_cdisp(void)
{
    /* not implemented yet */
	int result = mbx_create(CON_MBX_SIZE);
	if (result != TID_CON){
		tsk_exit();
	}
    //printf( "CDISP: %d\n", result );
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
	pUart->THR = '\b';
	pUart->THR = '>';
	pUart->THR = '>';
	pUart->THR = '>';

	while (1)
	{
		uint8_t * recv_buf = mem_alloc(CON_MBX_SIZE);
		if (recv_buf == NULL){
			printf("cdisplay can't receive! out of memory");
			tsk_yield();
		}
		recv_msg(recv_buf, CON_MBX_SIZE);
		RTX_MSG_HDR* header = (RTX_MSG_HDR*) recv_buf;
		//printf("received message in display \n\r");
		if (header->type == DISPLAY)
		{
			//display one character
			if (header->sender_tid == TID_KCD && header->length == MSG_HDR_SIZE + 1){
					char to_display = *(recv_buf + MSG_HDR_SIZE);
					if (to_display == '\b'){
						pUart->THR = '\b';
						pUart->THR = ' ';
						pUart->THR = '\b';
					} else if (to_display == '\r'){
						pUart->THR = '\r';
						pUart->THR = '\n';
					} else {
						pUart->THR = to_display;
					}
			} else {
				g_tx_irq = 1; //set transimission flag
				if (send_msg_nb(TID_UART, recv_buf) == RTX_OK){
					//set the flag to print the entire string
					pUart->IER |= IER_THRE;
				}
			}
		}
		mem_dealloc(recv_buf);
		recv_buf = NULL;
	}
}


/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

