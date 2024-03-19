#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"
#include "ae_timer.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */

#define NUM_TESTS 1      // number of tests
#define NUM_INIT_TASKS 1 // number of tasks during initialization
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT g_init_tasks[NUM_INIT_TASKS];
const char PREFIX[] = "G33-TS5";
const char PREFIX_LOG[] = "G33-TS5-LOG";
const char PREFIX_LOG2[] = "G33-TS5-LOG2";

AE_XTEST g_ae_xtest; // test data, re-use for each test

AE_CASE g_ae_cases[NUM_TESTS];
AE_CASE_TSK g_tsk_cases[NUM_TESTS];
task_t g_tasks[MAX_TASKS];

void set_ae_init_tasks(TASK_INIT **pp_tasks, int *p_num)
{
  *p_num = NUM_INIT_TASKS;
  *pp_tasks = g_init_tasks;
  set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
  for (int i = 0; i < num; i++)
  {
    tasks[i].u_stack_size = PROC_STACK_SIZE;
    tasks[i].prio = HIGH + i;
    tasks[i].priv = 1;
  }
  tasks[0].priv = 1;
  tasks[0].ptask = &task1;

  init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
  g_ae_xtest.test_id = 0;
  g_ae_xtest.index = 0;
  g_ae_xtest.num_tests = NUM_TESTS;
  g_ae_xtest.num_tests_run = 0;

  for (int i = 0; i < NUM_TESTS; i++)
  {
    g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
    g_tsk_cases[i].p_ae_case->results = 0x0;
    g_tsk_cases[i].p_ae_case->test_id = i;
    g_tsk_cases[i].p_ae_case->num_bits = 0;
    g_tsk_cases[i].pos = 0; // first avaiable slot to write exec seq tid
                            // *_expt fields are case specific, deligate to specific test case to initialize
  }
  printf("%s: START\r\n", PREFIX);
}

void update_ae_xtest(int test_id)
{
  g_ae_xtest.test_id = test_id;
  g_ae_xtest.index = 0;
  g_ae_xtest.num_tests_run++;
}

void gen_req0(int test_id)
{
  // bits[0:1] for two tsk_create, [2:5] for 4 tsk_yield return value checks
  g_tsk_cases[test_id].p_ae_case->num_bits = 4;
  g_tsk_cases[test_id].p_ae_case->results = 0;
  g_tsk_cases[test_id].p_ae_case->test_id = test_id;
  g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
  g_tsk_cases[test_id].pos_expt = 6;

  g_ae_xtest.test_id = test_id;
  g_ae_xtest.index = 0;
  g_ae_xtest.num_tests_run++;
}


/**
 * @brief   task yield exec order test
 * @param   test_id, the test function ID 
 * @param   ID of the test function that logs the testing data
 * @note    usually test data is logged by the same test function,
 *          but some time, we may have multiple tests to check the same test data
 *          logged by a particular test function
 */

int update_exec_seq(int test_id, task_t tid)
{
  U8 len = g_tsk_cases[test_id].len;
  U8 *p_pos = &g_tsk_cases[test_id].pos;
  task_t *p_seq = g_tsk_cases[test_id].seq;
  p_seq[*p_pos] = tid;
  (*p_pos)++;
  (*p_pos) = (*p_pos) % len; // preventing out of array bound
  return RTX_OK;
}

int period_sec = 1;
int exec_sec = 1;

struct ae_time_usec{
	size_t sec;
	size_t usec;
};

int ae_get_tick_us_diff(struct ae_time_usec *tm, TM_TICK *tk1, TM_TICK *tk2)
{   
    if (tk1 == NULL || tk2 == NULL || tm == NULL) {
        return -1;
    }
    
    int diff_pc = tk2->pc - tk1->pc;
    int diff_tc = tk2->tc - tk1->tc;
   
    
    if ( diff_tc < 0 ) {    
        diff_pc = - diff_pc;
        diff_tc = - diff_tc;
    } else if ( diff_tc == 0  && diff_pc < 0 ) {
        diff_pc = - diff_pc;
    }
   
    
    if ( diff_pc < 0 ) {
        tm->sec  = diff_tc - 1;
        tm->usec = (diff_pc + 100000000) / 100;
    } else {
        tm->sec  = diff_tc;
        tm->usec = diff_pc / 100;
    }
    return 0;
}


int prev_task = 0;
TM_TICK time_start_of_test = {0, 0};

TM_TICK timestamp[8];
task_t current_tid;
int time_of_execution_s = 1;

void delay(size_t time_in_s) {
    TM_TICK timestamp_local = {0,0};
    get_tick(&timestamp_local, TIMER1);
    
    size_t tick = 0;
    task_t tid = tsk_gettid();
    
    printf("task%d \r\n", tid);
		size_t time_diff_in_us = 0;
    struct ae_time_usec tm = {0,0};
    while(1){
        tid = tsk_gettid();			
        if(tid != current_tid){
					ae_get_tick_us_diff(&tm, &timestamp_local, &time_start_of_test);
					printf("(Time: %ds & %dus) task%d -> task%d \r\n", tm.sec, tm.usec,  current_tid, tid);
            get_tick(&(timestamp[current_tid]), TIMER1);
            current_tid = tid;
        }
				
        get_tick(&timestamp_local, TIMER1);
        ae_get_tick_us_diff(&tm, &timestamp_local, &(timestamp[current_tid]));
        time_diff_in_us = tm.sec * 1000000 + tm.usec;
        
				if (450 < time_diff_in_us && time_diff_in_us < 550){
            get_tick(&timestamp_local, TIMER1);
            ++tick;
        }
        
        if(tick == time_in_s*2000){
            return;
        }
    }
}



