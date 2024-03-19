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
const char PREFIX[] = "G33-TS7";
const char PREFIX_LOG[] = "G33-TS7-LOG";
const char PREFIX_LOG2[] = "G33-TS7-LOG2";

AE_XTEST g_ae_xtest; // test data, re-use for each test

AE_CASE g_ae_cases[NUM_TESTS];
AE_CASE_TSK g_tsk_cases[NUM_TESTS];
task_t g_tasks[MAX_TASKS];

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
	tasks[0].u_stack_size = PROC_STACK_SIZE;
	tasks[0].priv = 1;
	tasks[0].prio = MEDIUM;
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
	g_tsk_cases[test_id].p_ae_case->num_bits = 15;
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

void task1(void)
{
	int test_id = 0;
    U8  *p_index    = &(g_ae_xtest.index);
	int sub_result  = 0;
	int ret_val = 0;
	gen_req0( test_id );

	strcpy(g_ae_xtest.msg, "enters tasks1 and creates a mail box for itself");
	ret_val = mbx_create(256);
	sub_result = ret_val != RTX_ERR;
	process_sub_result(test_id, (*p_index)++, sub_result);

	char * reg_in = mem_alloc(MSG_HDR_SIZE + 1);
	if(reg_in == NULL){
		printf("task1: mem_alloc failed to give buffer to send to register. \n\r");
		test_exit();
	}

	RTX_MSG_HDR * header = (RTX_MSG_HDR *) reg_in;
	header->length = MSG_HDR_SIZE + 1;
	header->sender_tid = tsk_gettid();
	header->type = KCD_REG;
	reg_in [MSG_HDR_SIZE] = 'Q';
	
	ret_val = send_msg(TID_KCD, reg_in);
	strcpy(g_ae_xtest.msg, "task1 tries to register \'Q\' to KCD.");
	sub_result = (ret_val == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc (reg_in);
	
	char * recv_buf = mem_alloc(256);
	if(recv_buf == NULL){
		printf("task1: mem_alloc failed to give buffer to receive. \n\r");
		test_exit();
	}

	task_t task2_id;
	strcpy(g_ae_xtest.msg, "task1 creates task2");
	sub_result = (tsk_create(&task2_id, &task2, MEDIUM, PROC_STACK_SIZE) == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);
	
	//should receive from kcd
	ret_val = recv_msg(recv_buf, 256);
	RTX_MSG_HDR * recv_header = (RTX_MSG_HDR *) recv_buf;
	strcpy(g_ae_xtest.msg, "task1 recieved a message.");
	sub_result = (ret_val == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);
	*(recv_buf + recv_header->length) = '\0';
	printf("----%s----\n\r", recv_buf + MSG_HDR_SIZE);
	
	strcpy(g_ae_xtest.msg, "task1 sends received message to con_display,");
	char processed_message [] = " *** cmd processed by task1 ***";
	strcpy(recv_buf + recv_header->length, processed_message);
	recv_header->length += sizeof(processed_message);
	recv_header->sender_tid = tsk_gettid();
	recv_header->type = DISPLAY;

	sub_result = (send_msg(TID_CON, recv_buf) == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);

	//wait for task2 to send message
	ret_val = recv_msg(recv_buf, 256);
	strcpy(g_ae_xtest.msg, "task1 recieved a message.");
	sub_result = (ret_val == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);
	*(recv_buf + recv_header->length) = '\0';
	printf("----%s----\n\r", recv_buf + MSG_HDR_SIZE);

	strcpy(g_ae_xtest.msg, "task1's previous message should come from task2");
	sub_result = recv_header->sender_tid == task2_id;
	process_sub_result(test_id, (*p_index)++, sub_result);
	test_exit();
	tsk_exit();
}

void task2 (void){
    int    test_id = 0;
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;
	int 	 ret_val = 0;
	char processed_message [] = " *** cmd processed by task2 ***";

	strcpy(g_ae_xtest.msg, "enters tasks2 and creates a mail box for itself");
	ret_val = mbx_create(256);
	sub_result = ret_val != RTX_ERR;
	process_sub_result(test_id, (*p_index)++, sub_result);

	char * reg_in = mem_alloc(MSG_HDR_SIZE + 1);
	if(reg_in == NULL){
		printf("task2: mem_alloc failed to give buffer to send to register. \n\r");
		test_exit();
	}

	RTX_MSG_HDR * header = (RTX_MSG_HDR *) reg_in;
	header->length = MSG_HDR_SIZE + 1;
	header->sender_tid = tsk_gettid();
	header->type = KCD_REG;
	reg_in [MSG_HDR_SIZE] = 'P';
	
	ret_val = send_msg(TID_KCD, reg_in);
	strcpy(g_ae_xtest.msg, "task2 tries to register \'P\' to KCD.");
	sub_result = (ret_val == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);

	char * recv_buf = mem_alloc(256);
	if(recv_buf == NULL){
		printf("task2: mem_alloc failed to give buffer to receive. \n\r");
		test_exit();
	}
	
	ret_val = recv_msg(recv_buf, 256);
	strcpy(g_ae_xtest.msg, "task2 recieved first message.");
	sub_result = (ret_val == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);
	RTX_MSG_HDR * recv_header = (RTX_MSG_HDR *) recv_buf;
	*(recv_buf + recv_header->length) = '\0';
	printf("----%s----\n\r", recv_buf + MSG_HDR_SIZE);

	strcpy(g_ae_xtest.msg, "task2 sends first received message to con_display,");
	strcpy(recv_buf + recv_header->length, processed_message);
	recv_header->length += sizeof(processed_message);
	recv_header->sender_tid = tsk_gettid();
	recv_header->type = DISPLAY;

	sub_result = (send_msg(TID_CON, recv_buf) == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);

	reg_in [MSG_HDR_SIZE] = 'Q';
	strcpy(g_ae_xtest.msg, "task2 tries to register \'Q\' (The original cmd for task 1) to KCD.");
	ret_val = send_msg(TID_KCD, reg_in);
	sub_result = (ret_val == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);
	mem_dealloc (reg_in);

	ret_val = recv_msg(recv_buf, 256);
	strcpy(g_ae_xtest.msg, "task2 recieved the second message.");
	sub_result = (ret_val == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);
	*(recv_buf + recv_header->length) = '\0';
	printf("----%s----\n\r", recv_buf + MSG_HDR_SIZE);

	strcpy(g_ae_xtest.msg, "task2 sends the second message to con_display,");
	strcpy(recv_buf + recv_header->length, processed_message);
	recv_header->length += sizeof(processed_message);
	recv_header->sender_tid = tsk_gettid();
	recv_header->type = DISPLAY;

	sub_result = (send_msg(TID_CON, recv_buf) == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);

	char message_to_task1 [] = "I'm task2, unblocking task1.";
	strcpy(recv_buf + MSG_HDR_SIZE, message_to_task1);
	recv_header->length = MSG_HDR_SIZE + sizeof(message_to_task1);
	recv_header->sender_tid = tsk_gettid();
	recv_header->type = DEFAULT;

	sub_result = (send_msg_nb(1, recv_buf) == RTX_OK);
	process_sub_result(test_id, (*p_index)++, sub_result);

	mem_dealloc(recv_buf);

	tsk_exit();
}
