
/*
 * The idea behind this test is to give a basic use case.
 * The flow should be as follows:
 * Task1 is called automatically.
 * Task1 calls test 0.
 * Test 0 creates tasks 2 and 3
 * Test 0 continually yields until tasks 2 and 3 are done
 * task 2 continually yields until task 3 is done
 * task 3 calculates the next fibb number then yields
 * task 3 stops this after 5 iterations, and then exits after setting the result.
 * task 2 prints the number and exits.
 * Test 0 cleans up and finishes.
 * 
 * 
 * TODO:
 * Add a list of the previous 3 tasks that ran.
 * When each task gets control returned to them, set push their task number to the top (and yeet the bottom).
 * Assert that the list has correct task orderring.
 * 
 */


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
#define NUM_INIT_TASKS  1       // number of tasks during initialization
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];
const char   PREFIX[]      = "G33-TS2";
const char   PREFIX_LOG[]  = "G33-TS2-LOG";
const char   PREFIX_LOG2[] = "G33-TS2-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];
int          last_tasks[10] = {0};

void push_last_task( int x )
{
    int iterator;
    for( iterator = 0; iterator < 9; iterator++ )
    {
        last_tasks[iterator] = last_tasks[iterator+1];
    }
    last_tasks[9] = x;
}

void set_ae_init_tasks( TASK_INIT **pp_tasks, int *p_num )
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks( TASK_INIT *tasks, int num )
{
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = MEDIUM;
        tasks[i].priv = 1;
    }
    tasks[0].priv  = 0;
    tasks[0].ptask = &task1;
    
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
    printf( "%s: START\r\n", PREFIX );
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
    g_tsk_cases[test_id].p_ae_case->num_bits = 21;
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 6;
    
    update_ae_xtest( test_id );
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

/* n is the task we're expected to be in */
int check_last_task0( int n )
{
    int result = 1;
    int iterator;
    //printf( "=== %d\r\n", n );
    for( iterator = 7; iterator < 10; iterator++ )
    {
        // 1 2 3
        // 2 3 1
        // 3 1 2
        // 
        int expected_num = ( ( n + iterator - 7 ) % 3 + 1 );
        result &= ( last_tasks[iterator] == expected_num );
        //printf( "%d %d\r\n", last_tasks[iterator], expected_num );
    }
    return result;
}

int task2_done = 0;
int task3_done = 0;
int test0_start( int test_id )
{
    task_t tsk1;
    task_t tsk2;
    int sub_result = RTX_OK;
    gen_req0( test_id );
    U8 *p_index = &(g_ae_xtest.index);
    
    printf( "%s: Adding tasks 2 and 3 \r\n", PREFIX );
    
    sub_result = tsk_create( &tsk1, task2, MEDIUM, 0x200 ) == RTX_OK;
    process_sub_result( test_id, (*p_index)++, sub_result );
    
    sub_result = tsk_create( &tsk2, task3, MEDIUM, 0x200 ) == RTX_OK;
    process_sub_result( test_id, (*p_index)++, sub_result );
    
    int passedOnce = 0;
    
    printf( "%s: Yielding control until they finish.\r\n", PREFIX );
    while( !(task2_done && task3_done) )
    {
        push_last_task( 1 );
        if( passedOnce && !task3_done )
            sub_result &= check_last_task0( 1 );
        
        printf( "%s: Waiting on the other 2 tasks to finish.\r\n", PREFIX );
        process_sub_result( test_id, (*p_index)++, sub_result );
        tsk_yield();
        passedOnce = 1;
    }
    sub_result &= 1;
    printf( "%s: The 2 other tasks ran until completion.\r\n", PREFIX );
    process_sub_result( test_id, (*p_index)++, sub_result );
    
    return sub_result;
}

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

void task1(void)
{
    int test_id = 0;
    
    task_t tid = tsk_gettid();
    
    printf( "%s: Task1 is being hit.\r\n", PREFIX );
    
    test0_start( test_id );

    test_exit();
    tsk_exit();
}


int fibb_result = 0;
void task2(void)
{
    task_t tid = tsk_gettid();
    int    test_id = 0;
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 1;
    
    int passedOnce = 0;
    while( !task3_done )
    {
        push_last_task( 2 );
        if( passedOnce )
            sub_result &= check_last_task0( 2 );
        printf( "%s: Waiting for the 6th fibonacci number\r\n", PREFIX );
        process_sub_result( test_id, (*p_index)++, sub_result );
        tsk_yield();
        passedOnce = 1;
    }

    sub_result &= ( fibb_result == 8 );
    printf( "%s: The 6th fibb number is %d.\r\n", PREFIX, fibb_result );
    
    task2_done = 1;
    tsk_exit();
}

void task3(void)
{
    int    test_id = 0;
    task_t tid = tsk_gettid();
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 1;
    
    int fibb_n = 0;
    int fibb_nm1 = 1;
    int fibb_nm2 = 0;

    int iterator;
    for( iterator = 0; iterator < 5; iterator++ )
    {
        fibb_n = fibb_nm1 + fibb_nm2;
        
        fibb_nm2 = fibb_nm1;
        fibb_nm1 = fibb_n;
        
        push_last_task( 3 );
        sub_result &= check_last_task0( 3 );
        printf( "%s: Did a fibonacci round\r\n", PREFIX );
        process_sub_result( test_id, (*p_index)++, sub_result );
        tsk_yield();
    }

    fibb_result = fibb_n;
    task3_done = 1;
    tsk_exit();
}


void task4(void)
{
    // Does nothing, it's here because for whatever reason, using 1 initial task doesn't seem to work
    tsk_exit();
}
