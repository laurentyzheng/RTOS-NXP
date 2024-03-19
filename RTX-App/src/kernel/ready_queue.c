#include "k_inc.h"
#include "ready_queue.h"
#include "edf.h"

U8 get_prio_idx (TCB* task){
	if (task->tid == TID_NULL){
		printf("SHOULD NOT GET HERE, NULL TASK WANTS TO SNEAK IN\n\r");
		while(1);
	}
	if (task->prio == PRIO_RT){
		return 0;
	} else {
		return (task->prio & 0xF) + 1;
	}
}

// Grows left -> right
// eg) 11->24->3
// enqueue(4) would added to the back as such: 4->11->24->3
U8 enqueue(TCB* task_to_add, QUEUE* queue_start){
	
	if(task_to_add == NULL || queue_start == NULL )
		return 1;
	
	if (task_to_add->tid == TID_NULL)
		return 1;
	
	if (task_to_add->prio == PRIO_RT){
		return insert_rt_queue ( task_to_add, queue_start);
	}
	
	QUEUE * queue = queue_start + get_prio_idx(task_to_add);
	
	if(queue->head == NULL || queue->tail == NULL){
		queue->head = task_to_add;
		queue->tail = task_to_add;
	} else {
		queue->head->prev = task_to_add;
		task_to_add->next = queue->head;
		queue->head = task_to_add;
	}
	
	return 0;
}

// returns the tail of the queue
TCB* dequeue(QUEUE* queue)
{
	if( queue->tail == NULL || queue->head == NULL )
		return NULL;
	
	TCB* toReturn = queue->tail;
	
	if (toReturn->tid == TID_NULL){
		remove_from_queue(toReturn, queue);
	}

	if( queue->tail == queue->head ){
		queue->tail = NULL;
		queue->head = NULL;
	} else {
		//has more than one element in queue
		queue->tail = toReturn->prev;
		queue->tail->next = NULL;
		toReturn->prev = NULL;

		if (toReturn->prio == PRIO_RT)
		{
			//making new tail's deadline absolute.
			queue->tail->deadline += toReturn->deadline;
		}
	}
	
	return toReturn;
}

U8 enqueue_front(TCB* task_to_add, QUEUE* queue_start)
{	
	if(task_to_add == NULL || queue_start == NULL)
		return 1;
	
	if (task_to_add->tid == TID_NULL)
		return 1;
	
	QUEUE * queue = queue_start + get_prio_idx(task_to_add);
	
	if(queue->head == NULL || queue->tail == NULL){
		queue->head = task_to_add;
		queue->tail = task_to_add;
	} else {
		queue->tail->next = task_to_add;
		task_to_add->prev = queue->tail;
		queue->tail = task_to_add;
	}

	if ( (task_to_add->prio == PRIO_RT) && (queue->tail != queue->head) )
	{
		//set deadline relative to the new top (tail)
		queue->tail->prev->deadline -= queue->tail->deadline;
	}
	
	return 0;
}

U8 remove_from_queue( TCB* task_to_delete, QUEUE * queue_start )
{
	if(task_to_delete == NULL || queue_start == NULL)
		return 1;
	
	QUEUE * queue = queue_start + get_prio_idx(task_to_delete);
	
	/* Fix the head and tail of the queue if we are removing its head or tail. */
	if( ( task_to_delete == queue->head )&&( task_to_delete == queue->tail ) )
	{
			queue->head = NULL;
			queue->tail = NULL;
	}
	else if( task_to_delete == queue->head )
	{
			TCB * leHead = queue->head;
			queue->head = leHead->next;
	}
	else if( task_to_delete == queue->tail )
	{
			TCB * leTail = queue->tail;
			queue->tail = leTail->prev;
	}
   
	/* Remove from the queue structure */
	if(task_to_delete->prev != NULL)
		task_to_delete->prev->next = task_to_delete->next;

	if(task_to_delete->next != NULL)
		task_to_delete->next->prev = task_to_delete->prev;

	task_to_delete->next = NULL;
	task_to_delete->prev = NULL;

	return 0;
}

U8 insert_rt_queue ( TCB* to_insert, QUEUE * queue)
{
	if ( to_insert == NULL )
		return 1;
	
	if (to_insert->tid == TID_NULL)
		return 1;


	if (queue->head == NULL || queue->tail == NULL ){
		queue->head = to_insert;
		queue->tail = to_insert;
		return 0;
	}
	
	for (TCB* tcb_trav = queue->tail; tcb_trav != NULL; tcb_trav = tcb_trav->prev)
	{
		//printf( "chk: tcb = %x, tid = %d\n\r", (U32)tcb_trav, tcb_trav->tid );
		if (to_insert->deadline >= tcb_trav->deadline)
		{
			to_insert->deadline -= tcb_trav->deadline;
		} 
		else 
		{
			to_insert->prev = tcb_trav;
			if (tcb_trav->next != NULL)
			{
				tcb_trav->next->prev = to_insert;
				to_insert->next = tcb_trav->next;
			} else {
				//at tail
				queue->tail = to_insert;
			}
			tcb_trav->next = to_insert;
			//update deadline of the one before insertion
			tcb_trav->deadline -= to_insert->deadline;
			return 0;
		}
	}

	//traversed through all and insert at the end of the list (head)
	to_insert->next = queue->head;
	queue->head->prev = to_insert;
	queue->head = to_insert;

	return 0;
}
