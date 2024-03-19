#define ECE350_GN 33
#define Test_Suite_Num 1

#include "ae.h"
#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */
    
#define NUM_TESTS       3       // number of tests
#define NUM_INIT_TASKS  1       // number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT    g_init_tasks[NUM_INIT_TASKS];
const char   PREFIX[]      = "G33-TS1";
const char   PREFIX_LOG[]  = "G33-TS1-LOG ";
const char   PREFIX_LOG2[] = "G33-TS1-LOG2";

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
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = HIGH + i;
        tasks[i].priv = 1;
    }
    tasks[0].priv  = 1;
    tasks[0].ptask = &priv_task1;
    
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

int new_bits_to_load = 0;
void gen_req0(int test_id)
{
    g_tsk_cases[test_id].p_ae_case->num_bits = new_bits_to_load;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 0; // N/A for P1 tests
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

int mem_test_basic( int test_id )
{
	new_bits_to_load = 6;
	gen_req0( test_id );
	U8 *p_index = &( g_ae_xtest.index );
	int result = 1;

	*p_index = 0;
	strcpy( g_ae_xtest.msg, "G33-TS1: Begin basic test\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	void * stuff[10];
	void * stuff2[10];
	int iterator;

	strcpy( g_ae_xtest.msg, "G33-TS1: Allocating 10 items of size 32, 128 in each pool.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	for( iterator = 0; iterator < 10; iterator++ )
	{
		stuff[iterator] = mem_alloc(32);
		stuff2[iterator] = mem2_alloc(128);
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Checking if they are allocated.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	for( iterator = 0; iterator < 10; iterator++ )
	{
		result &= ( stuff[iterator] != NULL ) & ( stuff2[iterator] != NULL );
	}
	
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Some of the addresses are null, unexpectedly.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		mem2_dump();
		test_abort( test_id, *p_index );
		return 0; // abort()
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Checking for duplicate addresses.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	int jterator;
	for( iterator = 0; iterator < 10; iterator++ )
	{
		for( jterator = 0; jterator < iterator; jterator++ )
		{
			result &= ( stuff[iterator] != stuff[jterator] ) & ( stuff2[iterator] != stuff2[jterator] );
		}
	}
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Duplicate Addresses found!\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		mem2_dump();
		test_abort( test_id, *p_index );
		return 0; // abort()
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Deallocating.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	for( iterator = 0; iterator < 10; iterator++ )
	{
		result &= ( mem_dealloc( stuff[iterator] ) == RTX_OK ) & ( mem2_dealloc( stuff2[iterator] ) == RTX_OK );
		stuff[iterator] = NULL;
		stuff2[iterator] = NULL;
	}
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Could not deallocate everything.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		mem2_dump();
		test_abort( test_id, *p_index );
		return 0; // abort()
	}
	
	strcpy( g_ae_xtest.msg, "G33-TS1: End basic test\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	return RTX_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int mem_test_robust( int test_id )
{
	new_bits_to_load = 7;
	gen_req0( test_id );
	U8 *p_index = &( g_ae_xtest.index );
	U8 result = 1;
	
	*p_index = 0;
	strcpy( g_ae_xtest.msg, "G33-TS1: Begin robust test\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	int randSize = 1;
	int iterator;
	for( iterator = 0; iterator < 4096; iterator++ )
	{
		// Those are just 2 large enough primes, they could be anything
		randSize = (2938948301*randSize + 5254103177) % 829;
		void * mem = mem_alloc( randSize + 4 );
		result &= (mem != NULL);
		result &= (mem_dealloc(mem) == RTX_OK);
		mem = NULL;
	}
	for( iterator = 0; iterator < 4096; iterator++ )
	{
		// Those are just 2 large enough primes, they could be anything
		randSize = (2938948301*randSize + 5254103177) % 829;
		void * mem = mem2_alloc( randSize + 4 );
		result &= (mem != NULL);
		result &= (mem2_dealloc(mem) == RTX_OK);
		mem = NULL;
	}
	
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Could not allocate and deallocate 4096 times sequentially.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		mem2_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Was able to allocate and deallocate 4096 times successfully.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	void * memArray[128];
	for( iterator = 0; iterator < 32; iterator++ )
	{
		memArray[iterator] = mem_alloc(32);
		result &= (memArray[iterator] != NULL);
	}
	if( result )
		for( iterator = 0; iterator < 32; iterator++ )
		{
			result &= ( mem_dealloc( memArray[iterator] ) == RTX_OK );
			memArray[iterator] = NULL;
		}
	
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Could not maintain 32 memory blocks (at once) in mem1.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0; // abort()
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Successfully maintained 32 memory blocks (at once) in mem1.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	for( iterator = 0; iterator < 128; iterator++ )
	{
		memArray[iterator] = mem2_alloc(256);
		result &= (memArray[iterator] != NULL);
	}
	if( result )
		for( iterator = 0; iterator < 128; iterator++ )
		{
			result &= ( mem2_dealloc( memArray[iterator] ) == RTX_OK );
			memArray[iterator] = NULL;
		}
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Could not maintain 128 blocks of 256 at once in mem2.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem2_dump();
		test_abort( test_id, *p_index );
		return 0; // abort()
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Successfully maintained 128 blocks of 256 at once in mem2.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	for( iterator = 0; iterator < 4096; iterator++ )
	{
		void * mem1 = mem_alloc( 512 );
		void * mem2 = mem_alloc( 256 );
		void * mem3 = mem_alloc( 256 );
		
		result &= ( mem1 != NULL )&( mem2 != NULL )&( mem3 != NULL );
		
		if( result != 1 )
			break;
		
		result &= ( mem_dealloc( mem1 ) == RTX_OK );
		result &= ( mem_dealloc( mem2 ) == RTX_OK );
		result &= ( mem_dealloc( mem3 ) == RTX_OK );
		mem1 = NULL;
		mem2 = NULL;
		mem3 = NULL;
		if( result != 1	)
			break;
	}
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Could not allocate huge blocks of mem over and over again (mem1).\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Successfully allocated huge blocks of mem 4096 times.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	for( iterator = 0; iterator < 4096; iterator++ )
	{
		void * mem1 = mem2_alloc( 0x4000 );
		void * mem2 = mem2_alloc( 0x2000 );
		void * mem3 = mem2_alloc( 0x2000 );
		
		result &= ( mem1 != NULL )&( mem2 != NULL )&( mem3 != NULL );
		
		if( result != 1 )
			break;
		
		result &= ( mem2_dealloc( mem1 ) == RTX_OK );
		result &= ( mem2_dealloc( mem2 ) == RTX_OK );
		result &= ( mem2_dealloc( mem3 ) == RTX_OK );
		mem1 = NULL;
		mem2 = NULL;
		mem3 = NULL;
		if( !result )
			break;
	}
	
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: Could not allocate huge blocks of mem over and over again (mem2).\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Successfully allocated huge blocks of mem 4096 times (mem2).\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: End robust test.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	return result;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int mem_test_behavior( int test_id )
{
	new_bits_to_load = 16;
	gen_req0( test_id );
	U8 *p_index = &( g_ae_xtest.index );
	int result = 1;

	int cpErrno;
	*p_index = 0;
	strcpy( g_ae_xtest.msg, "G33-TS1: Begin behavior test.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: Allocating 32 bytes.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	errno = 0;
	void * mem = mem_alloc(32);
	result = ( mem != NULL ) && ( errno == 0 );
	if( !result )
	{
		cpErrno = errno;
		strcpy( g_ae_xtest.msg, "G33-TS1: alloc failed.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	result = ( mem_dealloc( mem ) == RTX_OK )&&( errno == 0 );
	if( !result )
	{
		cpErrno = errno;
		strcpy( g_ae_xtest.msg, "G33-TS1: dealloc failed.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: Standard behavior does not change errno, correct.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: Attempting to allocate 0x500, 0x250, then 0x500.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	void * mem_blocks[3];
	mem_blocks[0] = mem_alloc( 0x500 );
	mem_blocks[1] = mem_alloc( 0x250 );
	errno = 0;
	mem_blocks[2] = mem_alloc( 0x500 );
	cpErrno = errno;
	result = ( mem_blocks[2] == NULL )&&( cpErrno == ENOMEM );
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: mem_alloc did not return NULL OR errno was not set correctly.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		mem_dealloc( mem_blocks[0] );
		mem_dealloc( mem_blocks[1] );
		mem_blocks[0] = NULL;
		mem_blocks[1] = NULL;
		mem_dealloc( mem_blocks[2] );
		mem_blocks[2] = NULL;
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: mem_alloc correctly returned NULL AND errno was set correctly.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	mem_dealloc( mem_blocks[0] );
	mem_dealloc( mem_blocks[1] );
	mem_blocks[0] = NULL;
	mem_blocks[1] = NULL;
	
	strcpy( g_ae_xtest.msg, "G33-TS1: Attempting to allocate 1000000 bytes.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	mem = mem_alloc(1000000);
	cpErrno = errno;
	result = ( mem == NULL )&&( cpErrno == ENOMEM );
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: mem_alloc did not return NULL OR it did not set errno.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: mem_alloc returned NULL and errno was set correctly.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: Attempting to allocate 0 bytes.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	errno = 0;
	mem = mem_alloc(0);
	cpErrno = errno;
	result = ( mem == NULL )&&( errno == 0 );
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: mem_alloc did not return NULL OR it set errno.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: mem_alloc returned NULL and errno was not set, correct.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: Attempting to dealloc an out of range address.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	errno = 0;
	
	int dealloc_result = mem_dealloc( (void*) 0xdeadbeef );
	cpErrno = errno;
	result = ( dealloc_result == -1 )&&( cpErrno == EFAULT );
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: mem_dealloc returned -1 OR errno was incorrect.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	strcpy( g_ae_xtest.msg, "G33-TS1: mem_dealloc returned -1 and set errno correctly.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: Attempting to dealloc something in iram1 from mem2_dealloc.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	mem = mem_alloc(32);
	errno = 0;
	dealloc_result = mem2_dealloc( mem );
	cpErrno = errno;
	result = ( dealloc_result == -1 )&&( cpErrno == EFAULT );
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: mem2_dealloc returned -1 OR errno was incorrect.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	mem_dealloc( mem ); // Just assume this does not behave badly.
	mem = NULL;
	strcpy( g_ae_xtest.msg, "G33-TS1: mem2_dealloc treated the address as out of bounds. Correct.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: Attempting to dealloc in iram2 from mem_dealloc.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	mem = mem2_alloc(32);
	errno = 0;
	dealloc_result = mem_dealloc( mem );
	cpErrno = errno;
	result = ( dealloc_result == -1 )&&( cpErrno == EFAULT );
	if( !result )
	{
		strcpy( g_ae_xtest.msg, "G33-TS1: mem_dealloc returned -1 OR errno wasn't correct.\r\n" );
		process_sub_result( test_id, (*p_index)++, result ); 
		mem_dump();
		test_abort( test_id, *p_index );
		return 0;
	}
	mem_dealloc( mem );
	mem = NULL;
	strcpy( g_ae_xtest.msg, "G33-TS1: mem_dealloc treated the address as out of bounds. Correct.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	
	strcpy( g_ae_xtest.msg, "G33-TS1: End behavior test.\r\n" );
	process_sub_result( test_id, (*p_index)++, result ); 
	return RTX_OK;
}


void priv_task1(void)
{
    //int test_id = 0;
    
    printf( "%s: priv_task1: basic memory test on IRAM2\r\n", PREFIX_LOG2 );
#ifdef ECE350_P1    
//   	mem_test_basic( test_id++ );
//    mem_test_robust( test_id++ );
//    mem_test_behavior( test_id++ );
	
#endif // ECE350_P1
	
#ifdef ECE350_P2
#endif // ECE350_P2
    test_exit();
}
