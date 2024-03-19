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
    
#define NUM_TESTS       1       // number of tests
#define NUM_INIT_TASKS  2       // number of tasks during initialization
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];
const char   PREFIX[]      = "G33-TS3";
const char   PREFIX_LOG[]  = "G33-TS3-LOG";
const char   PREFIX_LOG2[] = "G33-TS3-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];

void test1 (void);

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = HIGH + i;
        tasks[i].priv = 1;
    }
    tasks[0].priv  = 1;
    tasks[0].ptask = &priv_task1;
    tasks[1].priv  = 0;
    tasks[1].ptask = &task1;
    
    init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
    g_ae_xtest.test_id = 0;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests = NUM_TESTS;
    g_ae_xtest.num_tests_run = 0;
    
    for ( int i = 0; i< NUM_TESTS; i++ ) {
        g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
        g_tsk_cases[i].p_ae_case->results  = 0x0;
        g_tsk_cases[i].p_ae_case->test_id  = i;
        g_tsk_cases[i].p_ae_case->num_bits = 0;
        g_tsk_cases[i].pos = 0;  // first avaiable slot to write exec seq tid
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
    g_tsk_cases[test_id].p_ae_case->num_bits = 26;  
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
    g_tsk_cases[test_id].len = 0;       // N/A for this test
    g_tsk_cases[test_id].pos_expt = 0;  // N/A for this test
    
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
    (*p_pos) = (*p_pos)%len;  // preventing out of array bound
    return RTX_OK;
}

/**************************************************************************//**
 * @brief       a task that prints AAAAA, BBBBB on each line.
 *              it yields the cpu after each line
 *****************************************************************************/

/**************************************************************************//**
 * @brief:      a task that prints 00000, 11111 and 22222 on each line.
 *              It yields the cpu every after each line is printed.
 *****************************************************************************/

