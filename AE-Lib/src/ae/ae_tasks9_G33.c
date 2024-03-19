#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae_util.h"
#include "ae_tasks_util.h"
#include "timer.h"

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
const char PREFIX[] = "G33-TS9";
const char PREFIX_LOG[] = "G33-TS9-LOG";
const char PREFIX_LOG2[] = "G33-TS9-LOG2";

const int proper_order = 2121221;

//change by append
static int order = 0;

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
  g_tsk_cases[test_id].p_ae_case->num_bits = 12;
  g_tsk_cases[test_id].p_ae_case->results = 0;
  g_tsk_cases[test_id].p_ae_case->test_id = test_id;
  g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
  g_tsk_cases[test_id].pos_expt = 6;

  g_ae_xtest.test_id = test_id;
  g_ae_xtest.index = 0;
  g_ae_xtest.num_tests_run++;
}

void append_to_order(){
	order *= 10;
	order += tsk_gettid();
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

int prev_task = 0;
TM_TICK time_start_of_test = {0, 0};

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
	
	ret_val = tsk_create(&(g_tasks[2]), task2, MEDIUM, 0x200); /*create a user task */
  strcpy(g_ae_xtest.msg, "task1: creatd task2");
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  process_sub_result(test_id, (*p_index)++, sub_result);

	TIMEVAL* p_tv = mem_alloc(sizeof(TIMEVAL));
	// 5.2130001s is not a multiple of 500us
	p_tv->sec = 5;
	p_tv->usec = 2130001;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_ERR) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task1: Try to updated to real-time with an invalid period and fail");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);
	
	
	p_tv = mem_alloc(sizeof(TIMEVAL));
	p_tv->sec = 5;
	p_tv->usec = 0;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task1: Udated to RT");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);
	printf("task1: DELAY for 2 * 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < 2 * DELAY; x++); // some artifical delay 
	
	printf("task1: Suspending. x1\r\n");
	ret_val = rt_tsk_susp();
	append_to_order();
  sub_result = ret_val == RTX_OK;
  strcpy(g_ae_xtest.msg, "task1: Returned x1");
  process_sub_result(test_id, (*p_index)++, sub_result);
	printf("task1: DELAY for 2 * 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < 2 * DELAY; x++); // some artifical delay 


	printf("task1: Suspending x2\r\n");
	ret_val = rt_tsk_susp();
	append_to_order();
  sub_result = ret_val == RTX_OK;
  strcpy(g_ae_xtest.msg, "task1: Returned x2");
  process_sub_result(test_id, (*p_index)++, sub_result);
	printf("task1: DELAY for 2 * 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < 2 * DELAY; x++); // some artifical delay 
	
	
	printf("task1: Suspending x3 \r\n");
	ret_val = rt_tsk_susp();
	append_to_order();
  sub_result = ret_val == RTX_OK;
  strcpy(g_ae_xtest.msg, "task1: Returned x3");
  process_sub_result(test_id, (*p_index)++, sub_result);
	printf("task1: DELAY for 2 * 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < 2 * DELAY; x++); // some artifical delay 
	
	printf ("expected order = %d, actual order = %d \r\n", proper_order, order);
	strcpy(g_ae_xtest.msg, "Checking final order.");
	sub_result = (proper_order == order);
	process_sub_result(test_id, (*p_index)++, sub_result);
	
  printf("%s: TID = %d, task1: exiting \r\n", PREFIX_LOG, tid);
	test_exit();
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
	p_tv->sec = 3;
	p_tv->usec = 0;
	ret_val = rt_tsk_set(p_tv);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task2: Updated to real-time with period 3s");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(p_tv);
	printf("task1: DELAY for 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < DELAY; x++); // some artifical delay 
	
	printf("task2: Suspending x1 \r\n");
	ret_val = rt_tsk_susp();
	append_to_order();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task2: Returning x1");
  process_sub_result(test_id, (*p_index)++, sub_result);
	printf("task1: DELAY for 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < DELAY; x++); // some artifical delay 
	
	printf("task2: Suspending x2 \r\n");
	ret_val = rt_tsk_susp();
	append_to_order();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task2: Returning x2");
  process_sub_result(test_id, (*p_index)++, sub_result);
	printf("task1: DELAY for 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < DELAY; x++); // some artifical delay 

	printf("task2: Suspending x3 \r\n");
	ret_val = rt_tsk_susp();
	append_to_order();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task2: Returning x3");
  process_sub_result(test_id, (*p_index)++, sub_result);
	printf("task1: DELAY for 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < DELAY; x++); // some artifical delay 

	printf("task2: Suspending x4 \r\n");
	ret_val = rt_tsk_susp();
	append_to_order();
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task2: Returning x4");
  process_sub_result(test_id, (*p_index)++, sub_result);
	printf("task1: DELAY for 10 ^ 8 instructions \r\n");
	for ( int x = 0; x < DELAY; x++); // some artifical delay 

	printf("%s: TID = %d, task2: exited \r\n", PREFIX_LOG2, tid);
  tsk_exit();
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
