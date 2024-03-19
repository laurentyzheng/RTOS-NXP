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

#define NUM_TESTS 1		 // number of tests
#define NUM_INIT_TASKS 1 // number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT g_init_tasks[NUM_INIT_TASKS];
const char PREFIX[] = "G33-TS6";
const char PREFIX_LOG[] = "G33-TS6-LOG";
const char PREFIX_LOG2[] = "G33-TS6-LOG2";

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
	g_tsk_cases[test_id].p_ae_case->num_bits = 32;
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
	g_tsk_cases[test_id].p_ae_case->num_bits = 10;
	g_tsk_cases[test_id].p_ae_case->results = 0;
	g_tsk_cases[test_id].p_ae_case->test_id = test_id;
	g_tsk_cases[test_id].len = 0;	   // N/A for this test
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

/**************************************************************************/ /**
 * @brief       a task that prints AAAAA, BBBBB on each line.
 *              it yields the cpu after each line
 *****************************************************************************/

/**************************************************************************/ /**
 * @brief:      a task that prints 00000, 11111 and 22222 on each line.
 *              It yields the cpu every after each line is printed.
 *****************************************************************************/

int test_mbx_create()
{
	int err_on_overflow_size = mbx_create(1000000) == RTX_ERR && errno == ENOMEM;
	int err_on_min_size = mbx_create(MIN_MSG_SIZE - 1) == RTX_ERR && errno == EINVAL;
	return err_on_overflow_size && err_on_min_size;
}

int x = sizeof(RTX_MSG_HDR);

