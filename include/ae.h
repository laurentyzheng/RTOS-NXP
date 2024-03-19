#ifndef AE_H_
#define AE_H_

#include "uart_polling.h"
#include "printf.h"
#include "rtx.h"
#include "rtx_errno.h"


/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */
int	ae_init                 (RTX_SYS_INFO *sys_info, 
                             TASK_INIT    **tasks, 
                             int          *num_tasks, 
                             int          (*cb_func) (void *(arg)), 
                             void         *arg);            

void ae_exit                (void);                            
int  ae_set_sys_info        (RTX_SYS_INFO *sys_info);
void ae_set_init_tasks_info (TASK_INIT **pp_tasks, int *p_num_tasks);                                                         
void set_ae_init_tasks      (TASK_INIT **tasks, int *num);  
void set_ae_tasks(TASK_INIT *task, int num);
                         
#endif // ! AE_H_
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

