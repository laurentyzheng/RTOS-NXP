#ifndef K_RTX_INIT_H_
#define K_RTX_INIT_H_

#include "k_inc.h"

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */
int  k_pre_pre_init(void *args);
int  k_rtx_init(RTX_SYS_INFO *sys_info, TASK_INIT *task, int num_tasks);
int  k_get_sys_info(RTX_SYS_INFO *buffer);

#endif /* ! K_RTX_INIT_H_ */

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