void task1(void)
{
	int test_id = 0;
	task_t tid = tsk_gettid();
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	int ret_val = 0;
	task_t curr_id = tsk_gettid();
	gen_req0(test_id);
	
	g_tasks[1] = 1;

	update_exec_seq(0, tsk_gettid());

	*p_index = 0;

	printf("%s: TID = %u, task1: entering\r\n", PREFIX, tid);

	int mbx_num = mbx_ls(m_tasks, MAX_TASKS);
	int t_id = mbx_create(3 * x);
	sub_result = mbx_num + 1 == mbx_ls(m_tasks, MAX_TASKS);
	strcpy(g_ae_xtest.msg, "task1: Create mailbox");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	
	ret_val = tsk_create(&g_tasks[2], task2, HIGH, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task1: creating task2");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	
	char *buf1 = mem_alloc(2*sizeof(RTX_MSG_HDR));
	sub_result = recv_msg(buf1, 2*sizeof(RTX_MSG_HDR)) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task1: Blocking receive from task2 TID(2)");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	sub_result = tsk_set_prio(g_tasks[2], MEDIUM) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task1: Changing prio of task2 TID(%u) to MEDIUM", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	ret_val = tsk_create(&g_tasks[3], task3, HIGH, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task1: creating task3");

	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	
	printf("%s: TID = %u, task1: yielding to task 3 \r\n", PREFIX_LOG, tid);
	ret_val = tsk_yield();
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task1: yielded to task 3 and now has resumed");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	RTX_MSG_HDR *buf2 = mem_alloc(x);
	buf2->length = x;
	buf2->sender_tid = tid;
	buf2->type = DEFAULT;
	sub_result = send_msg(g_tasks[2], buf2) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task1: Blocking send to task2 TID(%u), will not be received and switch to task 2", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf2);

	
	printf("%s: TID = %d, task1: resume \r\n", PREFIX, tid);
	
	sub_result = tsk_set_prio(g_tasks[1], MEDIUM) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task1: Changing prio of itself TID(%u) to medium", g_tasks[1]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	printf("%s: TID = %d, task1: exited \r\n", PREFIX, tid);

	tsk_exit();
}

void task2(void)
{
	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	int ret_val = 0;

	printf("%s: TID = %u, task2: entering\r\n", PREFIX_LOG, tid);
	
	sub_result = tsk_set_prio(g_tasks[1], LOWEST) == RTX_ERR;
	sprintf(g_ae_xtest.msg, "task2: Changing prio of task1 TID(%u) to MEDIUM - shoould fail since TID(1) is privileged", g_tasks[1]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	int mbx_num = mbx_ls(m_tasks, MAX_TASKS);
	int t_id = mbx_create(5 * x);
	sub_result = mbx_num + 1 == mbx_ls(m_tasks, MAX_TASKS);
	strcpy(g_ae_xtest.msg, "task2: Create mailbox");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	RTX_MSG_HDR *buf1 = mem_alloc(4 * x);
	buf1->length = 4 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	ret_val = send_msg(g_tasks[1], buf1);
	sub_result = (ret_val == RTX_ERR) ? 1 : 0;
	sprintf(g_ae_xtest.msg, "task2: send to task 1 a message that is 4x, fails");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(3 * x + 1);
	buf1->length = 3 * x + 1;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	ret_val = send_msg(g_tasks[1], buf1);
	sub_result = (ret_val == RTX_ERR) ? 1 : 0;
	sprintf(g_ae_xtest.msg, "task2: send to task 1 a message that is 3x + 1, fails");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(2 * x);
	buf1->length = 2 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	ret_val = send_msg(g_tasks[1], buf1);
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	sprintf(g_ae_xtest.msg, "task2: send to task 1 a message that is 2x, passes");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	printf("%s: TID = %u, task2: yielding to task 1\r\n", PREFIX_LOG, tid);
	ret_val = tsk_yield();
	sub_result = (ret_val == RTX_OK) ? 1 : 0; // we come here after blk send x from task 1
	strcpy(g_ae_xtest.msg, "task2: yielded to task 1 and has now resumed");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	buf1 = mem_alloc(2 * x);
	sub_result = recv_msg(buf1, (2 * x)) == RTX_OK; 
	strcpy(g_ae_xtest.msg, "task2: Blocking receive 2x, nothing gets popped from the waiting list");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	sub_result = tsk_set_prio(g_tasks[3], MEDIUM) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task2: Changing prio of task3 TID(%u) to MEDIUM", g_tasks[3]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	printf("returning to task 2 after task 1 set its own prio to MEDIUM\r\n");
	
	buf1 = mem_alloc(2 * x);
	sub_result = recv_msg(buf1, (2 * x)) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive 2x again"); // Task 2 should continue after this
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(x);
	sub_result = recv_msg(buf1, x) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive x");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(4 * x);
	sub_result = recv_msg(buf1, 4 * x) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive 4x");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	sub_result = mbx_get(g_tasks[2]) == 5 * x;
	strcpy(g_ae_xtest.msg, "task2: Checking mailbox is empty, it should be.");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	
	sub_result = tsk_set_prio(g_tasks[3], LOWEST) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task2: Changing prio of task3 TID(%u) to LOWEST", g_tasks[3]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	sub_result = tsk_set_prio(g_tasks[2], LOWEST) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task2: Changing prio of itself TID(%u) to LOWEST", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	sub_result = tsk_set_prio(g_tasks[5], LOWEST) == RTX_OK;
	sub_result = sub_result && tsk_set_prio(g_tasks[7], HIGH) == RTX_OK;
	sub_result = sub_result && tsk_set_prio(g_tasks[6], HIGH) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Changing prio of 5->LOWEST, 7->HIGHEST, 6->HIGHEST");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	buf1 = mem_alloc(2 * x);
	sub_result = recv_msg(buf1, (2 * x)) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive 2x");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(3 * x);
	sub_result = recv_msg(buf1, (3 * x)) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive 3x");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(5 * x);
	sub_result = recv_msg(buf1, (5 * x)) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive 5x, should only receive 4x");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(x);
	sub_result = recv_msg(buf1, (x)) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive x");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(2 * x);
	sub_result = recv_msg(buf1, (2 * x)) == RTX_OK;
	strcpy(g_ae_xtest.msg, "task2: Blocking receive 2x");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	printf("%s: TID = %d, task2: exited \r\n", PREFIX_LOG, tid);
	tsk_exit();
}

void task3(void)
{

	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	int ret_val = 0;

	printf("%s: TID = %u, task3: entering\r\n", PREFIX_LOG, tid);

	RTX_MSG_HDR *buf1 = mem_alloc(2 * x);
	buf1->length = 2 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[2], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task3: Blocking send to task2 TID(%u) of size 2x", 2);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	sub_result = send_msg(g_tasks[2], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task3: Blocking send to task2 TID(%u) of size 2x", 2);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(4 * x);
	buf1->length = 4 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[2], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task3: Blocking send to task2 TID(%u) of size 4x", 2);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	
	sub_result = tsk_set_prio(g_tasks[3], MEDIUM) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task3: Changing prio of itself TID(%u) to MEDIUM", 3);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	ret_val = tsk_create(&g_tasks[1], task4, MEDIUM, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task3: creating task4");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	ret_val = tsk_create(&g_tasks[4], task5, MEDIUM, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task3: creating task5");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	ret_val = tsk_create(&g_tasks[5], task6, MEDIUM, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task3: creating task6");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	ret_val = tsk_create(&g_tasks[6], task7, MEDIUM, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task3: creating task7");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	ret_val = tsk_create(&g_tasks[7], task8, MEDIUM, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task3: creating task8");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	ret_val = tsk_create(&g_tasks[8], task8, MEDIUM, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_ERR) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task3: creating task9 SHOULD FAIL");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	printf("%s: TID = %d, task3: exited \r\n", PREFIX, tid);
	tsk_exit();
}

void task4(void)
{
	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	int ret_val = 0;

	printf("%s: TID = %u, task4: entering\r\n", PREFIX_LOG, tid);

	ret_val = tsk_create(&g_tasks[9], task9, HIGH, 0x200); /*create a user task */
	sub_result = (ret_val == RTX_OK) ? 1 : 0;
	strcpy(g_ae_xtest.msg, "task4: created a new task9 (HIGH), back to task 4 (MEDIUM)");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	RTX_MSG_HDR *buf1 = mem_alloc(4 * x);
	buf1->length = 4 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[3], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task4: Blocking send to task3 TID(%u) of size 4x", g_tasks[3]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(2 * x + 1);
	buf1->length = 2 * x + 1;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[3], buf1) == RTX_ERR;
	sprintf(g_ae_xtest.msg, "task4: Blocking send to task3 TID(%u) of size 2x + 1 - SHOULD FAIL", g_tasks[3]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	printf("%s: TID = %d, task4: exited \r\n", PREFIX, tid);
	tsk_exit();
}

void task5(void)
{
	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	//int ret_val = 0;

	printf("%s: TID = %u, task5: entering\r\n", PREFIX_LOG, tid);

	sub_result = tsk_set_prio(tid, HIGH) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task5: Changing prio of itself TID(%u) to HIGH", tid);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	RTX_MSG_HDR *buf1 = mem_alloc(2 * x);
	buf1->length = 2 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[3], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task5: Blocking send to task3 TID(%u) of size 2x", g_tasks[3]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	sub_result = send_msg(g_tasks[3], buf1) == RTX_ERR;
	sprintf(g_ae_xtest.msg, "task5: Blocking send to task3 TID(%u) of size 2x SHOULD FAIL", g_tasks[3]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	printf("%s: TID = %d, task5: exited \r\n", PREFIX, tid);
	tsk_exit();
}

void task6(void)
{
	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	int ret_val = 0;

	printf("%s: TID = %u, task6: entering\r\n", PREFIX_LOG, tid);

	RTX_MSG_HDR *buf1 = mem_alloc(2 * x);
	buf1->length = 2 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[2], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task6: Blocking send to task2 TID(%u) of size 2x", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(3 * x);
	buf1->length = 3 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[2], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task6: Blocking send to task2 TID(%u) of size 3x", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	buf1 = mem_alloc(2 * x);
	buf1->length = 2 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	ret_val = send_msg(g_tasks[2], buf1);
	sub_result = ret_val == RTX_OK;
	sprintf(g_ae_xtest.msg, "task6: Blocking send to task2 TID(%u) of size 2x", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	// CHECK THE DEALLOCATION OF ALL
	// 1. MAILBOXES
	// 2. WAITING LISTS
	// 3. TASKS other than task 6
	
	printf("%s: TID = %d, task6: exited \r\n", PREFIX, tid);
	test_exit();
	tsk_exit();
}

void task7(void)
{
	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	//int ret_val = 0;

	printf("%s: TID = %u, task7: entering\r\n", PREFIX_LOG, tid);

	RTX_MSG_HDR *buf1 = mem_alloc(x);
	buf1->length = x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[2], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task7: Blocking send to task2 TID(%u) of size x", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	printf("%s: TID = %d, task7: exited \r\n", PREFIX, tid);
	tsk_exit();
}

void task8(void)
{
	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;
	//int ret_val = 0;

	printf("%s: TID = %u, task8: entering\r\n", PREFIX_LOG, tid);

	RTX_MSG_HDR *buf1 = mem_alloc(4 * x);
	buf1->length = 4 * x;
	buf1->sender_tid = tid;
	buf1->type = DEFAULT;
	sub_result = send_msg(g_tasks[2], buf1) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task8: Blocking send to task2 TID(%u) of size 4x", g_tasks[2]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}
	mem_dealloc(buf1);

	printf("%s: TID = %d, task8: exited \r\n", PREFIX, tid);
	tsk_exit();
}

void task9(void)
{
	task_t tid = tsk_gettid();
	int test_id = 0;
	U8 *p_index = &(g_ae_xtest.index);
	int sub_result = 0;

	printf("%s: TID = %u, task9: entering\r\n", PREFIX_LOG, tid);

	int mbx_num = mbx_ls(m_tasks, MAX_TASKS);
	int t_id = mbx_create(6 * x);
	sub_result = mbx_num + 1 == mbx_ls(m_tasks, MAX_TASKS);
	strcpy(g_ae_xtest.msg, "task9: Create mailbox");
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	sub_result = tsk_set_prio(g_tasks[9], LOWEST) == RTX_OK;
	sprintf(g_ae_xtest.msg, "task9: Changing prio of itself TID(%u) to LOWEST", g_tasks[9]);
	process_sub_result(test_id, (*p_index)++, sub_result);
	if (!sub_result)
	{
		test_exit();
	}

	printf("%s: TID = %d, task9: exited \r\n", PREFIX, tid);
	tsk_exit();
}
