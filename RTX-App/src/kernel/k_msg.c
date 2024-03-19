#include "k_inc.h"
#include "k_rtx.h"
#include "k_msg.h"
#include "common.h"
#include "common_ext.h"

TCB *scheduler_waiting(task_t receiver_tid)
{
    
    QUEUE *waiting_list = g_mbox[receiver_tid].wait_list;
    size_t available = g_mbox[receiver_tid].message_buffer.freeLength;
    
    for(U8 i = 0; i < 5; i++)
		{
				// stops at the highest priority task
        if (waiting_list[i].head)
				{  
           // TCB WILL STORE A POINTER TO THE MESSAGE IT IS SENDING
            RTX_MSG_HDR *message_header = waiting_list[i].tail->pmsg_yet_sent; // SAQIB
            size_t size_message = message_header->length;
						TCB* to_return = dequeue(&(waiting_list[i]));
            return ((gp_current_task->state == DORMANT)||(size_message <= available)) ? dequeue(&(waiting_list[i])) : NULL;
        }
    }
    
    return NULL;
}



int k_mbx_create(size_t size) {
#ifdef DEBUG_0
    printf("k_mbx_create: size = %u\r\n", size);
#endif /* DEBUG_0 */
    // Current task
    if( gp_current_task == NULL )
    {
        return RTX_ERR;
    }
        
    // The mailbox has the same id as the task
    int t_id = gp_current_task->tid;
    
    MailBox* curr_mailbox = &(g_mbox[t_id]);

    // if the calling task has a mailbox
    if( curr_mailbox->message_buffer.start != NULL )
    {
        errno = EEXIST;
        return RTX_ERR;
    }

    if( size < MIN_MSG_SIZE )
    {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    void* start = k_mpool_alloc( MPID_IRAM2, size );

    if( start == NULL )
    {
        errno = ENOMEM;
        return RTX_ERR;
    }
       
    curr_mailbox->message_buffer.start = start;
    curr_mailbox->message_buffer.head = start;
    curr_mailbox->message_buffer.tail = start;
    curr_mailbox->message_buffer.maxLength = size;
    curr_mailbox->message_buffer.freeLength = size;
 
    return t_id;
}


int k_send_msg(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
        
    if( receiver_tid > 9 || receiver_tid < 1 )
    { //---- check if receiver id is valid
        errno = EINVAL;
        return RTX_ERR;
    }

    if( buf == NULL )
    { // ---- check if buffer passed is NULL
        errno = EFAULT;  
        return RTX_ERR;
    }

    RTX_MSG_HDR *header = (RTX_MSG_HDR *) buf;
    if( header->length < MIN_MSG_SIZE )
    { // ---- length of buffer passed could be too small
        errno = EINVAL;
        return RTX_ERR;
    }
        
    TCB* p_rec_tcb = &(g_tcbs[receiver_tid]);
    if( p_rec_tcb->state == DORMANT || p_rec_tcb == gp_current_task )
    { //---- check if receiver is valid
        errno = EINVAL;
        return RTX_ERR;
    }

    RingBuffer *p_rec_buf = &(g_mbox[receiver_tid].message_buffer);
    if( p_rec_buf->start == NULL )
    { // ---- check if receiver has a mailbox
        errno = ENOENT;
        return RTX_ERR;
    }
        
    if( p_rec_buf->maxLength < header->length)
    { // ---- message could exceed max length of the receivers mailbox
        errno = EMSGSIZE;
        return RTX_ERR;
    }
        
    QUEUE* p_rec_wait_list = g_mbox[receiver_tid].wait_list;

    U8 wait_top_prio = PRIO_NULL;

    for(U8 i = 0; i < 5; i++){
        if (p_rec_wait_list[i].head){ 
            wait_top_prio = p_rec_wait_list[i].head->prio;
            break;
        }
    }

    if(( wait_top_prio <= gp_current_task->prio ) || 
       ( pushData_RingBuffer(p_rec_buf, (U8*)buf, header->length) == RTX_ERR ) )
    {	
        // block self
        gp_current_task->state = BLK_SEND;
        gp_current_task->pmsg_yet_sent = (void *)buf;
        gp_current_task->wait_tid = receiver_tid;
        enqueue(gp_current_task, p_rec_wait_list);
        k_tsk_run_new();
    }
        
    if( gp_current_task->pmsg_yet_sent != NULL )
    { //---- check if receiver still exists; it might have exited ----
        errno = EINVAL;
        return RTX_ERR;
    }
        
    if( p_rec_tcb->state == BLK_RECV )
    {
        g_tcbs[receiver_tid].state = READY;
        enqueue(p_rec_tcb, rQueues);
        enqueue_front(gp_current_task, rQueues);
        return k_tsk_run_new();
    }
    
    return RTX_OK;
}

int k_send_msg_nb(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg_nb: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
    
    if( buf == NULL )
    {
        errno = EFAULT;  
        return RTX_ERR;
    }

    RTX_MSG_HDR *header = (RTX_MSG_HDR *) buf;
    if( header->length < MIN_MSG_SIZE )
    { // ---- check if buffer length passed is too small
        errno = EINVAL;
        return RTX_ERR;
    }
		
    // push right away when sending to uart
    if( receiver_tid == TID_UART )
    {
        if (gp_current_task->tid != TID_CON){
            errno = EINVAL;
            return RTX_ERR;
        }
        return pushData_RingBuffer( &(uart_mbox), (U8*)buf, header->length);
    }
    
    if( receiver_tid >= MAX_TASKS || receiver_tid < 1 )
    { //---- check if receiver id is valid
        errno = EINVAL;
        return RTX_ERR;
    }
        
    TCB* p_rec_tcb =&(g_tcbs[receiver_tid]);
    if( p_rec_tcb->state == DORMANT || p_rec_tcb == gp_current_task )
    { //---- check if receiver is valid
        errno = EINVAL;
        return RTX_ERR;
    }
    
    RingBuffer *p_rec_buf = &(g_mbox[receiver_tid].message_buffer);
    if( p_rec_buf->start == NULL )
    { // ---- check if receiver has a mailbox
        errno = ENOENT;
        return RTX_ERR;
    }

    if( p_rec_buf->maxLength < header->length )
    { // ---- check if message exceeds max length of the mailbox
        errno = EMSGSIZE;
        return RTX_ERR;
    }
		
    U8 wait_top_prio = PRIO_NULL;

    for(U8 i = 0; i < 5; i++){
        if (g_mbox[receiver_tid].wait_list->head){
            wait_top_prio = g_mbox[receiver_tid].wait_list->head->prio;
        }
    }
            

    if (( wait_top_prio <= gp_current_task->prio ) || 
            ( pushData_RingBuffer(p_rec_buf, (U8*)buf, header->length) ) == RTX_ERR )
    {	
            errno = ENOSPC;
            return RTX_ERR;
    }
			


    if( p_rec_tcb->state == BLK_RECV )
    {
        p_rec_tcb->state = READY;
        enqueue( p_rec_tcb, rQueues );
		enqueue_front( gp_current_task, rQueues );
        return k_tsk_run_new();
    }

    return 0;
}

int k_recv_msg(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
    if( buf == NULL )
    { 
        errno = EFAULT;  
        return RTX_ERR;
    }

    task_t current_tid = k_tsk_gettid();
    
    
    RingBuffer * p_cur_buf = &(g_mbox[current_tid].message_buffer);
    if( p_cur_buf->start == NULL )
    { 	
				// ---- mail box hasnt been created
        errno = ENOENT;
        return RTX_ERR;
    }

    // length of first message
    size_t top_message_length = readMessageSize_RingBuffer( p_cur_buf );
		
    if( top_message_length > len )
    {
        //input buffer can't fit first mail
        errno = ENOSPC;
        return RTX_ERR;
    }

    if( top_message_length < MIN_MSG_SIZE )
    {		
        //nothing in the mailbox
        gp_current_task->state = BLK_RECV;
        k_tsk_run_new();
    }

    return k_recv_msg_nb(buf, len);
}

int k_recv_msg_nb(void *buf, size_t len)
{
#ifdef DEBUG_0
    printf("k_recv_msg_nb: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
    if( buf == NULL )
    { // ---- check if buffer passed is NULL
        errno = EFAULT;  
        return RTX_ERR;
    }

    task_t current_tid = k_tsk_gettid();

    RingBuffer * p_cur_buf = &(g_mbox[current_tid].message_buffer);
    if( p_cur_buf->start == NULL )
    {
        errno = ENOENT;
        return RTX_ERR;
    }

    size_t top_message_length = readMessageSize_RingBuffer( p_cur_buf );
		
    if( top_message_length < MIN_MSG_SIZE )
    {
        errno = ENOMSG;
        return RTX_ERR;
    }

    if( popMessage_RingBuffer( p_cur_buf, buf, len ) == RTX_ERR )
    {
        errno = ENOSPC;
        return RTX_ERR;
    }

    // Note that scheduler_waiting checks to see if there is space and only returns if there is enough space.
    TCB* unblocked = NULL;
    while( (unblocked = scheduler_waiting(current_tid)) != 0)
    {
        //get data from wait_list tasks to itself
        RTX_MSG_HDR *unblocked_message_head = (RTX_MSG_HDR*) unblocked->pmsg_yet_sent;
        pushData_RingBuffer( p_cur_buf, unblocked->pmsg_yet_sent, unblocked_message_head->length );

        //update waiting task
        unblocked->state = READY;
        unblocked->wait_tid = 0;
        unblocked->pmsg_yet_sent = NULL;
        enqueue(unblocked, rQueues);
    }
        
    enqueue_front(gp_current_task, rQueues );  
    return k_tsk_run_new(); 
}

int k_mbx_ls(task_t *buf, size_t count) {
#ifdef DEBUG_0
    printf("k_mbx_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
    
    if( buf == NULL )
    {
        errno = EFAULT;
        return RTX_ERR;
    }
    
    size_t numMailboxes = 0;
    size_t iterator;
    for( iterator = 0; ( iterator < MAX_TASKS ) && ( numMailboxes < count ); iterator++ )
    {
        if( ( g_tcbs[iterator].state != DORMANT ) && ( g_mbox[iterator].message_buffer.start != NULL ) )
        {
            numMailboxes++;
            *buf = g_tcbs[iterator].tid;
            buf++;
        }
    }
    return numMailboxes;
}

int k_mbx_get(task_t tid)
{
#ifdef DEBUG_0
    printf("k_mbx_get: tid=%u\r\n", tid);
#endif /* DEBUG_0 */
    // Extra check to make sure tid is valid.
    if( tid <= 0 || tid >= MAX_TASKS )
    {
        errno = ENOENT;
        return RTX_ERR;
    }
    
    if( ( g_tcbs[tid].state != DORMANT ) && ( g_mbox[tid].message_buffer.start != NULL ) )
    {
        RingBuffer * leBuff = &(g_mbox[tid].message_buffer);
        return leBuff->freeLength;
    }
    else
    {
        errno = ENOENT;
        return RTX_ERR;
    }
}


int irq_send_to_KCD (void *buf){
	RingBuffer * p_rec_buf = &(g_mbox[TID_KCD].message_buffer);
	if (pushData_RingBuffer(p_rec_buf, (U8*)buf, sizeof(RTX_MSG_HDR) + 1) == RTX_ERR){
			errno = ENOSPC;
			return RTX_ERR;
	}
	
	TCB * p_rec_tcb = &(g_tcbs[TID_KCD]);
	
	if( p_rec_tcb->state == BLK_RECV )
	{
			p_rec_tcb->state = READY;
			enqueue( p_rec_tcb, rQueues );
			if (gp_current_task->tid != TID_NULL) {
				enqueue_front( gp_current_task, rQueues );
			}
			return k_tsk_run_new();
	}
	
	return RTX_OK;
}

int irq_receive (void * buf) {
	
	if( buf == NULL )
	{ // ---- check if buffer passed is NULL
			errno = EFAULT;  
			return RTX_ERR;
	}
	
	if( popMessage_RingBuffer( &uart_mbox, buf, UART_MBX_SIZE) == RTX_ERR )
	{
			errno = ENOSPC;
			return RTX_ERR;
	}
	return RTX_OK;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

