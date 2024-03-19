#ifndef AE_TASKS_H_
#define AE_TASKS_H_

#include "rtx.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */
#ifdef SIM_TARGET       // using the simulator is slow
#define DELAY 100000
#else
#define DELAY 10000000
#endif // SIM_TARGET

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */
void set_ae_init_tasks  (TASK_INIT **tasks, int *num);  // NEW function
void set_ae_tasks       (TASK_INIT *tasks, int num);  
void priv_task1         (void);
void task0              (void);
void task1              (void);
void task2              (void);
void task3              (void);
void task4              (void);
void task5              (void);
void task6              (void);
void task7              (void);
void task8              (void);
void task9              (void);

void gen_req0           (int test_id);
int  test0_start        (int test_id);
void test0_end          (int test_id);
void init_ae_tsk_test   (void);
int  update_exec_seq    (int test_id, task_t tid);

#endif // !AE_TASKS_H_
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