void task1(void)
{
	get_tick(&time_start_of_test, TIMER1);
 	task_t tid = tsk_gettid();
	int    test_id = 0;
	U8     *p_index    = &(g_ae_xtest.index);
	int    sub_result  = 0;
	int ret_val = 0;
  gen_req0(test_id);

  update_exec_seq(0, tid);
	
	ret_val = tsk_create(&(g_tasks[2]), task2, HIGH, 0x200); /*create a user task */
  strcpy(g_ae_xtest.msg, "task1: created task2");
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  process_sub_result(test_id, (*p_index)++, sub_result);
	
	ret_val = tsk_create(&(g_tasks[3]), task3, HIGH, 0x200); /*create a user task */
  strcpy(g_ae_xtest.msg, "task1: created task3");
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  process_sub_result(test_id, (*p_index)++, sub_result);
	
	ret_val = tsk_create(&(g_tasks[4]), task4, HIGH, 0x200); /*create a user task */
  strcpy(g_ae_xtest.msg, "task1: created task4");
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  process_sub_result(test_id, (*p_index)++, sub_result);
	
	ret_val = tsk_create(&(g_tasks[5]), task5, HIGH, 0x200); /*create a user task */
  strcpy(g_ae_xtest.msg, "task1: created task5");
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  process_sub_result(test_id, (*p_index)++, sub_result);

	TIMEVAL* p_tv = mem_alloc(sizeof(TIMEVAL));
	p_tv->sec = 4;
	p_tv->usec = 0;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task1: Updated to real-time with period 5s");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);

	printf("task1: Suspending \r\n");
	ret_val = rt_tsk_susp();
  sub_result = ret_val == RTX_OK;
  strcpy(g_ae_xtest.msg, "task1: Returned");
  process_sub_result(test_id, (*p_index)++, sub_result);
	
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	
  printf("%s: TID = %d, task1: exiting \r\n", PREFIX_LOG, tid);
  tsk_exit();
}

void task2(void)
{
  task_t tid = tsk_gettid();
  int test_id = 0;
  U8 *p_index = &(g_ae_xtest.index);
  int sub_result = 0;
  int ret_val = 0;

  printf("%s: TID = %d, task2: entering \r\n", PREFIX_LOG, tid);
	
	TIMEVAL* p_tv = mem_alloc(sizeof(TIMEVAL));
	p_tv->sec = 5;
	p_tv->usec = 0;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task2: Updated to real-time with period 5s");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);
	
	printf("task2: Suspending \r\n");
	ret_val = rt_tsk_susp();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task2: Returning");
  process_sub_result(test_id, (*p_index)++, sub_result);

	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	
	printf("%s: TID = %d, task2: exited \r\n", PREFIX_LOG2, tid);
  tsk_exit();
}


void task3(void)
{
  task_t tid = tsk_gettid();
  int test_id = 0;
  U8 *p_index = &(g_ae_xtest.index);
  int sub_result = 0;
  int ret_val = 0;

  printf("%s: TID = %d, task3: entering \r\n", PREFIX_LOG, tid);
	
	TIMEVAL* p_tv = mem_alloc(sizeof(TIMEVAL));
	p_tv->sec = 6;
	p_tv->usec = 0;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task3: Updated to real-time with period 6s");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);
	
	printf("task3: Suspending \r\n");
	ret_val = rt_tsk_susp();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task3: Returning");
  process_sub_result(test_id, (*p_index)++, sub_result);

	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	
	printf("%s: TID = %d, task3: exited \r\n", PREFIX_LOG2, tid);
  tsk_exit();
}
void task4(void)
{
  task_t tid = tsk_gettid();
  int test_id = 0;
  U8 *p_index = &(g_ae_xtest.index);
  int sub_result = 0;
  int ret_val = 0;

  printf("%s: TID = %d, task4: entering \r\n", PREFIX_LOG, tid);
	
	TIMEVAL* p_tv = mem_alloc(sizeof(TIMEVAL));
	p_tv->sec = 7;
	p_tv->usec = 0;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task4: Updated to real-time with period 7s");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);
	
	printf("task4: Suspending \r\n");
	ret_val = rt_tsk_susp();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task4: Returning");
  process_sub_result(test_id, (*p_index)++, sub_result);

	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	
	printf("%s: TID = %d, task4: exited \r\n", PREFIX_LOG2, tid);
  tsk_exit();
}
void task5(void)
{
  task_t tid = tsk_gettid();
  int test_id = 0;
  U8 *p_index = &(g_ae_xtest.index);
  int sub_result = 0;
  int ret_val = 0;

  printf("%s: TID = %d, task5: entering \r\n", PREFIX_LOG, tid);
	
	TIMEVAL* p_tv = mem_alloc(sizeof(TIMEVAL));
	p_tv->sec = 8;
	p_tv->usec = 0;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task5: Updated to real-time with period 8s");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);
	
	printf("task5: Suspending \r\n");
	ret_val = rt_tsk_susp();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task5: Returning");
  process_sub_result(test_id, (*p_index)++, sub_result);

	delay(time_of_execution_s);
	delay(time_of_execution_s);
	delay(time_of_execution_s);
	
	printf("%s: TID = %d, task5: exited \r\n", PREFIX_LOG2, tid);
  tsk_exit();
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
