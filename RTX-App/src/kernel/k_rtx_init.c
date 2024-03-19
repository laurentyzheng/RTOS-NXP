#include "k_rtx_init.h"
#include "k_rtx.h"
#include "k_inc.h"
#include "ready_queue.h"
#include "k_task.h"
#include "common.h"
#include "common_ext.h"

int errno = 0;
QUEUE rQueues[5];
MailBox g_mbox[ MAX_TASKS ];
RingBuffer uart_mbox;
QUEUE susQueue;

/**************************************************************************//**
 * @brief     system set up before calling rtx_init() from thread mode  
 * @pre         processor should be in thread mode using MSP, rtx_init is not called
 * @post        PSP is intialized and points to NULL task user stack
 * @return      0 on success and non-zero on failure
 * @note        you may add extra code here
 *****************************************************************************/

int k_pre_rtx_init (void *args)
{
    if ( k_mem_init(((RTX_SYS_INFO *)(args))->mem_algo) != RTX_OK) {
        return RTX_ERR;
    }
    
    __set_PSP((U32) k_alloc_p_stack(TID_NULL));
    
    return RTX_OK;
}


int k_rtx_init(RTX_SYS_INFO *sys_info, TASK_INIT *tasks, int num_tasks)
{
    errno = 0;
    
    // Check if input args aare valid
    if( num_tasks >= MAX_TASKS - 3 || sys_info == NULL || tasks == NULL )
    {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    /* interrupts are already disabled when we enter here */
    if ( uart_irq_init(0) != RTX_OK )
    {
        return RTX_ERR;
    }
    
    /* add timer(s) initialization code */
		
		sys_info->mem_algo = BUDDY;
    sys_info->sched = DEFAULT;
		
		//allocate kernel stack dynamically.
	  g_k_stacks = k_mpool_alloc(MPID_IRAM2, (MAX_TASKS - 1) * KERN_STACK_SIZE);
		
		if ( g_k_stacks == NULL ) {
        errno = ENOMEM;
        return RTX_ERR;
    }
    
    if( k_tsk_init(tasks, num_tasks) != RTX_OK )
    {
        return RTX_ERR;
    }

    // initialize ready queues
    for( U8 i = 0; i < 5; i++ )
    {
        rQueues[i].head = NULL;
        rQueues[i].tail = NULL;
    }
		
		for( U8 i = 0; i < num_tasks; i++ ) 
		{
        enqueue( &(g_tcbs[i+1]), rQueues );
		}
		enqueue_front ( &g_tcbs[TID_WCLCK], rQueues ); 
		enqueue_front ( &g_tcbs[TID_CON], rQueues ); 

     // initialize the mailboxes
    for(U8 i = 1; i < MAX_TASKS; i++) 
		{
			g_mbox[i].mbx_id = i;
			g_mbox[i].message_buffer.start = NULL; 
			g_mbox[i].message_buffer.head = NULL; 
			g_mbox[i].message_buffer.tail = NULL;
			g_mbox[i].message_buffer.maxLength = g_mbox[i].message_buffer.freeLength = 0;
						
			// initialize priority queues
			for( U8 j = 0; j < 5; j++ )
			{
					g_mbox[i].wait_list[j].head = NULL;
					g_mbox[i].wait_list[j].tail = NULL;
			}
    }
		
		void * uart_buf = k_mpool_alloc(MPID_IRAM2, UART_MBX_SIZE);
		//initialize mailbox for uart irq
		uart_mbox.start = uart_buf;
		uart_mbox.head =	uart_buf;
		uart_mbox.tail = uart_buf;
		uart_mbox.freeLength = UART_MBX_SIZE;
		uart_mbox.maxLength = UART_MBX_SIZE;
		
    susQueue.head = NULL;
    susQueue.tail = NULL;
		
    gp_current_task = &g_tcbs[TID_KCD];
		gp_current_task->state = RUNNING;
		
    k_tsk_start();        // start the first task
    return RTX_OK;
}

int k_get_sys_info( RTX_SYS_INFO *buffer )
{
    return RTX_OK;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
