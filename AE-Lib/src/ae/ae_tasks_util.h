#ifndef AE_TASKS_UTIL_H_
#define AE_TASKS_UTIL_H_
#include "ae.h"
#include "ae_inc.h"

/*
 *===========================================================================
 *                             STRUCTURES AND TYPES
 *===========================================================================
 */

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */

int  dump_task_info (RTX_TASK_INFO *ptr);
int  dump_tasks     (task_t *p_tids, size_t len);
int  dump_mbx_info  (task_t tid);
int  dump_mailboxes (task_t *p_tids, size_t len);

#endif // ! AE_TASKS_UTIL_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

