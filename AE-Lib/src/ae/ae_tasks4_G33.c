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

#define NUM_TESTS 2      // number of tests
#define NUM_INIT_TASKS 1 // number of tasks during initialization
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT g_init_tasks[NUM_INIT_TASKS];
const char PREFIX[] = "G33-TS4";
const char PREFIX_LOG[] = "G33-TS4-LOG";
const char PREFIX_LOG2[] = "G33-TS4-LOG2";

AE_XTEST g_ae_xtest; // test data, re-use for each test

AE_CASE g_ae_cases[NUM_TESTS];
AE_CASE_TSK g_tsk_cases[NUM_TESTS];
task_t g_tasks[MAX_TASKS];
task_t m_tasks[MAX_TASKS];

void test1(void);

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
  g_tsk_cases[test_id].p_ae_case->num_bits = 2;
  g_tsk_cases[test_id].p_ae_case->results = 0;
  g_tsk_cases[test_id].p_ae_case->test_id = test_id;
  g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
  g_tsk_cases[test_id].pos_expt = 6;

  g_ae_xtest.test_id = test_id;
  g_ae_xtest.index = 0;
  g_ae_xtest.num_tests_run++;
}

void gen_req1(int test_id)
{
  //bits[0:3] pos check, bits[4:9] for exec order check
  g_tsk_cases[test_id].p_ae_case->num_bits = 14;
  g_tsk_cases[test_id].p_ae_case->results = 0;
  g_tsk_cases[test_id].p_ae_case->test_id = test_id;
  g_tsk_cases[test_id].len = 0;      // N/A for this test
  g_tsk_cases[test_id].pos_expt = 0; // N/A for this test

  update_ae_xtest(test_id);
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



void task1(void)
{
  int test_id = 0;
  U8 *p_index = &(g_ae_xtest.index);
  int sub_result = 0;
  int ret_val = 0;
  task_t tid = tsk_gettid();
  gen_req0(test_id);

  update_exec_seq(0, tid);

  printf("%s: TID = %d, task1: entering \r\n", PREFIX_LOG, tid);

  ret_val = tsk_create(&(g_tasks[2]), task2, HIGH, 0x200); /*create a user task */
  strcpy(g_ae_xtest.msg, "task1: creating task2 which receives message from task3");
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  process_sub_result(test_id, (*p_index)++, sub_result);


  ret_val = tsk_create(&(g_tasks[3]), task3, HIGH, 0x200);
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  strcpy(g_ae_xtest.msg, "task1: creating task3, to send messages to task2");
  process_sub_result(test_id, (*p_index)++, sub_result);

  printf("%s: TID = %d, task1: exiting \r\n", PREFIX_LOG, tid);

  tsk_exit();
}

void task2(void)
{
  gen_req1(1);

  task_t tid = tsk_gettid();
  int test_id = 1;
  U8 *p_index = &(g_ae_xtest.index);
  int sub_result = 0;
  int ret_val = 0;

  printf("%s: TID = %d, task2: entering \r\n", PREFIX_LOG, tid);

  char *buf1 = mem_alloc(x);
  sub_result = recv_msg(buf1, x) == RTX_ERR && errno == ENOENT;
  strcpy(g_ae_xtest.msg, "task2: Receiving message but fails to as mbx is not created");
  process_sub_result(test_id, (*p_index)++, sub_result);
  int mbx_num = mbx_ls(m_tasks, MAX_TASKS);
  int t_id = mbx_create(x + sizeof(int));
  sub_result = mbx_num + 1 == mbx_ls(m_tasks, MAX_TASKS); // i.e. a new mailbox has been created
  strcpy(g_ae_xtest.msg, "task2: Create mailbox");
  process_sub_result(test_id, (*p_index)++, sub_result);

  sub_result = recv_msg(buf1, x) == RTX_OK; // blocking recv to message		sent by task2
  strcpy(g_ae_xtest.msg, "task2: Recieving message from task3 and blocked until it gets it");
  process_sub_result(test_id, (*p_index)++, sub_result);


  sub_result = *(buf1 + 0x5) == DEFAULT &&
               *(buf1 + 0x0) == x &&
               *(buf1 + 0x4) == g_tasks[3];
  strcpy(g_ae_xtest.msg, "task2: Checking if the message is accurate");
  process_sub_result(test_id, (*p_index)++, sub_result);
  mem_dealloc(buf1);

  buf1 = mem_alloc(x + sizeof(100));
  sub_result = recv_msg(buf1, x + sizeof(100)) == RTX_OK;
  strcpy(g_ae_xtest.msg, "task2: Receiving message from task3 to be manipulated");
  process_sub_result(test_id, (*p_index)++, sub_result);

  ret_val = *(buf1 + 0x6);
  sub_result = ret_val == 10;
  strcpy(g_ae_xtest.msg, "task2: Checking validity of data");
  process_sub_result(test_id, (*p_index)++, sub_result);

  *(buf1 + 0x6) = *(buf1 + 0x6) * 4;
  sub_result = send_msg_nb(g_tasks[3], buf1) == RTX_OK;
  strcpy(g_ae_xtest.msg, "task2: Send to task3 with the modified data");
  process_sub_result(test_id, (*p_index)++, sub_result);

  mem_dealloc(buf1);

  *(buf1 + 0x6) = *(buf1 + 0x6) * 4;
  sub_result = tsk_yield() == RTX_OK;
  strcpy(g_ae_xtest.msg, "task2: Yielded to task3, now back to task2");
  process_sub_result(test_id, (*p_index)++, sub_result);

  printf("%s: TID = %d, task2: exiting \r\n", PREFIX_LOG, tid);

	test_exit();
  tsk_exit();
}

void task3(void)
{
  task_t tid = tsk_gettid();
  int test_id = 1;
  U8 *p_index = &(g_ae_xtest.index);
  int sub_result = 0;
  int ret_val = 0;

  printf("%s: TID = %d, task3: entering \r\n", PREFIX_LOG2, tid);

  char *buf1 = mem_alloc(x);
  *(buf1 + 0x5) = DEFAULT;
  *(buf1 + 0x0) = x;
  *(buf1 + 0x4) = tid;
  ret_val = send_msg(g_tasks[2], buf1); // blocking send to task2
  sub_result = (ret_val == RTX_OK) ? 1 : 0;
  sprintf(g_ae_xtest.msg, "task3: Send_msg to tid(%d)", g_tasks[2]);
  process_sub_result(test_id, (*p_index)++, sub_result);
  mem_dealloc(buf1);

  sprintf(g_ae_xtest.msg, "task3: send_msg_nb to tid(%d) which overflows memory", g_tasks[2]);
  buf1 = mem_alloc(3 * x);
  *(buf1 + 0x0) = 3 * x;
  *(buf1 + 0x4) = tid;
  *(buf1 + 0x5) = DEFAULT;
  ret_val = send_msg_nb(g_tasks[2], buf1); // no-blocking send to task2
  sub_result = ret_val == RTX_ERR && errno == EMSGSIZE;
  process_sub_result(test_id, (*p_index)++, sub_result);
  mem_dealloc(buf1);

  int mbx_num = mbx_ls(m_tasks, MAX_TASKS);
  int t_id = mbx_create(x + sizeof(int));
  sub_result = mbx_num + 1 == mbx_ls(m_tasks, MAX_TASKS); // i.e. a new mailbox has been created
  strcpy(g_ae_xtest.msg, "task3: Create mailbox");
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(buf1);
		
  buf1 = mem_alloc(x + sizeof(int));
  *(buf1 + 0x0) = x + sizeof(int);
  *(buf1 + 0x4) = tid;
  *(buf1 + 0x5) = DEFAULT;
  *(buf1 + 0x6) = 10; // Data for message
	ret_val = send_msg(g_tasks[2], buf1);
  sub_result = ret_val == RTX_OK;
  sprintf(g_ae_xtest.msg, "task3: sending a message to TID=%d to be manipulated", g_tasks[2]);
  process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(buf1);

  strcpy(g_ae_xtest.msg, "task3: Check validity of data");
	ret_val = recv_msg(buf1, x + sizeof(int)) == RTX_OK;
  process_sub_result(test_id, (*p_index)++, *(buf1 + 0x6) == 40);

	mbx_get(g_tasks[2]) == x + sizeof(int);
	strcpy(g_ae_xtest.msg, "task3: Check freespace of mbx is equal to the max size of mbx");
	sub_result = process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc(buf1);
	
  printf("%s: TID = %d, task3: exited \r\n", PREFIX_LOG2, tid);
  tsk_exit();
}


//}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
