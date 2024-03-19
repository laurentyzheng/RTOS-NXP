#include "k_inc.h"
U8 enqueue(TCB* task_to_add, QUEUE* queue);
TCB* dequeue(QUEUE* queue);
U8 enqueue_front(TCB* task_to_add, QUEUE* queue);
U8 remove_from_queue(TCB* task_to_delete, QUEUE * queue);
U8 get_prio_idx (TCB* task);
U8 insert_rt_queue ( TCB* to_insert, QUEUE * queue);
