#include "k_inc.h"
//#include "k_task.h"
#include "k_rtx.h"
#include "ready_queue.h"
#include "k_rtx_init.h"
#include "common.h"
#include "common_ext.h"
#include "edf.h"
#include "common_ext.h"

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

TCB             *gp_current_task = NULL;    // the current RUNNING task
TCB             g_tcbs[MAX_TASKS];          // an array of TCBs
//TASK_INIT       g_null_task_info;           // The null task info
U32             g_num_active_tasks = 0;     // number of non-dormant tasks
U32 						null_k_stack [KERN_STACK_SIZE >> 2]__attribute__((aligned(8)));


/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
             g_k_stacks[15]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */


/**************************************************************************//**
 * @brief   scheduler, pick the TCB of the next to run task
 *
 * @return  TCB pointer of the next to run task
 * @post    gp_curret_task is updated
 * @note    you need to change this one to be a priority scheduler
 *
 *****************************************************************************/

TCB* scheduler(void)
{
		
    for(U8 i = 0; i < 5; i++){
        if (rQueues[i].head != NULL){
            return dequeue(&(rQueues[i]));
        }
    }
    
    return &g_tcbs[TID_NULL];

}

/**
 * @brief initialzie the first system tasks
 */
void k_tsk_init_first(TASK_INIT *p_task)
{
    p_task->prio         = PRIO_NULL;
    p_task->priv         = 0;
    p_task->tid          = TID_NULL;
    p_task->ptask        = &task_null;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

void k_kcd_init(TASK_INIT *p_task)
{
		p_task->prio         = HIGH;
    p_task->priv         = 0;
    p_task->tid          = TID_KCD;
    p_task->ptask        = &task_kcd;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

void k_con_init(TASK_INIT *p_task)
{
		p_task->prio         = HIGH;
    p_task->priv         = 1;
    p_task->tid          = TID_CON;
    p_task->ptask        = &task_cdisp;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

void k_wallclk_init (TASK_INIT * p_task)
{
		p_task->prio         = HIGH;
    p_task->priv         = 0;
    p_task->tid          = TID_WCLCK;
    p_task->ptask        = &task_wall_clock;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

/**************************************************************************//**
 * @brief       initialize all boot-time tasks in the system,
 *
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       task_info   boot-time task information structure pointer
 * @param       num_tasks   boot-time number of tasks
 * @pre         memory has been properly initialized
 * @post        none
 * @see         k_tsk_create_first
 * @see         k_tsk_create_new
 *****************************************************************************/

int k_tsk_init( TASK_INIT *task, int num_tasks )
{
    if( num_tasks >= MAX_TASKS - 3 )
    {
        return RTX_ERR;
    }
    
    TASK_INIT taskinfo;
    
    k_tsk_init_first( &taskinfo );
    if( k_tsk_create_new( &taskinfo, &g_tcbs[TID_NULL], TID_NULL ) == RTX_OK )
    {
        g_num_active_tasks = 1;
    }
    else
    {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
		
		k_con_init( &taskinfo );
		if( k_tsk_create_new( &taskinfo, &g_tcbs[TID_CON], TID_CON ) == RTX_OK )
    {
        g_num_active_tasks++;
        gp_current_task = &g_tcbs[TID_CON];
    }
    else
    {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
		
		k_kcd_init( &taskinfo );
		if( k_tsk_create_new( &taskinfo, &g_tcbs[TID_KCD], TID_KCD ) == RTX_OK )
    {
        g_num_active_tasks++;
        gp_current_task = &g_tcbs[TID_KCD];
    }
    else
    {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
		
		k_wallclk_init ( &taskinfo );
		if( k_tsk_create_new( &taskinfo, &g_tcbs[TID_WCLCK], TID_WCLCK ) == RTX_OK )
		{
				g_num_active_tasks++;
        gp_current_task = &g_tcbs[TID_WCLCK];
		}
		else
    {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
		
    // create the rest of the tasks
    for( int i = 0; i < num_tasks; i++ )
    {
        TCB *p_tcb = &g_tcbs[i+1];
        if( k_tsk_create_new( &task[i], p_tcb, i+1 ) == RTX_OK )
        {
            g_num_active_tasks++;
        }

    }

    //initialize empty tasks to fill the rest of g_tcb
    for( int i = num_tasks + 1; i < MAX_TASKS - 3; i++ )
    {	
        TIMEVAL invalid_time = {
            //use garbase data non-rt tasks
            .sec = 0,
            .usec = 0
        };
				
        TCB tcb = {
            .prev = NULL,
            .next = NULL,
            .ptask = NULL,
            .msp = NULL,
            .psp = NULL,
            .u_sp_base = 0,
            .k_sp_base = 0,
            .u_stack_size = 0,
            .priv = 0,
            .tid = i,
            .prio = 0,
            .state = DORMANT,
            .pmsg_yet_sent = NULL,
            .wait_tid = 0,
			.period = invalid_time,
            .deadline = 0,
            .set_time = 0,
            .job_id = 0
        };

        g_tcbs[i] = tcb;
    }
    
    return RTX_OK;
}
/**************************************************************************//**
 * @brief       initialize a new task in the system,
 *              one dummy kernel stack frame, one dummy user stack frame
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       p_taskinfo  task initialization structure pointer
 * @param       p_tcb       the tcb the task is assigned to
 * @param       tid         the tid the task is assigned to
 *
 * @details     From bottom of the stack,
 *              we have user initial context (xPSR, PC, SP_USR, uR0-uR3)
 *              then we stack up the kernel initial context (kLR, kR4-kR12, PSP, CONTROL)
 *              The PC is the entry point of the user task
 *              The kLR is set to SVC_RESTORE
 *              20 registers in total
 * @note        YOU NEED TO MODIFY THIS FILE!!!
 *****************************************************************************/
int k_tsk_create_new(TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid)
{
    extern U32 SVC_RTE;

    U32 *usp;
    U32 *ksp;

    if( ( p_taskinfo == NULL )||( p_tcb == NULL ) )
    {
        return RTX_ERR;
    }
		
    TIMEVAL invalid_time = {
        //use garbase data at creation
        .sec = 0,
        .usec = 0
    };

    p_tcb->next = NULL;
    p_tcb->prev = NULL;
    p_tcb->tid   = tid;
    p_tcb->state = READY;
    p_tcb->prio  = p_taskinfo->prio;
    p_tcb->priv  = p_taskinfo->priv;
    p_tcb->ptask = p_taskinfo->ptask;
    p_tcb->u_stack_size = p_taskinfo->u_stack_size;
    p_tcb->pmsg_yet_sent = NULL;
    p_tcb->wait_tid = 0;
    p_tcb->period = invalid_time;
    p_tcb->deadline = 0;
    p_tcb->set_time = 0;
    p_tcb->job_id = 0;
    
    /*---------------------------------------------------------------
     *  Step1: allocate user stack for the task
     *         stacks grows down, stack base is at the high address
     * ATTENTION: you need to modify the following three lines of code
     *            so that you use your own dynamic memory allocator
     *            to allocate variable size user stack.
     * -------------------------------------------------------------*/
    
    usp = k_mpool_alloc( MPID_IRAM2, p_taskinfo->u_stack_size );            
    if( usp == NULL )
    {
        errno = ENOMEM;
        return RTX_ERR;
    }
    
    /* fix the size to grow downward */
    uintptr_t temp_ptr = (uintptr_t) usp;
    temp_ptr += p_taskinfo->u_stack_size;
    usp = (U32*)temp_ptr;

    /* set user tack base of tcb as the top address*/
    p_tcb->u_sp_base = (uintptr_t) usp;
    /*-------------------------------------------------------------------
     *  Step2: create task's thread mode initial context on the user stack.
     *         fabricate the stack so that the stack looks like that
     *         task executed and entered kernel from the SVC handler
     *         hence had the exception stack frame saved on the user stack.
     *         This fabrication allows the task to return
     *         to SVC_Handler before its execution.
     *
     *         8 registers listed in push order
     *         <xPSR, PC, uLR, uR12, uR3, uR2, uR1, uR0>
     * -------------------------------------------------------------*/

    // if kernel task runs under SVC mode, then no need to create user context stack frame for SVC handler entering
    // since we never enter from SVC handler in this case
    
    *(--usp) = INITIAL_xPSR;             // xPSR: Initial Processor State
    *(--usp) = (U32) (p_taskinfo->ptask);// PC: task entry point
        
    // uR14(LR), uR12, uR3, uR3, uR1, uR0, 6 registers
    for ( int j = 0; j < 6; j++ ) {
        
#ifdef DEBUG_0
        *(--usp) = 0xDEADAAA0 + j;
#else
        *(--usp) = 0x0;
#endif
    }

    //set tcb psp **added later**
    p_tcb->psp = usp;
    
    // allocate kernel stack for the task
    if (tid == TID_NULL) {
        ksp = (void*)((uintptr_t) null_k_stack + KERN_STACK_SIZE );
		}
    else { 
        ksp = (void*)((uintptr_t) g_k_stacks + KERN_STACK_SIZE * (tid)); 
		}
    
    
    if ((U32)ksp & 0x04)  // if sp not 8B aligned, then it must be 4B aligned
    ksp--;               		// adjust it to 8B aligned
    

    if ( ksp == NULL ) {
        errno = ENOMEM;
        return RTX_ERR;
    }

    //set tcb kernel base ***CHANGED***
    p_tcb->k_sp_base = (uintptr_t) ksp;

    /*---------------------------------------------------------------
     *  Step3: create task kernel initial context on kernel stack
     *
     *         12 registers listed in push order
     *         <kLR, kR4-kR12, PSP, CONTROL>
     * -------------------------------------------------------------*/
    // a task never run before directly exit
    *(--ksp) = (U32) (&SVC_RTE);
    // kernel stack R4 - R12, 9 registers
#define NUM_REGS 9    // number of registers to push
      for ( int j = 0; j < NUM_REGS; j++) {        
#ifdef DEBUG_0
        *(--ksp) = 0xDEADCCC0 + j;
#else
        *(--ksp) = 0x0;
#endif
    }
        
    // put user sp on to the kernel stack
    *(--ksp) = (U32) usp;
    
    // save control register so that we return with correct access level
    if (p_taskinfo->priv == 1) {  // privileged 
        *(--ksp) = __get_CONTROL() & ~BIT(0); 
    } else {                      // unprivileged
        *(--ksp) = __get_CONTROL() | BIT(0);
    }

    p_tcb->msp = ksp;

    return RTX_OK;
}

/**************************************************************************//**
 * @brief       switching kernel stacks of two TCBs
 * @param       p_tcb_old, the old tcb that was in RUNNING
 * @return      RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre         gp_current_task is pointing to a valid TCB
 *              gp_current_task->state = RUNNING
 *              gp_crrent_task != p_tcb_old
 *              p_tcb_old == NULL or p_tcb_old->state updated
 * @note        caller must ensure the pre-conditions are met before calling.
 *              the function does not check the pre-condition!
 * @note        The control register setting will be done by the caller
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
__asm void k_tsk_switch(TCB *p_tcb_old)
{
        PRESERVE8
        EXPORT  K_RESTORE
        
        PUSH    {R4-R12, LR}                // save general pupose registers and return address
        MRS     R4, CONTROL                 
        MRS     R5, PSP
        PUSH    {R4-R5}                     // save CONTROL, PSP
        STR     SP, [R0, #TCB_MSP_OFFSET]   // save SP to p_old_tcb->msp
K_RESTORE
        LDR     R1, =__cpp(&gp_current_task)
        LDR     R2, [R1]
        LDR     SP, [R2, #TCB_MSP_OFFSET]   // restore msp of the gp_current_task
        POP     {R4-R5}
        MSR     PSP, R5                     // restore PSP
        MSR     CONTROL, R4                 // restore CONTROL
        ISB                                 // flush pipeline, not needed for CM3 (architectural recommendation)
        POP     {R4-R12, PC}                // restore general purpose registers and return address
}


__asm void k_tsk_start(void)
{
        PRESERVE8
        B K_RESTORE
}

/**************************************************************************//**
 * @brief       run a new thread. The caller becomes READY and
 *              the scheduler picks the next ready to run task.
 * @return      RTX_ERR on error and zero on success
 * @pre         gp_current_task != NULL && gp_current_task == RUNNING
 * @post        gp_current_task gets updated to next to run task
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *****************************************************************************/
int k_tsk_run_new(void)
{
    TCB *p_tcb_old = NULL;
    
    if (gp_current_task == NULL) {
        return RTX_ERR;
    }

    p_tcb_old = gp_current_task;
    gp_current_task = scheduler();
    
    if ( gp_current_task == NULL  ) {
        gp_current_task = p_tcb_old;        // revert back to the old task
        return RTX_ERR;
    }

    // at this point, gp_current_task != NULL and p_tcb_old != NULL
    if (gp_current_task != p_tcb_old) {
        gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb
			
        if (p_tcb_old->state == RUNNING)
            p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb

        k_tsk_switch(p_tcb_old);            // switch kernel stacks       
    }

    return RTX_OK;
}

 
/**************************************************************************//**
 * @brief       yield the cpu
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task != NULL &&
 *              gp_current_task->state = RUNNING
 * @post        gp_current_task gets updated to next to run task
 * @note:       caller must ensure the pre-conditions before calling.
 *****************************************************************************/
int k_tsk_yield(void)
{		
		if (gp_current_task->prio == PRIO_RT)
			return RTX_ERR;
		
		//won't works with null task
		enqueue( gp_current_task , rQueues );
		
    return k_tsk_run_new();
}

/**
 * @brief   get task identification
 * @return  the task ID (TID) of the calling task
 */
task_t k_tsk_gettid(void)
{
	
	return gp_current_task->tid;
}

/*
 *===========================================================================
 *                             TO BE IMPLEMETED IN LAB2
 *===========================================================================
 */

int k_tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size)
{
#ifdef DEBUG_0
    printf("k_tsk_create: entering...\n\r");
    printf("task = 0x%x, task_entry = 0x%x, prio=%d, stack_size = %d\n\r", task, task_entry, prio, stack_size);
#endif /* DEBUG_0 */
    
    /* priority has to be valid */
    if( ( prio < HIGH )||( prio > LOWEST ) )
    {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    /* We require that the stack size is at least PROC_STACK_SIZE */
    stack_size = stack_size > PROC_STACK_SIZE ? stack_size : PROC_STACK_SIZE;
    
    TCB * leTCB = NULL;
    size_t iterator;
    // last two TIDs are reserved for UART (part b)
    for( iterator = 1; iterator < MAX_TASKS - 2; iterator++ )
    {
        if( g_tcbs[iterator].state == DORMANT )
        {
            leTCB = &g_tcbs[iterator];
            break;
        }
    }
    
    // Errors checking for if maximum tasks has reached
    if( leTCB == NULL )
    {
        errno = EAGAIN;
        return RTX_ERR;
    }
  
    
    TASK_INIT leInit = {
        .ptask = task_entry, 
        .u_stack_size = stack_size,
        .tid = iterator,
        .prio = prio,
        .priv = 0
    };
    
    /* k_tsk_create_new sets errno in it if there is not enough space
          for kernel and user stacks */
    int result = k_tsk_create_new( &leInit, leTCB, iterator );
    
    if( result != RTX_OK )
        return RTX_ERR;
    
    g_num_active_tasks++;
    
    *task = iterator;

    enqueue( leTCB, rQueues );
    enqueue_front( gp_current_task, rQueues );

    k_tsk_run_new();
    
    return RTX_OK;

}

void k_tsk_exit(void) 
{
#ifdef DEBUG_0
    printf("k_tsk_exit: entering...\n\r");
#endif /* DEBUG_0 */
    gp_current_task->state = DORMANT;
	
    void* p_u_stack = (void *) ((intptr_t)gp_current_task->u_sp_base - gp_current_task->u_stack_size);
    k_mpool_dealloc(MPID_IRAM2, p_u_stack);
	
    g_num_active_tasks--;
	
    TCB* unblocked = NULL;
		RingBuffer * p_cur_buf = &(g_mbox[k_tsk_gettid()].message_buffer);

		while( (unblocked = scheduler_waiting(gp_current_task->tid)) != 0)
		{
				unblocked->state = READY;
				unblocked->wait_tid = 0;
				enqueue(unblocked, rQueues );
		}
		k_mpool_dealloc(MPID_IRAM2, p_cur_buf->start);
		p_cur_buf->start = NULL;
    
    k_tsk_run_new();
    
    return;
}

int k_tsk_set_prio(task_t task_id, U8 prio) 
{

#ifdef DEBUG_0
    printf("k_tsk_set_prio: entering...\n\r");
    printf("task_id = %d, prio = %d.\n\r", task_id, prio);
#endif /* DEBUG_0 */
    /* priority has to be valid */
    if( (( prio < HIGH )||( prio > LOWEST )) && (prio != PRIO_RT))
    {
        errno = EINVAL;
        return RTX_ERR;
    }

    // error if no such task exists, or if the task is a null task
    // i.e. can not set the prio of a NULL task
    if( task_id >= MAX_TASKS || task_id <= TID_NULL ){
        errno = EINVAL;
        return RTX_ERR;
    }
    
    TCB* p_task = &(g_tcbs[task_id]);

    //error if the task changing it (the current running one) has lower privilege
    if (gp_current_task->priv < p_task->priv){
        errno = EPERM;
        return RTX_ERR;
    }
		
		//can't change rt tasks to non-rt
		if (p_task->prio == PRIO_RT && prio > PRIO_RT){
				errno = EPERM;
        return RTX_ERR;
		}
		
		//can't change non-rt tasks to rt
		if (p_task->prio > PRIO_RT && prio == PRIO_RT){
				errno = EPERM;
        return RTX_ERR;
		}
		
		if (p_task->prio == prio || p_task->state == DORMANT || p_task->state == SUSPENDED){
				return RTX_OK;
		}
		
		else if (p_task == gp_current_task){
				//RUNNING
				p_task->prio = prio;
				enqueue( gp_current_task , rQueues);
				return k_tsk_run_new();
		}
		
		else if (p_task->state == READY){
				remove_from_queue( p_task, rQueues );
				p_task->prio = prio;
			
				enqueue( p_task, rQueues );
				enqueue_front( gp_current_task , rQueues);
				return k_tsk_run_new();
		}
			
		else if (p_task->state == BLK_SEND){
			
				QUEUE * wait_list = g_mbox[p_task->wait_tid].wait_list;
			
				remove_from_queue( p_task, wait_list );
				p_task->prio = prio;
			
				enqueue( p_task, wait_list );
				enqueue_front( gp_current_task , rQueues);
			
				TCB* unblocked = NULL;
				RingBuffer * p_receiving_buf = &(g_mbox[p_task->wait_tid].message_buffer);
				
				while ((unblocked = scheduler_waiting(p_task->wait_tid)) != 0)
				{
						RTX_MSG_HDR *header = (RTX_MSG_HDR*) unblocked->pmsg_yet_sent;
						pushData_RingBuffer(p_receiving_buf, (U8*)unblocked->pmsg_yet_sent, header->length );
						unblocked->state = READY;
						unblocked->wait_tid = 0;
						unblocked->pmsg_yet_sent = NULL;
						enqueue(unblocked, rQueues);
				}
				
				return k_tsk_run_new();
		}
		
		else {
			//BLK_RECV
			p_task->prio = prio;
			return RTX_OK;
		}
}

/**
 * @brief   Retrieve task internal information 
 * @note    this is a dummy implementation, you need to change the code
 */
int k_tsk_get(task_t tid, RTX_TASK_INFO *buffer)
{
#ifdef DEBUG_0
    printf("k_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    
    if ( tid >= MAX_TASKS) {
        errno = EINVAL;
        return RTX_ERR;
    }

    TCB* leTcb = &(g_tcbs[tid]);
    

    buffer->tid           = tid;
    buffer->prio          = leTcb->prio;
    buffer->u_stack_size  = leTcb->u_stack_size;
    buffer->priv          = leTcb->priv;
    buffer->ptask         = leTcb->ptask;
    buffer->k_sp          = (leTcb == gp_current_task)? __get_MSP() : (U32)(leTcb->msp);
    buffer->k_sp_base     = leTcb->k_sp_base;
    buffer->k_stack_size  = KERN_STACK_SIZE;
    buffer->state         = leTcb->state;
    buffer->u_sp          = (leTcb == gp_current_task)? __get_PSP() : (U32)(leTcb->psp);
    buffer->u_sp_base     = leTcb->u_sp_base;
    return RTX_OK;     
}

int k_tsk_ls(task_t *buf, size_t count){
#ifdef DEBUG_0
  printf("k_tsk_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
    if( buf == NULL || count <= 0 )
    {
        errno = EFAULT;
        return RTX_ERR;
    }
    
    int result = 0;
    int iterator;
    for( iterator = 0; ( iterator < MAX_TASKS )&&( result < count ); iterator++ )
    {
        int stateIsNotDormant = ( g_tcbs[iterator].state != DORMANT );
        if( stateIsNotDormant ){
            *buf = g_tcbs[iterator].tid;
            buf++;
            result++;
        }
    }
  
    return result;
}

int k_rt_tsk_set(TIMEVAL *p_tv)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_set: p_tv = 0x%x\r\n", p_tv);
#endif /* DEBUG_0 */
    if(	gp_current_task->prio == PRIO_RT) {
        errno = EPERM;
        return RTX_ERR;
    }
    
    int period_in_tick = timeval_to_tick (*p_tv);
    
    if ( period_in_tick <= 0 || period_in_tick % MIN_PERIOD != 0) {
        errno = EINVAL;
        return RTX_ERR;
    }

    gp_current_task->prio = PRIO_RT;
    
    gp_current_task->period = *p_tv;

    //add extra tick to the deadline to compensate for the next tick decrement
    gp_current_task->deadline = period_in_tick + 1;
    gp_current_task->set_time = g_timer_count + 1;
    
    enqueue(gp_current_task, rQueues);

    return k_tsk_run_new();   
}

int k_rt_tsk_susp(void)
{
		if (gp_current_task->prio != PRIO_RT){
        errno = EPERM;
        return RTX_ERR;
    }
#ifdef DEBUG_0
		printf("task %d suspendeds, rt-dl: ", gp_current_task->tid);
		for(TCB* tsk = rQueues->tail; tsk != NULL; tsk = tsk->prev){
			printf("(t=%d):%d->",tsk->tid, tsk->deadline);
		}
		printf("NULL \r\n");
#endif /* DEBUG_0 */

    if (missed_deadline(gp_current_task)){
			
		printf("Job %d of task %d missed its deadline\r\n", gp_current_task->job_id, gp_current_task->tid);

        while(gp_current_task->deadline <= 0)
        {
            //update deadline
            gp_current_task->deadline += timeval_to_tick(gp_current_task->period);
        }
				
        insert_rt_queue (gp_current_task, &(rQueues[0]) );
        //add one to neglect the jobs in the overflowed period
        gp_current_task->job_id = get_num_jobs(gp_current_task);

    } else {
        gp_current_task -> state = SUSPENDED;
        insert_rt_queue(gp_current_task, &susQueue);
        gp_current_task->job_id ++;
    }
		
    return k_tsk_run_new();
}

int k_rt_tsk_get(task_t tid, TIMEVAL *buffer)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL) {
        return RTX_ERR;
    }   
		
    if( tid >= MAX_TASKS || tid <= TID_NULL ){
        errno = EINVAL;
        return RTX_ERR;
    }
		
    TCB* p_task  = &g_tcbs[tid];
    
    // Task ID has to be a RT task
    if( p_task->prio != PRIO_RT ){
        errno = EINVAL;
        return RTX_ERR;
    }
    
    // Check if calling task is not RT
    if (gp_current_task->prio != PRIO_RT){
        errno = EPERM;
        return RTX_ERR;
    }
		
    buffer->sec  = p_task->period.sec;
    buffer->usec = p_task->period.usec;

    return RTX_OK;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

