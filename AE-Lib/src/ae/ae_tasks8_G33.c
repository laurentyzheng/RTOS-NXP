

/*
 * This is a robust test. It is intended to ensure that there is no degradation of functionality over long periods use.
 * It should:
 * Create 8 other tasks and have each exit, do this a large number of times.
 * Create 8 other tasks that each yield a large number of times until they exit.
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
#define NUM_INIT_TASKS  4       // number of tasks during initialization
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

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    for( int i = 0; i < num; i++ )
    {
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = MEDIUM;
        tasks[i].priv = 1;
    }
    tasks[0].priv  = 0;
    tasks[0].ptask = &task4; // The purpose of these is to put the test in a weird position in the array.
    tasks[1].priv  = 0;
    tasks[1].ptask = &task4;
    tasks[2].priv  = 0;
    tasks[2].ptask = &task4;
    tasks[3].priv  = 0;
    tasks[3].ptask = &task1;
    
    init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
    g_ae_xtest.test_id = 0;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests = NUM_TESTS;
    g_ae_xtest.num_tests_run = 0;
    
    for ( int i = 0; i< NUM_TESTS; i++ )
    {
        g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
        g_tsk_cases[i].p_ae_case->results  = 0x0;
        g_tsk_cases[i].p_ae_case->test_id  = i;
        g_tsk_cases[i].p_ae_case->num_bits = 0;
        g_tsk_cases[i].pos = 0;  // first avaiable slot to write exec seq tid
        // *_expt fields are case specific, deligate to specific test case to initialize
    }
}

void update_ae_xtest( int test_id )
{
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

void gen_req0( int test_id )
{
    // bits[0:1] for two tsk_create, [2:5] for 4 tsk_yield return value checks
    g_tsk_cases[test_id].p_ae_case->num_bits = 5;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 6;

    update_ae_xtest( test_id );
}

int test0_start( int test_id )
{
    int result = RTX_OK;
    gen_req0( test_id );
    int sub_result = 1;
    U8 * p_index = &(g_ae_xtest.index);
    
    task_t leTasks[MAX_TASKS];
    
    printf( "%s: Creating 8 other tasks and exiting from each a large number of times.\r\n", PREFIX );
    
    int iterator;
    int jterator;
    for( iterator = 0; iterator < 0x1000; iterator++ )
    {
        for( jterator = 0; jterator < 8; jterator++ )
        {
            sub_result &= ( tsk_create( &leTasks[jterator], task4, MEDIUM, 0x200 ) == RTX_OK );
            if( !sub_result )
            {
                printf( "%s: failed to create task %d.\r\n", PREFIX, jterator );
                test_abort( test_id, *p_index );
            }
        }
        
        tsk_yield();
    }
    process_sub_result( test_id, (*p_index)++, sub_result );
    
    printf( "%s: Creating 8 tasks that yield for a long time each.\r\n", PREFIX );
    
    for( jterator = 0; jterator < 8; jterator++ )
    {
        sub_result &= ( tsk_create( &leTasks[jterator], task2, MEDIUM, 0x200 ) == RTX_OK );
    }
    
    for( iterator = 0; iterator < 0x800; iterator++ )
    {
        tsk_yield();
    }
    
    int current_num_tasks = tsk_ls( leTasks, 10 );
    printf( "%s: tsk_ls should return 10: %d.\r\n", PREFIX, current_num_tasks );
    sub_result = ( current_num_tasks == 10 );
    process_sub_result( test_id, (*p_index)++, sub_result );

    tsk_yield();
    current_num_tasks = tsk_ls( leTasks, 10 );
    printf( "%s: tsk_ls should return 2: %d.\r\n", PREFIX, current_num_tasks );
    sub_result = ( current_num_tasks == 2 );
    process_sub_result( test_id, (*p_index)++, sub_result );
    
    printf( "%s: Creating a HIGH priority task a large number of times.\r\n", PREFIX );
    
    sub_result = 1;
    for( iterator = 0; iterator < 0x800; iterator++ )
    {
        sub_result &= ( tsk_create( leTasks, task4, HIGH, 0x200 ) == RTX_OK );
    }
    process_sub_result( test_id, (*p_index)++, sub_result );
    
    printf( "%s: Creating MEDIUM priority tasks and changing them to HIGH.\r\n", PREFIX );
    
    for( iterator = 0; iterator < 0x800; iterator++ )
    {
        sub_result &= ( tsk_create( leTasks, task4, MEDIUM, 0x200 ) == RTX_OK );
        sub_result &= ( tsk_set_prio( leTasks[0], HIGH ) == RTX_OK );
    }
    process_sub_result( test_id, (*p_index)++, sub_result );
    sub_result &= ( tsk_ls( leTasks, 8 ) == 2 );
    
    
    
    test_exit();

    return sub_result ? RTX_OK : RTX_ERR;
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



void priv_task1(void)
{
    task_t tid = tsk_gettid();
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    // Kept so that no errors occur accidentally from excluding it.
    tsk_exit(); 
}


void task1(void)
{
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    
    task_t tid = tsk_gettid();
    
    test0_start( test_id );
    
    tsk_exit();
}

void task2(void)
{
    task_t tid = tsk_gettid();
    int    test_id = 0;
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;
    

    int iterator;
    for( iterator = 0; iterator < 0x800; iterator++ )
    {
        tsk_yield();
    }
    
    tsk_exit();
}

void task3(void)
{
    int    test_id = 0;
    task_t tid = tsk_gettid();
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;
    
    

    tsk_exit();
}

void task4(void)
{
    // Does nothing.
    tsk_exit();
}
