#ifndef K_TASK_H_
#define K_TASK_H_

#include "k_inc.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

extern TCB *gp_current_task;

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */

extern void task_null	(void);         /* added in lab2 */
extern void task_kcd    (void);         /* added in lab3 */
extern void task_cdisp  (void);         /* added in lab3 */
extern void task_wall_clock(void);      /* added in lab4 */


// Implemented by Starter Code
int  k_tsk_init         (TASK_INIT *task_info, int num_tasks);
                                 /* initialize all tasks in the system */
int  k_tsk_create_new   (TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid);
                                 /* create a new task with initial context sitting on a dummy stack frame */
void k_tsk_switch       (TCB *); /* kernel thread context switch, two stacks */
int  k_tsk_run_new      (void);  /* kernel runs a new thread  */
int  k_tsk_yield        (void);  /* kernel tsk_yield function */
void task_null          (void);  /* the null task */
void k_tsk_init_first   (TASK_INIT *p_task);    /* init the first task */
void k_tsk_start        (void);  /* start the first task */
task_t k_tsk_gettid     (void);  /* get tid of the current running task */

// Not implemented, to be done by students
int  k_tsk_create       (task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size);
void k_tsk_exit         (void);
int  k_tsk_set_prio     (task_t task_id, U8 prio);
int  k_tsk_get          (task_t task_id, RTX_TASK_INFO *buffer);
TCB  *scheduler         (void);  /* student needs to change this function */
int  k_tsk_ls           (task_t *buf, size_t count);
//int  k_rt_tsk_set       (TASK_RT *p_rt_task);
int  k_rt_tsk_set       (TIMEVAL *p_tv);
int  k_rt_tsk_susp      (void);
int  k_rt_tsk_get       (task_t task_id, TIMEVAL *buffer);
#endif // ! K_TASK_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

