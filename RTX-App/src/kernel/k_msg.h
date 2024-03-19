#ifndef K_MSG_H_
#define K_MSG_H_

#include "k_inc.h"

#include "ring_buffer.h"
#include "ready_queue.h"


int k_mbx_create    (size_t size);
int k_send_msg      (task_t receiver_tid, const void *buf);
int k_send_msg_nb   (task_t receiver_tid, const void *buf);
int k_recv_msg      (void *buf, size_t len);
int k_recv_msg_nb   (void *buf, size_t len);
int k_mbx_ls        (task_t *buf, size_t count);
int k_mbx_get       (task_t tid);

#endif // ! K_MSG_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