void priv_task1(void)
{
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
		int ret_val = 0;
	
    gen_req0( test_id );
	
		update_exec_seq(0, tsk_gettid());
	
    ///////// creating max number of tasks /////////
        *p_index = 0;
	
		task_t tasksA[8];
	
        sub_result = 1;
		for( size_t i = 0; i < 7; i++ )
        {
				ret_val = tsk_create(&tasksA[i], &task1, HIGH, 0x100);  /*create a user task */
				sub_result &= (ret_val == RTX_OK);
		}
        strcpy(g_ae_xtest.msg, "attempting to create a HIGH prio task that runs test1 function");
        process_sub_result(test_id, (*p_index)++, sub_result);
		
		///////// seeing how many tasks there are + tsk_LS /////////
		
		task_t buf[10];
		
		strcpy(g_ae_xtest.msg, "testing tsk_ls to check if we created the max number of tasks");
		ret_val = tsk_ls(buf, 15);
		sub_result = (ret_val == MAX_TASKS);
		process_sub_result(test_id, (*p_index)++, sub_result);
		

		///////// creating max number of tasks + 1th task /////////

        strcpy(g_ae_xtest.msg, "attemping to create a HIGH prio task that runs test1 function - SHOULD EXCEED MAX NUM OF TASKS");
        ret_val = tsk_create(&tasksA[8], &task1, HIGH, 0x200);  /*create a user task */
        sub_result = (ret_val != RTX_OK);
        process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// setting prio of task_2 to HIGH + tsk_get /////////
		RTX_TASK_INFO buffer;
        
		
        strcpy(g_ae_xtest.msg, "setting prio of task[2] -> HIGH, and also testing tsk_get to verify that the prio of task[2] is HIGH");
		ret_val = tsk_set_prio((task_t) 2, HIGH);
		if (tsk_get((task_t) 2, &buffer) != RTX_OK){
			ret_val = 0;
		}
		sub_result = (ret_val == RTX_OK && (buffer.tid == 2 && buffer.prio == HIGH));
        process_sub_result(test_id, (*p_index)++, sub_result);
        
    
		
		
		///////// YIELDING TASK#1 /////////
		
        strcpy(g_ae_xtest.msg, "task[1] yielded a while ago, we are back at task[1] now. Will tsk_get verify this?");
		ret_val = tsk_yield();
		tsk_get(tsk_gettid(), &buffer);
		sub_result = (ret_val == RTX_OK && buffer.tid == 1);    // ik using tsk_get is a little useless, but it's also a test
        process_sub_result(test_id, (*p_index)++, sub_result);
		
		update_exec_seq(0, tsk_gettid());
		
		
		///////// RETURNING TO TASK#1 - checking if it's the only task that is running /////////
		
		strcpy(g_ae_xtest.msg, "testing tsk_ls to check if task[1] is the only task running right now");
		ret_val = tsk_ls(buf, 15);
		sub_result = (ret_val == 2);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// MAKING MAX TASKS AGAIN /////////
		
		task_t tasksB[9];

        strcpy(g_ae_xtest.msg, "creating a HIGH prio task that runs test1 function");
        sub_result = 1;
		for( size_t i = 0; i < 8; i++ )
        {
			ret_val = tsk_create(&tasksB[i], &task1, HIGH, 0x100);  /*create a user task */
			sub_result &= (ret_val == RTX_OK);
		}
		process_sub_result(test_id, (*p_index)++, sub_result);
		///////// MAKING MAX + 1th task /////////
		
        strcpy(g_ae_xtest.msg, "attemping to create a LOWEST prio task that runs test1 function - SHOULD EXCEED MAX NUM OF TASKS");
        ret_val = tsk_create(&tasksB[9], &task1, LOWEST, 0x200);  /*create a user task */
        sub_result = (ret_val != RTX_OK);
        process_sub_result(test_id, (*p_index)++, sub_result);
		
		///////// checking if we are at max tasks using ls /////////
		
		strcpy(g_ae_xtest.msg, "testing tsk_ls to check if we created the max number of tasks");
		ret_val = tsk_ls(buf, 15);
		sub_result = (ret_val == MAX_TASKS);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// SET PRIO WITH INVALID PRIO /////////
		
		strcpy(g_ae_xtest.msg, "priveleged task attempting to set prio of a task, passing invalid prio");
		ret_val = tsk_set_prio((task_t) 5, 42);
		sub_result = (ret_val != RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		///////// YIELDING TASK#1 AGAIN/////////
		
        strcpy(g_ae_xtest.msg, "task[1] yielded a while ago, we are back at task[1] now. Will tsk_get verify this?");
		ret_val = tsk_yield();
		tsk_get(tsk_gettid(), &buffer);
		sub_result = (ret_val == RTX_OK && buffer.tid == 1);    // ik using tsk_get is a little useless, but it's also a test
        process_sub_result(test_id, (*p_index)++, sub_result);
		
		update_exec_seq(0, tsk_gettid());
	
		///////// SET PRIO OF TASK THAT DOESNT EXIST /////////
		
        strcpy(g_ae_xtest.msg, "setting prio of task that doesn't exist");
		ret_val = tsk_set_prio((task_t) 5, MEDIUM);
		sub_result = (ret_val == RTX_OK);
        process_sub_result(test_id, (*p_index)++, sub_result);
            
		///////// RETURNING TO TASK#1 - checking if it's the only task that is running /////////
		
		strcpy(g_ae_xtest.msg, "testing tsk_ls to check if task[1] is the only task running right now");
		ret_val = tsk_ls(buf, 15);
		sub_result = (ret_val == 2);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		///////// create 2 tasks- lowest and medium prio /////////
		
		task_t tasksC[2];
		strcpy(g_ae_xtest.msg, "creating a LOWEST prio task that runs task3 function");
		ret_val = tsk_create(&tasksC[0], &task3, LOWEST, 0x200);  /*create a user task */
		sub_result = (ret_val == RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		strcpy(g_ae_xtest.msg, "creating a MEDIUM prio task that runs task2 function");
		ret_val = tsk_create(&tasksC[1], &task2, MEDIUM, 0x200);  /*create a user task */
		sub_result = (ret_val == RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);

		///////// EXITING TASK 1 /////////
		
		tsk_exit();
}


void task1(void)
{
    update_exec_seq(0, tsk_gettid());
    tsk_exit();
}


/**
 * @brief: a dummy task2
 */
void task2(void)
{
    task_t tid = tsk_gettid();
    int    test_id = 0;
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;
		int 	 ret_val = 0;
    
   ///////// checking that we are in task[2] /////////
	
		printf("%s: TID = %d, task2: entering \r\n", PREFIX_LOG2, tid);
	
		update_exec_seq(0, tsk_gettid());
	
		///////// check current # of tasks [should equal to 3] /////////
		
		size_t count_new = 10;
		task_t buf_new;
		
		strcpy(g_ae_xtest.msg, "checking current # of tasks, should be 3");
		sub_result = (tsk_ls(&buf_new, count_new) == 3);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// CHECKING CURRENT TASK ID /////////
	
		strcpy(g_ae_xtest.msg, "current t_id should == task[3]");
		sub_result = (tsk_gettid() == (task_t) 3);
		process_sub_result(test_id, (*p_index)++, sub_result);
	
		///////// check current # of tasks [should equal to 3] /////////
		
		size_t count = 10;
		task_t buf[10];
		
		strcpy(g_ae_xtest.msg, "checking current # of tasks, should be 3");
		sub_result = (tsk_ls(buf, count) == 3);
		process_sub_result(test_id, (*p_index)++, sub_result);
	
		///////// SET PRIO BUT NOT PRIVELEGED /////////
		
    
        strcpy(g_ae_xtest.msg, "unprivileged task setting prio of another unpriveleged task - should fail");
		ret_val = tsk_set_prio((task_t) 2, LOW);
		sub_result = (ret_val == RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		///////// GET INFO WITH BAD BUFFER /////////
		
		strcpy(g_ae_xtest.msg, "getinfo with bad buffer - should fail");
		ret_val = tsk_get((task_t) 3, NULL);
		sub_result = (ret_val != RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// YIELD CURRENT TASK - NOTHING SHOULD HAPPEN /////////

		
        strcpy(g_ae_xtest.msg, "task[3] will yield - nothing should happen");
		ret_val = tsk_yield();
		sub_result = (ret_val == RTX_OK && tsk_gettid() == 3) ? 1 : 0;    // ik using tsk_get is a little useless, but it's also a test
        process_sub_result(test_id, (*p_index)++, sub_result);
		
		update_exec_seq(0, tsk_gettid());
		
		///////// EXITING TASK /////////
    tsk_exit();
}

/**
 * @brief: a dummy task3
 */
void task3(void)
{
    int    test_id = 0;
    task_t tid = tsk_gettid();
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;
		int		 ret_val = 0;
	
		RTX_TASK_INFO buffer;
    
    printf("%s: TID = %d, task3: entering \r\n", PREFIX_LOG2, tid);
	
		update_exec_seq(0, tsk_gettid());
    
		///////// GETINFO ON DORMANT TASK /////////
		strcpy(g_ae_xtest.msg, "getinfo on dormant task - nothing should happen");
		ret_val = tsk_get((task_t) 6, &buffer);
		sub_result = (ret_val == RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// CREATE A TASK WITH BAD PRIO /////////
		task_t test_task;
		
		strcpy(g_ae_xtest.msg, "creating a INVALID prio task that runs task1 function");
		ret_val = tsk_create(&test_task, &task1, 42, 0x100);  /*create a user task */
		sub_result = (ret_val != RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// tsk_LS with count = 0 /////////
		
		size_t count = 0;
		task_t *buf;
		
		strcpy(g_ae_xtest.msg, "calling tsk_LS with a count = 0");
		ret_val = tsk_ls(buf, count);  /*create a user task */
		sub_result = (ret_val != RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// tsk_LS with buffer = NULL /////////
		
		count = 0;
		
		strcpy(g_ae_xtest.msg, "calling tsk_ls with a NULL buffer");
		ret_val = tsk_ls(NULL, count);  /*create a user task */
		sub_result = (ret_val != RTX_OK);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		
		///////// check current # of tasks [should equal to 3] /////////
		
		size_t count_new = 10;
		task_t buf_new [10];
	
		strcpy(g_ae_xtest.msg, "checking current # of tasks, should be 2");
		sub_result = (tsk_ls(buf_new, count_new) == 2);
		process_sub_result(test_id, (*p_index)++, sub_result);
		
		///////// print tids of current tasks /////////
		
		test_exit();
		
    ///////// EXITING TASK /////////
    tsk_exit();
}


/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
