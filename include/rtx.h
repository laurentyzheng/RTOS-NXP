#ifndef _RTX_H_
#define _RTX_H_

#include "rtx_ext.h"
#include "common.h"
#include "lpc1768_mem.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */

/*
 *===========================================================================
 *                             FUNCTION PROTOTYPES
 *===========================================================================
 */
 
extern int   k_pre_rtx_init     (void *args);

__svc(SVC_RTX_INIT)     int     rtx_init(RTX_SYS_INFO *sys_info, TASK_INIT *tasks, int num_tasks);
__svc(SVC_MEM_ALLOC)    void   *mem_alloc(size_t size);
__svc(SVC_MEM_DEALLOC)  int     mem_dealloc(void *ptr);
__svc(SVC_MEM_DUMP)     int     mem_dump(void);
__svc(SVC_TSK_CREATE)   int     tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size);
__svc(SVC_TSK_EXIT)     void    tsk_exit(void);
__svc(SVC_TSK_YIELD)    int     tsk_yield(void);
__svc(SVC_TSK_SET_PRIO) int     tsk_set_prio(task_t task_id, U8 prio);
__svc(SVC_TSK_GET)      int     tsk_get(task_t task_id, RTX_TASK_INFO *buffer);
__svc(SVC_TSK_GETTID)   task_t  tsk_gettid(void);
__svc(SVC_TSK_LS)       int     tsk_ls(task_t *buf, size_t count);
__svc(SVC_MBX_CREATE)   int     mbx_create(size_t size);
__svc(SVC_MBX_SEND)     int     send_msg(task_t tid, const void* buf);
__svc(SVC_MBX_SEND_NB)  int     send_msg_nb(task_t tid, const void* buf);
__svc(SVC_MBX_RECV)     int     recv_msg(void *buf, size_t len);
__svc(SVC_MBX_RECV_NB)  int     recv_msg_nb(void *buf, size_t len);
__svc(SVC_MBX_LS)       int     mbx_ls(task_t *buf, size_t count);
__svc(SVC_MBX_GET)      int     mbx_get(task_t tid);
__svc(SVC_RT_TSK_SET)   int     rt_tsk_set(TIMEVAL *p_tv);
__svc(SVC_RT_TSK_SUSP)  int     rt_tsk_susp(void);
__svc(SVC_RT_TSK_GET)   int     rt_tsk_get(task_t task_id, TIMEVAL *buffer);
#endif // !_RTX_H_


#ifdef ECE350_P1
__svc(SVC_MEM2_ALLOC)    void   *mem2_alloc(size_t size);
__svc(SVC_MEM2_DEALLOC)  int     mem2_dealloc(void *ptr);
__svc(SVC_MEM2_DUMP)     int     mem2_dump(void);
#endif

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

