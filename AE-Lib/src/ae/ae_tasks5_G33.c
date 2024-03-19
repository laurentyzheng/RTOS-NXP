#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

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

int x = sizeof(RTX_MSG_HDR);

int test_mbx_create()
{
  int err_on_overflow_size = mbx_create(1000000) == RTX_ERR && errno == ENOMEM;
  int err_on_min_size = mbx_create(MIN_MSG_SIZE - 1) == RTX_ERR && errno == EINVAL;
  return err_on_overflow_size && err_on_min_size;
}

void task1(void)
{
 	task_t tid = tsk_gettid();
	int    test_id = 0;
	U8     *p_index    = &(g_ae_xtest.index);
	int    sub_result  = 0;
	int ret_val = 0;
  gen_req0(test_id);

  update_exec_seq(0, tid);
	printf("%s: TID = %d, task1: entering \r\n", PREFIX_LOG, tid);

 sub_result = test_mbx_create();
 strcpy(g_ae_xtest.msg, "task1: testing mbx_create");
 process_sub_result(test_id, (*p_index)++, sub_result);

	strcpy(g_ae_xtest.msg, "task1: Sending to itself without mailbox created");
	RTX_MSG_HDR *buf1 = mem_alloc(x);
	buf1->length = x;
	buf1->type = DEFAULT;
	buf1->sender_tid = tid;
	sub_result = send_msg_nb(tid, buf1) == RTX_ERR && send_msg(g_tasks[4], buf1) == RTX_ERR;
	process_sub_result(test_id, (*p_index)++, sub_result);


	strcpy(g_ae_xtest.msg, "task1: Create mailbox and see if it exists");
	int curr_mbx = mbx_ls(g_tasks, MAX_TASKS);
	int t_id = mbx_create(MIN_MSG_SIZE);
	sub_result = curr_mbx + 1 == mbx_ls(g_tasks, MAX_TASKS);
	process_sub_result(test_id, (*p_index)++, sub_result);


	strcpy(g_ae_xtest.msg, "task1: Sending to itself with mailbox created");
	sub_result = send_msg_nb(tid, buf1) == RTX_ERR && send_msg(g_tasks[4], buf1) == RTX_ERR;
	process_sub_result(test_id, (*p_index)++, sub_result);

  printf("%s: TID = %d, task1: exiting \r\n", PREFIX_LOG, tid);

  test_exit();
  tsk_exit();
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
