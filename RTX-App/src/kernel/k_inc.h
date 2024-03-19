#ifndef K_INC_H_
#define K_INC_H_

#include "LPC17xx.h"
#include "lpc1768_mem.h"
#include "common.h"
#include "common_ext.h"
#include "rtx_errno.h"
#include "uart_polling.h"
#include "printf.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */

#define NUM_TASKS 3     // only supports three tasks in the starter code 
                        // due to limited user stack space
/*
 *===========================================================================
 *                             STRUCTURES
 *===========================================================================
 */
/**
 * @brief TCB data structure definition to support two kernel tasks.
 * @note  You will need to modify this data structure!!!
 */
// The following offset macros needs to be modified if you modify
// the positions of msp field in the TCB structure
#define TCB_MSP_OFFSET  8       // TCB.msp offset 

typedef struct
{
    void * start;
    size_t maxLength;
    size_t freeLength;
    
    void * head;
    void * tail;
} RingBuffer;

typedef struct tcb {
    struct tcb *prev;         /**< prev tcb, not used in the starter code     */
    struct tcb *next;         /**< next tcb, not used in the starter code     */
    U32        *msp;          /**< kernel sp of the task, TCB_MSP_OFFSET = 8  */
	
    void        (*ptask)();   /**< task entry address                         */
    U32         k_sp_base;    /**< kernel stack base (high addr.)             */
    
    U32         u_sp_base;    /**< user stack base addr. (high addr.)         */ 
    void       *psp;          /**< user stack (==PSP)                         */
    U32         u_stack_size; /**< user stack size                            */

    U8          priv;         /**< = 0 unprivileged, =1 privileged,           */
    U8          tid;          /**< task id                                    */
    U8          prio;         /**< scheduling priority                        */
    U8          state;        /**< task state                                 */
    void*       pmsg_yet_sent;/**< message that is waiting to send            */
    U8          wait_tid;     /**< id of the task that it is sitting in the waiting list */
    TIMEVAL period;						/**< a fixed value for a rt task period */
    int deadline;					/**< deadline that is updated every 500 usec */
    U32 set_time;
    U32 job_id;
} TCB;

typedef struct queue {
	TCB * head;
	TCB * tail;
} QUEUE;

typedef struct mbox {
    mbx_t mbx_id;
    RingBuffer message_buffer; // with the header and data
    QUEUE wait_list[5];
} MailBox;

TCB *scheduler_waiting(task_t receiver_tid);
int irq_send_to_KCD (void *buf);
int irq_receive (void * buf);
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
extern int  errno;      // defined in k_rtx_init.c file

// Memory related globals are defined in k_mem.c
// kernel stack size
extern const U32 g_k_stack_size;    // kernel stack size
extern const U32 g_p_stack_size;    // process stack size

// task kernel stacks are statically allocated inside the OS image
extern void *g_k_stacks;

// task related globals are defined in k_task.c
extern TCB *gp_current_task;    // always point to the current RUNNING task

// TCBs are statically allocated inside the OS image
extern TCB g_tcbs[MAX_TASKS];
extern MailBox g_mbox[MAX_TASKS];
extern RingBuffer uart_mbox;
extern QUEUE rQueues[5];
extern QUEUE susQueue;

extern TASK_INIT g_null_task_info;
extern U32 g_num_active_tasks;	// number of non-dormant tasks */

extern volatile uint32_t g_timer_count;     // remove if you do not need this variable


#endif  // !K_INC_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
